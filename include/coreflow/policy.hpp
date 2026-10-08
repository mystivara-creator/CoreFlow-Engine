#pragma once

#include "coreflow/types.hpp"

namespace coreflow {

// Thermal thresholds shared by the policy and the engine's predictive guard.
// Enter ThermalGuard at >= kThermalGuardEnterC; leave only when the reading
// drops to <= kThermalGuardExitC (hysteresis prevents state bouncing).
inline constexpr double kThermalGuardEnterC = 43.0;
inline constexpr double kThermalGuardExitC = 41.5;

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
