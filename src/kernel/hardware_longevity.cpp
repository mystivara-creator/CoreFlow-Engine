#include "coreflow/hardware_longevity.hpp"
#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/baseline_store.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <unistd.h>
#include <utility>

namespace coreflow {
namespace {
std::string line(const std::string& path) {
    std::ifstream file(path);
    std::string value;
    std::getline(file, value);
    return value;
}

bool positive(const std::string& text, uint64_t& value) {
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size() && value > 0;
}

const NodeSnapshot* find_snapshot(const std::vector<NodeSnapshot>& audit,
                                  const std::string& path) {
    const auto it = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
        return node.path == path;
    });
    return it == audit.end() ? nullptr : &*it;
}
} // namespace

HardwareLongevityManager::HardwareLongevityManager(std::string sys_root)
    : root_(std::move(sys_root)),
      controller_name_("HardwareLongevityManager@" +
                       std::to_string(reinterpret_cast<uintptr_t>(this))) {
    (void)CentralMutationCoordinator::instance().register_preemption_handler(
        controller_name_,
        [this](MutationDomain domain, std::string_view transaction_id, std::string_view) noexcept {
            return handle_preemption(domain, transaction_id);
        });
}

HardwareLongevityManager::~HardwareLongevityManager() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!battery_transaction_id_.empty()) (void)rollback_battery_locked();
        if (!storage_transaction_id_.empty()) (void)rollback_storage_locked();
    }
    // unregister waits for any callback that was already dispatched.
    CentralMutationCoordinator::instance().unregister_preemption_handler(controller_name_);
}

HardwareLongevityManager& HardwareLongevityManager::instance() {
    static HardwareLongevityManager manager;
    return manager;
}

bool HardwareLongevityManager::acquire_lease_locked(
    MutationDomain domain, ControllerPriority priority,
    std::string& transaction_id, std::string_view prefix) {
    auto& coordinator = CentralMutationCoordinator::instance();
    if (!transaction_id.empty()) {
        return coordinator.acquire_lease(domain, controller_name_, transaction_id, priority);
    }
    transaction_id = std::string(prefix) + "-" + std::to_string(++transaction_generation_);
    if (!coordinator.acquire_lease(domain, controller_name_, transaction_id, priority)) {
        transaction_id.clear();
        return false;
    }
    return true;
}

void HardwareLongevityManager::release_lease_locked(
    MutationDomain domain, std::string& transaction_id) noexcept {
    if (transaction_id.empty()) return;
    CentralMutationCoordinator::instance().release_lease(domain, controller_name_, transaction_id);
    transaction_id.clear();
}

bool HardwareLongevityManager::rollback_battery_locked() noexcept {
    if (battery_transaction_id_.empty()) return !battery_mutation_active_;
    if (battery_node_.empty()) return false;

    auto& store = BaselineStore::instance();
    if (!store.rollback(battery_node_)) return false;
    const auto audit = store.get_audit_trail();
    const auto* snapshot = find_snapshot(audit, battery_node_);
    if (!snapshot || snapshot->is_modified || line(battery_node_) != snapshot->factory_value) {
        return false;
    }

    uint64_t restored = 0;
    if (!positive(line(battery_node_), restored)) return false;
    charge_baseline_ua_ = restored;
    last_charge_ua_ = restored;
    battery_stepped_ = false;
    battery_mutation_active_ = false;
    release_lease_locked(MutationDomain::PowerLongevity, battery_transaction_id_);
    return true;
}

bool HardwareLongevityManager::rollback_storage_locked() noexcept {
    if (storage_transaction_id_.empty()) return storage_mutation_paths_.empty();
    auto& store = BaselineStore::instance();
    for (const auto& path : storage_mutation_paths_) {
        if (!store.rollback(path)) return false;
        const auto audit = store.get_audit_trail();
        const auto* snapshot = find_snapshot(audit, path);
        if (!snapshot || snapshot->is_modified || line(path) != snapshot->factory_value) {
            return false;
        }
    }
    storage_mutation_paths_.clear();
    storage_paced_ = false;
    release_lease_locked(MutationDomain::BlockIo, storage_transaction_id_);
    return true;
}

bool HardwareLongevityManager::handle_preemption(
    MutationDomain domain, std::string_view transaction_id) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (domain == MutationDomain::PowerLongevity) {
        if (battery_transaction_id_.empty() || battery_transaction_id_ != transaction_id) return false;
        return rollback_battery_locked();
    }
    if (domain == MutationDomain::BlockIo) {
        if (storage_transaction_id_.empty() || storage_transaction_id_ != transaction_id) return false;
        return rollback_storage_locked();
    }
    return false;
}

void HardwareLongevityManager::initialize(const LongevityConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    if ((!battery_transaction_id_.empty() && !rollback_battery_locked()) ||
        (!storage_transaction_id_.empty() && !rollback_storage_locked())) {
        return;
    }
    config_ = config;
    battery_stepped_ = false;
    storage_paced_ = false;
    battery_mutation_active_ = false;
    discover_battery_node();
}

void HardwareLongevityManager::discover_battery_node() {
    battery_node_.clear();
    if (!config_.enable_battery_step_charging) return;
    // constant_charge_current_max has defined microamp units. Do not guess the
    // meaning of vendor current_max nodes or use BMS measurements as actuators.
    const std::string candidate =
        root_ + "/class/power_supply/battery/constant_charge_current_max";
    uint64_t current = 0;
    auto& store = BaselineStore::instance();
    if (::access(candidate.c_str(), R_OK | W_OK) == 0 &&
        positive(line(candidate), current) && current <= 20000000 &&
        store.capture(candidate)) {
        const auto audit = store.get_audit_trail();
        const auto* snapshot = find_snapshot(audit, candidate);
        // A journaled value from another or unresolved owner is not an OEM
        // charging baseline and must never be adopted by this manager.
        if (!snapshot || snapshot->is_modified) return;
        battery_node_ = candidate;
        charge_baseline_ua_ = current;
        last_charge_ua_ = current;
    }
}

bool HardwareLongevityManager::monitor_battery_thermals(float soc_temp) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!config_.enable_battery_step_charging || battery_node_.empty()) return false;

    auto& store = BaselineStore::instance();
    uint64_t current = 0;
    if (!positive(line(battery_node_), current)) {
        if (!battery_transaction_id_.empty()) (void)rollback_battery_locked();
        battery_node_.clear();
        battery_stepped_ = false;
        return false;
    }
    if (current != last_charge_ua_) {
        // An OEM/thermal service changed the limit. Relinquish only after the
        // journal confirms ownership loss and the current value is preserved.
        if (!battery_transaction_id_.empty() && !rollback_battery_locked()) return false;
        battery_node_.clear();
        battery_stepped_ = false;
        return false;
    }

    double raw_temp = 0;
    const bool battery_valid =
        parse_ascii_double(line(root_ + "/class/power_supply/battery/temp"), raw_temp) &&
        raw_temp >= 0 && raw_temp <= 800; // power_supply temperature: tenths Celsius
    (void)soc_temp; // Charge policy uses battery temperature, never CPU junction temperature.
    const bool charging = line(root_ + "/class/power_supply/battery/status") == "Charging";
    if (!battery_valid || !charging) {
        if (battery_stepped_) (void)rollback_battery_locked();
        return false;
    }

    const double temperature = raw_temp / 10.0;
    if (temperature >= config_.battery_step_temp_threshold && !battery_stepped_) {
        const auto target = std::min(
            charge_baseline_ua_,
            static_cast<uint64_t>(config_.stepped_charge_current_ma) * 1000);
        if (target >= current) return false; // Never raise an OEM charging limit.
        if (!acquire_lease_locked(
                MutationDomain::PowerLongevity,
                ControllerPriority::ThermalStabilization,
                battery_transaction_id_, "battery-charge")) {
            return false;
        }
        const bool applied = store.apply_mutation(battery_node_, std::to_string(target));
        uint64_t readback = 0;
        if (!positive(line(battery_node_), readback) || readback != target) {
            battery_mutation_active_ = readback != charge_baseline_ua_;
            if (battery_mutation_active_) (void)rollback_battery_locked();
            else release_lease_locked(MutationDomain::PowerLongevity, battery_transaction_id_);
            return false;
        }
        if (!applied) {
            battery_mutation_active_ = true;
            return false;
        }
        last_charge_ua_ = target;
        battery_stepped_ = true;
        battery_mutation_active_ = true;
        return true;
    }

    if (temperature <= config_.battery_step_temp_threshold - 3.0 && battery_stepped_) {
        return rollback_battery_locked();
    }
    return false;
}

bool HardwareLongevityManager::apply_storage_pacing() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!config_.enable_ufs_burst_pacing) return true;

    auto& store = BaselineStore::instance();
    if (!storage_transaction_id_.empty()) {
        const auto audit = store.get_audit_trail();
        for (const auto& path : storage_mutation_paths_) {
            const auto* snapshot = find_snapshot(audit, path);
            if (!snapshot || !store.owns_expected_state(path, snapshot->active_value)) {
                if (!rollback_storage_locked()) return false;
                return false; // OEM ownership changed; do not reassert in this cycle.
            }
        }
    }
    if (storage_paced_) return true;
    bool available = false;
    for (const auto& [group, weight] : {
        std::pair{"top-app", config_.top_app_io_weight},
        std::pair{"background", config_.background_io_weight}}) {
        const std::string path = root_ + "/fs/cgroup/" + group + "/io.weight";
        const auto before = line(path);
        if (before.rfind("default ", 0) != 0 || ::access(path.c_str(), W_OK) != 0) continue;
        uint64_t old_weight = 0;
        if (!positive(before.substr(8), old_weight) || old_weight > 10000) continue;
        if (!store.capture(path)) continue;
        const auto audit_before = store.get_audit_trail();
        const auto* snapshot_before = find_snapshot(audit_before, path);
        const bool path_already_owned = std::find(
            storage_mutation_paths_.begin(), storage_mutation_paths_.end(), path) !=
            storage_mutation_paths_.end();
        if (!snapshot_before ||
            (snapshot_before->is_modified && !path_already_owned)) {
            // Do not extend this controller's BlockIo transaction onto a path
            // already modified by another or unresolved owner.
            continue;
        }
        const std::string target =
            "default " + std::to_string(std::clamp(weight, 1u, 10000u));
        if (before == target) {
            available = true;
            continue;
        }
        if (!acquire_lease_locked(
                MutationDomain::BlockIo,
                ControllerPriority::WorkloadOptimization,
                storage_transaction_id_, "storage-pacing")) {
            if (!storage_mutation_paths_.empty()) (void)rollback_storage_locked();
            return false;
        }

        const bool applied = store.apply_mutation(path, target);
        const std::string readback = line(path);
        if (readback != before &&
            std::find(storage_mutation_paths_.begin(), storage_mutation_paths_.end(), path) ==
                storage_mutation_paths_.end()) {
            storage_mutation_paths_.push_back(path);
        }
        if (!applied || readback != target) {
            if (storage_mutation_paths_.empty()) {
                release_lease_locked(MutationDomain::BlockIo, storage_transaction_id_);
            } else {
                (void)rollback_storage_locked();
            }
            return false;
        }
        available = true;
    }

    storage_paced_ = available;
    if (storage_mutation_paths_.empty() && !storage_transaction_id_.empty()) {
        release_lease_locked(MutationDomain::BlockIo, storage_transaction_id_);
    }
    return true;
}

void HardwareLongevityManager::restore_oem_baseline() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!battery_transaction_id_.empty()) (void)rollback_battery_locked();
    if (!storage_transaction_id_.empty()) (void)rollback_storage_locked();
    if (storage_transaction_id_.empty() && storage_mutation_paths_.empty()) storage_paced_ = false;
}

bool HardwareLongevityManager::is_battery_guarded() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return battery_stepped_;
}

bool HardwareLongevityManager::is_storage_paced() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return storage_paced_;
}

std::string HardwareLongevityManager::battery_node_path() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return battery_node_;
}

} // namespace coreflow
