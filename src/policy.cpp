#include "coreflow/policy.hpp"

#include <algorithm>
#include <cmath>

namespace coreflow {

RuntimeState AdaptivePolicy::evaluate(
    const RuntimeSample& sample,
    RuntimeState previous
) const {
    constexpr long kThermalGuardEnter = 43000;
    constexpr long kThermalGuardExit = 40000;
    constexpr long kThermalWarm = 40000;

    constexpr double kMemoryPressureEnter = 0.10;
    constexpr double kMemoryPressureExit = 0.15;

    constexpr double kElevatedUtil = 0.78;
    constexpr double kElevatedLoad = 1.50;

    if (sample.thermal_available) {
        if (previous == RuntimeState::ThermalGuard &&
            sample.thermal_millidegrees > kThermalGuardExit) {
            return RuntimeState::ThermalGuard;
        }
        if (sample.thermal_millidegrees >= kThermalGuardEnter)
            return RuntimeState::ThermalGuard;
        if (sample.thermal_millidegrees >= kThermalWarm)
            return RuntimeState::Warming;
    }

    if (sample.mem_total_kb > 0) {
        if (previous == RuntimeState::Pressure &&
            sample.mem_available_ratio < kMemoryPressureExit) {
            return RuntimeState::Pressure;
        }
        if (sample.mem_available_ratio < kMemoryPressureEnter)
            return RuntimeState::Pressure;
    }

    const bool cpu_busy = sample.cpu_utilization_available &&
                          sample.cpu_utilization >= kElevatedUtil;
    if (cpu_busy || sample.load1 >= kElevatedLoad)
        return RuntimeState::Elevated;

    if (sample.load1 < 0.20)
        return RuntimeState::Idle;

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
        case RuntimeState::Warming:
        case RuntimeState::Elevated:
            return Decision::Observe;
        case RuntimeState::Idle:
        case RuntimeState::Normal:
        default:
            return Decision::NoAction;
    }
}

NotificationEvent AdaptivePolicy::notification(
    const RuntimeSample& sample,
    RuntimeState state,
    RuntimeState previous
) const {
    if (state == RuntimeState::ThermalGuard) {
        if (previous != RuntimeState::ThermalGuard)
            return NotificationEvent::ThermalGuard;
        if (sample.charging && sample.thermal_trend == Trend::Rising)
            return NotificationEvent::ChargingProtection;
        return NotificationEvent::None;
    }

    if (sample.thermal_available && sample.thermal_trend == Trend::Rising) {
        if (sample.thermal_millidegrees >= 42000)
            return NotificationEvent::ThermalWarning;
        if (sample.thermal_millidegrees >= 40000)
            return NotificationEvent::ThermalWarming;
    }

    return NotificationEvent::None;
}

const char* stateName(RuntimeState state) {
    switch (state) {
        case RuntimeState::Idle: return "IDLE";
        case RuntimeState::Normal: return "NORMAL";
        case RuntimeState::Warming: return "WARMING";
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
        case Decision::Notify: return "NOTIFY";
        case Decision::ReduceIntervention: return "REDUCE_INTERVENTION";
    }
    return "UNKNOWN";
}

const char* trendName(Trend trend) {
    switch (trend) {
        case Trend::Rising: return "RISING";
        case Trend::Stable: return "STABLE";
        case Trend::Falling: return "FALLING";
        case Trend::Unknown: return "UNKNOWN";
    }
    return "UNKNOWN";
}

const char* notificationEventName(NotificationEvent event) {
    switch (event) {
        case NotificationEvent::ThermalWarming: return "THERMAL_WARMING";
        case NotificationEvent::ThermalWarning: return "THERMAL_WARNING";
        case NotificationEvent::ThermalGuard: return "THERMAL_GUARD";
        case NotificationEvent::ChargingProtection: return "CHARGING_PROTECTION";
        case NotificationEvent::None: default: return "NONE";
    }
}

const char* mutationResultName(MutationResult result) {
    switch (result) {
        case MutationResult::Skipped: return "SKIPPED";
        case MutationResult::Applied: return "APPLIED";
        case MutationResult::Verified: return "VERIFIED";
        case MutationResult::Failed: return "FAILED";
        case MutationResult::RolledBack: return "ROLLED_BACK";
    }
    return "UNKNOWN";
}

} // namespace coreflow
