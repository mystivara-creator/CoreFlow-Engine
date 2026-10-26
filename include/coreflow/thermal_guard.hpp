#pragma once

#include <cstdint>

namespace coreflow {

// Pure thermal-guard decisions, separated from the engine so they can be
// tested on the host. Thresholds come from policy.hpp (Enter/Exit C).

// Consecutive samples in which ONLY the hottest policy-eligible zone is above
// the enter threshold required before the guard engages. A single spiky
// per-core reading must not force protection; a sustained one does.
inline constexpr std::uint32_t kHottestOnlyConfirmSamples = 2U;

// Early entry: enter this many C below the threshold when the representative
// sensor is rising (predictive buffer).
inline constexpr double kRisingThermalBufferC = 0.5;

struct ThermalGuardInputs {
    double selected_c{0.0};   // representative sensor used for trend/model
    double hottest_c{0.0};    // hottest valid policy-eligible zone
    double predicted_c{0.0};  // model/heuristic prediction, 3 ticks ahead
    bool rising{false};       // representative sensor trend is Rising
};

struct ThermalGuardState {
    std::uint32_t hottest_only_streak{0};
};

// Returns true when ThermalGuard should be entered on this sample. Updates the
// hottest-only streak. Representative, predicted and rising triggers act at
// once; a hottest-only trigger needs kHottestOnlyConfirmSamples in a row.
bool thermalGuardShouldEnter(const ThermalGuardInputs& in,
                             double enter_c,
                             ThermalGuardState& state) noexcept;

// Returns true when an active ThermalGuard must remain active (hysteresis).
bool thermalGuardShouldHold(const ThermalGuardInputs& in,
                            double exit_c) noexcept;

}  // namespace coreflow
