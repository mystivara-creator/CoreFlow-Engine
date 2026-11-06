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
#include <unistd.h>

namespace coreflow {
namespace fs = std::filesystem;
namespace {
bool number(const std::string& path, uint64_t& value) {
    std::ifstream file(path);
    std::string text;
    if (!(file >> text)) return false;
    auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size() && value > 0;
}
}
namespace {
constexpr std::string_view kFrequencyControllerName = "FrequencyController";
}
FrequencyController::FrequencyController(std::string root) : root_(std::move(root)) {
    (void)CentralMutationCoordinator::instance().register_preemption_handler(
        kFrequencyControllerName,
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
    CentralMutationCoordinator::instance().unregister_preemption_handler(kFrequencyControllerName);
}
bool FrequencyController::acquire_domain_lease(bool gpu) {
    auto& transaction_id = gpu ? gpu_transaction_id_ : cpu_transaction_id_;
    const auto domain = gpu ? MutationDomain::GpuDevfreq : MutationDomain::CpuPolicy;
    if (!transaction_id.empty()) {
        return CentralMutationCoordinator::instance().acquire_lease(
            domain, kFrequencyControllerName, transaction_id,
            ControllerPriority::WorkloadOptimization);
    }
    transaction_id = "frequency-" + std::string(gpu ? "gpu-" : "cpu-") +
                     std::to_string(++lease_generation_);
    if (!CentralMutationCoordinator::instance().acquire_lease(
            domain, kFrequencyControllerName, transaction_id,
            ControllerPriority::WorkloadOptimization)) {
        transaction_id.clear();
        return false;
    }
    return true;
}
void FrequencyController::release_domain_lease(bool gpu) noexcept {
    auto& transaction_id = gpu ? gpu_transaction_id_ : cpu_transaction_id_;
    if (transaction_id.empty()) return;
    const auto domain = gpu ? MutationDomain::GpuDevfreq : MutationDomain::CpuPolicy;
    CentralMutationCoordinator::instance().release_lease(
        domain, kFrequencyControllerName, transaction_id);
    transaction_id.clear();
}
bool FrequencyController::rollback_domain(bool gpu) {
    auto& store = BaselineStore::instance();
    for (auto& item : domains_) {
        if (item.gpu != gpu || !item.mutation_active) continue;
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
            // OEM/kernel ownership changed while the lease was active. Preserve the
            // adopted external value and retire this discovered domain until rescan.
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
    if (!BaselineStore::instance().capture(max)) return;
    domains_.push_back({max, min, ceiling, ceiling, std::move(values), gpu});
}
void FrequencyController::discover(bool cpu, bool gpu) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Never discard in-memory ownership metadata while a mutation is still active.
    if (std::any_of(domains_.begin(), domains_.end(), [](const Domain& d) { return d.mutation_active; })) return;
    domains_.clear();
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
    if (gpu) scan(root_ + "/class/devfreq", true);
}
bool FrequencyController::apply(double scale) {
    if (!std::isfinite(scale) || scale < 0.4 || scale > 1.0) return false;
    std::lock_guard<std::mutex> lock(mutex_);
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
        if (domain.retired) continue;
        uint64_t actual = 0, floor = 0;
        if (!number(domain.maximum, actual) || !number(domain.minimum, floor)) {
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

    if (cpu_needs_lease && !acquire_domain_lease(false)) return false;
    if (gpu_needs_lease && !acquire_domain_lease(true)) {
        if (!cpu_transaction_id_.empty()) {
            const bool cpu_active = std::any_of(domains_.begin(), domains_.end(),
                [](const Domain& d) { return !d.gpu && d.mutation_active; });
            if (!cpu_active) release_domain_lease(false);
        }
        return false;
    }

    for (auto& domain : domains_) {
        if (domain.retired) continue;
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

        if (!store.apply_mutation(domain.maximum, std::to_string(target))) {
            ok = false;
            continue;
        }
        domain.mutation_active = true;
        uint64_t readback = 0;
        if (!number(domain.maximum, readback) || readback != target) {
            ok = false;
            if (!restore_owned_domain(domain)) continue;
            continue;
        }
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
}
