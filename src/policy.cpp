#include "coreflow/policy.hpp"

namespace coreflow {

RuntimeState AdaptivePolicy::evaluate(
    const RuntimeSample& sample,
    RuntimeState previous
) const {
    // Conservative guardrails for the initial production foundation.
    // These thresholds are state-classification boundaries, not
    // device-specific tuning parameters.

    constexpr long kThermalGuardMillidegrees = 45000;
    constexpr double kElevatedLoad = 1.5;
    constexpr double kPressureMemoryRatio = 0.10;

    if (sample.thermal_available &&
        sample.hottest_thermal_millidegrees >=
            kThermalGuardMillidegrees)
        return RuntimeState::ThermalGuard;

    if (sample.mem_total_kb > 0 &&
        static_cast<double>(sample.mem_available_kb) /
        static_cast<double>(sample.mem_total_kb) <
        kPressureMemoryRatio)
        return RuntimeState::Pressure;

    if (sample.load1 >= kElevatedLoad)
        return RuntimeState::Elevated;

    if (sample.load1 < 0.20)
        return RuntimeState::Idle;

    if (previous == RuntimeState::ThermalGuard ||
        previous == RuntimeState::Pressure)
        return RuntimeState::Normal;

    return RuntimeState::Normal;
}

Decision AdaptivePolicy::decide(
    const RuntimeSample&,
    RuntimeState state
) const {
    switch (state) {
        case RuntimeState::ThermalGuard:
        case RuntimeState::Pressure:
            return Decision::ReduceIntervention;

        case RuntimeState::Elevated:
            return Decision::Observe;

        case RuntimeState::Idle:
        case RuntimeState::Normal:
        default:
            return Decision::NoAction;
    }
}

const char* stateName(RuntimeState state) {
    switch (state) {
        case RuntimeState::Idle: return "IDLE";
        case RuntimeState::Normal: return "NORMAL";
        case RuntimeState::Elevated: return "ELEVATED";
        case RuntimeState::Pressure: return "PRESSURE";
        case RuntimeState::ThermalGuard: return "THERMAL_GUARD";
    }

    return "UNKNOWN";
}

const char* decisionName(Decision decision) {
    switch (decision) {
        case Decision::NoAction: return "NO_ACTION";
        case Decision::Observe: return "OBSERVE";
        case Decision::ReduceIntervention: return "REDUCE_INTERVENTION";
    }

    return "UNKNOWN";
}

} // namespace coreflow
