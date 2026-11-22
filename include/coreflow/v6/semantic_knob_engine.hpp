/**
 * @file semantic_knob_engine.hpp
 * @brief CoreFlow v6.1 Dynamic Subsystem Scanner + Semantic Knob Classifier & Demand-Driven Actuator.
 * @license Apache-2.0
 */
#pragma once

#include "coreflow/baseline_store.hpp"
#include "coreflow/mutation_coordinator.hpp"
#include "coreflow/v5/context_engine.hpp"
#include "coreflow/v5/hardware_probe.hpp"
#include "coreflow/v6/oem_collaboration_map.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace coreflow::v6 {

enum class KnobFamily : uint8_t {
    CpuBoost = 0,
    GpuLatency = 1,
    StorageLatency = 2,
    MemoryCache = 3,
};

enum class KnobValueKind : uint8_t {
    BoolToggle = 0,     // 0/1 or Y/N toggle
    IntLevel = 1,       // Small discrete level (e.g. adrenoboost 0..3, rq_affinity 1..2)
    RelativeInt = 2,    // Integer adjusted proportionally relative to OEM baseline
    MigratePair = 3,    // WALT/EAS multi-cluster threshold pair ("85 95")
    OppFreq = 4,        // Frequency chosen strictly from discovered cluster/bus OPP table
};

struct DiscoveredKnob {
    std::string path;
    std::string leaf;
    KnobFamily family{KnobFamily::CpuBoost};
    KnobValueKind kind{KnobValueKind::BoolToggle};
    std::string baseline_value;
    int64_t baseline_int{0};
    int64_t active_burst_value{0};
    int64_t active_gaming_value{0};
    int64_t active_interactive_value{0};
    bool apply_in_launch_burst{false};
    bool apply_in_gaming{false};
    bool apply_in_interactive{false};
    bool uses_yn_bool{false};
    bool is_memory_bus_opp{false};
    std::vector<uint64_t> opp_table_khz;
};

struct SemanticKnobStats {
    uint32_t discovered_total{0};
    uint32_t active_managed{0};
    uint32_t cpu_knobs{0};
    uint32_t gpu_knobs{0};
    uint32_t storage_knobs{0};
    uint32_t memory_knobs{0};
    uint32_t memory_bus_knobs{0};
};

struct SymbioticHints {
    bool boost_gpu_memory_supply{false};
    bool encourage_cluster_spread{false};
    double memory_bus_scale{1.0};               // 1.00..1.25 coupled memory bus floor scale
    double scheduler_responsiveness_scale{1.0}; // 0.70..1.00 coupled scheduler responsiveness
    double thermal_stepdown_lead{0.0};          // 0.00..0.15 coupled thermal headroom lead
};

class SemanticKnobEngine {
public:
    SemanticKnobEngine(BaselineStore& baseline,
                       OemCollaborationMap& oem_map,
                       CentralMutationCoordinator* coordinator = nullptr) noexcept;

    // Dynamically scans safe CPU/WALT/EAS, GPU/Adreno/GED, UFS/Block, VM, and devfreq memory bus
    // subsystems, classifying discovered writable nodes with OEM-relative safe bounds.
    void discover(const v5::HardwareTopology& topo,
                  const std::string& sysfs_root = "/sys",
                  const std::string& procfs_root = "/proc") noexcept;

    // Instant Reflex + Release-on-Calm + Sovereign Tamper Guard actuator:
    //  - Activates relevant discovered knobs immediately on LaunchBurst / Gaming / Interactive load.
    //  - Locks actively leased nodes to 0444 (read-only) and enforces Reclaim-First + 3x/5s breaker
    //    if an external tool mutates them (outside passive OemCollaborationMap nodes).
    //  - Releases ALL managed semantic knobs (restoring original permissions + factory values) on
    //    Efficiency / ScreenOff / Excluded / ThermalGuard / thermal throttling (capacity_scale < 0.90).
    void apply(v5::ContextMode mode,
               v5::PerformanceProfile profile,
               double capacity_scale,
               bool thermal_guard,
               bool enabled,
               double avail_ram_ratio,
               double mem_psi,
               uint64_t now_ms,
               bool passive_media_only = false,
               const SymbioticHints& hints = {}) noexcept;

    // Clears quarantine and immediately reclaims + re-locks a leased node on RESYNC / ENFORCE.
    bool force_resync_node(const std::string& path, uint64_t now_ms) noexcept;

    void rollback_all() noexcept;

    [[nodiscard]] SemanticKnobStats stats() const noexcept { return stats_; }
    [[nodiscard]] const std::vector<DiscoveredKnob>& knobs() const noexcept { return knobs_; }
    [[nodiscard]] static const char* family_name(KnobFamily family) noexcept;
    [[nodiscard]] static bool is_forbidden_knob_name(std::string_view leaf) noexcept;

private:
    bool write_knob(const DiscoveredKnob& knob,
                    const std::string& target,
                    uint64_t now_ms) noexcept;
    void rollback_knob(const std::string& path) noexcept;
    void try_register_node(const std::string& path,
                           const std::string& leaf,
                           const std::vector<uint64_t>& cluster_opps,
                           std::unordered_map<std::string, bool>& seen) noexcept;
    void try_register_memory_bus_node(const v5::MemoryBusNodeInfo& bus,
                                      std::unordered_map<std::string, bool>& seen) noexcept;

    BaselineStore& baseline_;
    OemCollaborationMap& oem_map_;
    CentralMutationCoordinator* coordinator_{nullptr};
    std::vector<DiscoveredKnob> knobs_;
    std::unordered_map<std::string, std::string> last_written_;
    SemanticKnobStats stats_{};
};

} // namespace coreflow::v6
