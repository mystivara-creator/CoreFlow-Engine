#include "coreflow/outcome/outcome.hpp"
#include <cmath>

namespace coreflow {
namespace {
void compareLowerIsBetter(bool before_valid, double before_value,
                          bool after_valid, double after_value, double deadband,
                          Outcome& out, bool& comparable, bool& improved, bool& regressed) {
    if (!before_valid || !after_valid || !std::isfinite(before_value) ||
        !std::isfinite(after_value)) return;
    comparable = true;
    if (after_value < before_value - deadband) improved = true;
    else if (after_value > before_value + deadband) regressed = true;
    // Scores represent a dimensionless pressure burden, not a sum of raw units.
    out.before_score += before_value;
    out.after_score += after_value;
}
void compareHigherIsBetter(bool before_valid, double before_value,
                           bool after_valid, double after_value, double deadband,
                           Outcome& out, bool& comparable, bool& improved, bool& regressed) {
    if (!before_valid || !after_valid || !std::isfinite(before_value) ||
        !std::isfinite(after_value)) return;
    comparable = true;
    if (after_value > before_value + deadband) improved = true;
    else if (after_value < before_value - deadband) regressed = true;
    out.before_score += (1.0 - before_value);
    out.after_score += (1.0 - after_value);
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
    compareLowerIsBetter(before.temperature_valid, before.maximum_temperature_c / 100.0,
                         after.temperature_valid, after.maximum_temperature_c / 100.0,
                         0.002, out, comparable, improved, regressed);
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

Disposition OutcomePolicy::decide(const Outcome& o) const {
    if (!o.measurable) return Disposition::Unresolved;
    if (o.improved) return Disposition::Retain;
    return Disposition::Rollback;
}
}
