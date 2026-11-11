/**
 * @file foreground_task_resolver.cpp
 * @brief Conservative foreground process and render-thread discovery.
 * @license Apache-2.0
 */
#include "coreflow/foreground_task_resolver.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cctype>
#include <dirent.h>
#include <fcntl.h>
#include <map>
#include <string_view>
#include <time.h>
#include <unistd.h>

namespace coreflow {
namespace {

constexpr size_t kMaxPidListBytes = 8192U;
constexpr size_t kMaxTaskBytes = 4096U;
constexpr size_t kMaxProcesses = 128U;
constexpr size_t kMaxThreads = 1024U;
constexpr uint64_t kRefreshIntervalNs = 500'000'000ULL;

bool read_bounded(const std::string& path, std::string& output, size_t max_bytes) noexcept {
    output.clear();
    UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return false;
    char buffer[1024];
    for (;;) {
        const ssize_t count = ::read(fd.get(), buffer, sizeof(buffer));
        if (count < 0 && errno == EINTR) continue;
        if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
        if (count < 0) return false;
        if (count == 0) break;
        if (output.size() + static_cast<size_t>(count) > max_bytes) return false;
        output.append(buffer, static_cast<size_t>(count));
    }
    return true;
}

bool parse_positive_pid(std::string_view token, pid_t& pid) noexcept {
    if (token.empty()) return false;
    int value = 0;
    const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size() || value <= 0) return false;
    pid = static_cast<pid_t>(value);
    return true;
}

bool parse_uid(std::string_view status, uint32_t& uid) noexcept {
    size_t pos = 0;
    while (pos < status.size()) {
        const size_t end = status.find('\n', pos);
        const std::string_view line = status.substr(
            pos, end == std::string_view::npos ? status.size() - pos : end - pos);
        if (line.rfind("Uid:", 0) == 0) {
            size_t begin = 4;
            while (begin < line.size() && (line[begin] == ' ' || line[begin] == '\t')) ++begin;
            size_t finish = begin;
            while (finish < line.size() && line[finish] >= '0' && line[finish] <= '9') ++finish;
            if (finish == begin) return false;
            const auto parsed = std::from_chars(line.data() + begin, line.data() + finish, uid);
            return parsed.ec == std::errc{} && parsed.ptr == line.data() + finish;
        }
        if (end == std::string_view::npos) break;
        pos = end + 1;
    }
    return false;
}

std::string process_basename(std::string_view cmdline) {
    if (cmdline.empty()) return {};
    const size_t nul = cmdline.find('\0');
    std::string_view argv0 = cmdline.substr(0, nul == std::string_view::npos ? cmdline.size() : nul);
    const size_t slash = argv0.find_last_of('/');
    if (slash != std::string_view::npos) argv0.remove_prefix(slash + 1);
    return std::string(argv0);
}

bool valid_package_name(std::string_view name) noexcept {
    if (name.size() < 3U || name.find('.') == std::string_view::npos) return false;
    for (const unsigned char ch : name) {
        if (!(std::isalnum(ch) || ch == '.' || ch == '_' || ch == '-')) return false;
    }
    return name.front() != '.' && name.back() != '.';
}

std::string package_root(std::string process_name) {
    const size_t colon = process_name.find(':');
    if (colon != std::string::npos) process_name.resize(colon);
    return process_name;
}

bool parse_numeric_name(const char* value, pid_t& pid) noexcept {
    if (value == nullptr || *value == '\0') return false;
    for (const char* p = value; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') return false;
    }
    return parse_positive_pid(value, pid);
}

uint64_t steady_now_ns() noexcept {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

} // namespace

ForegroundTaskResolver::ForegroundTaskResolver(
    CgroupV2Controller& controller,
    std::string top_app_cgroup,
    std::string proc_root) noexcept
    : controller_(controller),
      top_app_cgroup_(std::move(top_app_cgroup)),
      proc_root_(std::move(proc_root)) {}

ForegroundTaskResolver::~ForegroundTaskResolver() {
    (void)restore_all();
}

bool ForegroundTaskResolver::is_excluded_process(std::string_view name) noexcept {
    constexpr std::array<std::string_view, 18> excluded{
        "system_server", "surfaceflinger", "audioserver", "cameraserver",
        "inputflinger", "servicemanager", "hwservicemanager", "vold",
        "netd", "installd", "zygote", "zygote64", "init",
        "android.hardware.graphics.composer", "android.hardware.audio",
        "android.hardware.camera", "com.android.systemui", "com.android.phone"
    };
    for (const auto value : excluded) if (name == value) return true;
    return false;
}

bool ForegroundTaskResolver::is_excluded_thread(std::string_view name) noexcept {
    constexpr std::array<std::string_view, 7> excluded_prefixes{
        "Jit code cache", "Signal Catcher", "HeapTaskDaemon", "Binder:",
        "binder:", "ReferenceQueueD", "FinalizerDaemon"
    };
    for (const auto prefix : excluded_prefixes) {
        if (name.rfind(prefix, 0) == 0) return true;
    }
    return false;
}

bool ForegroundTaskResolver::is_whitelisted_thread(
    pid_t pid, pid_t tid, std::string_view name) noexcept {
    if (pid <= 0 || tid <= 0 || is_excluded_thread(name)) return false;
    if (tid == pid) return true; // Main UI thread.
    if (name == "RenderThread" || name == "UnityMain") return true;
    return name.rfind("UnityChoreograp", 0) == 0 ||
        name.rfind("GLThread", 0) == 0 ||
        name.rfind("VkQueue", 0) == 0;
}

bool ForegroundTaskResolver::read_top_app_pids(std::vector<pid_t>& pids) const noexcept {
    std::string contents;
    const std::string path = top_app_cgroup_ + "/cgroup.procs";
    if (!read_bounded(path, contents, kMaxPidListBytes)) return false;
    size_t pos = 0;
    while (pos < contents.size() && pids.size() < kMaxProcesses) {
        const size_t end = contents.find('\n', pos);
        const std::string_view line = std::string_view(contents).substr(
            pos, end == std::string::npos ? contents.size() - pos : end - pos);
        size_t begin = 0;
        while (begin < line.size() && (line[begin] == ' ' || line[begin] == '\t' || line[begin] == '\r')) ++begin;
        size_t finish = begin;
        while (finish < line.size() && line[finish] >= '0' && line[finish] <= '9') ++finish;
        if (finish > begin) {
            pid_t pid = 0;
            if (parse_positive_pid(line.substr(begin, finish - begin), pid)) pids.push_back(pid);
        }
        if (end == std::string::npos) break;
        pos = end + 1;
    }
    std::sort(pids.begin(), pids.end());
    pids.erase(std::unique(pids.begin(), pids.end()), pids.end());
    return true;
}

bool ForegroundTaskResolver::read_process_identity(
    pid_t pid, std::string& package_name) const noexcept {
    std::string cmdline;
    std::string status;
    if (!read_bounded(proc_root_ + "/" + std::to_string(pid) + "/cmdline", cmdline, 4096U) ||
        !read_bounded(proc_root_ + "/" + std::to_string(pid) + "/status", status, 8192U)) return false;

    uint32_t uid = 0;
    if (!parse_uid(status, uid) || uid < 10000U) return false;
    const std::string process = process_basename(cmdline);
    if (process.empty() || is_excluded_process(process)) return false;

    package_name = package_root(process);
    return valid_package_name(package_name) && !is_excluded_process(package_name);
}

bool ForegroundTaskResolver::collect_render_threads(
    pid_t pid, std::vector<TaskIdentity>& threads) const noexcept {
    const std::string task_root = proc_root_ + "/" + std::to_string(pid) + "/task";
    DIR* directory = ::opendir(task_root.c_str());
    if (directory == nullptr) return false;

    size_t visited = 0;
    while (dirent* entry = ::readdir(directory)) {
        if (threads.size() >= kMaxThreads || visited >= kMaxThreads) break;
        pid_t tid = 0;
        if (!parse_numeric_name(entry->d_name, tid)) continue;
        ++visited;

        std::string comm;
        if (!read_bounded(task_root + "/" + entry->d_name + "/comm", comm, 128U)) continue;
        while (!comm.empty() && (comm.back() == '\n' || comm.back() == '\r')) comm.pop_back();
        if (comm.empty() || !is_whitelisted_thread(pid, tid, comm)) continue;

        pid_t actual_tgid = 0;
        uint64_t start_time = 0;
        if (!controller_.get_thread_identity(tid, actual_tgid, start_time) || actual_tgid != pid) continue;
        threads.push_back(TaskIdentity{.tid = tid, .tgid = actual_tgid, .start_time_ticks = start_time});
    }
    ::closedir(directory);
    return true;
}

bool ForegroundTaskResolver::restore_stale(
    const std::unordered_map<pid_t, TaskIdentity>& selected) noexcept {
    bool complete = true;
    for (auto it = tracked_.begin(); it != tracked_.end();) {
        const auto next = selected.find(it->first);
        if (next != selected.end() &&
            next->second.tgid == it->second.tgid &&
            next->second.start_time_ticks == it->second.start_time_ticks) {
            ++it;
            continue;
        }
        if (!controller_.restore_thread(it->first)) {
            complete = false;
            ++it;
        } else {
            it = tracked_.erase(it);
        }
    }
    return complete;
}

bool ForegroundTaskResolver::refresh(bool force) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    const uint64_t now = steady_now_ns();
    if (!force && last_refresh_ns_ != 0 && now >= last_refresh_ns_ &&
        now - last_refresh_ns_ < kRefreshIntervalNs) return true;
    last_refresh_ns_ = now;
    snapshot_ = ForegroundTaskSnapshot{};

    std::vector<pid_t> pids;
    if (!read_top_app_pids(pids)) {
        snapshot_.source_available = false;
        return false; // A transient read failure must not trigger rollback.
    }
    snapshot_.source_available = true;
    snapshot_.process_count = static_cast<uint32_t>(pids.size());

    std::map<std::string, std::vector<pid_t>> packages;
    for (const pid_t pid : pids) {
        std::string package;
        if (!read_process_identity(pid, package)) continue;
        packages[package].push_back(pid);
    }
    snapshot_.package_count = static_cast<uint32_t>(packages.size());
    if (packages.size() > 1U) {
        snapshot_.ambiguous = true;
        snapshot_.rollback_complete = restore_stale({});
        return false;
    }
    if (packages.empty()) {
        snapshot_.rollback_complete = restore_stale({});
        return snapshot_.rollback_complete;
    }

    snapshot_.package_name = packages.begin()->first;
    std::vector<TaskIdentity> candidates;
    for (const pid_t pid : packages.begin()->second) {
        (void)collect_render_threads(pid, candidates);
    }
    std::sort(candidates.begin(), candidates.end(), [](const TaskIdentity& lhs, const TaskIdentity& rhs) {
        return lhs.tid < rhs.tid;
    });
    candidates.erase(std::unique(candidates.begin(), candidates.end(),
        [](const TaskIdentity& lhs, const TaskIdentity& rhs) {
            return lhs.tid == rhs.tid && lhs.tgid == rhs.tgid &&
                lhs.start_time_ticks == rhs.start_time_ticks;
        }), candidates.end());
    snapshot_.discovered_thread_count = static_cast<uint32_t>(candidates.size());

    std::unordered_map<pid_t, TaskIdentity> selected;
    selected.reserve(candidates.size());
    for (const auto& candidate : candidates) selected.emplace(candidate.tid, candidate);
    snapshot_.rollback_complete = restore_stale(selected);

    for (const auto& candidate : candidates) {
        const auto tracked = tracked_.find(candidate.tid);
        if (tracked != tracked_.end() &&
            tracked->second.tgid == candidate.tgid &&
            tracked->second.start_time_ticks == candidate.start_time_ticks) {
            ++snapshot_.migrated_thread_count;
            continue;
        }
        if (!controller_.migrate_thread(
                candidate.tid, candidate.tgid, candidate.start_time_ticks, CgroupTarget::TopApp)) {
            ++snapshot_.rejected_thread_count;
            continue;
        }
        tracked_[candidate.tid] = candidate;
        ++snapshot_.migrated_thread_count;
    }
    return snapshot_.rollback_complete && snapshot_.rejected_thread_count == 0U;
}

bool ForegroundTaskResolver::restore_all() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    bool complete = true;
    for (auto it = tracked_.begin(); it != tracked_.end();) {
        if (!controller_.restore_thread(it->first)) {
            complete = false;
            ++it;
        } else {
            it = tracked_.erase(it);
        }
    }
    if (tracked_.empty()) snapshot_.migrated_thread_count = 0;
    snapshot_.rollback_complete = complete;
    return complete;
}

ForegroundTaskSnapshot ForegroundTaskResolver::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_;
}

size_t ForegroundTaskResolver::tracked_threads() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return tracked_.size();
}

} // namespace coreflow
