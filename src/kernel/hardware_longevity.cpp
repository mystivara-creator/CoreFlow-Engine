#include "coreflow/hardware_longevity.hpp"
#include "coreflow/mutation_coordinator.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/mount_topology.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <system_error>
#include <sys/stat.h>
#include <fstream>
#include <unistd.h>
#include <utility>

namespace coreflow {
namespace fs = std::filesystem;
namespace {
std::string line(const std::string& path) {
    std::ifstream file(path);
    std::string value;
    std::getline(file, value);
    while (!value.empty() && (value.back() == '\r' || value.back() == ' ' || value.back() == '\t')) {
        value.pop_back();
    }
    size_t start = 0;
    while (start < value.size() && (value[start] == ' ' || value[start] == '\t')) {
        ++start;
    }
    return start > 0 ? value.substr(start) : value;
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

fs::path canonical_under(const fs::path& input, const fs::path& root) {
    std::error_code ec;
    const auto canonical_root = fs::canonical(root, ec);
    if (ec) return {};
    const auto canonical = fs::canonical(input, ec);
    if (ec || !fs::is_directory(canonical, ec) || ec) return {};
    const auto relative = canonical.lexically_relative(canonical_root);
    if (relative.empty() || relative.native().rfind("..", 0) == 0) return {};
    return canonical;
}

bool regular_non_symlink(const fs::path& path) noexcept {
    struct stat st{};
    return ::lstat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode);
}

struct DiscoveredBatteryPaths {
    std::string dir;
    std::string temp_node;
    std::string status_node;
    std::string current_node;
    std::string voltage_node;
    std::string charge_max_node;
    std::string capacity_node;
    std::string bypass_node;
    std::string bypass_enable_val;
    std::string bypass_disable_val;
    bool valid{false};
};

void probe_bypass_node(const fs::path& dir, DiscoveredBatteryPaths& item) {
    struct Candidate {
        const char* leaf;
        const char* enable_val;
        const char* disable_val;
    };
    constexpr Candidate kCandidates[] = {
        {"input_suspend", "1", "0"},
        {"charging_enabled", "0", "1"},
        {"battery_charging_enabled", "0", "1"},
    };
    for (const auto& cand : kCandidates) {
        const fs::path p = dir / cand.leaf;
        if (!regular_non_symlink(p)) continue;
        if (::access(p.c_str(), R_OK | W_OK) != 0) continue;
        const std::string val = line(p.string());
        if (val != "0" && val != "1") continue;
        item.bypass_node = p.string();
        item.bypass_enable_val = cand.enable_val;
        item.bypass_disable_val = cand.disable_val;
        return;
    }
}

DiscoveredBatteryPaths find_unambiguous_battery_supply(const fs::path& sys_root) {
    const fs::path supply_root = sys_root / "class/power_supply";
    std::error_code ec;
    std::vector<fs::path> entries;
    for (fs::directory_iterator it(supply_root, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end && entries.size() < 64U; it.increment(ec)) {
        entries.push_back(it->path());
    }
    std::sort(entries.begin(), entries.end());

    std::vector<DiscoveredBatteryPaths> matches;
    for (const auto& entry : entries) {
        // Linux /sys/class/power_supply/* entries are typically symlinks into
        // /sys/devices/...; resolve strictly within sys_root to accept real
        // kernel layouts while rejecting escaping symlinks.
        const auto resolved = canonical_under(entry, sys_root);
        if (resolved.empty()) continue;
        const std::string resolved_str = resolved.string();
        const bool duplicate = std::any_of(matches.begin(), matches.end(),
            [&](const DiscoveredBatteryPaths& item) { return item.dir == resolved_str; });
        if (duplicate) continue;

        if (!regular_non_symlink(resolved / "type")) continue;
        if (line((resolved / "type").string()) != "Battery") continue;

        const auto temp_path = resolved / "temp";
        const auto status_path = resolved / "status";
        if (!regular_non_symlink(temp_path) || !regular_non_symlink(status_path)) continue;

        DiscoveredBatteryPaths item{};
        item.dir = resolved_str;
        item.temp_node = temp_path.string();
        item.status_node = status_path.string();
        const auto current_now_path = resolved / "current_now";
        const auto voltage_now_path = resolved / "voltage_now";
        const auto charge_max_path = resolved / "constant_charge_current_max";
        const auto capacity_path = resolved / "capacity";
        if (regular_non_symlink(current_now_path)) item.current_node = current_now_path.string();
        if (regular_non_symlink(voltage_now_path)) item.voltage_node = voltage_now_path.string();
        if (regular_non_symlink(charge_max_path)) item.charge_max_node = charge_max_path.string();
        if (regular_non_symlink(capacity_path)) item.capacity_node = capacity_path.string();
        probe_bypass_node(resolved, item);
        item.valid = true;
        matches.push_back(std::move(item));
    }

    if (matches.empty()) return {};
    if (matches.size() == 1U) return matches.front();

    // Dual-cell fast-charging phones often expose both an aggregate main "battery"
    // supply and secondary fuel-gauge/sub-cell supplies (e.g. bms, sub_battery).
    // Prefer a single unambiguous supply exposing writable charge control + telemetry,
    // or a single canonical "battery" aggregate supply; otherwise fail closed.
    std::vector<const DiscoveredBatteryPaths*> full_ctrl;
    const DiscoveredBatteryPaths* canonical_main = nullptr;
    size_t canonical_main_count = 0U;
    for (const auto& item : matches) {
        if (!item.charge_max_node.empty() &&
            (!item.current_node.empty() || !item.voltage_node.empty()) &&
            ::access(item.charge_max_node.c_str(), R_OK | W_OK) == 0) {
            full_ctrl.push_back(&item);
        }
        const std::string base = fs::path(item.dir).filename().string();
        if (base == "battery" || base == "BAT0") {
            canonical_main = &item;
            ++canonical_main_count;
        }
    }
    if (full_ctrl.size() == 1U) return *full_ctrl.front();
    if (canonical_main_count == 1U && canonical_main != nullptr) return *canonical_main;
    return {};
}

BatteryPowerTelemetry read_battery_telemetry(
    const std::string& battery_dir,
    const std::string& temp_node,
    const std::string& status_node,
    const std::string& current_node,
    const std::string& voltage_node,
    const std::string& capacity_node,
    const std::string& bypass_node,
    bool thermal_step_active,
    bool bypass_active,
    const std::string& bypass_reason,
    uint32_t charge_limit_pct,
    uint64_t charge_limit_ua,
    uint64_t oem_baseline_ua) {
    BatteryPowerTelemetry out{};
    if (temp_node.empty() || status_node.empty()) return out;

    double raw_temp = 0.0;
    if (parse_ascii_double(line(temp_node), raw_temp) && raw_temp >= 0.0 && raw_temp <= 800.0) {
        out.temperature_celsius = raw_temp / 10.0;
        out.available = true;
    }

    const std::string st = line(status_node);
    if (!st.empty()) {
        out.status = st;
        out.charging = (st == "Charging" || st == "Full");
        out.available = true;
    }

    const std::string cap_path = !capacity_node.empty()
        ? capacity_node
        : (!battery_dir.empty() ? (fs::path(battery_dir) / "capacity").string() : std::string{});
    if (!cap_path.empty() && regular_non_symlink(cap_path)) {
        double raw_cap = -1.0;
        if (parse_ascii_double(line(cap_path), raw_cap) && raw_cap >= 0.0 && raw_cap <= 100.0) {
            out.battery_capacity_pct = static_cast<int>(std::lround(raw_cap));
        }
    }

    bool fuel_gauge_uses_micro_units = (oem_baseline_ua > 20000U) || (charge_limit_ua > 20000U);
    if (!fuel_gauge_uses_micro_units && !battery_dir.empty()) {
        const auto charge_max_path = fs::path(battery_dir) / "constant_charge_current_max";
        if (regular_non_symlink(charge_max_path)) {
            uint64_t raw_max = 0;
            if (positive(line(charge_max_path.string()), raw_max) && raw_max > 20000U) {
                fuel_gauge_uses_micro_units = true;
            }
        }
    }
    const std::string volt_path = !voltage_node.empty()
        ? voltage_node
        : (!battery_dir.empty() ? (fs::path(battery_dir) / "voltage_now").string() : std::string{});
    if (!volt_path.empty() && regular_non_symlink(volt_path)) {
        double raw_volt = 0.0;
        if (parse_ascii_double(line(volt_path), raw_volt) && raw_volt > 0.0) {
            // Standard power_supply voltage_now is microvolts (uV).
            if (raw_volt > 20000.0) {
                fuel_gauge_uses_micro_units = true;
                out.voltage_mv = raw_volt / 1000.0;
            } else {
                out.voltage_mv = raw_volt;
            }
        }
    }

    const std::string cur_path = !current_node.empty()
        ? current_node
        : (!battery_dir.empty() ? (fs::path(battery_dir) / "current_now").string() : std::string{});
    if (!cur_path.empty() && regular_non_symlink(cur_path)) {
        double raw_cur = 0.0;
        if (parse_ascii_double(line(cur_path), raw_cur) && std::isfinite(raw_cur)) {
            // Linux power_supply current_now is microamperes (uA). When voltage_now,
            // oem_baseline_ua, charge_limit_ua, or constant_charge_current_max confirms
            // standard micro-unit ABI, always divide by 1000 so low idle/trickle currents
            // (e.g. 15000 uA = 15 mA) are never misreported as multi-amp fast charging.
            const double abs_cur = std::abs(raw_cur);
            out.current_ma = (fuel_gauge_uses_micro_units || abs_cur > 20000.0)
                ? (raw_cur / 1000.0)
                : raw_cur;
        }
    }

    if (out.voltage_mv > 0.0 && std::abs(out.current_ma) > 0.0) {
        out.power_mw = (std::abs(out.current_ma) * out.voltage_mv) / 1000.0;
    }

    out.fast_charging = (st == "Charging") && (std::abs(out.current_ma) >= 2200.0 || out.power_mw >= 10000.0);
    out.thermal_step_active = thermal_step_active;
    out.bypass_supported = !bypass_node.empty();
    out.bypass_active = bypass_active;
    out.bypass_reason = bypass_reason;
    out.bypass_node_path = bypass_node;
    out.charge_limit_pct = charge_limit_pct;
    out.charge_limit_ua = charge_limit_ua;
    out.oem_baseline_ua = oem_baseline_ua;
    return out;
}
} // namespace

HardwareLongevityManager::HardwareLongevityManager(CentralMutationCoordinator& coordinator, std::string sys_root,
                                                     std::string proc_root)
    : coordinator_(coordinator), root_(std::move(sys_root)), proc_root_(std::move(proc_root)),
      controller_name_(CentralMutationCoordinator::make_controller_identity("HardwareLongevityManager")) {
    (void)coordinator_.register_preemption_handler(
        controller_name_,
        [this](MutationDomain domain, std::string_view transaction_id, std::string_view) noexcept {
            return handle_preemption(domain, transaction_id);
        });
}

HardwareLongevityManager::~HardwareLongevityManager() {
    std::vector<PendingLeaseRelease> releases;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (bypass_active_) (void)rollback_bypass_locked(releases);
        if (battery_mutation_active_ || !battery_transaction_id_.empty()) {
            (void)rollback_battery_locked(releases);
        }
        if (!storage_transaction_id_.empty()) (void)rollback_storage_locked(releases);
    }
    flush_releases(releases);
    // unregister waits for any callback that was already dispatched.
    coordinator_.unregister_preemption_handler(controller_name_);
}

void HardwareLongevityManager::flush_releases(
    std::vector<PendingLeaseRelease>& releases) noexcept {
    for (const auto& item : releases) {
        if (!item.transaction_id.empty()) {
            coordinator_.release_lease(item.domain, controller_name_, item.transaction_id);
        }
    }
    releases.clear();
}

void HardwareLongevityManager::queue_power_release_if_idle_locked(
    std::vector<PendingLeaseRelease>& releases) {
    if (!battery_transaction_id_.empty() && !battery_mutation_active_ && !bypass_active_) {
        releases.push_back({MutationDomain::PowerLongevity, std::move(battery_transaction_id_)});
        battery_transaction_id_.clear();
    }
}

bool HardwareLongevityManager::rollback_bypass_locked(
    std::vector<PendingLeaseRelease>& releases) noexcept {
    if (bypass_node_.empty() || !bypass_active_) {
        bypass_active_ = false;
        bypass_reason_ = "off";
        queue_power_release_if_idle_locked(releases);
        return true;
    }
    auto& store = BaselineStore::instance();
    const bool ok = store.rollback(bypass_node_);
    if (ok) {
        bypass_active_ = false;
        bypass_reason_ = "off";
        queue_power_release_if_idle_locked(releases);
    }
    return ok;
}

bool HardwareLongevityManager::rollback_battery_locked(
    std::vector<PendingLeaseRelease>& releases) noexcept {
    if (!battery_mutation_active_) {
        battery_stepped_ = false;
        queue_power_release_if_idle_locked(releases);
        return true;
    }
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
    queue_power_release_if_idle_locked(releases);
    return true;
}

bool HardwareLongevityManager::rollback_storage_locked(
    std::vector<PendingLeaseRelease>& releases) noexcept {
    if (storage_transaction_id_.empty()) {
        if (storage_mutation_paths_.empty()) storage_paced_ = false;
        return storage_mutation_paths_.empty();
    }
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
    next_storage_probe_ = {};
    releases.push_back({MutationDomain::BlockIo, std::move(storage_transaction_id_)});
    storage_transaction_id_.clear();
    return true;
}

bool HardwareLongevityManager::handle_preemption(
    MutationDomain domain, std::string_view transaction_id) noexcept {
    std::vector<PendingLeaseRelease> releases;
    bool ok = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (domain == MutationDomain::PowerLongevity) {
            if (battery_transaction_id_.empty() || battery_transaction_id_ != transaction_id) {
                return false;
            }
            const bool bypass_ok = !bypass_active_ || rollback_bypass_locked(releases);
            const bool batt_ok = !battery_mutation_active_ || rollback_battery_locked(releases);
            ok = bypass_ok && batt_ok;
            if (ok && !battery_transaction_id_.empty()) {
                releases.push_back({MutationDomain::PowerLongevity, std::move(battery_transaction_id_)});
                battery_transaction_id_.clear();
            }
        } else if (domain == MutationDomain::BlockIo) {
            if (storage_transaction_id_.empty() || storage_transaction_id_ != transaction_id) {
                return false;
            }
            ok = rollback_storage_locked(releases);
        }
    }
    flush_releases(releases);
    return ok;
}

void HardwareLongevityManager::initialize(const LongevityConfig& config) {
    std::vector<PendingLeaseRelease> releases;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const bool rollback_ok =
            (!bypass_active_ || rollback_bypass_locked(releases)) &&
            ((!battery_mutation_active_ && battery_transaction_id_.empty()) ||
             rollback_battery_locked(releases)) &&
            (storage_transaction_id_.empty() || rollback_storage_locked(releases));
        if (rollback_ok) {
            config_ = config;
            battery_stepped_ = false;
            bypass_active_ = false;
            ceiling_latched_ = false;
            soc_floor_latched_ = false;
            active_charge_limit_pct_ = 0;
            bypass_reason_ = "off";
            storage_paced_ = false;
            battery_mutation_active_ = false;
            next_storage_probe_ = {};
            discover_battery_node();
        }
    }
    flush_releases(releases);
}

void HardwareLongevityManager::discover_battery_node() {
    battery_dir_.clear();
    battery_node_.clear();
    battery_temp_node_.clear();
    battery_status_node_.clear();
    battery_current_node_.clear();
    battery_voltage_node_.clear();
    battery_capacity_node_.clear();
    bypass_node_.clear();
    bypass_enable_val_.clear();
    bypass_disable_val_.clear();

    // Enumerate power-supply devices and select by the power_supply type ABI,
    // never by a vendor device-directory name. Only the standardized charging
    // current control is promoted to an actuator.
    const auto discovered = find_unambiguous_battery_supply(fs::path(root_));
    if (!discovered.valid) return;

    battery_dir_ = discovered.dir;
    battery_temp_node_ = discovered.temp_node;
    battery_status_node_ = discovered.status_node;
    battery_current_node_ = discovered.current_node;
    battery_voltage_node_ = discovered.voltage_node;
    battery_capacity_node_ = discovered.capacity_node;
    bypass_node_ = discovered.bypass_node;
    bypass_enable_val_ = discovered.bypass_enable_val;
    bypass_disable_val_ = discovered.bypass_disable_val;

    if (config_.enable_battery_step_charging && !discovered.charge_max_node.empty()) {
        const std::string& candidate = discovered.charge_max_node;
        uint64_t current = 0;
        auto& store = BaselineStore::instance();
        if (::access(candidate.c_str(), R_OK | W_OK) == 0 &&
            positive(line(candidate), current) && current <= 20000000U &&
            store.capture(candidate)) {
            const auto audit = store.get_audit_trail();
            const auto* snapshot = find_snapshot(audit, candidate);
            if (snapshot && !snapshot->is_modified) {
                battery_node_ = candidate;
                charge_baseline_ua_ = current;
                last_charge_ua_ = current;
            }
        }
    }
}

bool HardwareLongevityManager::monitor_battery_thermals(float soc_temp) {
    (void)soc_temp; // Charge policy uses battery temperature, never CPU junction temperature.
    std::vector<PendingLeaseRelease> releases;
    std::string tx_id;
    uint64_t target = 0;
    std::string target_node;
    bool need_step_down = false;
    bool rolled_back = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!config_.enable_battery_step_charging || battery_node_.empty()) return false;

        auto& store = BaselineStore::instance();
        const auto battery_audit = store.get_audit_trail();
        const auto* battery_snapshot = find_snapshot(battery_audit, battery_node_);
        if (!battery_snapshot ||
            (battery_snapshot->is_modified && !battery_mutation_active_)) {
            // A journaled charging mutation not owned by this manager must never be
            // treated as its OEM baseline, even if the current value looks familiar.
            battery_node_.clear();
            battery_stepped_ = false;
            return false;
        }
        uint64_t current = 0;
        if (!positive(line(battery_node_), current)) {
            if (battery_mutation_active_) (void)rollback_battery_locked(releases);
            battery_node_.clear();
            battery_stepped_ = false;
            goto finish;
        }
        if (current != last_charge_ua_) {
            // An OEM/thermal service changed the limit. Relinquish only after the
            // journal confirms ownership loss and the current value is preserved.
            if (battery_mutation_active_ && !rollback_battery_locked(releases)) goto finish;
            battery_node_.clear();
            battery_stepped_ = false;
            goto finish;
        }

        double raw_temp = 0;
        const bool battery_valid =
            parse_ascii_double(line(battery_temp_node_), raw_temp) &&
            raw_temp >= 0 && raw_temp <= 800; // power_supply temperature: tenths Celsius
        const bool charging = line(battery_status_node_) == "Charging";
        if (!battery_valid || !charging) {
            if (battery_stepped_) (void)rollback_battery_locked(releases);
            goto finish;
        }

        const double temperature = raw_temp / 10.0;
        if (temperature >= config_.battery_step_temp_threshold && !battery_stepped_) {
            target = std::min(
                charge_baseline_ua_,
                static_cast<uint64_t>(config_.stepped_charge_current_ma) * 1000);
            if (target >= current) goto finish; // Never raise an OEM charging limit.
            tx_id = battery_transaction_id_.empty()
                ? ("battery-charge-" + std::to_string(++transaction_generation_))
                : battery_transaction_id_;
            target_node = battery_node_;
            need_step_down = true;
        } else if (temperature <= config_.battery_step_temp_threshold - 3.0 && battery_stepped_) {
            rolled_back = rollback_battery_locked(releases);
        }
    }

finish:
    flush_releases(releases);
    if (!need_step_down) return rolled_back;

    // Acquire PowerLongevity lease strictly outside `mutex_` (T7 lock-ordering invariant).
    if (!coordinator_.acquire_lease(
            MutationDomain::PowerLongevity,
            controller_name_,
            tx_id,
            ControllerPriority::ThermalStabilization)) {
        return false;
    }

    bool applied_ok = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (battery_node_.empty() || battery_node_ != target_node || battery_stepped_) {
            if (battery_transaction_id_.empty()) {
                releases.push_back({MutationDomain::PowerLongevity, tx_id});
            }
        } else {
            if (battery_transaction_id_.empty()) battery_transaction_id_ = tx_id;
            auto& store = BaselineStore::instance();
            const bool applied = store.apply_mutation(battery_node_, std::to_string(target));
            uint64_t readback = 0;
            if (!positive(line(battery_node_), readback) || readback != target) {
                battery_mutation_active_ = readback != charge_baseline_ua_;
                if (battery_mutation_active_) {
                    (void)rollback_battery_locked(releases);
                } else {
                    queue_power_release_if_idle_locked(releases);
                }
            } else if (!applied) {
                battery_mutation_active_ = true;
                (void)rollback_battery_locked(releases);
            } else {
                last_charge_ua_ = target;
                battery_stepped_ = true;
                battery_mutation_active_ = true;
                applied_ok = true;
            }
        }
    }
    flush_releases(releases);
    return applied_ok;
}

bool HardwareLongevityManager::apply_storage_pacing() {
    std::vector<PendingLeaseRelease> releases;
    std::string proc_root_copy;
    uint32_t top_app_weight = 800U;
    std::chrono::steady_clock::time_point now{};

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!config_.enable_ufs_burst_pacing) return true;

        auto& store = BaselineStore::instance();
        if (!storage_transaction_id_.empty()) {
            const auto audit = store.get_audit_trail();
            for (const auto& path : storage_mutation_paths_) {
                const auto* snapshot = find_snapshot(audit, path);
                if (!snapshot || !store.owns_expected_state(path, snapshot->active_value)) {
                    (void)rollback_storage_locked(releases);
                    goto release_and_fail;
                }
            }
        }
        if (storage_paced_) return true;

        // Foreground I/O discovery walks mountinfo and up to 4096 cgroup
        // directories, then inspects process membership. Back off only when the
        // resolver cannot find an eligible foreground io.weight target.
        now = std::chrono::steady_clock::now();
        if (storage_transaction_id_.empty() && now < next_storage_probe_) return true;
        proc_root_copy = proc_root_;
        top_app_weight = config_.top_app_io_weight;
    }

    // Perform potentially expensive cgroup filesystem traversal strictly outside `mutex_` (T7).
    {
        MountTopologyResolver topology(proc_root_copy);
        const auto foreground = topology.resolve_foreground_io_group();
        if (!foreground) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (storage_transaction_id_.empty()) {
                next_storage_probe_ = now + std::chrono::seconds(5);
            }
            storage_paced_ = false;
            return true; // Unsupported/ambiguous I/O ABI remains observation-only.
        }

        const std::string& path = foreground->io_weight_path;
        std::string before;
        std::string target;
        std::string tx_id;
        bool need_mutation = false;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            next_storage_probe_ = {};
            before = line(path);
            if (before.rfind("default ", 0) == 0 && ::access(path.c_str(), W_OK) == 0) {
                uint64_t old_weight = 0;
                if (positive(before.substr(8), old_weight) && old_weight <= 10000U) {
                    auto& store = BaselineStore::instance();
                    if (store.capture(path)) {
                        const auto audit_before = store.get_audit_trail();
                        const auto* snapshot_before = find_snapshot(audit_before, path);
                        const bool path_already_owned = std::find(
                            storage_mutation_paths_.begin(), storage_mutation_paths_.end(), path) !=
                            storage_mutation_paths_.end();
                        if (snapshot_before && (!snapshot_before->is_modified || path_already_owned)) {
                            target = "default " + std::to_string(std::clamp(top_app_weight, 1u, 10000u));
                            if (before == target) {
                                storage_paced_ = true;
                            } else {
                                tx_id = storage_transaction_id_.empty()
                                    ? ("storage-pacing-" + std::to_string(++transaction_generation_))
                                    : storage_transaction_id_;
                                need_mutation = true;
                            }
                        }
                    }
                }
            }
            if (!need_mutation) {
                if (storage_mutation_paths_.empty() && !storage_transaction_id_.empty()) {
                    releases.push_back({MutationDomain::BlockIo, std::move(storage_transaction_id_)});
                    storage_transaction_id_.clear();
                }
            }
        }
        flush_releases(releases);
        if (!need_mutation) return true;

        // Acquire BlockIo lease strictly outside `mutex_` (T7 lock-ordering invariant).
        if (!coordinator_.acquire_lease(
                MutationDomain::BlockIo,
                controller_name_,
                tx_id,
                ControllerPriority::WorkloadOptimization)) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (!storage_mutation_paths_.empty()) (void)rollback_storage_locked(releases);
                storage_paced_ = false;
            }
            flush_releases(releases);
            return false;
        }

        bool ok = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (storage_transaction_id_.empty()) storage_transaction_id_ = tx_id;
            auto& store = BaselineStore::instance();
            const bool applied = store.apply_mutation(path, target);
            const std::string readback = line(path);
            if (readback != before &&
                std::find(storage_mutation_paths_.begin(), storage_mutation_paths_.end(), path) ==
                    storage_mutation_paths_.end()) {
                storage_mutation_paths_.push_back(path);
            }
            if (!applied || readback != target) {
                if (storage_mutation_paths_.empty()) {
                    releases.push_back({MutationDomain::BlockIo, std::move(storage_transaction_id_)});
                    storage_transaction_id_.clear();
                    storage_paced_ = false;
                } else {
                    (void)rollback_storage_locked(releases);
                }
            } else {
                storage_paced_ = true;
                ok = true;
            }
        }
        flush_releases(releases);
        return ok;
    }

release_and_fail:
    flush_releases(releases);
    return false;
}

bool HardwareLongevityManager::apply_power_control(
    bool manual_bypass, bool gaming_mode, bool bypass_on_gaming, uint32_t charge_limit_pct) {
    std::vector<PendingLeaseRelease> releases;
    std::string tx_id;
    std::string bypass_node_copy;
    std::string bypass_enable_copy;
    std::string reason = "off";
    ControllerPriority lease_priority = ControllerPriority::WorkloadOptimization;
    bool need_engage = false;
    bool immediate_result = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        const uint32_t sanitized_limit =
            (charge_limit_pct >= 60U && charge_limit_pct <= 95U) ? charge_limit_pct : 0U;
        active_charge_limit_pct_ = sanitized_limit;

        if (bypass_node_.empty()) {
            bypass_active_ = false;
            ceiling_latched_ = false;
            soc_floor_latched_ = false;
            bypass_reason_ = "off";
            return false;
        }

        int cap = -1;
        if (!battery_capacity_node_.empty() && regular_non_symlink(battery_capacity_node_)) {
            double raw_cap = -1.0;
            if (parse_ascii_double(line(battery_capacity_node_), raw_cap) &&
                raw_cap >= 0.0 && raw_cap <= 100.0) {
                cap = static_cast<int>(std::lround(raw_cap));
            }
        }

        // Charge ceiling hysteresis (3% band)
        if (sanitized_limit == 0U || cap < 0) {
            ceiling_latched_ = false;
        } else if (cap >= static_cast<int>(sanitized_limit)) {
            ceiling_latched_ = true;
        } else if (cap <= static_cast<int>(sanitized_limit) - 3) {
            ceiling_latched_ = false;
        }

        const bool want_bypass = manual_bypass || (bypass_on_gaming && gaming_mode) || ceiling_latched_;

        // T3: Hard lithium-ion SOC safety floor (fail-closed if capacity unreadable,
        // auto-release at <= 20%, require >= 25% hysteresis before re-enabling).
        if (cap < 0 || cap <= kBypassMinSocFloorPct) {
            soc_floor_latched_ = true;
        } else if (soc_floor_latched_ && cap >= kBypassResumeSocPct) {
            soc_floor_latched_ = false;
        }

        if (soc_floor_latched_) {
            ceiling_latched_ = false;
            const bool released = !bypass_active_ || rollback_bypass_locked(releases);
            if (!bypass_active_) {
                bypass_reason_ = want_bypass ? "soc_floor" : "off";
            }
            // Refuse bypass activation when below safe SOC floor.
            immediate_result = want_bypass ? false : released;
            goto finish;
        }

        if (manual_bypass) reason = "manual";
        else if (bypass_on_gaming && gaming_mode) reason = "gaming";
        else if (ceiling_latched_) reason = "charge_limit";

        if (!want_bypass) {
            if (bypass_active_) {
                immediate_result = rollback_bypass_locked(releases);
            } else {
                bypass_reason_ = "off";
                immediate_result = true;
            }
            goto finish;
        }

        // Prepare coordinated PowerLongevity lease acquisition outside `mutex_` (T3 + T7).
        lease_priority = battery_stepped_
            ? ControllerPriority::ThermalStabilization
            : ControllerPriority::WorkloadOptimization;
        tx_id = battery_transaction_id_.empty()
            ? ("power-bypass-" + std::to_string(++transaction_generation_))
            : battery_transaction_id_;
        bypass_node_copy = bypass_node_;
        bypass_enable_copy = bypass_enable_val_;
        need_engage = true;
    }

finish:
    flush_releases(releases);
    if (!need_engage) return immediate_result;

    if (!coordinator_.acquire_lease(
            MutationDomain::PowerLongevity,
            controller_name_,
            tx_id,
            lease_priority)) {
        return false;
    }

    bool ok = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (bypass_node_.empty() || bypass_node_ != bypass_node_copy || soc_floor_latched_) {
            if (battery_transaction_id_.empty()) {
                releases.push_back({MutationDomain::PowerLongevity, tx_id});
            }
        } else {
            if (battery_transaction_id_.empty()) battery_transaction_id_ = tx_id;
            auto& store = BaselineStore::instance();
            if (!store.capture(bypass_node_)) {
                queue_power_release_if_idle_locked(releases);
            } else {
                const std::string cur = line(bypass_node_);
                if (cur == bypass_enable_copy) {
                    bypass_active_ = true;
                    bypass_reason_ = std::move(reason);
                    ok = true;
                } else {
                    const bool applied = store.apply_mutation(bypass_node_, bypass_enable_copy);
                    const std::string readback = line(bypass_node_);
                    if (!applied || readback != bypass_enable_copy) {
                        (void)store.rollback(bypass_node_);
                        bypass_active_ = false;
                        bypass_reason_ = "off";
                        queue_power_release_if_idle_locked(releases);
                    } else {
                        bypass_active_ = true;
                        bypass_reason_ = std::move(reason);
                        ok = true;
                    }
                }
            }
        }
    }
    flush_releases(releases);
    return ok;
}

void HardwareLongevityManager::restore_oem_baseline() {
    std::vector<PendingLeaseRelease> releases;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (bypass_active_) (void)rollback_bypass_locked(releases);
        ceiling_latched_ = false;
        soc_floor_latched_ = false;
        if (battery_mutation_active_ || !battery_transaction_id_.empty()) {
            (void)rollback_battery_locked(releases);
        }
        if (!storage_transaction_id_.empty()) (void)rollback_storage_locked(releases);
        if (storage_transaction_id_.empty() && storage_mutation_paths_.empty()) {
            storage_paced_ = false;
        }
    }
    flush_releases(releases);
}

bool HardwareLongevityManager::is_battery_guarded() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return battery_stepped_;
}

bool HardwareLongevityManager::is_bypass_supported() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return !bypass_node_.empty();
}

bool HardwareLongevityManager::is_bypass_active() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return bypass_active_;
}

bool HardwareLongevityManager::is_storage_paced() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return storage_paced_;
}

std::string HardwareLongevityManager::battery_node_path() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return battery_node_;
}

BatteryPowerTelemetry HardwareLongevityManager::sample_battery_telemetry() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return read_battery_telemetry(
        battery_dir_, battery_temp_node_, battery_status_node_,
        battery_current_node_, battery_voltage_node_,
        battery_capacity_node_, bypass_node_,
        battery_stepped_, bypass_active_, bypass_reason_,
        active_charge_limit_pct_, last_charge_ua_, charge_baseline_ua_);
}

BatteryPowerTelemetry HardwareLongevityManager::probe_battery_telemetry(std::string_view sys_root) {
    const fs::path root_path = sys_root.empty() ? fs::path("/sys") : fs::path(sys_root);
    const auto discovered = find_unambiguous_battery_supply(root_path);
    if (!discovered.valid) return {};
    uint64_t charge_limit_ua = 0;
    if (!discovered.charge_max_node.empty()) {
        uint64_t parsed = 0;
        if (positive(line(discovered.charge_max_node), parsed) && parsed <= 20000000U) {
            charge_limit_ua = parsed;
        }
    }
    return read_battery_telemetry(
        discovered.dir, discovered.temp_node, discovered.status_node,
        discovered.current_node, discovered.voltage_node,
        discovered.capacity_node, discovered.bypass_node,
        false, false, "off", 0U, charge_limit_ua, 0U);
}

} // namespace coreflow
