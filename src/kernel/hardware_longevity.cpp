/**
 * @file hardware_longevity.cpp
 * @brief Dynamic Battery Step-Charging and UFS Storage Longevity Controller
 * @author mystivara-creator
 * @license Apache-2.0
 */

#include "coreflow/hardware_longevity.hpp"
#include "coreflow/baseline_store.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace coreflow {

namespace fs = std::filesystem;

HardwareLongevityManager& HardwareLongevityManager::instance() {
    static HardwareLongevityManager inst;
    return inst;
}

void HardwareLongevityManager::initialize(const LongevityConfig& config) {
    config_ = config;
    discover_battery_node();

    if (config_.enable_ufs_burst_pacing) {
        apply_storage_pacing();
    }
}

void HardwareLongevityManager::discover_battery_node() {
    battery_node_.clear();

    const std::vector<std::string> candidates = {
        "/sys/class/power_supply/battery/current_max",
        "/sys/class/power_supply/battery/constant_charge_current_max",
        "/sys/class/power_supply/bms/current_max",
        "/sys/class/power_supply/main/current_max"
    };

    for (const auto& path : candidates) {
        if (fs::exists(path)) {
            battery_node_ = path;
            std::cout << "[CoreFlow][Longevity] Discovered battery charge controller at: " 
                      << battery_node_ << std::endl;
            // Capture OEM initial value into transactional BaselineStore
            BaselineStore::instance().capture(battery_node_);
            break;
        }
    }

    if (battery_node_.empty()) {
        std::cout << "[CoreFlow][Longevity] Standard battery current sysfs not exposed on this kernel. Longevity guard inactive." << std::endl;
    }
}

bool HardwareLongevityManager::monitor_battery_thermals(float current_soc_temp) {
    if (!config_.enable_battery_step_charging || battery_node_.empty()) {
        return false;
    }

    // If thermal load is elevated, step down charging current to protect lithium cells
    if (current_soc_temp >= config_.battery_step_temp_threshold) {
        if (!battery_stepped_) {
            std::string target_ua = std::to_string(config_.stepped_charge_current_ma * 1000); // mA to uA
            if (BaselineStore::instance().apply_mutation(battery_node_, target_ua)) {
                battery_stepped_ = true;
                std::cout << "[CoreFlow][Longevity] Elevated thermal (" << current_soc_temp 
                          << "°C): Stepped charging current to " 
                          << config_.stepped_charge_current_ma << "mA" << std::endl;
                return true;
            }
        }
    } else if (current_soc_temp <= (config_.battery_step_temp_threshold - 3.0f)) {
        // Temperature cooled down: restore OEM factory charging speed
        if (battery_stepped_) {
            BaselineStore::instance().rollback(battery_node_);
            battery_stepped_ = false;
            std::cout << "[CoreFlow][Longevity] Thermals normalized: Restored OEM charging baseline." << std::endl;
            return true;
        }
    }

    return false;
}

bool HardwareLongevityManager::apply_storage_pacing() {
    bool ok = false;

    // 1. Prioritize UFS I/O for top-app and restrict background write bursts
    const std::string top_app_io = "/sys/fs/cgroup/top-app/io.weight";
    const std::string bg_io = "/sys/fs/cgroup/background/io.weight";

    if (fs::exists(top_app_io)) {
        BaselineStore::instance().capture(top_app_io);
        BaselineStore::instance().apply_mutation(top_app_io, std::to_string(config_.top_app_io_weight));
        ok = true;
    }

    if (fs::exists(bg_io)) {
        BaselineStore::instance().capture(bg_io);
        BaselineStore::instance().apply_mutation(bg_io, std::to_string(config_.background_io_weight));
        ok = true;
    }

    // 2. Tune Linux kernel VM dirty ratios to prevent large I/O blocking flushes
    const std::string dirty_ratio = "/proc/sys/vm/dirty_ratio";
    const std::string dirty_bg = "/proc/sys/vm/dirty_background_ratio";

    if (fs::exists(dirty_ratio)) {
        BaselineStore::instance().capture(dirty_ratio);
        BaselineStore::instance().apply_mutation(dirty_ratio, "15");
    }

    if (fs::exists(dirty_bg)) {
        BaselineStore::instance().capture(dirty_bg);
        BaselineStore::instance().apply_mutation(dirty_bg, "5");
    }

    storage_paced_ = ok;
    if (ok) {
        std::cout << "[CoreFlow][Longevity] UFS Storage write pacing active (cgroups v2 io.weight)." << std::endl;
    }
    return ok;
}

void HardwareLongevityManager::restore_oem_baseline() {
    if (!battery_node_.empty() && battery_stepped_) {
        BaselineStore::instance().rollback(battery_node_);
        battery_stepped_ = false;
    }
    storage_paced_ = false;
}

} // namespace coreflow
