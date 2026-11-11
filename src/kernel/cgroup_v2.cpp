/**
 * @file cgroup_v2.cpp
 * @brief cgroups v2 implementation with coordinator-owned CPU policy transactions.
 * @author mystivara-creator
 * @license Apache-2.0
 */

#include "coreflow/cgroup_v2.hpp"
#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/device_profiler.hpp"
#include "coreflow/unique_fd.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {
namespace fs = std::filesystem;

namespace {

constexpr std::array<const char*, 4> kManagedCgroups{
    "top-app", "foreground", "background", "system"
};
constexpr std::array<const char*, 3> kDelegatedControllers{
    "cpu", "memory", "io"
};

bool token_present(const char* data, size_t size, std::string_view token) noexcept {
    size_t pos = 0;
    while (pos < size) {
        while (pos < size && (data[pos] == ' ' || data[pos] == '\n' ||
                              data[pos] == '\t' || data[pos] == '+' )) ++pos;
        const size_t begin = pos;
        while (pos < size && data[pos] != ' ' && data[pos] != '\n' &&
               data[pos] != '\t') ++pos;
        if (pos - begin == token.size() &&
            std::memcmp(data + begin, token.data(), token.size()) == 0) return true;
    }
    return false;
}

ssize_t read_at(int directory_fd, const char* name, char* buffer, size_t capacity) noexcept {
    UniqueFd fd(::openat(directory_fd, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return -1;
    ssize_t count;
    do { count = ::read(fd.get(), buffer, capacity); } while (count < 0 && errno == EINTR);
    return count;
}

int open_cgroup_dir(int root_fd, const char* name) noexcept {
    if (name == nullptr || name[0] == '\\0') return ::fcntl(root_fd, F_DUPFD_CLOEXEC, 0);
    return ::openat(root_fd, name, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
}
bool capture_thread_membership(pid_t tid) noexcept {
    char status_path[64];
    const int status_length = std::snprintf(status_path, sizeof(status_path),
                                             "/proc/%d/status", tid);
    if (status_length <= 0 || static_cast<size_t>(status_length) >= sizeof(status_path)) return false;
    UniqueFd status_fd(::open(status_path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!status_fd.valid()) return false;
    char status[4096];
    ssize_t count;
    do { count = ::read(status_fd.get(), status, sizeof(status)); }
    while (count < 0 && errno == EINTR);
    if (count <= 0) return false;

    pid_t tgid = 0;
    size_t pos = 0;
    while (pos < static_cast<size_t>(count)) {
        size_t end = pos;
        while (end < static_cast<size_t>(count) && status[end] != '\n') ++end;
        constexpr std::string_view key{"Tgid:"};
        if (end - pos >= key.size() &&
            std::memcmp(status + pos, key.data(), key.size()) == 0) {
            size_t value = pos + key.size();
            while (value < end && (status[value] == ' ' || status[value] == '\t')) ++value;
            const auto parsed = std::from_chars(status + value, status + end, tgid);
            if (parsed.ec != std::errc{} || parsed.ptr != status + end || tgid <= 0) return false;
            break;
        }
        pos = end + 1;
    }
    if (tgid <= 0) return false;

    char membership_path[96];
    const int path_length = std::snprintf(membership_path, sizeof(membership_path),
        "/proc/%d/task/%d/cgroup", tgid, tid);
    if (path_length <= 0 || static_cast<size_t>(path_length) >= sizeof(membership_path)) return false;
    // This per-thread proc record captures the exact original placement before
    // the cgroup.threads write; unlike a cgroup.threads snapshot it is TID-specific.
    return BaselineStore::instance().capture(std::string(membership_path));
}


} // namespace


CgroupV2Controller::CgroupV2Controller(CentralMutationCoordinator& coordinator, std::string_view root_path)
    : coordinator_(coordinator), root_path_(root_path),
      controller_name_(CentralMutationCoordinator::make_controller_identity("CgroupV2Controller")) {
    const std::string controllers_file = root_path_ + "/cgroup.controllers";
    supported_ = fs::exists(controllers_file);
    (void)coordinator_.register_preemption_handler(
        controller_name_,
        [this](MutationDomain domain, std::string_view transaction_id, std::string_view) noexcept {
            return handle_preemption(domain, transaction_id);
        });

    if (supported_) {
        std::cout << "[CoreFlow][CgroupV2] Modern cgroups v2 verified at "
                  << root_path_ << std::endl;
    } else {
        std::cerr << "[CoreFlow][CgroupV2] Notice: cgroups v2 not mounted at "
                  << root_path_ << ", attempting fallback discovery..." << std::endl;
    }
}

CgroupV2Controller::~CgroupV2Controller() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!transaction_id_.empty()) (void)rollback_transaction_locked();
    }
    // unregister waits for any callback that was already dispatched.
    coordinator_.unregister_preemption_handler(controller_name_);
}

bool CgroupV2Controller::is_supported() const noexcept {
    return supported_;
}

std::string CgroupV2Controller::get_cgroup_path(CgroupTarget target) const {
    switch (target) {
        case CgroupTarget::TopApp: return root_path_ + "/top-app";
        case CgroupTarget::Foreground: return root_path_ + "/foreground";
        case CgroupTarget::Background: return root_path_ + "/background";
        case CgroupTarget::System: return root_path_ + "/system";
    }
    return root_path_;
}

bool CgroupV2Controller::migrate_task(pid_t tid, CgroupTarget target) {
    if (!supported_ || tid <= 0 || root_path_.empty()) return false;

    const char* target_name = nullptr;
    switch (target) {
        case CgroupTarget::TopApp: target_name = "top-app"; break;
        case CgroupTarget::Foreground: target_name = "foreground"; break;
        case CgroupTarget::Background: target_name = "background"; break;
        case CgroupTarget::System: target_name = "system"; break;
        default: return false;
    }

    UniqueFd root_fd(::open(root_path_.c_str(),
        O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    if (!root_fd.valid()) return false;

    // Resolve the TID's current managed membership before touching the destination.
    // Only cgroup.threads is used: cgroup.procs would migrate the entire process.
    char tid_list[4096];
    const int destination_fd_raw = open_cgroup_dir(root_fd.get(), target_name);
    UniqueFd destination_fd(destination_fd_raw);
    if (!destination_fd.valid()) return false;
    const ssize_t destination_size = read_at(destination_fd.get(), "cgroup.threads",
                                               tid_list, sizeof(tid_list));
    if (destination_size < 0) return false;
    const std::string_view tid_text = [&]() noexcept {
        static thread_local char value[16];
        const auto converted = std::to_chars(value, value + sizeof(value), tid);
        return converted.ec == std::errc{} ?
            std::string_view(value, static_cast<size_t>(converted.ptr - value)) :
            std::string_view{};
    }();
    if (tid_text.empty()) return false;

    int source_fd_raw = -1;
    const char* source_name = nullptr;
    char membership[4096];
    const auto locate_tid = [&](int directory_fd, const char* name) noexcept {
        const ssize_t count = read_at(directory_fd, "cgroup.threads",
                                      membership, sizeof(membership));
        if (count < 0) return false;
        if (!token_present(membership, static_cast<size_t>(count), tid_text)) return false;
        source_fd_raw = ::dup(directory_fd);
        source_name = name;
        return source_fd_raw >= 0;
    };

    if (!locate_tid(root_fd.get(), "")) {
        for (const char* name : kManagedCgroups) {
            UniqueFd candidate(open_cgroup_dir(root_fd.get(), name));
            if (!candidate.valid()) continue;
            if (locate_tid(candidate.get(), name)) break;
        }
    }
    UniqueFd source_fd(source_fd_raw);
    if (!source_fd.valid() || source_name == nullptr) return false;

    // Already in the destination: success is based on an actual membership readback.
    if (std::strcmp(source_name, target_name) == 0) return true;

    // Capture the exact thread's original /proc membership record. This avoids
    // treating the whole cgroup.threads list as a rollback value (which could move
    // unrelated threads) and remains immutable after the migration.
    if (!capture_thread_membership(tid)) return false;

    UniqueFd target_threads(::openat(destination_fd.get(), "cgroup.threads",
        O_WRONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!target_threads.valid()) {
        // ENOENT (task/group lifecycle), EPERM and EINVAL are expected kernel denials.
        return false;
    }

    char tid_buffer[24];
    const int tid_length = std::snprintf(tid_buffer, sizeof(tid_buffer), "%d", tid);
    if (tid_length <= 0 || static_cast<size_t>(tid_length) >= sizeof(tid_buffer)) return false;
    ssize_t written;
    do { written = ::write(target_threads.get(), tid_buffer, static_cast<size_t>(tid_length)); }
    while (written < 0 && errno == EINTR);
    if (written != tid_length) return false;

    // The kernel's cgroup.threads view is authoritative; a successful write alone
    // is insufficient because a task may exit or be rejected during migration.
    char verify[4096];
    const ssize_t verified_size = read_at(destination_fd.get(), "cgroup.threads",
                                           verify, sizeof(verify));
    return verified_size >= 0 &&
        token_present(verify, static_cast<size_t>(verified_size), tid_text);
}

bool CgroupV2Controller::set_cpuset(CgroupTarget target, std::string_view cpus) {
    if (!supported_) return false;
    return write_node(get_cgroup_path(target) + "/cpuset.cpus", cpus);
}

bool CgroupV2Controller::set_cpu_weight(CgroupTarget target, uint32_t weight) {
    if (!supported_) return false;
    weight = std::clamp(weight, 1u, 10000u);
    return write_node(get_cgroup_path(target) + "/cpu.weight", std::to_string(weight));
}

bool CgroupV2Controller::unlock_vendor_restrictions() {
    if (!supported_ || root_path_.empty()) return false;

    // The configured root descriptor is the authority boundary. All subsequent
    // operations are relative openat() calls and cannot escape via child symlinks.
    UniqueFd root_fd(::open(root_path_.c_str(),
        O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    if (!root_fd.valid()) return false;

    char available[512];
    const ssize_t available_size = read_at(root_fd.get(), "cgroup.controllers",
                                            available, sizeof(available));
    if (available_size <= 0) return false;

    char command[64];
    size_t command_size = 0;
    bool selected[3]{false, false, false};
    for (size_t i = 0; i < kDelegatedControllers.size(); ++i) {
        const std::string_view controller(kDelegatedControllers[i]);
        if (!token_present(available, static_cast<size_t>(available_size), controller)) continue;
        selected[i] = true;
        char token[16];
        const int n = std::snprintf(token, sizeof(token), "%s%s",
                                    command_size == 0 ? "+" : " +", kDelegatedControllers[i]);
        if (n <= 0 || static_cast<size_t>(n) >= sizeof(token) ||
            command_size + static_cast<size_t>(n) > sizeof(command)) return false;
        std::memcpy(command + command_size, token, static_cast<size_t>(n));
        command_size += static_cast<size_t>(n);
    }
    if (command_size == 0) return false;

    char enabled[512];
    ssize_t enabled_size = read_at(root_fd.get(), "cgroup.subtree_control",
                                   enabled, sizeof(enabled));
    if (enabled_size < 0) return false;
    bool complete = true;
    for (size_t i = 0; i < selected.size(); ++i) {
        if (selected[i] && !token_present(enabled, static_cast<size_t>(enabled_size),
                                          kDelegatedControllers[i])) complete = false;
    }
    if (!complete) {
        UniqueFd control(::openat(root_fd.get(), "cgroup.subtree_control",
            O_WRONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
        if (!control.valid()) return false; // EACCES/ENOENT: remain fail-closed.
        ssize_t written;
        do { written = ::write(control.get(), command, command_size); }
        while (written < 0 && errno == EINTR);
        if (written != static_cast<ssize_t>(command_size)) {
            // EBUSY (populated hierarchy) and EINVAL (unsupported controller /
            // hierarchy constraints) are ordinary capability denials.
            return false;
        }
    }

    enabled_size = read_at(root_fd.get(), "cgroup.subtree_control",
                           enabled, sizeof(enabled));
    if (enabled_size < 0) return false;
    for (size_t i = 0; i < selected.size(); ++i) {
        if (selected[i] && !token_present(enabled, static_cast<size_t>(enabled_size),
                                          kDelegatedControllers[i])) return false;
    }
    return true;
}

bool CgroupV2Controller::acquire_lease_locked() {
    if (!transaction_id_.empty()) {
        return coordinator_.acquire_lease(
            MutationDomain::CpuPolicy, controller_name_, transaction_id_,
            ControllerPriority::WorkloadOptimization);
    }
    transaction_id_ = "cgroup-cpu-" + std::to_string(++transaction_generation_);
    if (!coordinator_.acquire_lease(
            MutationDomain::CpuPolicy, controller_name_, transaction_id_,
            ControllerPriority::WorkloadOptimization)) {
        transaction_id_.clear();
        return false;
    }
    return true;
}

void CgroupV2Controller::release_lease_locked() noexcept {
    if (transaction_id_.empty()) return;
    coordinator_.release_lease(
        MutationDomain::CpuPolicy, controller_name_, transaction_id_);
    transaction_id_.clear();
}

bool CgroupV2Controller::rollback_transaction_locked() noexcept {
    if (transaction_id_.empty()) return active_mutation_paths_.empty();
    auto& store = BaselineStore::instance();
    for (const auto& path : active_mutation_paths_) {
        if (!store.rollback(path)) return false;
        const auto audit = store.get_audit_trail();
        const auto it = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
            return node.path == path;
        });
        if (it == audit.end() || it->is_modified || read_node(path) != it->factory_value) {
            return false;
        }
    }
    active_mutation_paths_.clear();
    release_lease_locked();
    return true;
}

bool CgroupV2Controller::handle_preemption(
    MutationDomain domain, std::string_view transaction_id) noexcept {
    if (domain != MutationDomain::CpuPolicy) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    if (transaction_id_.empty() || transaction_id_ != transaction_id) return false;
    return rollback_transaction_locked();
}

bool CgroupV2Controller::write_node(const std::string& path, std::string_view value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& store = BaselineStore::instance();
    if (!store.capture(path)) return false;

    const auto audit_before = store.get_audit_trail();
    const auto* snapshot_before = [&]() -> const NodeSnapshot* {
        const auto it = std::find_if(audit_before.begin(), audit_before.end(),
            [&](const NodeSnapshot& node) { return node.path == path; });
        return it == audit_before.end() ? nullptr : &*it;
    }();
    if (!snapshot_before) return false;

    const bool path_already_owned = std::find(
        active_mutation_paths_.begin(), active_mutation_paths_.end(), path) !=
        active_mutation_paths_.end();
    if (snapshot_before->is_modified && !path_already_owned) {
        // BaselineStore has a live mutation on this path, but this controller
        // did not add it to its transaction. Never extend or overwrite a foreign
        // or legacy mutation merely because this controller already owns the domain.
        return false;
    }

    // Verify every node still belongs to this transaction before adding another
    // CPU-policy mutation. An OEM change causes a verified handoff, never reassertion.
    if (!active_mutation_paths_.empty()) {
        for (const auto& owned_path : active_mutation_paths_) {
            const auto it = std::find_if(audit_before.begin(), audit_before.end(),
                [&](const NodeSnapshot& node) { return node.path == owned_path; });
            if (it == audit_before.end() ||
                !store.owns_expected_state(owned_path, it->active_value)) {
                if (!rollback_transaction_locked()) return false;
                return false;
            }
        }
    } else if (snapshot_before->is_modified) {
        // A mutation without this controller's lease is not ours to extend.
        return false;
    }

    const std::string current = read_node(path);
    const std::string requested(value);
    if (current.empty()) return false;
    if (current == requested) return true;

    if (requested == snapshot_before->factory_value) {
        if (!store.rollback(path) || read_node(path) != snapshot_before->factory_value) return false;
        active_mutation_paths_.erase(
            std::remove(active_mutation_paths_.begin(), active_mutation_paths_.end(), path),
            active_mutation_paths_.end());
        if (active_mutation_paths_.empty()) release_lease_locked();
        return true;
    }

    if (!acquire_lease_locked()) return false;
    const bool applied = store.apply_mutation(path, requested);
    const std::string readback = read_node(path);
    if (readback != snapshot_before->factory_value &&
        std::find(active_mutation_paths_.begin(), active_mutation_paths_.end(), path) ==
            active_mutation_paths_.end()) {
        // A failed write can still have partially changed a control node. Track
        // every non-baseline readback before deciding whether the transaction failed.
        active_mutation_paths_.push_back(path);
    }

    if (!applied || readback != requested) {
        if (active_mutation_paths_.empty()) {
            // First write did not change the node: do not strand a lease that owns
            // no mutation. Existing controller-owned mutations, if any, are handled
            // by the transaction rollback below.
            release_lease_locked();
        } else {
            // Keep CgroupV2Controller atomic at transaction scope: if adding a node
            // fails, restore every node this controller changed before releasing.
            (void)rollback_transaction_locked();
        }
        return false;
    }
    return true;
}

std::string CgroupV2Controller::read_node(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) return {};
    std::string value;
    file >> value;
    return value;
}

} // namespace coreflow

