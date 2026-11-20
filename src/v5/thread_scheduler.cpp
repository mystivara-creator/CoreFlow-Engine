/**
 * @file thread_scheduler.cpp
 * @brief CoreFlow v5 Pillar 3A: Per-Thread UClamp actuator via Linux SYS_sched_setattr.
 * @license MIT
 */
#include "coreflow/v5/thread_scheduler.hpp"

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
    bool thermal_guard) noexcept {
    const double bounded_scale = std::isfinite(capacity_scale)
        ? std::clamp(capacity_scale, 0.25, 1.0) : 1.0;

    if (mode == ContextMode::ScreenOff || mode == ContextMode::Excluded ||
        mode == ContextMode::Efficiency) {
        return {0U, 1024U};
    }

    // Base util_min floor designed to keep critical UI/Render threads off stalled little cores
    // without pinning CPU clocks to max frequency.
    uint32_t base_min = 0U;
    switch (mode) {
        case ContextMode::LaunchBurst:
            base_min = (profile == PerformanceProfile::Battery) ? 384U :
                       (profile == PerformanceProfile::Smoothness) ? 640U :
                       (profile == PerformanceProfile::Gaming) ? 640U : 512U;
            break;
        case ContextMode::Interactive:
            base_min = (profile == PerformanceProfile::Battery) ? 192U :
                       (profile == PerformanceProfile::Smoothness) ? 416U :
                       (profile == PerformanceProfile::Gaming) ? 384U : 288U;
            break;
        case ContextMode::Gaming:
            base_min = (profile == PerformanceProfile::Battery) ? 288U :
                       (profile == PerformanceProfile::Gaming) ? 512U : 384U;
            break;
        default:
            return {0U, 1024U};
    }

    // Smooth PID thermal ceiling: prevents sudden vendor cliff-throttling.
    uint32_t target_max = static_cast<uint32_t>(std::lround(1024.0 * bounded_scale));
    if (thermal_guard) {
        target_max = std::min(target_max, 768U);
        // Under thermal guard, gently relax the floor so we never fight thermal pacing.
        base_min = std::min(base_min, 256U);
    }
    const uint32_t scaled_min = static_cast<uint32_t>(
        std::lround(static_cast<double>(base_min) * std::clamp(bounded_scale, 0.50, 1.0)));
    const uint32_t final_min = std::min(scaled_min, target_max);
    return {final_min, std::max(final_min, target_max)};
}

void ThreadSchedulerActuator::apply(
    ContextMode mode,
    PerformanceProfile profile,
    double capacity_scale,
    bool thermal_guard,
    bool enabled,
    const std::vector<int32_t>& surfaceflinger_tids,
    const std::vector<int32_t>& foreground_tids) noexcept {
    if (!enabled || !stats_.supported) {
        if (!applied_tids_.empty()) reset_all();
        stats_.last_util_min = 0U;
        stats_.last_util_max = 1024U;
        stats_.active_boosted_tids = 0U;
        return;
    }

    const ThreadUclampTarget target = compute_target(mode, profile, capacity_scale, thermal_guard);
    const bool neutral = (target.util_min == 0U && target.util_max == 1024U);
    if (neutral) {
        if (!applied_tids_.empty()) reset_all();
        stats_.last_util_min = 0U;
        stats_.last_util_max = 1024U;
        stats_.active_boosted_tids = 0U;
        return;
    }

    std::unordered_set<int32_t> desired_set;
    desired_set.reserve(surfaceflinger_tids.size() + foreground_tids.size());
    for (int32_t tid : surfaceflinger_tids) {
        if (tid > 0) desired_set.insert(tid);
    }
    for (int32_t tid : foreground_tids) {
        if (tid > 0) desired_set.insert(tid);
    }

    // Reset any previously boosted TIDs that are no longer in the active foreground set.
    for (auto it = applied_tids_.begin(); it != applied_tids_.end();) {
        if (!desired_set.contains(it->first)) {
            (void)apply_tid_raw(it->first, 0U, 1024U);
            it = applied_tids_.erase(it);
        } else {
            ++it;
        }
    }

    // Apply target to active critical TIDs only when the target changed.
    for (int32_t tid : desired_set) {
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

    stats_.last_util_min = target.util_min;
    stats_.last_util_max = target.util_max;
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
