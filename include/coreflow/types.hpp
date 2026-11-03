/**
 * @file types.hpp
 * @brief CoreFlow Engine Modern C++20 Core Type Definitions
 * @author mystivara-creator
 * @license Apache-2.0
 */

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <chrono>
#include <optional>
#include <system_error>
#include <cerrno>
#include <cstdlib>

namespace coreflow {

// ============================================================================
// Robust ASCII Numeric Parsing (Noexcept & Safe)
// ============================================================================
inline bool parse_ascii_double(std::string_view text, double& value) noexcept {
    const size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return false;
    const size_t last = text.find_last_not_of(" \t\r\n");
    std::string_view trimmed = text.substr(first, last - first + 1);
    if (trimmed.empty()) return false;

    char* endptr = nullptr;
    std::string s(trimmed);
    errno = 0;
    double d = std::strtod(s.c_str(), &endptr);
    if (endptr == s.c_str() || *endptr != '\0' || errno == ERANGE) return false;
    value = d;
    return true;
}

// ============================================================================
// Device Tier & Architecture Taxonomy
// ============================================================================
enum class DeviceTier : uint8_t {
    Entry,       ///< 4x LITTLE + 4x Mid (e.g. Snapdragon 680, Helio G99)
    MidRange,    ///< 4x Efficiency + 4x Performance (e.g. Snapdragon 6 Gen 3 'Kunzite')
    UpperMid,    ///< High IPC 4+4 or 1+3+4 (e.g. Dimensity 8300, Snapdragon 7+ Gen 3)
    Flagship     ///< Tri/Quad-cluster Prime (e.g. Snapdragon 8 Gen 3/4, Dimensity 9300)
};

enum class WorkloadState : uint8_t {
    Balanced = 0,
    SustainedLoad = 1,
    PressureGuard = 2,
    ThermalGuard = 3,

    // Aliases for compatibility with different submodules
    Interactive = Balanced,
    DailyIdle = Balanced,
    DeepSleep = Balanced,
    BurstLaunch = SustainedLoad,
    SustainedGame = SustainedLoad,
    ThermalThrottled = ThermalGuard
};

enum class ClusterType : uint8_t {
    Little,      ///< High efficiency, background tasks
    Medium,      ///< Balanced performance, UI rendering
    BigPrime     ///< Max IPC, burst launches, heavy frames
};

// ============================================================================
// Hardware Topology Descriptors
// ============================================================================
struct CpuCluster {
    ClusterType type;
    uint32_t first_cpu;
    uint32_t last_cpu;
    uint32_t base_freq_khz;
    uint32_t max_freq_khz;
    std::string cpufreq_policy_path;
    std::string energy_model_path;
};

struct DeviceTopology {
    std::string soc_name;
    std::string hardware_platform;
    DeviceTier tier;
    uint32_t total_cpus;
    std::string online_cpus_mask{"0-7"};
    std::string kernel_version;
    std::string primary_thermal_zone_type;
    std::string battery_control_node;
    std::string gpu_type;
    std::vector<CpuCluster> clusters;
    bool has_cgroups_v2{false};
    bool has_uclamp{false};
    bool has_surfaceflinger_hints{false};
};

// ============================================================================
// Uclamp & Scheduling Constants (Linux kernel 5.10+ / 5.15+)
// Reference: Documentation/scheduler/sched-util-clamping.rst
// Scale: 0 to 1024 (SCHED_CAPACITY_SCALE)
// ============================================================================
struct UclampValues {
    uint32_t uclamp_min{0};    ///< Guaranteed minimum capacity (0 - 1024)
    uint32_t uclamp_max{1024}; ///< Hard frequency ceiling (0 - 1024)
    bool boost_render_thread{false};
};

// ============================================================================
// Cgroup v2 Migration Targets
// ============================================================================
enum class CgroupTarget : uint8_t {
    TopApp,      ///< /sys/fs/cgroup/top-app
    Foreground,  ///< /sys/fs/cgroup/foreground
    Background,  ///< /sys/fs/cgroup/background
    System       ///< /sys/fs/cgroup/system
};

// ============================================================================
// Error Codes for Zero-Exception Native Execution
// ============================================================================
enum class CoreFlowError {
    Success = 0,
    NodeNotFound,
    PermissionDenied,
    EpollCreateFailed,
    EpollCtlFailed,
    InvalidTopology,
    BaselineSnapshotFailed,
    RollbackFailed,
    ThermalEmergency
};

std::error_code make_error_code(CoreFlowError e);

} // namespace coreflow
