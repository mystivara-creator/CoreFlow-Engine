#include "coreflow/thermal_guard.hpp"

#include <algorithm>

namespace coreflow {

bool thermalGuardShouldEnter(const ThermalGuardInputs& in,
                             double enter_c,
                             ThermalGuardState& state) noexcept {
    const bool selected_hot = in.selected_c >= enter_c;
    const bool predicted_hot = in.predicted_c >= enter_c;
    const bool rising_near =
        in.rising && in.selected_c >= enter_c - kRisingThermalBufferC;

    if (selected_hot || predicted_hot || rising_near) {
        state.hottest_only_streak = 0U;
        return true;
    }

    if (in.hottest_c >= enter_c) {
        // Only the hottest zone is hot. Require it to persist.
        if (state.hottest_only_streak < kHottestOnlyConfirmSamples) {
            ++state.hottest_only_streak;
        }
        return state.hottest_only_streak >= kHottestOnlyConfirmSamples;
    }

    state.hottest_only_streak = 0U;
    return false;
}

bool thermalGuardShouldHold(const ThermalGuardInputs& in,
                            double exit_c) noexcept {
    return std::max(in.selected_c, in.hottest_c) > exit_c;
}

}  // namespace coreflow
