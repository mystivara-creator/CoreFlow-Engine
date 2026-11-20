/**
 * @file hardware_probe.hpp
 * @brief CoreFlow v5 Pillar 1: Single-pass capability-based hardware & kernel topology probe.
 * @license MIT
 */
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace coreflow::v5 {

enum class ClusterRole : uint8_t {
    Unknown = 0,
    Little = 1,
    Mid = 2,
    Big = 3,
};

enum class StorageType : uint8_t {
    Unknown = 0,
    UFS = 1,
    NVMe = 2,
    eMMC = 3,
    SolidState = 4,
};

struct CpuClusterInfo {
    std::string policy_path;
    std::string governor;
    ClusterRole role{ClusterRole::Unknown};
    std::vector<uint64_t> opps_khz;
    uint64_t capacity{0};
    std::string hispeed_freq_path;
    std::string hispeed_load_path;
    std::string up_rate_limit_path;
    std::string down_rate_limit_path;
    std::string boost_path;
};

struct GpuInfo {
    bool available{false};
    bool is_kgsl{false};
    std::string model{"unavailable"};
    std::string device_path;
    std::string busy_path;
    std::string idle_timer_path;
    // KGSL paired levels (0 = fastest, num_pwrlevels - 1 = slowest)
    std::string kgsl_max_pwrlevel_path;
    std::string kgsl_min_pwrlevel_path;
    uint64_t kgsl_num_pwrlevels{0};
    // Generic devfreq GPU
    std::string devfreq_max_freq_path;
    std::vector<uint64_t> devfreq_opps_hz;
};

struct StorageQueueInfo {
    std::string device_name;
    StorageType type{StorageType::Unknown};
    std::string read_ahead_kb_path;
    std::string nr_requests_path;
    std::string scheduler_path;
    uint64_t baseline_read_ahead_kb{128};
};

struct VmNodesInfo {
    std::string swappiness_path;
    std::string page_cluster_path;
    std::string vfs_cache_pressure_path;
    std::string mglru_min_ttl_path;
    std::string compaction_proactiveness_path;
    std::string watermark_scale_factor_path;
};

struct CgroupPathsInfo {
    std::string top_app_dir;
    std::string top_app_procs_path;
    std::string background_dir;
    std::string background_io_weight_path;
    std::string background_uclamp_max_path;
    bool uclamp_uses_percent{true};
};

struct HardwareTopology {
    std::string soc_model{"Unknown SoC"};
    std::string kernel_version{"Unknown"};
    uint64_t total_ram_bytes{0};
    uint64_t zram_bytes{0};
    bool zram_active{false};
    uint32_t refresh_hz{60};
    StorageType primary_storage{StorageType::Unknown};
    std::vector<CpuClusterInfo> cpu_clusters;
    GpuInfo gpu;
    std::vector<StorageQueueInfo> storage_queues;
    VmNodesInfo vm;
    CgroupPathsInfo cgroups;
    std::vector<std::string> backlight_dirs;
};

class HardwareProbe {
public:
    HardwareProbe(std::string sysfs_root = "/sys",
                  std::string procfs_root = "/proc") noexcept;

    [[nodiscard]] HardwareTopology discover() const noexcept;

    // Lightweight live telemetry readers (strictly read-only, bounded I/O)
    [[nodiscard]] bool is_screen_off(const HardwareTopology& topo) const noexcept;
    [[nodiscard]] std::optional<double> read_gpu_busy_pct(const HardwareTopology& topo) const noexcept;
    [[nodiscard]] std::optional<double> read_psi_avg10(const std::string& resource) const noexcept;

    [[nodiscard]] static const char* storage_type_name(StorageType type) noexcept;
    [[nodiscard]] static const char* cluster_role_name(ClusterRole role) noexcept;

private:
    std::string sysfs_root_;
    std::string procfs_root_;
};

} // namespace coreflow::v5
