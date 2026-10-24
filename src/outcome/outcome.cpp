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

namespace {
// A comparison is made only when both samples are valid and the device has learned
// a noise floor for the signal. An unlearned floor makes the result inconclusive;
// it never falls back to a guessed threshold.
bool comparableSignal(bool before_valid, bool after_valid, const SignalBaseline& baseline) {
    return before_valid && after_valid && baseline.ready && baseline.deadband > 0.0;
}
// Frame-timing signals that are only comparable inside one foreground package.
void compareFrameLowerIsBetter(const SystemObservation& before, const SystemObservation& after,
                               const NoiseFloor& noise, Outcome& out, bool& comparable,
                               bool& improved, bool& regressed) {
    const auto& p95 = noise[Signal::FrameP95];
    compareLowerIsBetter(comparableSignal(true, true, p95), before.frame_p95_interval_ms, true,
                         after.frame_p95_interval_ms, p95.deadband, out, comparable, improved, regressed);
    const auto& p99 = noise[Signal::FrameP99];
    compareLowerIsBetter(comparableSignal(true, true, p99), before.frame_p99_interval_ms, true,
                         after.frame_p99_interval_ms, p99.deadband, out, comparable, improved, regressed);
    const auto& slow = noise[Signal::SlowFrameRatio];
    compareLowerIsBetter(comparableSignal(true, true, slow), before.slow_frame_ratio, true,
                         after.slow_frame_ratio, slow.deadband, out, comparable, improved, regressed);
}
} // namespace

Outcome OutcomeVerifier::verify(const SystemObservation& before,
                                const SystemObservation& after,
                                const NoiseFloor& noise) const {
    Outcome out;
    bool comparable = false;
    bool regressed = false;
    bool improved = false;

    const auto& temperature = noise[Signal::Temperature];
    compareLowerIsBetter(comparableSignal(before.temperature_valid, after.temperature_valid, temperature),
                         before.maximum_temperature_c, true, after.maximum_temperature_c,
                         temperature.deadband, out, comparable, improved, regressed);
    const auto& memory = noise[Signal::MemoryAvailable];
    compareHigherIsBetter(comparableSignal(before.memory_valid, after.memory_valid, memory),
                          before.memory_available_ratio, true, after.memory_available_ratio,
                          memory.deadband, out, comparable, improved, regressed);
    const auto& cpu = noise[Signal::CpuPressure];
    compareLowerIsBetter(comparableSignal(before.cpu_pressure_valid, after.cpu_pressure_valid, cpu),
                         before.cpu_pressure_some_avg10, true, after.cpu_pressure_some_avg10,
                         cpu.deadband, out, comparable, improved, regressed);
    const auto& cpu_busy = noise[Signal::CpuBusy];
    compareLowerIsBetter(comparableSignal(before.cpu_valid, after.cpu_valid, cpu_busy),
                         before.cpu_busy_percent, true, after.cpu_busy_percent,
                         cpu_busy.deadband, out, comparable, improved, regressed);
    const auto& memory_pressure = noise[Signal::MemoryPressure];
    compareLowerIsBetter(comparableSignal(before.memory_pressure_valid, after.memory_pressure_valid, memory_pressure),
                         before.memory_pressure_some_avg10, true, after.memory_pressure_some_avg10,
                         memory_pressure.deadband, out, comparable, improved, regressed);
    const auto& io = noise[Signal::IoPressure];
    compareLowerIsBetter(comparableSignal(before.io_pressure_valid, after.io_pressure_valid, io),
                         before.io_pressure_some_avg10, true, after.io_pressure_some_avg10,
                         io.deadband, out, comparable, improved, regressed);

    out.measurable = comparable;
    out.improved = comparable && improved && !regressed;
    out.explanation = !comparable
        ? "OUTCOME_INCONCLUSIVE: no comparable pressure, thermal or memory signal with a learned noise floor"
        : (regressed
            ? "OUTCOME_REGRESSION: a comparable system-pressure signal worsened"
            : (improved
                ? "Outcome improved without regression beyond the measured noise floor"
                : "Outcome neutral: changes are within the measured noise floor or no signal improved"));
    return out;
}

Outcome OutcomeVerifier::verify(const SystemObservation& before,
                                const SystemObservation& after,
                                const ChangeProposal& proposal,
                                const NoiseFloor& noise) const {
    Outcome out;
    bool comparable = false;
    bool regressed = false;
    bool improved = false;

    // Judge a probe against signals from the subsystem it changed. A foreground-app
    // switch invalidates the frame comparison window, so CPU-domain probes are
    // inconclusive when the package changed during measurement.
    if (isCpuDomain(proposal.domain) &&
        before.frame_timing_valid && after.frame_timing_valid &&
        !before.frame_package.empty() && !after.frame_package.empty() &&
        before.frame_package != after.frame_package) {
        out.explanation = "OUTCOME_INCONCLUSIVE: foreground package changed during measurement";
        return out;
    }
    const bool same_frame_window = before.frame_timing_valid && after.frame_timing_valid &&
        !before.frame_package.empty() && before.frame_package == after.frame_package;

    if (proposal.domain == "proc_sys.vm") {
        const auto& pressure = noise[Signal::MemoryPressure];
        if (comparableSignal(before.memory_pressure_valid, after.memory_pressure_valid, pressure)) {
            compareLowerIsBetter(true, before.memory_pressure_some_avg10, true,
                                 after.memory_pressure_some_avg10, pressure.deadband,
                                 out, comparable, improved, regressed);
        }
        // MemAvailable is an independent headroom signal, not a substitute for
        // PSI only when PSI is absent. Compare it whenever its own floor is ready
        // so a memory-headroom regression can veto an otherwise positive outcome.
        const auto& available = noise[Signal::MemoryAvailable];
        if (comparableSignal(before.memory_valid, after.memory_valid, available)) {
            compareHigherIsBetter(true, before.memory_available_ratio, true,
                                  after.memory_available_ratio, available.deadband,
                                  out, comparable, improved, regressed);
        }
    } else if (isCpuDomain(proposal.domain)) {
        const auto& pressure = noise[Signal::CpuPressure];
        compareLowerIsBetter(comparableSignal(before.cpu_pressure_valid, after.cpu_pressure_valid, pressure),
                             before.cpu_pressure_some_avg10, true, after.cpu_pressure_some_avg10,
                             pressure.deadband, out, comparable, improved, regressed);
        // CPU PSI is not exposed by every Android kernel. Keep CPU-busy as an
        // independently normalized fallback so a CPU-domain probe is not doomed
        // to an inconclusive result solely because /proc/pressure/cpu is absent.
        // If both signals are available, either signal can veto a regression.
        const auto& busy = noise[Signal::CpuBusy];
        compareLowerIsBetter(comparableSignal(before.cpu_valid, after.cpu_valid, busy),
                             before.cpu_busy_percent, true, after.cpu_busy_percent,
                             busy.deadband, out, comparable, improved, regressed);
        if (same_frame_window) {
            compareFrameLowerIsBetter(before, after, noise, out, comparable, improved, regressed);
        }
    } else if (isBlockDomain(proposal.domain)) {
        const auto& pressure = noise[Signal::IoPressure];
        compareLowerIsBetter(comparableSignal(before.io_pressure_valid, after.io_pressure_valid, pressure),
                             before.io_pressure_some_avg10, true, after.io_pressure_some_avg10,
                             pressure.deadband, out, comparable, improved, regressed);
    } else if (proposal.domain == "sysfs.devfreq") {
        if (same_frame_window) {
            const auto& fps = noise[Signal::FrameFps];
            const bool fps_ok = comparableSignal(true, true, fps);
            compareHigherRawIsBetter(fps_ok, before.frame_average_fps, true, after.frame_average_fps,
                                     fps.deadband, out, comparable, improved, regressed);
            compareFrameLowerIsBetter(before, after, noise, out, comparable, improved, regressed);
        }
    }

    // A rise beyond the device's own temperature noise vetoes retention. Without a
    // learned floor the veto is strict: any measured rise counts.
    const auto& temperature = noise[Signal::Temperature];
    if (observation::thermal_policy::shouldVetoRetention(
            before.temperature_valid, before.maximum_temperature_c,
            after.temperature_valid, after.maximum_temperature_c,
            temperature.ready ? temperature.deadband : 0.0)) {
        // This guard supplements, and never replaces or disables, the platform's
        // own thermal mitigation.
        regressed = true;
    }
    // Retention must not consume headroom beyond the device's noise, and must not
    // move the hottest zone into its kernel-published hysteresis band.
    const auto& headroom = noise[Signal::ThermalHeadroom];
    if (observation::thermal_policy::shouldVetoHeadroom(
            before.thermal_headroom_valid, before.thermal_headroom_c,
            after.thermal_headroom_valid, after.thermal_headroom_c,
            before.thermal_headroom_margin_valid ? before.thermal_headroom_margin_c : 0.0,
            headroom.ready ? headroom.deadband : 0.0)) {
        regressed = true;
    }

    out.measurable = comparable;
    out.improved = comparable && improved && !regressed;
    out.explanation = !comparable
        ? "OUTCOME_INCONCLUSIVE: no comparable signal with a learned noise floor for this subsystem"
        : (regressed
            ? "OUTCOME_REGRESSION: a comparable subsystem or thermal safety signal worsened"
            : (improved
                ? "Outcome improved without regression beyond the measured noise floor"
                : "Outcome neutral: changes are within the measured noise floor or no signal improved"));
    return out;
}

Disposition OutcomePolicy::decide(const Outcome& o) const {
    if (!o.measurable) return Disposition::Unresolved;
    if (o.improved) return Disposition::Retain;
    return Disposition::Rollback;
}
}
