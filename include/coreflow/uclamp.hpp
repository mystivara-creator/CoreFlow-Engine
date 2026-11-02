/**
 * @file uclamp.hpp
 * @brief EAS Utilization Clamping (Uclamp) Governor
 * @author mystivara-creator
 * @license Apache-2.0
 * 
 * Manages cpu.uclamp.min and cpu.uclamp.max directly on Android 14/15+ kernels.
 * Replaces brute-force governor frequency locking with cooperative EAS clamping.
 */

#pragma once

#include "types.hpp"
#include <string>
#include <string_view>

namespace coreflow {

class UclampGovernor {
public:
    explicit UclampGovernor(std::string_view cgroup_root = "/sys/fs/cgroup");
    ~UclampGovernor() = default;

    /// Checks if kernel supports sched_util_clamping
    [[nodiscard]] bool is_supported() const noexcept;

    /// Apply dynamic uclamp profile based on workload state
    bool apply_state(WorkloadState state, DeviceTier tier);

    /// Apply transient burst boost (e.g. touch or cold launch)
    bool apply_boost(uint32_t min_clamp, uint32_t max_clamp, std::chrono::milliseconds duration_ms);

    /// Fine-tune global RT clamp default: /proc/sys/kernel/sched_util_clamp_min_rt_default
    bool set_rt_clamp_min(uint32_t clamp_val);

    /// Read active uclamp.min of top-app
    [[nodiscard]] uint32_t get_top_app_min() const;

    /// Read active uclamp.max of top-app
    [[nodiscard]] uint32_t get_top_app_max() const;

private:
    std::string cgroup_root_;
    bool supported_{false};

    bool write_uclamp(CgroupTarget target, uint32_t min_val, uint32_t max_val);
    uint32_t read_uclamp_node(const std::string& path) const;
};

} // namespace coreflow
