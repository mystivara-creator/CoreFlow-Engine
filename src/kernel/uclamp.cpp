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

std::string decode_mount_path(std::string_view encoded) {
    std::string out;
    out.reserve(encoded.size());
    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '\\' && i + 3 < encoded.size()) {
            const std::string_view code = encoded.substr(i + 1, 3);
            if (code == "040") { out.push_back(' '); i += 3; continue; }
            if (code == "011") { out.push_back('\t'); i += 3; continue; }
            if (code == "012") { out.push_back('\n'); i += 3; continue; }
            if (code == "134") { out.push_back('\\'); i += 3; continue; }
        }
        out.push_back(encoded[i]);
    }
    return out;
}

struct CgroupMount {
    std::string path;
    bool is_v2{false};
};

std::vector<CgroupMount> cgroup_mounts() {
    std::vector<CgroupMount> mounts;
    std::ifstream file("/proc/self/mountinfo");
    std::string line;
    while (std::getline(file, line)) {
        const auto separator = line.find(" - ");
        if (separator == std::string::npos) continue;

        std::istringstream before(line.substr(0, separator));
        std::string id, parent, major_minor, root, mount_point;
        if (!(before >> id >> parent >> major_minor >> root >> mount_point)) continue;

        std::istringstream after(line.substr(separator + 3));
        std::string fs_type;
        if (!(after >> fs_type)) continue;
        if (fs_type != "cgroup" && fs_type != "cgroup2") continue;

        auto decoded = decode_mount_path(mount_point);
        if (decoded.empty()) continue;
        const bool already_present = std::any_of(
            mounts.begin(), mounts.end(),
            [&decoded](const CgroupMount& m) { return m.path == decoded; });
        if (!already_present) {
            mounts.push_back(CgroupMount{std::move(decoded), fs_type == "cgroup2"});
        }
    }
    return mounts;
}

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

UclampGovernor::UclampGovernor(std::string_view explicit_root) {
    discover(explicit_root);
    if (supported_) {
        controller_name_ = "UclampGovernor:" + target_path_;
        (void)CentralMutationCoordinator::instance().register_preemption_handler(
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
        CentralMutationCoordinator::instance().unregister_preemption_handler(controller_name_);
    }
}
bool UclampGovernor::acquire_mutation_lease_locked(ControllerPriority priority) {
    if (!transaction_id_.empty()) return true;
    transaction_id_ = "uclamp-" + std::to_string(++transaction_generation_);
    if (!CentralMutationCoordinator::instance().acquire_lease(
            MutationDomain::UclampCgroup, controller_name_, transaction_id_,
            priority)) {
        transaction_id_.clear();
        return false;
    }
    return true;
}
void UclampGovernor::release_mutation_lease_locked() noexcept {
    if (transaction_id_.empty()) return;
    CentralMutationCoordinator::instance().release_lease(
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
    const auto matches_store = [&](const std::string& path, const std::string& raw,
                                   const std::string& expected) {
        const auto it = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
            return node.path == path;
        });
        if (it == audit.end()) return raw == expected;
        return !it->is_modified && it->factory_value == raw;
    };
    if (!matches_store(min_path, min_raw, oem_min_raw_) ||
        !matches_store(max_path, max_raw, oem_max_raw_)) {
        return false;
    }

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

    struct RootEntry {
        std::string path;
        MountKind kind;
    };
    std::vector<RootEntry> roots;
    if (!explicit_root.empty()) {
        roots.push_back(RootEntry{std::string(explicit_root), MountKind::Auto});
    } else {
        for (auto& mount : cgroup_mounts()) {
            roots.push_back(RootEntry{
                std::move(mount.path),
                mount.is_v2 ? MountKind::CgroupV2 : MountKind::CgroupV1,
            });
        }
    }

    struct Pending { fs::path path; unsigned depth; };
    size_t visited = 0;
    constexpr size_t kMaxDirectories = 192;
    constexpr unsigned kMaxDepth = 2;

    for (const auto& root : roots) {
        std::vector<Pending> pending{{fs::path(root.path), 0}};
        while (!pending.empty() && visited < kMaxDirectories) {
            Pending current = std::move(pending.back());
            pending.pop_back();
            ++visited;

            Encoding candidate_encoding = Encoding::Unknown;
            if (current.path.filename() == "top-app" &&
                validate_target(current.path.string(), root.kind, candidate_encoding)) {
                target_path_ = current.path.string();
                encoding_ = candidate_encoding;
                supported_ = true;
                oem_min_ = get_top_app_min();
                oem_max_ = get_top_app_max();
                oem_min_raw_ = read_token(target_path_ + "/cpu.uclamp.min");
                oem_max_raw_ = read_token(target_path_ + "/cpu.uclamp.max");
                return;
            }

            // Common case: controller mount directly owns top-app.
            const fs::path direct = current.path / "top-app";
            if (validate_target(direct.string(), root.kind, candidate_encoding)) {
                target_path_ = direct.string();
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
            for (fs::directory_iterator it(current.path, fs::directory_options::skip_permission_denied, ec), end;
                 !ec && it != end && visited + pending.size() < kMaxDirectories;
                 it.increment(ec)) {
                std::error_code type_error;
                // Do not traverse symlinked cgroup subdirectories.
                if (it->is_symlink(type_error) || type_error) continue;
                if (!it->is_directory(type_error) || type_error) continue;
                pending.push_back({it->path(), current.depth + 1});
            }
        }
    }
}

bool UclampGovernor::validate_target(const std::string& directory,
                                     MountKind kind,
                                     Encoding& encoding) const {
    if (directory.empty()) return false;
    const fs::path dir(directory);
    if (dir.filename() != "top-app") return false;

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

    double min_value = 0.0, max_value = 0.0;
    if (!parse_ascii_double(min_raw, min_value) || min_value < 0.0) return false;

    std::error_code ctrl_ec1, ctrl_ec2;
    const bool has_v2_controllers =
        (fs::exists(dir / "cgroup.controllers", ctrl_ec1) && !ctrl_ec1) ||
        (fs::exists(dir.parent_path() / "cgroup.controllers", ctrl_ec2) && !ctrl_ec2);
    const bool is_cgroup_v2 = (kind == MountKind::CgroupV2) || has_v2_controllers;

    if (max_raw == "max") {
        if (min_value > 100.0) return false;
        encoding = Encoding::Percentage;
        return true;
    }
    if (!parse_ascii_double(max_raw, max_value) || max_value < 0.0) return false;

    if (is_cgroup_v2 ||
        min_raw.find('.') != std::string::npos ||
        max_raw.find('.') != std::string::npos) {
        encoding = Encoding::Percentage;
        return min_value <= 100.0 && max_value <= 100.0;
    }
    if (min_value <= 1024.0 && max_value <= 1024.0) {
        encoding = Encoding::Capacity1024;
        return true;
    }
    return false;
}

bool UclampGovernor::apply_state(WorkloadState state,
                                 uint32_t balanced_min,
                                 uint32_t balanced_max) {
    if (!supported_) return false;

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

std::string UclampGovernor::encode(uint32_t capacity, bool maximum) const {
    capacity = std::min(capacity, 1024u);
    if (encoding_ == Encoding::Capacity1024) return std::to_string(capacity);
    if (maximum && capacity >= 1024u) return "max";

    // Pure integer fixed-point formatting (2 decimal places), immune to LC_NUMERIC locale.
    const uint32_t hundredths = (capacity * 10000u + 512u) / 1024u;
    const uint32_t whole = hundredths / 100u;
    const uint32_t frac = hundredths % 100u;
    std::string out = std::to_string(whole);
    out.push_back('.');
    out.push_back(static_cast<char>('0' + (frac / 10u)));
    out.push_back(static_cast<char>('0' + (frac % 10u)));
    return out;
}

bool UclampGovernor::apply_budget(WorkloadState state, uint32_t floor,
                                 uint32_t ceiling, uint32_t capacity) {
    if (!supported_ || !owned_values_intact() || capacity > 1024u) return false;
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
    const uint32_t current_min = read_uclamp_node(min_path);
    if (read_uclamp_node(max_path) == max_val && current_min == min_val) return true;
    if (!acquire_mutation_lease_locked(priority)) return false;

    bool success = false;
    if (max_val < current_min) {
        success = baseline.apply_mutation(min_path, encode(min_val, false)) &&
                  baseline.apply_mutation(max_path, encode(max_val, true));
    } else {
        success = baseline.apply_mutation(max_path, encode(max_val, true)) &&
                  baseline.apply_mutation(min_path, encode(min_val, false));
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
    if (target_path_.empty()) return false;
    if (!transaction_id_.empty() && !rollback_transaction_locked()) return false;
    supported_ = true;
    has_written_ = false;
    mutation_active_ = false;
    auto& baseline = BaselineStore::instance();
    if (!baseline.rollback(target_path_ + "/cpu.uclamp.min") ||
        !baseline.rollback(target_path_ + "/cpu.uclamp.max")) {
        return false;
    }
    oem_min_ = get_top_app_min();
    oem_max_ = get_top_app_max();
    oem_min_raw_ = read_token(target_path_ + "/cpu.uclamp.min");
    oem_max_raw_ = read_token(target_path_ + "/cpu.uclamp.max");
    last_min_ = oem_min_;
    last_max_ = oem_max_;
    return true;
}

uint32_t UclampGovernor::get_top_app_min() const {
    return supported_ ? read_uclamp_node(target_path_ + "/cpu.uclamp.min") : 0;
}

uint32_t UclampGovernor::get_top_app_max() const {
    return supported_ ? read_uclamp_node(target_path_ + "/cpu.uclamp.max") : 0;
}

uint32_t UclampGovernor::read_uclamp_node(const std::string& path) const {
    const std::string raw = read_token(path);
    if (raw.empty()) return 0;
    if (raw == "max") return 1024;

    double value = 0.0;
    if (!parse_ascii_double(raw, value) || value < 0.0) return 0;
    if (encoding_ == Encoding::Capacity1024) {
        return static_cast<uint32_t>(std::clamp(std::lround(value), 0l, 1024l));
    }
    if (value > 100.0) return 0;
    return static_cast<uint32_t>(std::clamp(std::lround(value * 1024.0 / 100.0), 0l, 1024l));
}

} // namespace coreflow
