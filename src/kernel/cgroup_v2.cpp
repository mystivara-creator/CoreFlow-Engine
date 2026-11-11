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
#include <iterator>
#include <sstream>
#include <string_view>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {
namespace fs = std::filesystem;

namespace {

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

bool read_bounded_file(const std::string& path, std::string& output,
                       size_t maximum = 16384U) noexcept {
    output.clear();
    UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return false;
    char buffer[1024];
    for (;;) {
        ssize_t count = ::read(fd.get(), buffer, sizeof(buffer));
        if (count < 0 && errno == EINTR) continue;
        if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
        if (count < 0) return false;
        if (count == 0) break;
        if (output.size() + static_cast<size_t>(count) > maximum) return false;
        output.append(buffer, static_cast<size_t>(count));
    }
    return true; // Empty cgroup.procs is a valid no-foreground-app snapshot.
}

bool parse_tgid_from_status(std::string_view status, pid_t& tgid) noexcept {
    size_t pos = 0;
    while (pos < status.size()) {
        const size_t end = status.find('\n', pos);
        const std::string_view line = status.substr(
            pos, end == std::string_view::npos ? status.size() - pos : end - pos);
        if (line.rfind("Tgid:", 0) == 0) {
            size_t value = 5;
            while (value < line.size() && (line[value] == ' ' || line[value] == '\t')) ++value;
            const auto parsed = std::from_chars(line.data() + value, line.data() + line.size(), tgid);
            return parsed.ec == std::errc{} && parsed.ptr == line.data() + line.size() && tgid > 0;
        }
        if (end == std::string_view::npos) break;
        pos = end + 1;
    }
    return false;
}

bool parse_start_time_from_stat(std::string_view stat, uint64_t& start_time) noexcept {
    const size_t close = stat.rfind(')');
    if (close == std::string_view::npos || close + 1 >= stat.size()) return false;
    size_t pos = close + 1;
    unsigned field = 3;
    while (pos < stat.size()) {
        while (pos < stat.size() && (stat[pos] == ' ' || stat[pos] == '\t')) ++pos;
        const size_t begin = pos;
        while (pos < stat.size() && stat[pos] != ' ' && stat[pos] != '\t' &&
               stat[pos] != '\n') ++pos;
        if (begin == pos) break;
        if (field == 22) {
            const auto parsed = std::from_chars(stat.data() + begin, stat.data() + pos, start_time);
            return parsed.ec == std::errc{} && parsed.ptr == stat.data() + pos && start_time > 0;
        }
        ++field;
    }
    return false;
}

bool read_task_identity(const std::string& proc_root, pid_t tid, pid_t& tgid,
                        uint64_t& start_time_ticks) noexcept {
    if (tid <= 0) return false;
    std::string status;
    if (!read_bounded_file(proc_root + "/" + std::to_string(tid) + "/status", status, 8192U) ||
        !parse_tgid_from_status(status, tgid)) return false;
    std::string stat;
    const std::string stat_path = proc_root + "/" + std::to_string(tgid) +
        "/task/" + std::to_string(tid) + "/stat";
    return read_bounded_file(stat_path, stat, 4096U) &&
        parse_start_time_from_stat(stat, start_time_ticks);
}

bool parse_proc_cgroup_relative(const std::string& proc_root, pid_t tgid, pid_t tid,
                                std::string& relative) noexcept {
    std::string contents;
    const std::string path = proc_root + "/" + std::to_string(tgid) +
        "/task/" + std::to_string(tid) + "/cgroup";
    if (!read_bounded_file(path, contents, 8192U)) return false;
    size_t pos = 0;
    while (pos < contents.size()) {
        const size_t end = contents.find('\n', pos);
        const std::string_view line(contents.data() + pos,
            (end == std::string::npos ? contents.size() : end) - pos);
        if (line.rfind("0::", 0) == 0) {
            std::string_view value = line.substr(3);
            while (!value.empty() && value.front() == '/') value.remove_prefix(1);
            if (value.find("..") != std::string_view::npos) return false;
            relative.assign(value);
            return true;
        }
        if (end == std::string::npos) break;
        pos = end + 1;
    }
    return false;
}

bool open_cgroup_relative(int root_fd, std::string_view relative, UniqueFd& output) noexcept {
    UniqueFd current(::fcntl(root_fd, F_DUPFD_CLOEXEC, 0));
    if (!current.valid()) return false;
    size_t pos = 0;
    while (pos < relative.size()) {
        const size_t slash = relative.find('/', pos);
        const std::string_view component = relative.substr(
            pos, slash == std::string_view::npos ? relative.size() - pos : slash - pos);
        if (component.empty() || component == "." || component == "..") return false;
        const std::string name(component);
        UniqueFd next(::openat(current.get(), name.c_str(),
            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
        if (!next.valid()) return false;
        current = std::move(next);
        if (slash == std::string_view::npos) break;
        pos = slash + 1;
    }
    output = std::move(current);
    return true;
}

bool path_has_thread(const std::string& path, pid_t tid) noexcept {
    UniqueFd dir(::open(path.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    if (!dir.valid()) return false;
    char buffer[8192];
    const ssize_t count = read_at(dir.get(), "cgroup.threads", buffer, sizeof(buffer));
    if (count < 0) return false;
    char token[24];
    const int size = std::snprintf(token, sizeof(token), "%d", tid);
    return size > 0 && static_cast<size_t>(size) < sizeof(token) &&
        token_present(buffer, static_cast<size_t>(count),
                      std::string_view(token, static_cast<size_t>(size)));
}

bool find_thread_membership(const std::string& root_path, const std::string& proc_root,
                            pid_t tid, pid_t tgid, std::string& relative) noexcept {
    std::vector<std::string> matches;
    if (path_has_thread(root_path, tid)) matches.emplace_back();
    std::error_code ec;
    size_t visited = 0;
    for (fs::recursive_directory_iterator it(
             root_path, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end && visited < 1024U; it.increment(ec)) {
        ++visited;
        if (it.depth() > 8) { it.disable_recursion_pending(); continue; }
        const fs::file_status status = it->symlink_status(ec);
        if (ec) { ec.clear(); continue; }
        if (fs::is_symlink(status)) { it.disable_recursion_pending(); continue; }
        if (!fs::is_directory(status)) continue;
        if (path_has_thread(it->path().string(), tid)) {
            std::error_code relative_error;
            const fs::path value = fs::relative(it->path(), fs::path(root_path), relative_error);
            if (!relative_error) matches.push_back(value.generic_string());
        }
    }
    if (matches.empty()) return false;
    if (matches.size() == 1U) { relative = matches.front(); return true; }
    std::string proc_relative;
    if (!parse_proc_cgroup_relative(proc_root, tgid, tid, proc_relative)) return false;
    const auto found = std::find(matches.begin(), matches.end(), proc_relative);
    if (found == matches.end() ||
        std::find(std::next(found), matches.end(), proc_relative) != matches.end()) return false;
    relative = *found;
    return true;
}

bool read_cgroup_type(int root_fd, std::string_view relative, std::string& type) noexcept {
    UniqueFd directory;
    if (!open_cgroup_relative(root_fd, relative, directory)) return false;
    char buffer[64];
    const ssize_t count = read_at(directory.get(), "cgroup.type", buffer, sizeof(buffer));
    if (count <= 0) return false;
    type.assign(buffer, static_cast<size_t>(count));
    while (!type.empty() && (type.back() == '\n' || type.back() == '\r' ||
                             type.back() == ' ' || type.back() == '\t')) type.pop_back();
    return !type.empty();
}

bool resource_domain_root(int root_fd, std::string_view relative,
                          std::string& domain_root) noexcept {
    std::string current(relative);
    for (unsigned depth = 0; depth <= 8; ++depth) {
        std::string type;
        if (!read_cgroup_type(root_fd, current, type)) return false;
        if (type == "domain" || type == "domain threaded") {
            domain_root = current;
            return true;
        }
        if (type != "threaded") return false;
        const size_t slash = current.rfind('/');
        current = slash == std::string::npos ? std::string{} : current.substr(0, slash);
    }
    return false;
}

bool write_thread_at(int root_fd, std::string_view relative, pid_t tid) noexcept {
    UniqueFd directory;
    if (!open_cgroup_relative(root_fd, relative, directory)) return false;
    UniqueFd threads(::openat(directory.get(), "cgroup.threads",
        O_WRONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!threads.valid()) return false;
    char value[24];
    const int length = std::snprintf(value, sizeof(value), "%d", tid);
    if (length <= 0 || static_cast<size_t>(length) >= sizeof(value)) return false;
    ssize_t count;
    do { count = ::write(threads.get(), value, static_cast<size_t>(length)); }
    while (count < 0 && errno == EINTR);
    if (count != length) return false;
    char verify[8192];
    const ssize_t size = read_at(directory.get(), "cgroup.threads", verify, sizeof(verify));
    return size >= 0 && token_present(
        verify, static_cast<size_t>(size), std::string_view(value, static_cast<size_t>(length)));
}


} // namespace


CgroupV2Controller::CgroupV2Controller(
    CentralMutationCoordinator& coordinator, std::string_view root_path, std::string_view proc_root)
    : coordinator_(coordinator), root_path_(root_path), proc_root_(proc_root),
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
        if (!transaction_id_.empty() || !thread_migrations_.empty() || !active_mutation_paths_.empty()) {
            (void)rollback_transaction_locked();
        }
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

bool CgroupV2Controller::get_thread_identity(
    pid_t tid, pid_t& tgid, uint64_t& start_time_ticks) const noexcept {
    return read_task_identity(proc_root_, tid, tgid, start_time_ticks);
}

bool CgroupV2Controller::migrate_task(pid_t tid, CgroupTarget target) {
    pid_t tgid = 0;
    uint64_t start_time = 0;
    if (!get_thread_identity(tid, tgid, start_time)) return false;
    return migrate_thread(tid, tgid, start_time, target);
}

bool CgroupV2Controller::migrate_thread(
    pid_t tid, pid_t expected_tgid, uint64_t expected_start_time_ticks, CgroupTarget target) {
    if (!supported_ || tid <= 0 || expected_tgid <= 0 || expected_start_time_ticks == 0 ||
        root_path_.empty()) return false;
    const char* target_name = nullptr;
    switch (target) {
        case CgroupTarget::TopApp: target_name = "top-app"; break;
        case CgroupTarget::Foreground: target_name = "foreground"; break;
        case CgroupTarget::Background: target_name = "background"; break;
        case CgroupTarget::System: target_name = "system"; break;
        default: return false;
    }
    const std::string target_relative(target_name);
    std::lock_guard<std::mutex> lock(mutex_);

    pid_t actual_tgid = 0;
    uint64_t actual_start = 0;
    if (!read_task_identity(proc_root_, tid, actual_tgid, actual_start) ||
        actual_tgid != expected_tgid || actual_start != expected_start_time_ticks) return false;

    std::string source_relative;
    if (!find_thread_membership(root_path_, proc_root_, tid, expected_tgid, source_relative)) return false;
    if (source_relative == target_relative) return true;

    const auto existing = std::find_if(thread_migrations_.begin(), thread_migrations_.end(),
        [&](const ThreadMigration& entry) { return entry.tid == tid; });
    if (existing != thread_migrations_.end()) return false;

    UniqueFd root_fd(::open(root_path_.c_str(),
        O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    if (!root_fd.valid()) return false;
    std::string source_type, target_type;
    if (!read_cgroup_type(root_fd.get(), source_relative, source_type) ||
        !read_cgroup_type(root_fd.get(), target_relative, target_type) ||
        (source_type != "threaded" && source_type != "domain threaded") ||
        (target_type != "threaded" && target_type != "domain threaded")) return false;
    std::string source_domain, target_domain;
    if (!resource_domain_root(root_fd.get(), source_relative, source_domain) ||
        !resource_domain_root(root_fd.get(), target_relative, target_domain) ||
        source_domain != target_domain) {
        // Never fall back to cgroup.procs: that would migrate every app thread.
        return false;
    }

    if (!acquire_lease_locked()) return false;
    thread_migrations_.push_back(ThreadMigration{
        .tid = tid, .tgid = expected_tgid,
        .start_time_ticks = expected_start_time_ticks,
        .source_relative_path = source_relative,
        .target_relative_path = target_relative,
    });
    if (!write_thread_at(root_fd.get(), target_relative, tid)) {
        (void)restore_thread_locked(tid);
        release_lease_locked();
        return false;
    }
    return true;
}

bool CgroupV2Controller::restore_thread_locked(pid_t tid) noexcept {
    const auto it = std::find_if(thread_migrations_.begin(), thread_migrations_.end(),
        [&](const ThreadMigration& entry) { return entry.tid == tid; });
    if (it == thread_migrations_.end()) return true;

    pid_t actual_tgid = 0;
    uint64_t actual_start = 0;
    if (!read_task_identity(proc_root_, it->tid, actual_tgid, actual_start) ||
        actual_tgid != it->tgid || actual_start != it->start_time_ticks) {
        thread_migrations_.erase(it);
        return true;
    }

    std::string current_relative;
    if (!find_thread_membership(root_path_, proc_root_, it->tid, it->tgid, current_relative)) return false;
    if (current_relative != it->target_relative_path) {
        // Android/OEM already moved the task. Retire our journal without overwriting it.
        thread_migrations_.erase(it);
        return true;
    }

    UniqueFd root_fd(::open(root_path_.c_str(),
        O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    if (!root_fd.valid()) return false;
    std::string source_domain, target_domain;
    if (!resource_domain_root(root_fd.get(), it->source_relative_path, source_domain) ||
        !resource_domain_root(root_fd.get(), it->target_relative_path, target_domain) ||
        source_domain != target_domain) return false;
    if (!write_thread_at(root_fd.get(), it->source_relative_path, tid)) return false;
    thread_migrations_.erase(it);
    return true;
}

bool CgroupV2Controller::restore_thread(pid_t tid) {
    std::lock_guard<std::mutex> lock(mutex_);
    const bool restored = restore_thread_locked(tid);
    if (restored) release_lease_locked();
    return restored;
}

bool CgroupV2Controller::restore_all_migrated_threads() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!thread_migrations_.empty()) {
        if (!restore_thread_locked(thread_migrations_.back().tid)) return false;
    }
    release_lease_locked();
    return true;
}

size_t CgroupV2Controller::pending_thread_migrations() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return thread_migrations_.size();
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
    std::array<bool, 3> selected{};
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
    if (transaction_id_.empty() || !active_mutation_paths_.empty() || !thread_migrations_.empty()) return;
    coordinator_.release_lease(
        MutationDomain::CpuPolicy, controller_name_, transaction_id_);
    transaction_id_.clear();
}

bool CgroupV2Controller::rollback_transaction_locked() noexcept {
    if (transaction_id_.empty()) return active_mutation_paths_.empty() && thread_migrations_.empty();
    while (!thread_migrations_.empty()) {
        if (!restore_thread_locked(thread_migrations_.back().tid)) return false;
    }
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

