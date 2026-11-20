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
    bool thermal_guard,
    const ThreadDemandContext& demand) noexcept {
    const double bounded_scale = std::isfinite(capacity_scale)
        ? std::clamp(capacity_scale, 0.25, 1.0) : 1.0;

    if (mode == ContextMode::ScreenOff || mode == ContextMode::Excluded ||
        mode == ContextMode::Efficiency) {
        return {0U, 1024U};
    }

    // Derive EAS migration anchors directly from the device's discovered CPU capacities.
    const double big_cap = static_cast<double>(std::clamp(demand.big_capacity, 512U, 1024U));
    const double little_cap = demand.little_capacity > 0U
        ? static_cast<double>(std::clamp(demand.little_capacity, 128U, static_cast<uint32_t>(big_cap * 0.75)))
        : big_cap * 0.40;
    const double mid_cap = demand.mid_capacity > static_cast<uint32_t>(little_cap)
        ? static_cast<double>(std::clamp(demand.mid_capacity, static_cast<uint32_t>(little_cap), static_cast<uint32_t>(big_cap)))
        : (little_cap + big_cap) * 0.50;

    // Continuous frame-budget urgency (60 Hz = 1.00x, 90 Hz = 1.11x, 120 Hz = 1.18x, 144 Hz = 1.23x).
    const double hz = static_cast<double>(std::clamp(demand.refresh_hz, 60U, 240U));
    const double hz_factor = std::clamp(1.0 + std::log2(hz / 60.0) * 0.18, 1.0, 1.28);

    // Live scheduling contention feedback from PSI & GPU utilization.
    const double psi_norm = std::isfinite(demand.cpu_psi) ? std::clamp(demand.cpu_psi / 100.0, 0.0, 1.0) : 0.0;
    const double gpu_norm = std::isfinite(demand.gpu_busy_pct) ? std::clamp(demand.gpu_busy_pct / 100.0, 0.0, 1.0) : 0.0;

    const double profile_bias =
        profile == PerformanceProfile::Smoothness ? 1.18 :
        profile == PerformanceProfile::Gaming ? 1.15 :
        profile == PerformanceProfile::Battery ? 0.78 : 1.00;

    double anchor_floor = 0.0;
    switch (mode) {
        case ContextMode::LaunchBurst:
            // Place cold-start UI/Render threads cleanly above little_cap onto mid/big cores.
            anchor_floor = (mid_cap * 0.82 + little_cap * 0.18) * (1.0 + psi_norm * 0.15);
            break;
        case ContextMode::Interactive:
            // Hover smoothly around little_cap knee; lift across cluster boundary under high-Hz or PSI.
            anchor_floor = little_cap * (0.68 + psi_norm * 0.35) * hz_factor;
            break;
        case ContextMode::Gaming:
            // Scale dynamically between little_cap and mid_cap with live GPU + CPU pressure.
            anchor_floor = (little_cap + (mid_cap - little_cap) * std::clamp(0.35 + gpu_norm * 0.45 + psi_norm * 0.25, 0.20, 1.0)) * hz_factor;
            break;
        default:
            return {0U, 1024U};
    }

    // Smooth PID thermal ceiling: prevents sudden vendor cliff-throttling.
    uint32_t target_max = static_cast<uint32_t>(std::lround(big_cap * bounded_scale));
    double raw_min = anchor_floor * profile_bias * std::clamp(bounded_scale, 0.50, 1.0);
    if (thermal_guard) {
        target_max = std::min(target_max, static_cast<uint32_t>(std::lround(big_cap * 0.75)));
        raw_min = std::min(raw_min, little_cap * 0.70);
    }
    const uint32_t final_min = std::min(
        static_cast<uint32_t>(std::clamp(std::lround(raw_min), 0L, 1024L)),
        target_max);
    return {final_min, std::max(final_min, target_max)};
}

void ThreadSchedulerActuator::apply(
    ContextMode mode,
    PerformanceProfile profile,
    double capacity_scale,
    bool thermal_guard,
    bool enabled,
    const std::vector<int32_t>& surfaceflinger_tids,
    const std::vector<int32_t>& foreground_tids,
    const ThreadDemandContext& demand) noexcept {
    if (!enabled || !stats_.supported) {
        if (!applied_tids_.empty()) reset_all();
        stats_.last_util_min = 0U;
        stats_.last_util_max = 1024U;
        stats_.active_boosted_tids = 0U;
        return;
    }

    const ThreadUclampTarget target = compute_target(mode, profile, capacity_scale, thermal_guard, demand);
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
