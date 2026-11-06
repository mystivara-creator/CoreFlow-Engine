/**
 * @file hardware_longevity.hpp
 * @brief Battery Thermal Step-Charging & UFS Storage Longevity Subsystem
 * @author mystivara-creator
 * @license Apache-2.0
 */

#pragma once

#include <string>
#include <mutex>
#include <vector>
#include <cstdint>
#include "types.hpp"
#include <string_view>

namespace coreflow {
enum class MutationDomain : uint8_t;
enum class ControllerPriority : uint8_t;

struct LongevityConfig {
    bool enable_battery_step_charging{true};
    bool enable_ufs_burst_pacing{true};
    float battery_step_temp_threshold{41.5f}; ///< Thermal threshold to step down charging
    uint32_t stepped_charge_current_ma{1500};   ///< Reduced charging current during high loads
    uint32_t top_app_io_weight{800};             ///< cgroups v2 I/O priority for foreground
    uint32_t background_io_weight{50};           ///< cgroups v2 I/O limit for background apps
};

class HardwareLongevityManager {
public:
    explicit HardwareLongevityManager(std::string sys_root = "/sys");
    ~HardwareLongevityManager();
    HardwareLongevityManager(const HardwareLongevityManager&) = delete;
    HardwareLongevityManager& operator=(const HardwareLongevityManager&) = delete;
    static HardwareLongevityManager& instance();

    /// Dynamically discover battery and storage control nodes
    void initialize(const LongevityConfig& config);

    /// Evaluate battery thermal load and step-charge if overheating
    bool monitor_battery_thermals(float current_soc_temp);

    /// Apply UFS storage write pacing to minimize write amplification and frame stutter
    bool apply_storage_pacing();

    /// Revert any battery/storage changes back to OEM factory baseline
    void restore_oem_baseline();

    [[nodiscard]] bool is_battery_guarded() const noexcept;
    [[nodiscard]] bool is_storage_paced() const noexcept;
    [[nodiscard]] std::string battery_node_path() const;

private:
    std::string root_;
    mutable std::mutex mutex_;

    LongevityConfig config_;
    std::string battery_node_;
    bool battery_stepped_{false};
    bool storage_paced_{false};
    uint64_t charge_baseline_ua_{0};
    uint64_t last_charge_ua_{0};
    bool battery_mutation_active_{false};
    std::string battery_transaction_id_;
    std::string storage_transaction_id_;
    uint64_t transaction_generation_{0};
    std::vector<std::string> storage_mutation_paths_;

    void discover_battery_node();
    bool acquire_lease_locked(MutationDomain domain, ControllerPriority priority,
                              std::string& transaction_id, std::string_view prefix);
    void release_lease_locked(MutationDomain domain, std::string& transaction_id) noexcept;
    bool rollback_battery_locked() noexcept;
    bool rollback_storage_locked() noexcept;
    bool handle_preemption(MutationDomain domain, std::string_view transaction_id) noexcept;
};

} // namespace coreflow

