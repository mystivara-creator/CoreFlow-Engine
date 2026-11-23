/**
 * @file thread_scheduler.hpp
 * @brief CoreFlow v5 Pillar 3A: Per-Thread UClamp actuator via Linux SYS_sched_setattr.
 *
 * Targets SurfaceFlinger, RenderThread, HWUI, and foreground UI/game threads directly
 * per-TID so animations and touch responses stay smooth without fighting PowerHAL's
 * global top-app cgroup writes.
 * @license Apache-2.0
 */
#pragma once

#include "coreflow/v5/context_engine.hpp"

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

namespace coreflow::v5 {

struct ThreadUclampTarget {
    uint32_t util_min{0};    // 0..1024
    uint32_t util_max{1024}; // 0..1024
};

struct ThreadSchedulerStats {
    bool supported{false};
    uint32_t active_boosted_tids{0};
    uint32_t last_util_min{0};
    uint32_t last_util_max{1024};
    uint64_t applied_calls{0};
    uint64_t failed_calls{0};
};

struct ThreadDemandContext {
    uint32_t little_capacity{0};  // Discovered EAS little cluster capacity (0..1024)
    uint32_t mid_capacity{0};     // Discovered EAS mid cluster capacity (0..1024)
    uint32_t big_capacity{1024};  // Discovered EAS big cluster capacity (0..1024)
    uint32_t refresh_hz{60};      // Discovered display refresh rate
    double cpu_psi{0.0};          // Live CPU PSI avg10 (0..100)
    double gpu_busy_pct{0.0};     // Live GPU busy percentage (0..100)
    double render_pressure{0.0};  // Closed-loop RenderThread/GameThread CPU saturation (0.0..1.0)
    double thermal_velocity_cpm{0.0}; // Live SoC thermal slope (°C/min) from Rust OLS analyzer
    double jank_ratio{-1.0};      // Live missed-frame ratio (0.0..1.0), -1.0 when unavailable
    double app_uclamp_bias{1.0};  // Learned per-app UClamp bias (0.75..1.20)
    bool feedforward_freeze{false}; // Active during pre-emptive 500-800ms load spike / touch window
    double symbiotic_cpu_scale{1.0}; // Orthogonal Symbiotic Orchestrator CPU floor multiplier (0.88..1.10)
    double helper_uclamp_ratio{0.65}; // Cooperative Tier-2 helper ratio (0.65 default, up to 0.82 under hotspot spread)
    double starvation_pressure{-1.0}; // Universal schedstat runqueue starvation pressure (0..1, -1 when unavailable)
};

class ThreadSchedulerActuator {
public:
    using ApplierFn = std::function<int(int32_t tid, uint32_t util_min, uint32_t util_max)>;

    explicit ThreadSchedulerActuator(ApplierFn custom_applier = nullptr) noexcept;
    ~ThreadSchedulerActuator();

    ThreadSchedulerActuator(const ThreadSchedulerActuator&) = delete;
    ThreadSchedulerActuator& operator=(const ThreadSchedulerActuator&) = delete;

    // Probes whether the active kernel supports SCHED_FLAG_UTIL_CLAMP via sched_setattr.
    [[nodiscard]] bool probe_support() noexcept;

    // Computes hardware-calibrated (util_min, util_max) target from EAS silicon capacities,
    // display refresh budget, live PSI/GPU load, closed-loop jank, App DNA, and PID thermal budget.
    [[nodiscard]] static ThreadUclampTarget compute_target(
        ContextMode mode,
        PerformanceProfile profile,
        double capacity_scale,
        bool thermal_guard,
        const ThreadDemandContext& demand = {}) noexcept;

    // Applies role-tiered per-TID uclamp:
    // - Tier 1 (SurfaceFlinger + primary UI/Render/Game threads): 100% of target util_min.
    // - Tier 2 (helper_tids: hwuiTask/MediaCodec/AudioTrack/Camera workers): 65% of target util_min.
    // Cleanly resets any stale TIDs no longer in the active sets.
    void apply(ContextMode mode,
               PerformanceProfile profile,
               double capacity_scale,
               bool thermal_guard,
               bool enabled,
               const std::vector<int32_t>& surfaceflinger_tids,
               const std::vector<int32_t>& foreground_tids,
               const ThreadDemandContext& demand = {},
               const std::vector<int32_t>& helper_tids = {}) noexcept;

    // Restores all tracked TIDs to kernel defaults (0..1024).
    void reset_all() noexcept;

    [[nodiscard]] ThreadSchedulerStats stats() const noexcept { return stats_; }

private:
    int apply_tid_raw(int32_t tid, uint32_t util_min, uint32_t util_max) noexcept;

    ApplierFn applier_;
    std::unordered_map<int32_t, ThreadUclampTarget> applied_tids_;
    ThreadSchedulerStats stats_{};
};

} // namespace coreflow::v5
