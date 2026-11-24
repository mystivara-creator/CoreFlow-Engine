/**
 * @file storage_memory_actuator.hpp
 * @brief CoreFlow v5 Pillar 3B: Zero-Fixed-Value Dynamic UFS/eMMC Burst, Background QoS, MGLRU Anti-Stutter, and Smart Pre-Read.
 * @license Apache-2.0
 */
#pragma once

#include "coreflow/baseline_store.hpp"
#include "coreflow/v5/context_engine.hpp"
#include "coreflow/v5/hardware_probe.hpp"
#include "coreflow/v6/causal_verifier.hpp"
#include "coreflow/v6/oem_collaboration_map.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace coreflow::v5 {

struct StorageMemoryTelemetry {
    double io_psi{0.0};
    double mem_psi{0.0};
    double cpu_psi{0.0};
    MemorySnapshot memory{};
    bool io_supply_boost{false};
    double capacity_scale{1.0};
    bool thermal_guard{false};
    bool low_battery_eco{false};
    // Universal /proc/vmstat thrashing & schedstat runqueue starvation telemetry
    double thrashing_pressure{0.0};
    double refault_file_per_sec{0.0};
    double direct_reclaim_per_sec{0.0};
    double pswpin_per_sec{0.0};
    bool direct_reclaim_storm{false};
    double starvation_pressure{-1.0};
};

struct StorageMemoryStats {
    uint64_t active_read_ahead_kb{0};
    bool burst_active{false};
    bool background_isolated{false};
    uint64_t preread_files_total{0};
    uint64_t preread_bytes_total{0};
    uint64_t idle_compactions_total{0};
    uint64_t predictive_preloads_total{0};
    uint32_t memory_health_pct{100};
    uint32_t contended_nodes{0};
    bool vm_or_storage_active{false};
};

class StorageMemoryActuator {
public:
    static constexpr uint64_t kInitialContentionCooldownMs = 30000ULL;
    static constexpr uint64_t kMaxContentionCooldownMs = 300000ULL;
    static constexpr uint64_t kIdleCompactionCooldownMs = 600000ULL; // 10 minutes
    static constexpr uint64_t kPredictivePreloadCooldownMs = 180000ULL; // 3 minutes

    StorageMemoryActuator(BaselineStore& baseline,
                          std::string procfs_root = "/proc",
                          v6::OemCollaborationMap* oem_map = nullptr,
                          const v6::CausalVerifier* causal_verifier = nullptr) noexcept;

    void apply(const HardwareTopology& topo,
               const ContextDecision& ctx,
               PerformanceProfile profile,
               bool enable_storage_burst,
               bool enable_memory_guard,
               uint64_t now_ms,
               const StorageMemoryTelemetry& telemetry = {}) noexcept;

    // Smart asynchronous kernel page-cache prefetch for a newly launched foreground app.
    // Also caches discovered APK/ODEX/SO paths for future predictive preloading.
    uint64_t prefetch_app_assets(int32_t pid,
                                 const std::string& package_name,
                                 uint64_t available_or_total_ram_bytes,
                                 uint64_t now_ms,
                                 bool allow_any_path_for_test = false) noexcept;

    // Proactively warms cached APK/ODEX/SO pages for a Rust-predicted next app during cool idle.
    uint64_t prefetch_cached_package(const std::string& package_name,
                                     uint64_t available_or_total_ram_bytes,
                                     uint64_t now_ms,
                                     bool allow_any_path_for_test = false) noexcept;

    // 9-hour anti-aging guard: runs one-shot kernel memory compaction during cool ScreenOff idle.
    bool maybe_run_idle_compaction(const HardwareTopology& topo,
                                   const StorageMemoryTelemetry& telemetry,
                                   float soc_temp_celsius,
                                   uint64_t now_ms) noexcept;

    void reset_contention() noexcept;
    [[nodiscard]] StorageMemoryStats stats() const noexcept { return stats_; }
    [[nodiscard]] std::vector<std::string> contended_paths() const;

    // Hardware-calibrated dynamic read-ahead calculation from queue geometry & live I/O PSI.
    [[nodiscard]] static uint64_t compute_read_ahead_kb(
        const StorageQueueInfo& queue,
        ContextMode mode,
        PerformanceProfile profile,
        double io_psi = 0.0) noexcept;

    // Convenience overload for generic callers/unit tests.
    [[nodiscard]] static uint64_t compute_read_ahead_kb(
        StorageType type,
        ContextMode mode,
        PerformanceProfile profile) noexcept;

private:
    struct ContentionEntry {
        uint64_t retry_after_ms{0};
        uint32_t strikes{0};
        std::string observed_value;
    };

    void record_contention(const std::string& path,
                           const std::string& observed,
                           uint64_t now_ms) noexcept;
    bool write_managed_node(const std::string& path,
                            const std::string& target,
                            uint64_t now_ms,
                            double numeric_tolerance = 0.0,
                            bool verify_nonzero_mask = false,
                            double deadband = 0.0) noexcept;
    void rollback_managed_node(const std::string& path) noexcept;

    BaselineStore& baseline_;
    std::string procfs_root_;
    v6::OemCollaborationMap* oem_map_{nullptr};
    const v6::CausalVerifier* causal_verifier_{nullptr};
    std::unordered_map<std::string, std::string> last_written_;
    std::unordered_map<std::string, ContentionEntry> contended_paths_;
    std::unordered_map<std::string, uint64_t> last_preread_ms_;
    std::unordered_map<std::string, uint64_t> last_predictive_ms_;
    std::unordered_map<std::string, std::vector<std::string>> cached_pkg_assets_;
    uint64_t last_compaction_ms_{0};
    StorageMemoryStats stats_{};
};

} // namespace coreflow::v5
