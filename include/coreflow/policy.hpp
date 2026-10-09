#pragma once

#include "coreflow/types.hpp"

namespace coreflow {

// Thermal thresholds shared by the policy and the engine's predictive guard.
// Enter ThermalGuard at >= kThermalGuardEnterC; leave only when the reading
// drops to <= kThermalGuardExitC (hysteresis prevents state bouncing).
//
// Values are calibrated for modern mobile SoCs (Qualcomm / MediaTek). Zone
// readings of 42–50 °C under light load are normal; blocking at 43 °C caused
// permanent THERMAL_GUARD on real devices. Guard is reserved for sustained
// high temperature where reducing intervention is actually protective.
inline constexpr double kThermalGuardEnterC = 55.0;
inline constexpr double kThermalGuardExitC = 52.0;

class AdaptivePolicy {
public:
    RuntimeState evaluate(const RuntimeSample&, RuntimeState previous) const;
    Decision decide(const RuntimeSample&, RuntimeState state) const;

    NotificationEvent notification(
        const RuntimeSample&,
        RuntimeState state,
        RuntimeState previous
    ) const;
};

const char* stateName(RuntimeState);
const char* decisionName(Decision);

} // namespace coreflow
