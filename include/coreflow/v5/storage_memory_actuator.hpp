/**
 * @file storage_memory_actuator.hpp
 * @brief CoreFlow v5 Pillar 3B: Dynamic UFS/eMMC Burst, Background QoS, MGLRU Anti-Stutter, and Smart Pre-Read.
 * @license MIT
 */
#pragma once

#include "coreflow/baseline_store.hpp"
#include "coreflow/v5/context_engine.hpp"
#include "coreflow/v5/hardware_probe.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace coreflow::v5 {

struct StorageMemoryStats {
    uint64_t active_read_ahead_kb{0};
    bool burst_active{false};
    bool background_isolated{false};
    uint64_t preread_files_total{0};
    uint64_t preread_bytes_total{0};
    uint32_t contended_nodes{0};
};

class StorageMemoryActuator {
public:
    StorageMemoryActuator(BaselineStore& baseline,
                          std::string procfs_root = "/proc") noexcept;

    void apply(const HardwareTopology& topo,
               const ContextDecision& ctx,
               PerformanceProfile profile,
               bool enable_storage_burst,
               bool enable_memory_guard,
               uint64_t now_ms) noexcept;

    // Smart asynchronous kernel page-cache prefetch for a newly launched foreground app.
    // Returns total bytes advised via posix_fadvise(POSIX_FADV_WILLNEED).
    uint64_t prefetch_app_assets(int32_t pid,
                                 const std::string& package_name,
                                 uint64_t total_ram_bytes,
                                 uint64_t now_ms,
                                 bool allow_any_path_for_test = false) noexcept;

    void reset_contention() noexcept;
    [[nodiscard]] StorageMemoryStats stats() const noexcept { return stats_; }

    [[nodiscard]] static uint64_t compute_read_ahead_kb(
        StorageType type,
        ContextMode mode,
        PerformanceProfile profile) noexcept;

private:
    bool write_managed_node(const std::string& path, const std::string& target) noexcept;
    void rollback_managed_node(const std::string& path) noexcept;

    BaselineStore& baseline_;
    std::string procfs_root_;
    std::unordered_map<std::string, std::string> last_written_;
    std::unordered_set<std::string> contended_paths_;
    std::unordered_map<std::string, uint64_t> last_preread_ms_;
    StorageMemoryStats stats_{};
};

} // namespace coreflow::v5
