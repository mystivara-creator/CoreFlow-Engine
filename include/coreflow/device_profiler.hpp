/**
 * @file device_profiler.hpp
 * @brief Root-injectable CoreFlow Dynamic Device Profiling (CDDP).
 * @license Apache-2.0
 */
#pragma once
#include "coreflow/device_profile.hpp"
#include "coreflow/types.hpp"
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>
namespace coreflow {
class DeviceProfiler {
public:
    explicit DeviceProfiler(std::string sys_root = "/sys", std::string proc_root = "/proc");
    [[nodiscard]] DeviceProfileHandle profile();
    [[nodiscard]] DeviceProfileHandle refresh();
    [[nodiscard]] static bool parse_cpu_list(std::string_view text, std::vector<uint32_t>& cpus) noexcept;
    // Compatibility projection only; runtime controllers consume DeviceProfileHandle.
    [[nodiscard]] DeviceTopology probe();
    [[nodiscard]] float get_soc_temperature() const;
    [[nodiscard]] std::vector<uint32_t> get_cluster_frequencies() const;
private:
    [[nodiscard]] DeviceProfileHandle build_profile(uint64_t generation) const;
    std::string sys_root_;
    std::string proc_root_;
    mutable std::mutex mutex_;
    DeviceProfileHandle current_;
    uint64_t generation_{0};
    DeviceTopology cached_topology_{};
};
} // namespace coreflow
