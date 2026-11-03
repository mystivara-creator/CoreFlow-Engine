#include "coreflow/hardware_longevity.hpp"
#include "coreflow/baseline_store.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <unistd.h>

namespace coreflow {
namespace {
std::string line(const std::string& path) {
    std::ifstream file(path); std::string value;
    std::getline(file, value); return value;
}
bool positive(const std::string& text, uint64_t& value) {
    auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size() && value > 0;
}
}
HardwareLongevityManager& HardwareLongevityManager::instance() {
    static HardwareLongevityManager manager; return manager;
}
void HardwareLongevityManager::initialize(const LongevityConfig& config) {
    config_ = config;
    battery_stepped_ = false; storage_paced_ = false;
    discover_battery_node();
}
void HardwareLongevityManager::discover_battery_node() {
    battery_node_.clear();
    if (!config_.enable_battery_step_charging) return;
    // constant_charge_current_max has defined microamp units. Do not guess the
    // meaning of vendor current_max nodes or use BMS measurements as actuators.
    const std::string candidate = root_ + "/class/power_supply/battery/constant_charge_current_max";
    uint64_t current = 0;
    if (::access(candidate.c_str(), R_OK | W_OK) == 0 && positive(line(candidate), current) &&
        current <= 20000000 && BaselineStore::instance().capture(candidate)) {
        battery_node_ = candidate;
        charge_baseline_ua_ = current;
        last_charge_ua_ = current;
    }
}
bool HardwareLongevityManager::monitor_battery_thermals(float soc_temp) {
    if (!config_.enable_battery_step_charging || battery_node_.empty()) return false;
    auto& store = BaselineStore::instance();
    uint64_t current = 0;
    if (!positive(line(battery_node_), current) || current != last_charge_ua_) {
        store.rollback(battery_node_);
        battery_node_.clear(); battery_stepped_ = false;
        return false;
    }
    double raw_temp = 0;
    const bool battery_valid = parse_ascii_double(line(root_ + "/class/power_supply/battery/temp"), raw_temp)
        && raw_temp >= 0 && raw_temp <= 800; // power_supply temperature: tenths Celsius
    const bool charging = line(root_ + "/class/power_supply/battery/status") == "Charging";
    if (!battery_valid || !charging || !std::isfinite(soc_temp) || soc_temp < 0) {
        if (battery_stepped_ && store.rollback(battery_node_)) {
            battery_stepped_ = false; last_charge_ua_ = charge_baseline_ua_;
        }
        return false;
    }
    const auto temperature = std::max(static_cast<double>(soc_temp), raw_temp / 10.0);
    if (temperature >= config_.battery_step_temp_threshold && !battery_stepped_) {
        const auto target = std::min(charge_baseline_ua_, static_cast<uint64_t>(config_.stepped_charge_current_ma) * 1000);
        if (target >= current) return false; // Never raise an OEM charging limit.
        if (store.apply_mutation(battery_node_, std::to_string(target))) {
            last_charge_ua_ = target; battery_stepped_ = true; return true;
        }
    } else if (temperature <= config_.battery_step_temp_threshold - 3.0 && battery_stepped_) {
        if (store.rollback(battery_node_)) {
            last_charge_ua_ = charge_baseline_ua_; battery_stepped_ = false; return true;
        }
    }
    return false;
}
bool HardwareLongevityManager::apply_storage_pacing() {
    if (!config_.enable_ufs_burst_pacing) return true;
    if (storage_paced_) return true;
    auto& store = BaselineStore::instance();
    bool available = false;
    for (const auto& [group, weight] : {
        std::pair{"top-app", config_.top_app_io_weight},
        std::pair{"background", config_.background_io_weight}}) {
        const std::string path = root_ + "/fs/cgroup/" + group + "/io.weight";
        const auto before = line(path);
        if (before.rfind("default ", 0) != 0 || ::access(path.c_str(), W_OK) != 0) continue;
        uint64_t old_weight = 0;
        if (!positive(before.substr(8), old_weight) || old_weight > 10000) continue;
        if (!store.capture(path) || !store.apply_mutation(path, "default " + std::to_string(std::clamp(weight, 1u, 10000u)))) return false;
        available = true;
    }
    // io.weight is relative arbitration, not a bandwidth cap or a UFS health guarantee.
    // Global dirty-ratio writes are intentionally avoided: dirty_bytes may override them.
    storage_paced_ = available;
    return true;
}
void HardwareLongevityManager::restore_oem_baseline() {
    if (!battery_node_.empty() && battery_stepped_ && BaselineStore::instance().rollback(battery_node_)) battery_stepped_ = false;
    storage_paced_ = false;
}
}
