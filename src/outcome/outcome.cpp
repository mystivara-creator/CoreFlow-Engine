#include "coreflow/outcome/outcome.hpp"
#include "coreflow/observation/thermal_policy.hpp"
#include <algorithm>
#include <cmath>

namespace coreflow {
namespace {
double normalizedLowerCost(double value) {
    const double nonnegative = std::max(0.0, value);
    return nonnegative / (1.0 + nonnegative);
}

// Domain families. A governor or elevator tunable belongs to the same kernel
// subsystem as its parent policy/queue, so it is judged on that subsystem's
// signals rather than on unrelated global metrics.
bool isCpuDomain(const std::string& domain) {
    return domain == "proc_sys.kernel" || domain == "sysfs.cpu.cpufreq" ||
           domain == "sysfs.cpu.governor";
}
bool isBlockDomain(const std::string& domain) {
    return domain == "sysfs.block.queue" || domain == "sysfs.block.iosched";
}

void compareLowerIsBetter(bool before_valid, double before_value,
                          bool after_valid, double after_value, double deadband,
                          Outcome& out, bool& comparable, bool& improved, bool& regressed) {
    if (!before_valid || !after_valid || !std::isfinite(before_value) ||
        !std::isfinite(after_value)) return;
    comparable = true;
    if (after_value < before_value - deadband) improved = true;
    else if (after_value > before_value + deadband) regressed = true;
    // Normalize nonnegative metrics so milliseconds, PSI percentages, and ratios
    // do not dominate the diagnostic score merely because their units are larger.
    out.before_score += normalizedLowerCost(before_value);
    out.after_score += normalizedLowerCost(after_value);
}
void compareHigherIsBetter(bool before_valid, double before_value,
                           bool after_valid, double after_value, double deadband,
                           Outcome& out, bool& comparable, bool& improved, bool& regressed) {
    if (!before_valid || !after_valid || !std::isfinite(before_value) ||
        !std::isfinite(after_value)) return;
    comparable = true;
    if (after_value > before_value + deadband) improved = true;
    else if (after_value < before_value - deadband) regressed = true;
    out.before_score += 1.0 - std::clamp(before_value, 0.0, 1.0);
    out.after_score += 1.0 - std::clamp(after_value, 0.0, 1.0);
}
void compareHigherRawIsBetter(bool before_valid, double before_value,
                              bool after_valid, double after_value, double deadband,
                              Outcome& out, bool& comparable, bool& improved, bool& regressed) {
    if (!before_valid || !after_valid || !std::isfinite(before_value) ||
        !std::isfinite(after_value)) return;
    comparable = true;
    if (after_value > before_value + deadband) improved = true;
    else if (after_value < before_value - deadband) regressed = true;
    // Inverse normalization keeps a higher-is-better metric bounded regardless
    // of units, unlike subtracting an FPS value from one.
    out.before_score += 1.0 / (1.0 + std::max(0.0, before_value));
    out.after_score += 1.0 / (1.0 + std::max(0.0, after_value));
}
}

Outcome OutcomeVerifier::verify(const SystemObservation& before,
                                const SystemObservation& after) const {
    Outcome out;
    bool comparable = false;
    bool regressed = false;
    bool improved = false;

    // Compare only signals valid on both sides. PSI provides CPU, memory and I/O
    // stall pressure; temperature and available-memory ratio are additional
    // stability signals. Deadbands reduce retention decisions caused by sampling noise.
    // Temperature is stored in Celsius; use the same unit as the
    // candidate-aware verifier. A 0.2 C deadband filters sampling noise.
    compareLowerIsBetter(before.temperature_valid, before.maximum_temperature_c,
                         after.temperature_valid, after.maximum_temperature_c,
                         0.2, out, comparable, improved, regressed);
    compareHigherIsBetter(before.memory_valid, before.memory_available_ratio,
                          after.memory_valid, after.memory_available_ratio,
                          0.01, out, comparable, improved, regressed);
    compareLowerIsBetter(before.cpu_pressure_valid, before.cpu_pressure_some_avg10 / 100.0,
                         after.cpu_pressure_valid, after.cpu_pressure_some_avg10 / 100.0,
                         0.001, out, comparable, improved, regressed);
    compareLowerIsBetter(before.memory_pressure_valid, before.memory_pressure_some_avg10 / 100.0,
                         after.memory_pressure_valid, after.memory_pressure_some_avg10 / 100.0,
                         0.001, out, comparable, improved, regressed);
    compareLowerIsBetter(before.io_pressure_valid, before.io_pressure_some_avg10 / 100.0,
                         after.io_pressure_valid, after.io_pressure_some_avg10 / 100.0,
                         0.001, out, comparable, improved, regressed);

    out.measurable = comparable;
    out.improved = comparable && improved && !regressed;
    out.explanation = !comparable
        ? "OUTCOME_INCONCLUSIVE: no comparable pressure, thermal or memory signal"
        : (regressed
            ? "OUTCOME_REGRESSION: a comparable system-pressure signal worsened"
            : (improved
                ? "Outcome improved without regression beyond metric deadbands"
                : "Outcome neutral: changes are below deadbands or no signal improved"));
    return out;
}

Outcome OutcomeVerifier::verify(const SystemObservation& before,
                                const SystemObservation& after,
                                const ChangeProposal& proposal) const {
    Outcome out;
    bool comparable = false;
    bool regressed = false;
    bool improved = false;

    // Judge a probe against signals from the subsystem it changed. Mixing global
    // memory, CPU and I/O metrics lets unrelated background work retain a bad
    // governor or scheduler change, so only relevant signals are primary.
    // A foreground-app switch invalidates the frame comparison window. Do not
    // let a CPU-pressure change sampled across two different foreground apps
    // make a CPU-frequency/governor probe look beneficial by coincidence.
    if (isCpuDomain(proposal.domain) &&
        before.frame_timing_valid && after.frame_timing_valid &&
        !before.frame_package.empty() && !after.frame_package.empty() &&
        before.frame_package != after.frame_package) {
        out.explanation = "OUTCOME_INCONCLUSIVE: foreground package changed during measurement";
        return out;
    }

    if (proposal.domain == "proc_sys.vm") {
        compareLowerIsBetter(before.memory_pressure_valid, before.memory_pressure_some_avg10,
                             after.memory_pressure_valid, after.memory_pressure_some_avg10,
                             0.1, out, comparable, improved, regressed);
        if (!before.memory_pressure_valid || !after.memory_pressure_valid) {
            compareHigherIsBetter(before.memory_valid, before.memory_available_ratio,
                                  after.memory_valid, after.memory_available_ratio,
                                  0.01, out, comparable, improved, regressed);
        }
    } else if (isCpuDomain(proposal.domain)) {
        compareLowerIsBetter(before.cpu_pressure_valid, before.cpu_pressure_some_avg10,
                             after.cpu_pressure_valid, after.cpu_pressure_some_avg10,
                             0.1, out, comparable, improved, regressed);
        if (before.frame_timing_valid && after.frame_timing_valid &&
            !before.frame_package.empty() && before.frame_package == after.frame_package) {
            compareLowerIsBetter(true, before.frame_p95_interval_ms,
                                 true, after.frame_p95_interval_ms,
                                 0.5, out, comparable, improved, regressed);
            compareLowerIsBetter(true, before.frame_p99_interval_ms,
                                 true, after.frame_p99_interval_ms,
                                 1.0, out, comparable, improved, regressed);
            compareLowerIsBetter(true, before.slow_frame_ratio,
                                 true, after.slow_frame_ratio,
                                 0.02, out, comparable, improved, regressed);
        }
    } else if (isBlockDomain(proposal.domain)) {
        compareLowerIsBetter(before.io_pressure_valid, before.io_pressure_some_avg10,
                             after.io_pressure_valid, after.io_pressure_some_avg10,
                             0.1, out, comparable, improved, regressed);
    } else if (proposal.domain == "sysfs.devfreq") {
        if (before.frame_timing_valid && after.frame_timing_valid &&
            !before.frame_package.empty() && before.frame_package == after.frame_package) {
            compareHigherRawIsBetter(true, before.frame_average_fps,
                                     true, after.frame_average_fps,
                                     1.0, out, comparable, improved, regressed);
            compareLowerIsBetter(true, before.frame_p95_interval_ms,
                                 true, after.frame_p95_interval_ms,
                                 0.5, out, comparable, improved, regressed);
            compareLowerIsBetter(true, before.frame_p99_interval_ms,
                                 true, after.frame_p99_interval_ms,
                                 1.0, out, comparable, improved, regressed);
            compareLowerIsBetter(true, before.slow_frame_ratio,
                                 true, after.slow_frame_ratio,
                                 0.02, out, comparable, improved, regressed);
        }
    }

    // A meaningful temperature rise vetoes retention even when the primary
    // performance signal improved. This is a regression guard, not an absolute
    // temperature target; vendor thermal control remains authoritative.
    if (observation::thermal_policy::shouldVetoRetention(
            before.temperature_valid, before.maximum_temperature_c,
            after.temperature_valid, after.maximum_temperature_c)) {
        // This conservative temperature heuristic supplements, but never
        // replaces or disables, the platform's own thermal mitigation.
        regressed = true;
    }
    // Retention must not consume the margin to the vendor's passive trip point.
    if (observation::thermal_policy::shouldVetoHeadroom(
            before.thermal_headroom_valid, before.thermal_headroom_c,
            after.thermal_headroom_valid, after.thermal_headroom_c)) {
        regressed = true;
    }

    out.measurable = comparable;
    out.improved = comparable && improved && !regressed;
    out.explanation = !comparable
        ? "OUTCOME_INCONCLUSIVE: no comparable signal for the changed subsystem"
        : (regressed
            ? "OUTCOME_REGRESSION: a subsystem or thermal safety signal worsened"
            : (improved
                ? "Subsystem outcome improved without a measured regression"
                : "Outcome neutral: improvement did not exceed measurement deadbands"));
    return out;
}

Disposition OutcomePolicy::decide(const Outcome& o) const {
    if (!o.measurable) return Disposition::Unresolved;
    if (o.improved) return Disposition::Retain;
    return Disposition::Rollback;
}
}
