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

    ThreadDemandContext mid_soc{};
    mid_soc.little_capacity = 448;
    mid_soc.mid_capacity = 736;
    mid_soc.big_capacity = 1024;
    mid_soc.refresh_hz = 120;
    mid_soc.cpu_psi = 10.0;

    // 1. LaunchBurst boosts SurfaceFlinger + foreground UI/RenderThread above little_capacity
    sched.apply(ContextMode::LaunchBurst, PerformanceProfile::Balanced, 1.0, false, true,
                {900, 901}, {4321, 4322}, mid_soc);
    assert(sched.stats().active_boosted_tids == 4U);
    assert(kernel_state[900].first > mid_soc.little_capacity && kernel_state[900].second == 1024U);
    assert(kernel_state[4322].first == kernel_state[900].first);

    // Idempotent re-apply with identical targets makes zero redundant syscalls
    const uint32_t calls_before = calls;
    sched.apply(ContextMode::LaunchBurst, PerformanceProfile::Balanced, 1.0, false, true,
                {900, 901}, {4321, 4322}, mid_soc);
    assert(calls == calls_before);

    // 2. Dynamic calibration: 120Hz raises floor above 60Hz, higher cpu_psi raises floor, Smoothness > Battery
    ThreadDemandContext hz60 = mid_soc;
    hz60.refresh_hz = 60;
    const auto t60 = ThreadSchedulerActuator::compute_target(
        ContextMode::Interactive, PerformanceProfile::Balanced, 1.0, false, hz60);
    const auto t120 = ThreadSchedulerActuator::compute_target(
        ContextMode::Interactive, PerformanceProfile::Balanced, 1.0, false, mid_soc);
    assert(t120.util_min > t60.util_min);

    ThreadDemandContext heavy_psi = mid_soc;
    heavy_psi.cpu_psi = 55.0;
    const auto t_psi = ThreadSchedulerActuator::compute_target(
        ContextMode::Interactive, PerformanceProfile::Balanced, 1.0, false, heavy_psi);
    assert(t_psi.util_min > t120.util_min);

    ThreadDemandContext heavy_render = mid_soc;
    heavy_render.render_pressure = 0.85;
    const auto t_render = ThreadSchedulerActuator::compute_target(
        ContextMode::Interactive, PerformanceProfile::Balanced, 1.0, false, heavy_render);
    assert(t_render.util_min > t120.util_min);

    const auto smooth_target = ThreadSchedulerActuator::compute_target(
        ContextMode::Interactive, PerformanceProfile::Smoothness, 1.0, false, mid_soc);
    const auto battery_target = ThreadSchedulerActuator::compute_target(
        ContextMode::Interactive, PerformanceProfile::Battery, 1.0, false, mid_soc);
    assert(smooth_target.util_min > battery_target.util_min);

    // 3. PID thermal pacing lowers util_max and caps floor smoothly under thermal_guard
    const auto hot_target = ThreadSchedulerActuator::compute_target(
        ContextMode::Gaming, PerformanceProfile::Gaming, 0.65, true, mid_soc);
    assert(hot_target.util_max <= 768U);
    assert(hot_target.util_min < mid_soc.little_capacity);
    assert(hot_target.util_min <= hot_target.util_max);

    // 4. Role-tiered asymmetric UClamp: Tier-2 helper_tids receive ~65% of Tier-1 util_min
    //    and switching foreground app automatically resets old Tier-1 & Tier-2 TIDs back to 0..1024
    sched.apply(ContextMode::Interactive, PerformanceProfile::Balanced, 1.0, false, true,
                {900, 901}, {5500}, mid_soc, {5501, 5502});
    assert(kernel_state[4321].first == 0U && kernel_state[4321].second == 1024U);
    assert(kernel_state[4322].first == 0U && kernel_state[4322].second == 1024U);
    assert(kernel_state[5500].first > 0U && kernel_state[5500].first < 1024U);
    assert(kernel_state[5501].first > 0U && kernel_state[5501].first < kernel_state[5500].first);
    assert(kernel_state[5502].first == kernel_state[5501].first);
    assert(sched.stats().active_boosted_tids == 5U);

    // 5. Entering Efficiency or Excluded resets all Tier-1 and Tier-2 tracked TIDs cleanly
    sched.apply(ContextMode::Efficiency, PerformanceProfile::Balanced, 1.0, false, true,
                {900, 901}, {5500}, mid_soc, {5501, 5502});
    assert(sched.stats().active_boosted_tids == 0U);
    assert(kernel_state[900].first == 0U && kernel_state[900].second == 1024U);
    assert(kernel_state[5500].first == 0U && kernel_state[5500].second == 1024U);
    assert(kernel_state[5501].first == 0U && kernel_state[5501].second == 1024U);
    assert(kernel_state[5502].first == 0U && kernel_state[5502].second == 1024U);

    std::cout << "v6.2 Role-Tiered ThreadSchedulerActuator tests passed\n";
    return 0;
}
