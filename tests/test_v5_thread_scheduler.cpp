#include "coreflow/v5/thread_scheduler.hpp"

#include <cassert>
#include <iostream>
#include <unordered_map>

using namespace coreflow::v5;

int main() {
    std::unordered_map<int32_t, std::pair<uint32_t, uint32_t>> kernel_state;
    uint32_t calls = 0;

    ThreadSchedulerActuator sched([&](int32_t tid, uint32_t umin, uint32_t umax) -> int {
        assert(umin <= umax && umax <= 1024U);
        kernel_state[tid] = {umin, umax};
        ++calls;
        return 0;
    });

    assert(sched.probe_support());
    assert(sched.stats().supported);

    // 1. LaunchBurst boosts SurfaceFlinger + foreground UI/RenderThread
    sched.apply(ContextMode::LaunchBurst, PerformanceProfile::Balanced, 1.0, false, true,
                {900, 901}, {4321, 4322});
    assert(sched.stats().active_boosted_tids == 4U);
    assert(kernel_state[900].first == 512U && kernel_state[900].second == 1024U);
    assert(kernel_state[4322].first == 512U && kernel_state[4322].second == 1024U);

    // Idempotent re-apply with identical targets makes zero redundant syscalls
    const uint32_t calls_before = calls;
    sched.apply(ContextMode::LaunchBurst, PerformanceProfile::Balanced, 1.0, false, true,
                {900, 901}, {4321, 4322});
    assert(calls == calls_before);

    // 2. Smoothness profile raises interactive floor above Battery profile
    const auto smooth_target = ThreadSchedulerActuator::compute_target(
        ContextMode::Interactive, PerformanceProfile::Smoothness, 1.0, false);
    const auto battery_target = ThreadSchedulerActuator::compute_target(
        ContextMode::Interactive, PerformanceProfile::Battery, 1.0, false);
    assert(smooth_target.util_min > battery_target.util_min);

    // 3. PID thermal pacing lowers util_max and caps floor smoothly under thermal_guard
    const auto hot_target = ThreadSchedulerActuator::compute_target(
        ContextMode::Gaming, PerformanceProfile::Gaming, 0.65, true);
    assert(hot_target.util_max <= 768U);
    assert(hot_target.util_min <= 256U);
    assert(hot_target.util_min <= hot_target.util_max);

    // 4. Switching foreground app automatically resets old app TIDs back to 0..1024
    sched.apply(ContextMode::Interactive, PerformanceProfile::Balanced, 1.0, false, true,
                {900, 901}, {5500});
    assert(kernel_state[4321].first == 0U && kernel_state[4321].second == 1024U);
    assert(kernel_state[4322].first == 0U && kernel_state[4322].second == 1024U);
    assert(kernel_state[5500].first == 288U);

    // 5. Entering Efficiency or Excluded resets all tracked TIDs cleanly
    sched.apply(ContextMode::Efficiency, PerformanceProfile::Balanced, 1.0, false, true,
                {900, 901}, {5500});
    assert(sched.stats().active_boosted_tids == 0U);
    assert(kernel_state[900].first == 0U && kernel_state[900].second == 1024U);
    assert(kernel_state[5500].first == 0U && kernel_state[5500].second == 1024U);

    std::cout << "v5 ThreadSchedulerActuator tests passed\n";
    return 0;
}
