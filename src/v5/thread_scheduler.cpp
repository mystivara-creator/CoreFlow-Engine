/**
 * @file thread_scheduler.cpp
 * @brief CoreFlow v5 Pillar 3A: Per-Thread UClamp actuator via Linux SYS_sched_setattr.
 * @license Apache-2.0
 */
#include "coreflow/v5/thread_scheduler.hpp"
#include "coreflow-engine/src/bridge.rs.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <sys/syscall.h>
#include <unistd.h>
#include <unordered_set>

namespace coreflow::v5 {

namespace {

// Linux kernel UAPI struct sched_attr (5.3+ with uclamp fields).
struct KernelSchedAttr {
    uint32_t size;
    uint32_t sched_policy;
    uint64_t sched_flags;
    int32_t  sched_nice;
    uint32_t sched_priority;
    uint64_t sched_runtime;
    uint64_t sched_deadline;
    uint64_t sched_period;
    uint32_t sched_util_min;
    uint32_t sched_util_max;
};

constexpr uint64_t kSchedFlagKeepPolicy    = 0x08ULL;
constexpr uint64_t kSchedFlagKeepParams    = 0x10ULL;
constexpr uint64_t kSchedFlagUtilClampMin  = 0x20ULL;
constexpr uint64_t kSchedFlagUtilClampMax  = 0x40ULL;

int kernel_set_tid_uclamp(int32_t tid, uint32_t util_min, uint32_t util_max) noexcept {
#if defined(SYS_sched_setattr)
    KernelSchedAttr attr{};
    attr.size = sizeof(KernelSchedAttr);
    attr.sched_flags = kSchedFlagKeepPolicy | kSchedFlagKeepParams |
                       kSchedFlagUtilClampMin | kSchedFlagUtilClampMax;
    attr.sched_util_min = std::min(util_min, 1024U);
    attr.sched_util_max = std::clamp(util_max, attr.sched_util_min, 1024U);
    errno = 0;
    const long rc = ::syscall(SYS_sched_setattr, static_cast<pid_t>(tid), &attr, 0U);
    return rc == 0 ? 0 : (errno != 0 ? errno : EIO);
#else
    (void)tid;
    (void)util_min;
    (void)util_max;
    return ENOSYS;
#endif
}

} // namespace

ThreadSchedulerActuator::ThreadSchedulerActuator(ApplierFn custom_applier) noexcept
    : applier_(std::move(custom_applier)) {}

ThreadSchedulerActuator::~ThreadSchedulerActuator() {
    reset_all();
}

int ThreadSchedulerActuator::apply_tid_raw(int32_t tid, uint32_t util_min, uint32_t util_max) noexcept {
    if (tid <= 0) return EINVAL;
    const uint32_t clamped_min = std::min(util_min, 1024U);
    const uint32_t clamped_max = std::clamp(util_max, clamped_min, 1024U);
    if (applier_) return applier_(tid, clamped_min, clamped_max);
    return kernel_set_tid_uclamp(tid, clamped_min, clamped_max);
}

bool ThreadSchedulerActuator::probe_support() noexcept {
    // Non-invasive probe on our own thread (0..1024 is the kernel default).
    const int err = apply_tid_raw(static_cast<int32_t>(::getpid()), 0U, 1024U);
    stats_.supported = (err == 0);
    return stats_.supported;
}

ThreadUclampTarget ThreadSchedulerActuator::compute_target(
    ContextMode mode,
    PerformanceProfile profile,
    double capacity_scale,
    bool thermal_guard,
    const ThreadDemandContext& demand) noexcept {
    const coreflow::engine::UclampInput in{
        .mode = static_cast<uint8_t>(mode),
        .profile = static_cast<uint8_t>(profile),
        .capacity_scale = capacity_scale,
        .thermal_guard = thermal_guard,
        .little_capacity = demand.little_capacity,
        .mid_capacity = demand.mid_capacity,
        .big_capacity = demand.big_capacity,
        .refresh_hz = demand.refresh_hz,
        .cpu_psi = demand.cpu_psi,
        .gpu_busy_pct = demand.gpu_busy_pct,
        .render_pressure = demand.render_pressure,
        .thermal_velocity_cpm = demand.thermal_velocity_cpm,
        .jank_ratio = demand.jank_ratio,
        .app_uclamp_bias = demand.app_uclamp_bias,
    };
    const auto out = coreflow::engine::compute_uclamp_target(in);
    return {out.util_min, out.util_max};
}

void ThreadSchedulerActuator::apply(
    ContextMode mode,
    PerformanceProfile profile,
    double capacity_scale,
    bool thermal_guard,
    bool enabled,
    const std::vector<int32_t>& surfaceflinger_tids,
    const std::vector<int32_t>& foreground_tids,
    const ThreadDemandContext& demand,
    const std::vector<int32_t>& helper_tids) noexcept {
    if (!enabled || !stats_.supported) {
        if (!applied_tids_.empty()) reset_all();
        stats_.last_util_min = 0U;
        stats_.last_util_max = 1024U;
        stats_.active_boosted_tids = 0U;
        return;
    }

    const ThreadUclampTarget primary_target = compute_target(mode, profile, capacity_scale, thermal_guard, demand);
    const bool neutral = (primary_target.util_min == 0U && primary_target.util_max == 1024U);
    if (neutral) {
        if (!applied_tids_.empty()) reset_all();
        stats_.last_util_min = 0U;
        stats_.last_util_max = 1024U;
        stats_.active_boosted_tids = 0U;
        return;
    }

    const uint32_t helper_min = static_cast<uint32_t>(
        std::clamp<long>(std::lround(static_cast<double>(primary_target.util_min) * 0.65),
                         0L, static_cast<long>(primary_target.util_max)));
    const ThreadUclampTarget helper_target{helper_min, primary_target.util_max};

    std::unordered_map<int32_t, ThreadUclampTarget> desired_targets;
    desired_targets.reserve(surfaceflinger_tids.size() + foreground_tids.size() + helper_tids.size());
    for (int32_t tid : helper_tids) {
        if (tid > 0) desired_targets[tid] = helper_target;
    }
    // Tier 1 overrides Tier 2 if a TID appears in both lists.
    for (int32_t tid : surfaceflinger_tids) {
        if (tid > 0) desired_targets[tid] = primary_target;
    }
    for (int32_t tid : foreground_tids) {
        if (tid > 0) desired_targets[tid] = primary_target;
    }

    // Reset any previously boosted TIDs that are no longer in the active set.
    for (auto it = applied_tids_.begin(); it != applied_tids_.end();) {
        if (!desired_targets.contains(it->first)) {
            (void)apply_tid_raw(it->first, 0U, 1024U);
            it = applied_tids_.erase(it);
        } else {
            ++it;
        }
    }

    // Apply role-tiered target to active TIDs only when the target changed.
    for (const auto& [tid, target] : desired_targets) {
        const auto existing = applied_tids_.find(tid);
        if (existing != applied_tids_.end() &&
            existing->second.util_min == target.util_min &&
            existing->second.util_max == target.util_max) {
            continue;
        }
        const int err = apply_tid_raw(tid, target.util_min, target.util_max);
        if (err == 0) {
            applied_tids_[tid] = target;
            ++stats_.applied_calls;
        } else if (err == ESRCH) {
            applied_tids_.erase(tid);
        } else if (err == ENOSYS || err == EOPNOTSUPP) {
            stats_.supported = false;
            ++stats_.failed_calls;
            break;
        } else {
            ++stats_.failed_calls;
        }
    }

    stats_.last_util_min = primary_target.util_min;
    stats_.last_util_max = primary_target.util_max;
    stats_.active_boosted_tids = static_cast<uint32_t>(applied_tids_.size());
}

void ThreadSchedulerActuator::reset_all() noexcept {
    for (const auto& [tid, _] : applied_tids_) {
        (void)apply_tid_raw(tid, 0U, 1024U);
    }
    applied_tids_.clear();
    stats_.active_boosted_tids = 0U;
    stats_.last_util_min = 0U;
    stats_.last_util_max = 1024U;
}

} // namespace coreflow::v5
