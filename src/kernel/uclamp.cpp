/**
 * @file uclamp.cpp
 * @brief Capability-driven UClamp discovery and bounded top-app control
 * @author mystivara-creator
 * @license MIT
 */
#include "coreflow/uclamp.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/config.hpp"
#include "coreflow/unique_fd.hpp"
#include "coreflow/mount_topology.hpp"
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>
#include <vector>

namespace coreflow {
namespace fs = std::filesystem;

namespace {

std::string read_token(const fs::path& path) {
    const std::string p = path.string();
    if (p.empty() || p.find('\0') != std::string::npos) return {};
    UniqueFd fd(::open(p.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return {};

    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode)) return {};

    char buf[128];
    ssize_t n = 0;
    do {
        n = ::read(fd.get(), buf, sizeof(buf));
    } while (n < 0 && errno == EINTR);
    if (n <= 0) return {};

    const std::string_view raw(buf, static_cast<size_t>(n));
    const size_t first = raw.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    const size_t end = raw.find_first_of(" \t\r\n", first);
    return std::string(raw.substr(first, end == std::string_view::npos ? std::string_view::npos : end - first));
}

} // namespace

UclampGovernor::UclampGovernor(CentralMutationCoordinator& coordinator, std::string_view explicit_root)
    : coordinator_(coordinator) {
    discover(explicit_root);
    if (supported_) {
        controller_name_ = CentralMutationCoordinator::make_controller_identity("UclampGovernor");
        (void)coordinator_.register_preemption_handler(
            controller_name_,
            [this](MutationDomain domain, std::string_view transaction_id, std::string_view) noexcept {
                return handle_preemption(domain, transaction_id);
            });
        std::cerr << "[CoreFlow][Uclamp] discovered " << target_path_
                  << " (" << encoding_name() << ")\n";
    } else {
        std::cerr << "[CoreFlow][Uclamp] no writable semantic top-app UClamp target found\n";
    }
}
UclampGovernor::~UclampGovernor() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!transaction_id_.empty()) (void)rollback_transaction_locked();
    }
    if (!controller_name_.empty()) {
        coordinator_.unregister_preemption_handler(controller_name_);
    }
}
UclampGovernor& UclampGovernor::operator=(UclampGovernor&& other) noexcept {
    if (this == &other) return *this;

    std::string other_handler_name;
    bool register_this_handler = false;
    {
        std::scoped_lock lock(mutex_, other.mutex_);
        // Never discard either object's live transaction unless rollback verifies.
        if (!transaction_id_.empty() && !rollback_transaction_locked()) {
            supported_ = false;
            return *this;
        }
        if (!other.transaction_id_.empty() && !other.rollback_transaction_locked()) {
            other.supported_ = false;
            return *this;
        }

        // Keep this object's stable callback identity. The source object's handler
        // is retired only after both object mutexes are released.
        other_handler_name = other.controller_name_;
        target_path_ = std::move(other.target_path_);
        transaction_id_.clear();
        transaction_generation_ = std::max(transaction_generation_, other.transaction_generation_);
        encoding_ = other.encoding_;
        supported_ = other.supported_;
        if (supported_ && controller_name_.empty()) {
            controller_name_ = CentralMutationCoordinator::make_controller_identity("UclampGovernor");
            register_this_handler = true;
        }
        has_written_ = false;
        oem_min_ = other.oem_min_;
        oem_max_ = other.oem_max_;
        oem_min_raw_ = std::move(other.oem_min_raw_);
        oem_max_raw_ = std::move(other.oem_max_raw_);
        mutation_active_ = false;
        last_min_ = other.last_min_;
        last_max_ = other.last_max_;

        other.controller_name_.clear();
        other.transaction_id_.clear();
        other.supported_ = false;
        other.has_written_ = false;
        other.mutation_active_ = false;
    }

    // The source callback was registered with the source object's
    // coordinator, which may differ from this object's coordinator after
    // move-assignment. Unregister there so it also drains in-flight callbacks.
    if (!other_handler_name.empty()) {
        other.coordinator_.unregister_preemption_handler(other_handler_name);
    }
    if (register_this_handler) {
        (void)coordinator_.register_preemption_handler(
            controller_name_,
            [this](MutationDomain domain, std::string_view transaction_id, std::string_view) noexcept {
                return handle_preemption(domain, transaction_id);
            });
    }
    return *this;
}

bool UclampGovernor::acquire_mutation_lease_locked(ControllerPriority priority) {
    if (!transaction_id_.empty()) {
        return coordinator_.acquire_lease(
            MutationDomain::UclampCgroup, controller_name_, transaction_id_, priority);
    }
    transaction_id_ = "uclamp-" + std::to_string(++transaction_generation_);
    if (!coordinator_.acquire_lease(
            MutationDomain::UclampCgroup, controller_name_, transaction_id_,
            priority)) {
        transaction_id_.clear();
        return false;
    }
    return true;
}
void UclampGovernor::release_mutation_lease_locked() noexcept {
    if (transaction_id_.empty()) return;
    coordinator_.release_lease(
        MutationDomain::UclampCgroup, controller_name_, transaction_id_);
    transaction_id_.clear();
}
bool UclampGovernor::rollback_transaction_locked() noexcept {
    if (transaction_id_.empty()) return !mutation_active_;
    auto& store = BaselineStore::instance();
    const std::string min_path = target_path_ + "/cpu.uclamp.min";
    const std::string max_path = target_path_ + "/cpu.uclamp.max";
    const bool min_ok = store.rollback(min_path);
    const bool max_ok = store.rollback(max_path);
    if (!min_ok || !max_ok) return false;

    const std::string min_raw = read_token(min_path);
    const std::string max_raw = read_token(max_path);
    if (min_raw.empty() || max_raw.empty()) return false;
    const auto audit = store.get_audit_trail();
    const auto matches_store = [&](const std::string& path, const std::string& raw) {
        const auto it = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
            return node.path == path;
        });
        return it != audit.end() && !it->is_modified && it->factory_value == raw;
    };
    if (!matches_store(min_path, min_raw) || !matches_store(max_path, max_raw)) return false;

    const uint32_t restored_min = read_uclamp_node(min_path);
    const uint32_t restored_max = read_uclamp_node(max_path);
    const bool original_baseline_restored =
        restored_min == oem_min_ && restored_max == oem_max_;
    last_min_ = restored_min;
    last_max_ = restored_max;
    mutation_active_ = false;
    has_written_ = false;
    if (!original_baseline_restored) {
        // BaselineStore adopted a changed OEM value after detecting an external owner.
        // The transaction is now relinquished, but this controller must rediscover.
        supported_ = false;
    }
    release_mutation_lease_locked();
    return true;
}
bool UclampGovernor::handle_preemption(
    MutationDomain domain, std::string_view transaction_id) noexcept {
    if (domain != MutationDomain::UclampCgroup) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    if (transaction_id_.empty() || transaction_id_ != transaction_id) return false;
    return rollback_transaction_locked();
}
bool UclampGovernor::owned_values_intact_locked() {
    if (!supported_) return false;
    if (!has_written_) return !mutation_active_;
    if (get_top_app_min() == last_min_ && get_top_app_max() == last_max_) return true;
    // An external actor changed one of the paired values. Reconcile the journal,
    // relinquish the lease only after both nodes are verified, then disable control.
    std::cerr << "[CoreFlow][Uclamp] external clamp change; relinquishing control\n";
    if (!transaction_id_.empty()) (void)rollback_transaction_locked();
    supported_ = false;
    return false;
}

bool UclampGovernor::is_supported() const noexcept {
    return supported_;
}

std::string_view UclampGovernor::encoding_name() const noexcept {
    switch (encoding_) {
        case Encoding::Percentage: return "percentage";
        case Encoding::Capacity1024: return "capacity1024";
        case Encoding::Unknown: break;
    }
    return "unknown";
}

void UclampGovernor::discover(std::string_view explicit_root) {
    supported_ = false;
    target_path_.clear();
    encoding_ = Encoding::Unknown;

    struct RootEntry { std::string path; MountKind kind; };
    std::vector<RootEntry> roots;
    if (!explicit_root.empty()) {
        // Explicit roots are a hermetic fixture/bring-up seam only. Production
        // roots always originate from the central mount topology resolver.
        roots.push_back(RootEntry{std::string(explicit_root), MountKind::Auto});
    } else {
        MountTopologyResolver topology{};
        const auto foreground = topology.resolve_foreground_uclamp_group();
        if (foreground) {
            roots.push_back(RootEntry{foreground->absolute_path,
                foreground->mount.filesystem == CgroupFilesystem::V2
                    ? MountKind::CgroupV2 : MountKind::CgroupV1});
        }
    }

    const auto clean_baseline = [](const std::string& directory) {
        auto& store = BaselineStore::instance();
        const std::string min_path = directory + "/cpu.uclamp.min";
        const std::string max_path = directory + "/cpu.uclamp.max";
        if (!store.capture(min_path) || !store.capture(max_path)) return false;
        const auto audit = store.get_audit_trail();
        const auto min_snapshot = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
            return node.path == min_path;
        });
        const auto max_snapshot = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
            return node.path == max_path;
        });
        return min_snapshot != audit.end() && max_snapshot != audit.end() &&
               !min_snapshot->is_modified && !max_snapshot->is_modified;
    };

    struct Pending { fs::path path; unsigned depth; };
    std::size_t visited = 0;
    constexpr std::size_t kMaxDirectories = 2048;
    constexpr unsigned kMaxDepth = 8;
    for (const auto& root : roots) {
        std::vector<Pending> pending{{fs::path(root.path), 0U}};
        while (!pending.empty() && visited < kMaxDirectories) {
            Pending current = std::move(pending.back());
            pending.pop_back();
            ++visited;
            Encoding candidate_encoding = Encoding::Unknown;
            if (validate_target(current.path.string(), root.kind, candidate_encoding) &&
                clean_baseline(current.path.string())) {
                target_path_ = current.path.string();
                encoding_ = candidate_encoding;
                supported_ = true;
                oem_min_ = get_top_app_min();
                oem_max_ = get_top_app_max();
                oem_min_raw_ = read_token(target_path_ + "/cpu.uclamp.min");
                oem_max_raw_ = read_token(target_path_ + "/cpu.uclamp.max");
                return;
            }
            if (current.depth >= kMaxDepth) continue;
            std::error_code ec;
            for (fs::directory_iterator it(current.path,
                    fs::directory_options::skip_permission_denied, ec), end;
                 !ec && it != end && visited + pending.size() < kMaxDirectories;
                 it.increment(ec)) {
                std::error_code type_error;
                if (it->is_symlink(type_error) || type_error) continue;
                if (!it->is_directory(type_error) || type_error) continue;
                pending.push_back({it->path(), current.depth + 1U});
            }
        }
    }
}

bool UclampGovernor::validate_target(const std::string& directory,
                                     MountKind kind,
                                     Encoding& encoding) const {
    (void)kind;
    if (directory.empty()) return false;
    const fs::path dir(directory);
    const fs::path min_path = dir / "cpu.uclamp.min";
    const fs::path max_path = dir / "cpu.uclamp.max";
    std::error_code ec;
    if (fs::is_symlink(min_path, ec) || ec || fs::is_symlink(max_path, ec) || ec) return false;
    if (!fs::exists(min_path, ec) || ec || !fs::exists(max_path, ec) || ec) return false;
    if (::access(min_path.c_str(), R_OK | W_OK) != 0 ||
        ::access(max_path.c_str(), R_OK | W_OK) != 0) return false;

    const std::string min_raw = read_token(min_path);
    const std::string max_raw = read_token(max_path);
    if (min_raw.empty() || max_raw.empty()) return false;
    const auto min_value = MountTopologyResolver::parse_uclamp_value(min_raw);
    const auto max_value = MountTopologyResolver::parse_uclamp_value(max_raw);
    if (!min_value || !max_value || min_value->is_max) return false;

    // The baseline bytes determine the ABI encoding. A literal max is an
    // unbounded ceiling sentinel and is accepted with either numeric encoding.
    if (max_value->is_max) {
        encoding = min_value->encoding == UclampEncoding::Capacity1024
            ? Encoding::Capacity1024 : Encoding::Percentage;
        return true;
    }
    if (min_value->encoding != max_value->encoding || min_value->capacity > max_value->capacity)
        return false;
    encoding = min_value->encoding == UclampEncoding::Capacity1024
        ? Encoding::Capacity1024 : Encoding::Percentage;
    return encoding != Encoding::Unknown;
}

bool UclampGovernor::apply_state(WorkloadState state,
                                 uint32_t balanced_min,
                                 uint32_t balanced_max) {
    if (!owned_values_intact()) return false;
    // A user/OEM ceiling is authoritative, including a temporary thermal cap.
    const uint32_t base_max = std::min({balanced_max, oem_max_, 1024u});
    const uint32_t base_min = balanced_min > base_max ? 0u : std::min(balanced_min, base_max);

    UclampValues top{.uclamp_min = base_min, .uclamp_max = base_max};
    switch (state) {
        case WorkloadState::Balanced:
            break;
        case WorkloadState::SustainedLoad:
            top.uclamp_min = std::min(base_max, base_min + std::max(1u, base_min / 2u));
            break;
        case WorkloadState::PressureGuard:
            top.uclamp_min = base_min / 4u;
            break;
        case WorkloadState::ThermalGuard:
            top.uclamp_min = 0;
            top.uclamp_max = base_max == 0 ? 0u : std::max(1u, (base_max * 7u) / 8u);
            break;
    }
    const auto priority = state == WorkloadState::ThermalGuard
        ? ControllerPriority::ThermalStabilization
        : state == WorkloadState::PressureGuard
            ? ControllerPriority::PressureGuard
            : state == WorkloadState::Balanced
                ? ControllerPriority::BackgroundEfficiency
                : ControllerPriority::WorkloadOptimization;
    return write_top_app(top.uclamp_min, top.uclamp_max, priority);
}

bool UclampGovernor::apply_budget(WorkloadState state, uint32_t floor,
                                 uint32_t ceiling, uint32_t capacity) {
    if (capacity > 1024u || !owned_values_intact()) return false;
    const auto base_max = std::min({ceiling, oem_max_, 1024u});
    const auto maximum = (base_max * capacity) / 1024u;
    uint32_t minimum = floor > base_max ? 0u : std::min(floor, base_max);
    if (state == WorkloadState::ThermalGuard) minimum = 0;
    else if (state == WorkloadState::PressureGuard) minimum /= 4;
    else if (state == WorkloadState::SustainedLoad) minimum += minimum / 2;
    const auto priority = state == WorkloadState::ThermalGuard
        ? ControllerPriority::ThermalStabilization
        : state == WorkloadState::PressureGuard
            ? ControllerPriority::PressureGuard
            : state == WorkloadState::Balanced
                ? ControllerPriority::BackgroundEfficiency
                : ControllerPriority::WorkloadOptimization;
    return write_top_app(std::min(minimum, maximum), maximum, priority);
}

bool UclampGovernor::write_top_app(uint32_t min_val, uint32_t max_val, ControllerPriority priority) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!supported_ || !owned_values_intact_locked()) return false;
    min_val = std::clamp(min_val, 0u, 1024u);
    max_val = std::clamp(max_val, min_val, 1024u);

    auto& baseline = BaselineStore::instance();
    const std::string min_path = target_path_ + "/cpu.uclamp.min";
    const std::string max_path = target_path_ + "/cpu.uclamp.max";
    const auto audit_before = baseline.get_audit_trail();
    const auto min_snapshot = std::find_if(audit_before.begin(), audit_before.end(),
        [&](const NodeSnapshot& node) { return node.path == min_path; });
    const auto max_snapshot = std::find_if(audit_before.begin(), audit_before.end(),
        [&](const NodeSnapshot& node) { return node.path == max_path; });
    if (min_snapshot == audit_before.end() || max_snapshot == audit_before.end()) return false;
    if (!mutation_active_ && (min_snapshot->is_modified || max_snapshot->is_modified)) {
        // Do not adopt a pre-existing journaled pair that this governor did not mutate.
        return false;
    }
    const uint32_t current_min = read_uclamp_node(min_path);
    if (read_uclamp_node(max_path) == max_val && current_min == min_val) return true;
    if (!acquire_mutation_lease_locked(priority)) return false;

    // Reuse the exact captured OEM representation when restoring a factory
    // value. Semantic aliases ("max" and "100.00") must not leave the journal dirty.
    const std::string encoded_min = min_val == oem_min_
        ? oem_min_raw_ : encode(min_val, false);
    const std::string encoded_max = max_val == oem_max_
        ? oem_max_raw_ : encode(max_val, true);

    bool success = false;
    if (max_val < current_min) {
        success = baseline.apply_mutation(min_path, encoded_min) &&
                  baseline.apply_mutation(max_path, encoded_max);
    } else {
        success = baseline.apply_mutation(max_path, encoded_max) &&
                  baseline.apply_mutation(min_path, encoded_min);
    }
    if (!success) {
        mutation_active_ = true; // A paired write may have partially committed.
        if (!rollback_transaction_locked()) return false;
        return false;
    }

    mutation_active_ = true;
    last_min_ = get_top_app_min();
    last_max_ = get_top_app_max();
    if (last_min_ != min_val || last_max_ != max_val) {
        if (!rollback_transaction_locked()) return false;
        return false;
    }
    has_written_ = true;
    if (last_min_ == oem_min_ && last_max_ == oem_max_) {
        mutation_active_ = false;
        has_written_ = false;
        release_mutation_lease_locked();
    }
    return true;
}

bool UclampGovernor::owned_values_intact() {
    std::lock_guard<std::mutex> lock(mutex_);
    return owned_values_intact_locked();
}

bool UclampGovernor::resync() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (target_path_.empty()) {
        supported_ = false;
        return false;
    }
    if (!transaction_id_.empty() && !rollback_transaction_locked()) {
        // Preserve the active transaction and lease for later recovery, but never
        // continue issuing policy writes after an unverified resynchronization.
        supported_ = false;
        return false;
    }
    auto& baseline = BaselineStore::instance();
    const std::string min_path = target_path_ + "/cpu.uclamp.min";
    const std::string max_path = target_path_ + "/cpu.uclamp.max";
    const auto audit_before = baseline.get_audit_trail();
    const auto min_snapshot = std::find_if(audit_before.begin(), audit_before.end(),
        [&](const NodeSnapshot& node) { return node.path == min_path; });
    const auto max_snapshot = std::find_if(audit_before.begin(), audit_before.end(),
        [&](const NodeSnapshot& node) { return node.path == max_path; });
    if (min_snapshot == audit_before.end() || max_snapshot == audit_before.end() ||
        min_snapshot->is_modified || max_snapshot->is_modified) {
        // Resync may not globally adopt or roll back a pair owned by another controller.
        supported_ = false;
        return false;
    }
    if (!baseline.rollback(min_path) || !baseline.rollback(max_path)) {
        supported_ = false;
        return false;
    }
    const std::string min_raw = read_token(target_path_ + "/cpu.uclamp.min");
    const std::string max_raw = read_token(target_path_ + "/cpu.uclamp.max");
    if (min_raw.empty() || max_raw.empty()) {
        supported_ = false;
        return false;
    }
    const uint32_t restored_min = read_uclamp_node(target_path_ + "/cpu.uclamp.min");
    const uint32_t restored_max = read_uclamp_node(target_path_ + "/cpu.uclamp.max");
    if (restored_min > restored_max || restored_max > 1024u) {
        supported_ = false;
        return false;
    }
    oem_min_ = restored_min;
    oem_max_ = restored_max;
    oem_min_raw_ = min_raw;
    oem_max_raw_ = max_raw;
    last_min_ = restored_min;
    last_max_ = restored_max;
    has_written_ = false;
    mutation_active_ = false;
    supported_ = true;
    return true;
}


std::string UclampGovernor::encode(uint32_t capacity, bool maximum) const {
    return MountTopologyResolver::encode_uclamp_value(
        capacity, encoding_ == Encoding::Capacity1024
            ? UclampEncoding::Capacity1024 : UclampEncoding::Percent, maximum);
}

uint32_t UclampGovernor::get_top_app_min() const {
    return supported_ ? read_uclamp_node(target_path_ + "/cpu.uclamp.min") : 0;
}

uint32_t UclampGovernor::get_top_app_max() const {
    return supported_ ? read_uclamp_node(target_path_ + "/cpu.uclamp.max") : 0;
}

uint32_t UclampGovernor::read_uclamp_node(const std::string& path) const {
    const std::string raw = read_token(path);
    const auto parsed = MountTopologyResolver::parse_uclamp_value(raw);
    if (!parsed) return 0;
    return parsed->is_max ? 1024U : parsed->capacity;
}

} // namespace coreflow
