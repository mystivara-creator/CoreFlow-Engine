#include "coreflow/frequency_controller.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/autonomous_tuner.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {
namespace fs = std::filesystem;
namespace {
bool number_allow_zero(const std::string& path, uint64_t& value) {
    std::ifstream file(path);
    std::string text;
    if (!(file >> text)) return false;
    auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}
bool number(const std::string& path, uint64_t& value) {
    return number_allow_zero(path, value) && value > 0U;
}
bool verified_regular_writable(const std::string& path) noexcept {
    struct stat st{};
    return ::lstat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode) &&
        (st.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) != 0 && ::access(path.c_str(), W_OK) == 0;
}
bool snapshot_is_oem(const std::vector<NodeSnapshot>& audit, const std::string& path,
                     uint64_t value) {
    const auto found = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
        return node.path == path;
    });
    return found != audit.end() && !found->is_modified &&
        found->factory_value == std::to_string(value);
}
}
FrequencyController::FrequencyController(CentralMutationCoordinator& coordinator, std::string root, DeviceProfileHandle profile)
    : coordinator_(coordinator), root_(std::move(root)), profile_(std::move(profile)),
      controller_name_(CentralMutationCoordinator::make_controller_identity("FrequencyController")) {
    (void)coordinator_.register_preemption_handler(
        controller_name_,
        [this](MutationDomain domain, std::string_view transaction_id, std::string_view) noexcept {
            return handle_preemption(domain, transaction_id);
        });
}
FrequencyController::~FrequencyController() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!cpu_transaction_id_.empty()) (void)rollback_domain(false);
        if (!gpu_transaction_id_.empty()) (void)rollback_domain(true);
    }
    coordinator_.unregister_preemption_handler(controller_name_);
}
bool FrequencyController::acquire_domain_lease(bool gpu, bool thermal_guard) {
    auto& transaction_id = gpu ? gpu_transaction_id_ : cpu_transaction_id_;
    const auto domain = gpu ? MutationDomain::GpuDevfreq : MutationDomain::CpuPolicy;
    if (!transaction_id.empty()) {
        return coordinator_.acquire_lease(
            domain, controller_name_, transaction_id,
            thermal_guard ? ControllerPriority::ThermalStabilization
                          : ControllerPriority::WorkloadOptimization);
    }
    transaction_id = "frequency-" + std::string(gpu ? "gpu-" : "cpu-") +
                     std::to_string(++lease_generation_);
    if (!coordinator_.acquire_lease(
            domain, controller_name_, transaction_id,
            thermal_guard ? ControllerPriority::ThermalStabilization
                          : ControllerPriority::WorkloadOptimization)) {
        transaction_id.clear();
        return false;
    }
    return true;
}
void FrequencyController::release_domain_lease(bool gpu) noexcept {
    auto& transaction_id = gpu ? gpu_transaction_id_ : cpu_transaction_id_;
    if (transaction_id.empty()) return;
    const auto domain = gpu ? MutationDomain::GpuDevfreq : MutationDomain::CpuPolicy;
    coordinator_.release_lease(
        domain, controller_name_, transaction_id);
    transaction_id.clear();
}
bool FrequencyController::rollback_domain(bool gpu) {
    auto& store = BaselineStore::instance();
    for (auto& item : domains_) {
        if (item.gpu != gpu || !item.mutation_active) continue;
        if (item.kgsl_power_levels) {
            uint64_t actual_max = 0U, actual_min = 0U;
            const std::string expected_max = std::to_string(item.last);
            const std::string expected_min = std::to_string(item.last_minimum);
            if (!number_allow_zero(item.maximum, actual_max) ||
                !number_allow_zero(item.minimum, actual_min) ||
                actual_max != item.last || actual_min != item.last_minimum ||
                actual_max > actual_min || actual_min > item.max_valid_level ||
                !store.owns_expected_state(item.maximum, expected_max) ||
                !store.owns_expected_state(item.minimum, expected_min)) {
                // A changed inode/value is an ownership handoff. Never overwrite
                // a foreign KGSL limit while trying to release our paired lease.
                return false;
            }

            const auto rollback_pair_node = [&](const std::string& path,
                                                uint64_t expected_value,
                                                uint64_t other_value) {
                if (!store.rollback(path)) return false;
                uint64_t read_max = 0U, read_min = 0U;
                if (!number_allow_zero(item.maximum, read_max) ||
                    !number_allow_zero(item.minimum, read_min) ||
                    read_max > read_min || read_min > item.max_valid_level) return false;
                if (path == item.maximum) {
                    return read_max == expected_value && read_min == other_value;
                }
                return read_min == expected_value && read_max == other_value;
            };

            // Both endpoints are restored in an order that preserves the KGSL
            // invariant after each individual sysfs write. A two-node sysfs pair
            // cannot be physically atomic, so the coordinator lease + WAL +
            // intermediate readback form the transactional boundary.
            if (item.baseline <= actual_min) {
                if (!rollback_pair_node(item.maximum, item.baseline, actual_min) ||
                    !rollback_pair_node(item.minimum, item.baseline_minimum, item.baseline)) {
                    return false;
                }
            } else if (actual_max <= item.baseline_minimum) {
                if (!rollback_pair_node(item.minimum, item.baseline_minimum, actual_max) ||
                    !rollback_pair_node(item.maximum, item.baseline, item.baseline_minimum)) {
                    return false;
                }
            } else {
                return false;
            }

            uint64_t restored_max = 0U, restored_min = 0U;
            if (!number_allow_zero(item.maximum, restored_max) ||
                !number_allow_zero(item.minimum, restored_min) ||
                restored_max != item.baseline ||
                restored_min != item.baseline_minimum ||
                restored_max > restored_min || restored_min > item.max_valid_level) return false;
            const auto audit = store.get_audit_trail();
            if (!snapshot_is_oem(audit, item.maximum, restored_max) ||
                !snapshot_is_oem(audit, item.minimum, restored_min) ||
                !store.owns_expected_state(item.maximum, std::to_string(restored_max)) ||
                !store.owns_expected_state(item.minimum, std::to_string(restored_min))) return false;
            item.last = restored_max;
            item.last_minimum = restored_min;
            item.mutation_active = false;
            continue;
        }
        if (!store.rollback(item.maximum)) return false;
        uint64_t actual = 0;
        if (!number(item.maximum, actual)) return false;
        const auto audit = store.get_audit_trail();
        const auto snapshot = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
            return node.path == item.maximum;
        });
        if (snapshot == audit.end() || snapshot->is_modified ||
            snapshot->factory_value != std::to_string(actual)) return false;
        if (actual != item.baseline) {
            item.baseline = actual;
            item.retired = true;
        }
        item.last = actual;
        item.mutation_active = false;
    }
    release_domain_lease(gpu);
    return true;
}
bool FrequencyController::handle_preemption(
    MutationDomain domain, std::string_view transaction_id) noexcept {
    const bool gpu = domain == MutationDomain::GpuDevfreq;
    if (domain != MutationDomain::CpuPolicy && !gpu) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    const auto& active_id = gpu ? gpu_transaction_id_ : cpu_transaction_id_;
    if (active_id.empty() || active_id != transaction_id) return false;
    return rollback_domain(gpu);
}
void FrequencyController::add(const std::string& dir, bool gpu) {
    const auto max = dir + (gpu ? "/max_freq" : "/scaling_max_freq");
    const auto min = dir + (gpu ? "/min_freq" : "/scaling_min_freq");
    uint64_t ceiling = 0, floor = 0;
    if (!number(max, ceiling) || !number(min, floor) || floor > ceiling || ::access(max.c_str(), W_OK) != 0) return;
    std::ifstream table(dir + (gpu ? "/available_frequencies" : "/scaling_available_frequencies"));
    std::vector<uint64_t> values;
    uint64_t frequency;
    while (table >> frequency) {
        if (frequency > 0 && frequency <= ceiling) values.push_back(frequency);
        if (values.size() > 512) return;
    }
    if (!table.eof() || values.empty()) return;
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    auto& store = BaselineStore::instance();
    if (!store.capture(max)) return;
    const auto audit = store.get_audit_trail();
    const auto snapshot = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
        return node.path == max;
    });
    // A pre-existing journaled mutation has no ownership relationship to this
    // newly discovered controller instance. Do not adopt it as an OEM baseline.
    if (snapshot == audit.end() || snapshot->is_modified) return;
    domains_.push_back({max, min, ceiling, ceiling, std::move(values), gpu});
}

void FrequencyController::add_kgsl(const std::string& dir) {
    const std::string maximum = dir + "/max_pwrlevel";
    const std::string minimum = dir + "/min_pwrlevel";
    const std::string count_path = dir + "/num_pwrlevels";
    const std::string table_path = dir + "/gpu_available_frequencies";
    uint64_t current_max = 0U, current_min = 0U, reported_levels = 0U;
    if (!number_allow_zero(maximum, current_max) || !number_allow_zero(minimum, current_min) ||
        !number_allow_zero(count_path, reported_levels) ||
        !verified_regular_writable(maximum) || !verified_regular_writable(minimum)) return;
    std::ifstream table(table_path);
    std::vector<uint64_t> frequencies;
    uint64_t frequency = 0U;
    while (table >> frequency) {
        if (frequency == 0U || frequencies.size() >= 512U) return;
        frequencies.push_back(frequency);
    }
    if (!table.eof() || frequencies.size() < 2U || reported_levels != frequencies.size()) return;
    const uint64_t table_last = static_cast<uint64_t>(frequencies.size() - 1U);
    if (reported_levels != frequencies.size()) return;
    if (current_max > current_min || current_min > table_last) return;
    const bool increasing = std::adjacent_find(frequencies.begin(), frequencies.end(),
        [](uint64_t a, uint64_t b) { return b <= a; }) == frequencies.end();
    const bool decreasing = std::adjacent_find(frequencies.begin(), frequencies.end(),
        [](uint64_t a, uint64_t b) { return b >= a; }) == frequencies.end();
    if (!increasing && !decreasing) return;

    auto& store = BaselineStore::instance();
    if (!store.capture(maximum) || !store.capture(minimum)) return;
    const auto audit = store.get_audit_trail();
    if (!snapshot_is_oem(audit, maximum, current_max) ||
        !snapshot_is_oem(audit, minimum, current_min)) return;
    Domain domain{};
    domain.maximum = maximum;
    domain.minimum = minimum;
    domain.baseline = current_max;
    domain.last = current_max;
    domain.baseline_minimum = current_min;
    domain.last_minimum = current_min;
    domain.max_valid_level = table_last;
    domain.frequencies = std::move(frequencies);
    domain.gpu = true;
    domain.kgsl_power_levels = true;
    domains_.push_back(std::move(domain));
}

bool FrequencyController::apply_gpu_frame_policy(
    double gpu_busy_pct, double frame_miss_pct, double thermal_headroom,
    bool thermal_guard) {
    if (!std::isfinite(gpu_busy_pct) || gpu_busy_pct < 0.0 || gpu_busy_pct > 100.0 ||
        !std::isfinite(frame_miss_pct) || frame_miss_pct < 0.0 || frame_miss_pct > 100.0 ||
        !std::isfinite(thermal_headroom) || thermal_headroom < 0.0 || thermal_headroom > 1.0) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    const bool has_kgsl = std::any_of(domains_.begin(), domains_.end(), [](const Domain& item) {
        return item.gpu && item.kgsl_power_levels && !item.retired;
    });
    if (!has_kgsl) return false;

    if (thermal_guard || thermal_headroom < 0.35 || gpu_busy_pct < 70.0 || frame_miss_pct < 0.5) {
        return rollback_domain(true);
    }
    if (!acquire_domain_lease(true, false)) return false;

    const uint64_t step = gpu_busy_pct >= 90.0 && frame_miss_pct >= 5.0 &&
        thermal_headroom >= 0.55 ? 2U : 1U;
    auto& store = BaselineStore::instance();
    for (auto& item : domains_) {
        if (!item.gpu || !item.kgsl_power_levels || item.retired) continue;
        uint64_t actual_max = 0U, actual_min = 0U;
        if (!number_allow_zero(item.maximum, actual_max) ||
            !number_allow_zero(item.minimum, actual_min) || actual_max > actual_min ||
            actual_min > item.max_valid_level || actual_max != item.last ||
            actual_min != item.last_minimum ||
            !store.owns_expected_state(item.maximum, std::to_string(actual_max)) ||
            !store.owns_expected_state(item.minimum, std::to_string(actual_min))) {
            item.retired = true;
            if (item.mutation_active && !rollback_domain(true)) return false;
            continue;
        }

        const auto audit = store.get_audit_trail();
        const auto max_node = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
            return node.path == item.maximum;
        });
        const auto min_node = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
            return node.path == item.minimum;
        });
        const bool baseline_pair_valid =
            max_node != audit.end() && min_node != audit.end() &&
            max_node->factory_value == std::to_string(item.baseline) &&
            min_node->factory_value == std::to_string(item.baseline_minimum) &&
            (item.mutation_active ||
             (!max_node->is_modified && !min_node->is_modified));
        if (!baseline_pair_valid) {
            item.retired = true;
            if (item.mutation_active && !rollback_domain(true)) return false;
            continue;
        }

        const uint64_t reducible = item.baseline_minimum > item.baseline
            ? item.baseline_minimum - item.baseline : 0U;
        const uint64_t target_max = item.baseline;
        const uint64_t target_min = std::max(item.baseline,
            item.baseline_minimum - std::min(step, reducible));
        if (target_max > target_min || target_min > item.max_valid_level) {
            item.retired = true;
            if (item.mutation_active && !rollback_domain(true)) return false;
            continue;
        }
        if (target_max == actual_max && target_min == actual_min) continue;

        // The max/min pair shares one GpuDevfreq lease and two durable baseline
        // records. Mark the pair active before the first endpoint write so any
        // partial failure routes through the same deterministic pair rollback.
        item.mutation_active = true;
        uint64_t working_max = actual_max;
        uint64_t working_min = actual_min;
        const auto write_max = [&]() {
            if (working_max == target_max) return true;
            if (target_max > working_min) return false;
            if (!store.apply_mutation(item.maximum, std::to_string(target_max))) return false;
            uint64_t read_max = 0U, read_min = 0U;
            if (!number_allow_zero(item.maximum, read_max) ||
                !number_allow_zero(item.minimum, read_min) ||
                read_max != target_max || read_min != working_min ||
                read_max > read_min || read_min > item.max_valid_level) return false;
            working_max = read_max;
            item.last = read_max;
            item.last_minimum = read_min;
            return true;
        };
        const auto write_min = [&]() {
            if (working_min == target_min) return true;
            if (working_max > target_min) return false;
            if (!store.apply_mutation(item.minimum, std::to_string(target_min))) return false;
            uint64_t read_max = 0U, read_min = 0U;
            if (!number_allow_zero(item.maximum, read_max) ||
                !number_allow_zero(item.minimum, read_min) ||
                read_max != working_max || read_min != target_min ||
                read_max > read_min || read_min > item.max_valid_level) return false;
            working_max = read_max;
            working_min = read_min;
            item.last = read_max;
            item.last_minimum = read_min;
            return true;
        };

        // If tightening max first would cross the current min, lower min first.
        // Otherwise max can be committed first. In either order the ABI invariant
        // remains true between writes.
        bool applied = true;
        if (target_max != working_max && target_max > working_min) {
            applied = write_min();
        }
        if (applied && target_max != working_max) applied = write_max();
        if (applied && target_min != working_min) applied = write_min();

        uint64_t read_max = 0U, read_min = 0U;
        const bool readback = applied &&
            number_allow_zero(item.maximum, read_max) &&
            number_allow_zero(item.minimum, read_min) &&
            read_max == target_max && read_min == target_min &&
            read_max <= read_min && read_min <= item.max_valid_level;
        const auto after_audit = store.get_audit_trail();
        const auto owns_endpoint = [&](const std::string& path, uint64_t value, uint64_t baseline) {
            const auto node = std::find_if(after_audit.begin(), after_audit.end(), [&](const NodeSnapshot& entry) {
                return entry.path == path;
            });
            const std::string expected = std::to_string(value);
            return node != after_audit.end() &&
                node->factory_value == std::to_string(baseline) &&
                node->active_value == expected &&
                node->is_modified == (value != baseline) &&
                store.owns_expected_state(path, expected);
        };
        const bool owned = readback &&
            owns_endpoint(item.maximum, read_max, item.baseline) &&
            owns_endpoint(item.minimum, read_min, item.baseline_minimum);
        if (!owned) {
            // Keep the latest verified pair for the ownership-aware rollback path.
            uint64_t observed_max = 0U, observed_min = 0U;
            if (number_allow_zero(item.maximum, observed_max) &&
                number_allow_zero(item.minimum, observed_min) &&
                observed_max <= observed_min && observed_min <= item.max_valid_level) {
                item.last = observed_max;
                item.last_minimum = observed_min;
            }
            if (!rollback_domain(true)) return false;
            return false;
        }
        item.last = read_max;
        item.last_minimum = read_min;
    }
    const bool active = std::any_of(domains_.begin(), domains_.end(), [](const Domain& item) {
        return item.gpu && item.kgsl_power_levels && item.mutation_active;
    });
    if (!active) release_domain_lease(true);
    return active;
}
bool FrequencyController::restore_gpu_frame_policy() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return rollback_domain(true);
}

bool FrequencyController::gpu_frame_policy_owned_intact() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    bool observed_active_pair = false;
    const auto& store = BaselineStore::instance();
    for (const auto& item : domains_) {
        if (!item.gpu || !item.kgsl_power_levels || !item.mutation_active) continue;
        observed_active_pair = true;
        uint64_t actual_max = 0U, actual_min = 0U;
        if (!number_allow_zero(item.maximum, actual_max) ||
            !number_allow_zero(item.minimum, actual_min) ||
            actual_max != item.last || actual_min != item.last_minimum ||
            actual_max > actual_min || actual_min > item.max_valid_level) return false;
        const auto audit = store.get_audit_trail();
        const auto owned_endpoint = [&](const std::string& path, uint64_t value, uint64_t baseline) {
            const auto node = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& entry) {
                return entry.path == path;
            });
            const std::string expected = std::to_string(value);
            return node != audit.end() &&
                node->factory_value == std::to_string(baseline) &&
                node->active_value == expected &&
                node->is_modified == (value != baseline) &&
                store.owns_expected_state(path, expected);
        };
        if (!owned_endpoint(item.maximum, actual_max, item.baseline) ||
            !owned_endpoint(item.minimum, actual_min, item.baseline_minimum)) return false;
    }
    return observed_active_pair;
}
void FrequencyController::discover(bool cpu, bool gpu) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Never discard in-memory ownership metadata while a mutation is active.
    if (std::any_of(domains_.begin(), domains_.end(), [](const Domain& d) { return d.mutation_active; })) return;
    domains_.clear();

    if (profile_) {
        const auto eligible = [&](const std::string& path, std::string_view semantic) {
            return std::any_of(profile_->node_capabilities.begin(), profile_->node_capabilities.end(),
                [&](const NodeCapability& cap) {
                    return cap.path == path && cap.semantic_id == semantic && cap.mutation_eligible();
                });
        };
        if (cpu) {
            for (const auto& policy : profile_->cpu_policies) {
                const std::string max_path = policy.policy_path + "/scaling_max_freq";
                if (eligible(max_path, "cpufreq.scaling_max_freq")) add(policy.policy_path, false);
            }
        }
        if (gpu) {
            for (const auto& domain : profile_->gpu_domains) {
                if (domain.device_path.find("/class/kgsl/") != std::string::npos) {
                    add_kgsl(domain.device_path);
                    continue;
                }
                const std::string max_path = domain.device_path + "/max_freq";
                if (eligible(max_path, "devfreq.max_freq")) add(domain.device_path, true);
            }
            if (std::none_of(profile_->gpu_domains.begin(), profile_->gpu_domains.end(),
                    [](const GpuDomainProfile& domain) {
                        return domain.device_path.find("/class/kgsl/") != std::string::npos;
                    })) add_kgsl(root_ + "/class/kgsl/kgsl-3d0");
        }
        // An injected profile is authoritative for CPU/GPU actuator admission.
        // Do not run a second independent scan when a subsystem was not admitted.
        return;
    }

    // Legacy compatibility path for tests/callers that have not yet injected CDDP.
    auto scan = [&](const std::string& root, bool is_gpu) {
        std::error_code ec;
        std::set<std::string> seen;
        for (fs::directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec)) {
            if (domains_.size() >= 32) break;
            const auto name = it->path().filename().string();
            auto real = fs::canonical(it->path(), ec);
            if (ec) { ec.clear(); continue; }
            if (!seen.insert(real.string()).second) continue;
            if (is_gpu) {
                std::string device_name;
                std::ifstream(it->path() / "name") >> device_name;
                const auto identity = name + device_name + real.string();
                if (identity.find("gpu") == std::string::npos && identity.find("mali") == std::string::npos &&
                    identity.find("kgsl") == std::string::npos) continue;
            } else if (name.rfind("policy", 0) != 0) continue;
            add(it->path().string(), is_gpu);
        }
    };
    if (cpu) scan(root_ + "/devices/system/cpu/cpufreq", false);
    if (gpu) {
        scan(root_ + "/class/devfreq", true);
        add_kgsl(root_ + "/class/kgsl/kgsl-3d0");
    }
}
bool FrequencyController::apply(double scale, bool thermal_guard) {
    // 0.2 is the emergency thermal floor; hardware policy floors still apply per domain.
    if (!std::isfinite(scale) || scale < 0.2 || scale > 1.0) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    if (thermal_guard && !rollback_domain(true)) return false;
    auto& store = BaselineStore::instance();
    bool ok = true;
    const auto restore_owned_domain = [&](Domain& domain) {
        if (!store.rollback(domain.maximum)) return false;
        uint64_t restored = 0;
        if (!number(domain.maximum, restored) || restored != domain.baseline) return false;
        domain.last = domain.baseline;
        domain.mutation_active = false;
        return true;
    };
    bool cpu_needs_lease = false;
    bool gpu_needs_lease = false;

    // First determine whether each domain actually needs a write, then acquire the
    // corresponding exclusive lease before the first mutation in that domain.
    for (auto& domain : domains_) {
        if (domain.retired || domain.kgsl_power_levels) continue;
        uint64_t actual = 0, floor = 0;
        if (!number(domain.maximum, actual) || !number(domain.minimum, floor)) {
            domain.retired = true;
            ok = false;
            continue;
        }
        const auto ownership_audit = store.get_audit_trail();
        const auto ownership_snapshot = std::find_if(
            ownership_audit.begin(), ownership_audit.end(), [&](const NodeSnapshot& node) {
                return node.path == domain.maximum;
            });
        if (ownership_snapshot == ownership_audit.end() ||
            (ownership_snapshot->is_modified && !domain.mutation_active)) {
            // A journaled mutation appeared after discovery but is not owned by
            // this controller instance. Retire instead of adopting its active value.
            domain.retired = true;
            ok = false;
            continue;
        }
        if (actual != domain.last) {
            // An external owner changed the node. Do not overwrite its value.
            domain.retired = true;
            if (domain.mutation_active) {
                // BaselineStore's ownership check prevents rollback from clobbering
                // an external value; retain the lease if safe restoration is unproven.
                if (!store.rollback(domain.maximum)) ok = false;
                else domain.mutation_active = false;
            }
            continue;
        }
        const auto requested = static_cast<uint64_t>(static_cast<double>(domain.baseline) * scale);
        auto upper = std::upper_bound(domain.frequencies.begin(), domain.frequencies.end(), requested);
        uint64_t target = upper == domain.frequencies.begin() ? *upper : *std::prev(upper);
        if (target < floor) {
            const auto next = std::lower_bound(domain.frequencies.begin(), domain.frequencies.end(), floor);
            if (next == domain.frequencies.end()) {
                domain.retired = true;
                if (domain.mutation_active && !restore_owned_domain(domain)) ok = false;
                continue;
            }
            target = *next;
        }
        if (scale == 1.0) target = domain.baseline;
        if (target < floor || target > domain.baseline) {
            domain.retired = true;
            if (domain.mutation_active && !restore_owned_domain(domain)) ok = false;
            continue;
        }
        if (target == actual) continue;
        if (domain.gpu) gpu_needs_lease = true;
        else cpu_needs_lease = true;
    }

    if (cpu_needs_lease && !acquire_domain_lease(false, thermal_guard)) return false;
    if (gpu_needs_lease && !acquire_domain_lease(true, thermal_guard)) {
        if (!cpu_transaction_id_.empty()) {
            const bool cpu_active = std::any_of(domains_.begin(), domains_.end(),
                [](const Domain& d) { return !d.gpu && d.mutation_active; });
            if (!cpu_active) release_domain_lease(false);
        }
        return false;
    }

    for (auto& domain : domains_) {
        if (domain.retired || domain.kgsl_power_levels) continue;
        uint64_t actual = 0, floor = 0;
        if (!number(domain.maximum, actual) || !number(domain.minimum, floor) || actual != domain.last) {
            domain.retired = true;
            ok = false;
            continue;
        }
        const auto requested = static_cast<uint64_t>(static_cast<double>(domain.baseline) * scale);
        auto upper = std::upper_bound(domain.frequencies.begin(), domain.frequencies.end(), requested);
        uint64_t target = upper == domain.frequencies.begin() ? *upper : *std::prev(upper);
        if (target < floor) {
            const auto next = std::lower_bound(domain.frequencies.begin(), domain.frequencies.end(), floor);
            if (next == domain.frequencies.end()) {
                domain.retired = true;
                if (domain.mutation_active && !restore_owned_domain(domain)) ok = false;
                continue;
            }
            target = *next;
        }
        if (scale == 1.0) target = domain.baseline;
        if (target < floor || target > domain.baseline) {
            domain.retired = true;
            if (domain.mutation_active && !restore_owned_domain(domain)) ok = false;
            continue;
        }
        if (target == actual) continue;

        if (target == domain.baseline) {
            if (!store.rollback(domain.maximum)) {
                ok = false;
                continue;
            }
            uint64_t restored = 0;
            if (!number(domain.maximum, restored) || restored != domain.baseline) {
                ok = false;
                continue;
            }
            domain.last = domain.baseline;
            domain.mutation_active = false;
            continue;
        }

        const bool applied = store.apply_mutation(domain.maximum, std::to_string(target));
        uint64_t readback = 0;
        const bool readback_valid = number(domain.maximum, readback);
        const auto audit = store.get_audit_trail();
        const auto snapshot = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
            return node.path == domain.maximum;
        });
        const bool store_still_owns_node =
            snapshot != audit.end() && snapshot->is_modified &&
            store.owns_expected_state(domain.maximum, snapshot->active_value);

        if (!applied || !readback_valid || readback != target) {
            ok = false;
            if (!readback_valid) {
                // The node became unreadable after a write attempt. Retain the lease
                // and recovery metadata regardless of the journal's last visible state.
                domain.mutation_active = true;
                continue;
            }

            if (store_still_owns_node) {
                // A failed apply can leave either the prior CoreFlow value or a
                // partially committed target. Restore this controller's path before
                // considering its lease releasable.
                domain.mutation_active = true;
                domain.last = readback;
                if (!restore_owned_domain(domain)) {
                    const auto after_audit = store.get_audit_trail();
                    const auto after_snapshot = std::find_if(
                        after_audit.begin(), after_audit.end(), [&](const NodeSnapshot& node) {
                            return node.path == domain.maximum;
                        });
                    if (after_snapshot == after_audit.end() || !after_snapshot->is_modified) {
                        // BaselineStore adopted an OEM/external value during rollback.
                        // Relinquish and retire instead of reasserting on a later tick.
                        domain.mutation_active = false;
                        domain.retired = true;
                        domain.last = readback;
                    }
                    continue;
                }
            } else {
                // BaselineStore no longer recognizes this as our mutation. Preserve
                // an OEM/external value and retire the path instead of reasserting.
                domain.mutation_active = false;
                domain.last = readback;
                if (readback != domain.baseline) domain.retired = true;
            }
            continue;
        }

        domain.mutation_active = true;
        domain.last = target;
    }

    for (const bool gpu : {false, true}) {
        const bool active = std::any_of(domains_.begin(), domains_.end(),
            [gpu](const Domain& d) { return d.gpu == gpu && d.mutation_active; });
        if (!active) release_domain_lease(gpu);
    }
    return ok;
}
size_t FrequencyController::cpu_count() const noexcept {
    return std::count_if(domains_.begin(), domains_.end(), [](const auto& d) { return !d.gpu && !d.retired; });
}
size_t FrequencyController::gpu_count() const noexcept {
    return std::count_if(domains_.begin(), domains_.end(), [](const auto& d) { return d.gpu && !d.retired; });
}
size_t FrequencyController::kgsl_count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::count_if(domains_.begin(), domains_.end(),
        [](const Domain& d) { return d.gpu && d.kgsl_power_levels && !d.retired; });
}
}
