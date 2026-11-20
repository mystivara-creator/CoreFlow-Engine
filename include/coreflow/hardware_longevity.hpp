/**
 * @file hardware_longevity.hpp
 * @brief Battery Thermal Step-Charging & UFS Storage Longevity Subsystem
 * @author mystivara-creator
 * @license Apache-2.0
 */

#pragma once

#include <string>
#include <chrono>
#include <mutex>
#include <vector>
#include <cstdint>
#include "types.hpp"
#include <string_view>

namespace coreflow {
class CentralMutationCoordinator;
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

struct BatteryPowerTelemetry {
    bool available{false};
    bool charging{false};
    bool fast_charging{false};
    bool thermal_step_active{false};
    double temperature_celsius{-1.0};
    double current_ma{0.0};
    double voltage_mv{0.0};
    double power_mw{0.0};
    uint64_t charge_limit_ua{0};
    uint64_t oem_baseline_ua{0};
    int battery_capacity_pct{-1};
    bool bypass_supported{false};
    bool bypass_active{false};
    uint32_t charge_limit_pct{0};
    std::string bypass_reason{"off"};
    std::string bypass_node_path{};
    std::string status{"Unknown"};
};

class HardwareLongevityManager {
public:
    explicit HardwareLongevityManager(CentralMutationCoordinator& coordinator, std::string sys_root = "/sys",
                                      std::string proc_root = "/proc");
    ~HardwareLongevityManager();
    HardwareLongevityManager(const HardwareLongevityManager&) = delete;
    HardwareLongevityManager& operator=(const HardwareLongevityManager&) = delete;

    /// Dynamically discover battery and storage control nodes
    void initialize(const LongevityConfig& config);

    /// Evaluate battery thermal load and step-charge if overheating
    bool monitor_battery_thermals(float current_soc_temp);

    /// Apply CFJ4-journaled Bypass Charging & Charge Ceiling % (fail-closed when unsupported)
    bool apply_power_control(bool manual_bypass, bool gaming_mode,
                             bool bypass_on_gaming, uint32_t charge_limit_pct);

    /// Apply UFS storage write pacing to minimize write amplification and frame stutter
    bool apply_storage_pacing();

    /// Revert any battery/storage/bypass changes back to OEM factory baseline
    void restore_oem_baseline();

    [[nodiscard]] bool is_battery_guarded() const noexcept;
    [[nodiscard]] bool is_bypass_supported() const noexcept;
    [[nodiscard]] bool is_bypass_active() const noexcept;
    [[nodiscard]] bool is_storage_paced() const noexcept;
    [[nodiscard]] std::string battery_node_path() const;
    [[nodiscard]] BatteryPowerTelemetry sample_battery_telemetry() const;
    [[nodiscard]] static BatteryPowerTelemetry probe_battery_telemetry(std::string_view sys_root = "/sys");

private:
    CentralMutationCoordinator& coordinator_;
    std::string root_;
    std::string proc_root_;
    std::string controller_name_;
    mutable std::mutex mutex_;

    LongevityConfig config_;
    std::string battery_dir_;
    std::string battery_node_;
    std::string battery_temp_node_;
    std::string battery_status_node_;
    std::string battery_current_node_;
    std::string battery_voltage_node_;
    std::string battery_capacity_node_;
    std::string bypass_node_;
    std::string bypass_enable_val_;
    std::string bypass_disable_val_;
    bool battery_stepped_{false};
    bool bypass_active_{false};
    bool ceiling_latched_{false};
    uint32_t active_charge_limit_pct_{0};
    std::string bypass_reason_{"off"};
    bool storage_paced_{false};
    uint64_t charge_baseline_ua_{0};
    uint64_t last_charge_ua_{0};
    bool battery_mutation_active_{false};
    std::chrono::steady_clock::time_point next_storage_probe_{};
    std::string battery_transaction_id_;
    std::string storage_transaction_id_;
    uint64_t transaction_generation_{0};
    std::vector<std::string> storage_mutation_paths_;

    void discover_battery_node();
    bool acquire_lease_locked(MutationDomain domain, ControllerPriority priority,
                              std::string& transaction_id, std::string_view prefix);
    void release_lease_locked(MutationDomain domain, std::string& transaction_id) noexcept;
    bool rollback_battery_locked() noexcept;
    bool rollback_bypass_locked() noexcept;
    bool rollback_storage_locked() noexcept;
    bool handle_preemption(MutationDomain domain, std::string_view transaction_id) noexcept;
};

} // namespace coreflow

