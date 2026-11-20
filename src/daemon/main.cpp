/**
 * @file main.cpp
 * @brief CoreFlow Engine v5.1.0 — Lean 4-Pillar Adaptive Android Daemon.
 *
 * Pillars:
 *  1. HardwareProbe + CFJ4 BaselineStore (single-pass discovery & OEM rollback safety)
 *  2. ContextEngine (5-mode context + foreground app + critical render TIDs + 4-profile per-app rules)
 *  3. Focused Actuators:
 *     - ThreadSchedulerActuator (per-TID sched_setattr UClamp with closed-loop render pressure)
 *     - StorageMemoryActuator (dynamic UFS/eMMC burst, background QoS, MGLRU, smart pre-read, timed contention recovery)
 *     - ThermalEnvelopeActuator (Rust PID + Online RLS/MLP 10s lookahead CPU/GPU smooth pacing)
 *     - HardwareLongevityManager (battery thermal step-charging with dual-cell primary selection)
 *  4. Clean JSON status/config/diag contract for the v5 MD3 WebUI & Android QS Tile.
 * @license Apache-2.0
 */
#include "coreflow/baseline_store.hpp"
#include "coreflow/config.hpp"
#include "coreflow/cooling_feedback.hpp"
#include "coreflow/hardware_longevity.hpp"
#include "coreflow/input_event_source.hpp"
#include "coreflow/mount_topology.hpp"
#include "coreflow/mutation_coordinator.hpp"
#include "coreflow/neural_predictor.hpp"
#include "coreflow/reactor.hpp"
#include "coreflow/thermal_monitor.hpp"
#include "coreflow/types.hpp"
#include "coreflow/unique_fd.hpp"
#include "coreflow/v5/context_engine.hpp"
#include "coreflow/v5/hardware_probe.hpp"
#include "coreflow/v5/storage_memory_actuator.hpp"
#include "coreflow/v5/thermal_envelope_actuator.hpp"
#include "coreflow/v5/thread_scheduler.hpp"
#include "coreflow-engine/src/bridge.rs.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <clocale>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/file.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#ifdef __ANDROID__
#include <android/log.h>
#endif

using namespace coreflow;
using namespace coreflow::v5;

namespace {

constexpr char kStateDir[] = "/data/adb/coreflow";
constexpr char kPidPath[] = "/data/adb/coreflow/coreflowd.pid";
constexpr char kVersion[] = "5.1.0";

void emit_log(bool is_error, const std::string& message) {
    if (is_error) std::cerr << message << "\n";
    else std::cout << message << "\n";
#ifdef __ANDROID__
    __android_log_write(
        is_error ? ANDROID_LOG_WARN : ANDROID_LOG_INFO,
        "CoreFlow",
        message.c_str());
#endif
}

// Async-signal-safe fatal handoff to Android debuggerd
constexpr int kFatalSignalSlots = 128;
struct sigaction old_sigaction[kFatalSignalSlots]{};
sigset_t fatal_unblock_sets[kFatalSignalSlots]{};
volatile sig_atomic_t old_sigaction_valid[kFatalSignalSlots]{};

void on_fatal_signal(int signum) {
    constexpr char msg[] = "[CoreFlow] Fatal signal; handing off to debuggerd; next boot will recover CFJ4 journal.\n";
    (void)::write(STDERR_FILENO, msg, sizeof(msg) - 1U);
    if (signum > 0 && signum < kFatalSignalSlots && old_sigaction_valid[signum] != 0) {
        if (::sigaction(signum, &old_sigaction[signum], nullptr) == 0 &&
            ::sigprocmask(SIG_UNBLOCK, &fatal_unblock_sets[signum], nullptr) == 0) {
            (void)::raise(signum);
        }
    }
    ::_exit(128 + signum);
}

void install_signal_hardening() noexcept {
    struct sigaction ignore_sa{};
    ignore_sa.sa_handler = SIG_IGN;
    sigemptyset(&ignore_sa.sa_mask);
    (void)::sigaction(SIGPIPE, &ignore_sa, nullptr);

    struct sigaction fatal_sa{};
    fatal_sa.sa_handler = on_fatal_signal;
    sigemptyset(&fatal_sa.sa_mask);
    for (int sig : {SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL, SIGTRAP, SIGSYS}) {
        if (sig <= 0 || sig >= kFatalSignalSlots) continue;
        sigset_t set{};
        if (::sigemptyset(&set) == 0 && ::sigaddset(&set, sig) == 0) {
            fatal_unblock_sets[sig] = set;
            if (::sigaction(sig, &fatal_sa, &old_sigaction[sig]) == 0) {
                old_sigaction_valid[sig] = 1;
            }
        }
    }
}

bool marker_exists(const char* path) noexcept {
    struct stat st{};
    return ::lstat(path, &st) == 0;
}

bool safety_halt_requested() noexcept {
    return marker_exists("/data/adb/coreflow/SAFE_MODE") ||
           marker_exists("/data/adb/coreflow/DISABLE") ||
           marker_exists("/data/adb/modules/cfae_framework/disable") ||
           marker_exists("/data/adb/modules/coreflow_engine/disable");
}

uint64_t steady_now_ms() noexcept {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

uint64_t epoch_now_ms() noexcept {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

double safe_num(double v, double fallback = 0.0) noexcept {
    return std::isfinite(v) ? v : fallback;
}

std::string json_str(std::string_view text) {
    std::string out = "\"";
    for (unsigned char c : text) {
        if (c == '\"' || c == '\\') { out.push_back('\\'); out.push_back(static_cast<char>(c)); }
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c >= 32 && c < 127) out.push_back(static_cast<char>(c));
        else out.push_back('?');
    }
    out.push_back('\"');
    return out;
}

bool process_is_coreflowd(pid_t pid) {
    if (pid <= 0) return false;
    if (::kill(pid, 0) != 0 && errno != EPERM) return false;
    const std::string cmd_path = "/proc/" + std::to_string(pid) + "/cmdline";
    UniqueFd fd(::open(cmd_path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return false;
    char buf[256];
    ssize_t n = 0;
    do { n = ::read(fd.get(), buf, sizeof(buf)); } while (n < 0 && errno == EINTR);
    if (n <= 0) return false;
    const std::string_view raw(buf, static_cast<size_t>(n));
    const size_t nul = raw.find('\0');
    const std::string_view argv0 = nul == std::string_view::npos ? raw : raw.substr(0, nul);
    const size_t slash = argv0.find_last_of('/');
    const std::string_view base = slash == std::string_view::npos ? argv0 : argv0.substr(slash + 1);
    return base == "coreflowd";
}

pid_t daemon_pid() {
    UniqueFd fd(::open(kPidPath, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return 0;
    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode)) return 0;
    char buf[32];
    ssize_t n = 0;
    do { n = ::read(fd.get(), buf, sizeof(buf)); } while (n < 0 && errno == EINTR);
    if (n <= 0) return 0;
    const std::string trimmed = DaemonConfig::trim(std::string(buf, static_cast<size_t>(n)));
    long value = 0;
    const auto r = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), value);
    if (r.ec != std::errc{} || r.ptr != trimmed.data() + trimmed.size() || value < 1 || value > 2147483647L) {
        return 0;
    }
    return process_is_coreflowd(static_cast<pid_t>(value)) ? static_cast<pid_t>(value) : 0;
}

bool service_launcher_authorized() {
    const pid_t parent = ::getppid();
    if (parent <= 1) return false;
    UniqueFd state_dir(::open(kStateDir, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    if (!state_dir.valid()) return false;
    UniqueFd service_lock(::openat(state_dir.get(), "service.lock", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    if (!service_lock.valid()) return false;
    UniqueFd owner_fd(::openat(service_lock.get(), "owner.pid", O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!owner_fd.valid()) return false;
    struct stat st{};
    if (::fstat(owner_fd.get(), &st) != 0 || !S_ISREG(st.st_mode)) return false;
    char buf[32];
    ssize_t n = 0;
    do { n = ::read(owner_fd.get(), buf, sizeof(buf)); } while (n < 0 && errno == EINTR);
    if (n <= 0) return false;
    const std::string trimmed = DaemonConfig::trim(std::string(buf, static_cast<size_t>(n)));
    long owner_pid = 0;
    const auto r = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), owner_pid);
    return r.ec == std::errc{} && r.ptr == trimmed.data() + trimmed.size() &&
           owner_pid == static_cast<long>(parent) &&
           (::kill(parent, 0) == 0 || errno == EPERM);
}

bool ensure_secure_state_dir() {
    if (::mkdir(kStateDir, 0700) != 0 && errno != EEXIST) return false;
    struct stat st{};
    if (::lstat(kStateDir, &st) != 0 || !S_ISDIR(st.st_mode)) return false;
    (void)::chmod(kStateDir, 0700);
    return true;
}

bool write_pid(UniqueFd& lock) {
    if (!ensure_secure_state_dir()) return false;
    lock.reset(::open(kPidPath, O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK, 0600));
    if (!lock.valid()) return false;
    struct stat st{};
    if (::fstat(lock.get(), &st) != 0 || !S_ISREG(st.st_mode)) return false;
    if (::flock(lock.get(), LOCK_EX | LOCK_NB) != 0) return false;
    (void)::fchmod(lock.get(), 0600);
    const auto pid = std::to_string(::getpid());
    if (::ftruncate(lock.get(), 0) != 0 || ::lseek(lock.get(), 0, SEEK_SET) < 0) return false;
    if (::write(lock.get(), pid.data(), pid.size()) != static_cast<ssize_t>(pid.size())) return false;
    return ::fsync(lock.get()) == 0;
}

void clear_pid(const UniqueFd& lock) noexcept {
    if (lock.valid() && ::ftruncate(lock.get(), 0) == 0) (void)::fsync(lock.get());
}

bool journal_clear() {
    const std::string jpath = std::string(kStateDir) + "/baseline.journal";
    UniqueFd fd(::open(jpath.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return errno == ENOENT;
    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0 || st.st_size > 65536) {
        return false;
    }
    char buf[4096];
    std::string raw;
    while (raw.size() <= 65536U) {
        const ssize_t n = ::read(fd.get(), buf, sizeof(buf));
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        raw.append(buf, static_cast<size_t>(n));
    }
    std::istringstream ss(raw);
    std::string header;
    if (!std::getline(ss, header) ||
        (header.rfind("CFJ1\t", 0) != 0 && header.rfind("CFJ2\t", 0) != 0 &&
         header.rfind("CFJ3\t", 0) != 0 && header.rfind("CFJ4\t", 0) != 0)) {
        return false;
    }
    std::string line;
    while (std::getline(ss, line)) {
        if (!DaemonConfig::trim(line).empty()) return false;
    }
    return true;
}

size_t count_journal_entries() {
    const std::string jpath = std::string(kStateDir) + "/baseline.journal";
    UniqueFd fd(::open(jpath.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return 0;
    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode) || st.st_size <= 0 || st.st_size > 65536) {
        return 0;
    }
    char buf[4096];
    std::string raw;
    while (raw.size() <= 65536U) {
        const ssize_t n = ::read(fd.get(), buf, sizeof(buf));
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        raw.append(buf, static_cast<size_t>(n));
    }
    std::istringstream ss(raw);
    std::string header;
    if (!std::getline(ss, header) || header.rfind("CFJ", 0) != 0) return 0;
    size_t count = 0;
    std::string line;
    while (std::getline(ss, line)) {
        if (!DaemonConfig::trim(line).empty()) ++count;
    }
    return count;
}

bool write_status_snapshot(std::string_view payload) noexcept {
    if (payload.empty() || payload.size() > 512U * 1024U || !ensure_secure_state_dir()) return false;
    UniqueFd dir(::open(kStateDir, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    if (!dir.valid()) return false;
    static std::atomic<uint64_t> nonce{0};
    const uint64_t seq = nonce.fetch_add(1U, std::memory_order_relaxed) + 1U;
    const std::string tmp = "status.json.tmp." + std::to_string(::getpid()) + "." + std::to_string(seq);
    UniqueFd fd(::openat(dir.get(), tmp.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600));
    if (!fd.valid()) return false;
    size_t off = 0;
    while (off < payload.size()) {
        const ssize_t n = ::write(fd.get(), payload.data() + off, payload.size() - off);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { (void)::unlinkat(dir.get(), tmp.c_str(), 0); return false; }
        off += static_cast<size_t>(n);
    }
    if (::renameat(dir.get(), tmp.c_str(), dir.get(), "status.json") != 0) {
        (void)::unlinkat(dir.get(), tmp.c_str(), 0);
        return false;
    }
    return true;
}

bool print_live_status_snapshot(pid_t expected_pid) {
    if (expected_pid <= 0) return false;
    UniqueFd fd(::open("/data/adb/coreflow/status.json", O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return false;
    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode) || st.st_size <= 0 || st.st_size > 512 * 1024) {
        return false;
    }
    const auto now = std::chrono::system_clock::now();
    const auto mod = std::chrono::system_clock::from_time_t(st.st_mtime);
    if (now < mod || now - mod > std::chrono::seconds(4)) return false;
    std::string payload(static_cast<size_t>(st.st_size), '\0');
    size_t off = 0;
    while (off < payload.size()) {
        const ssize_t n = ::read(fd.get(), payload.data() + off, payload.size() - off);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        off += static_cast<size_t>(n);
    }
    const std::string pid_field = "\"pid\":" + std::to_string(expected_pid) + ",";
    if (payload.find("\"running\":true") == std::string::npos ||
        payload.find(pid_field) == std::string::npos ||
        payload.front() != '{' || payload.back() != '}') {
        return false;
    }
    std::cout << payload << "\n" << std::flush;
    return true;
}

void escape_app_cgroups_and_protect_oom() {
    const std::string pid_line = std::to_string(::getpid()) + "\n";
    MountTopologyResolver topology{};
    for (const auto& mount : topology.discover_cgroup_mounts()) {
        for (const char* file : {"/cgroup.procs", "/tasks"}) {
            const std::string path = mount.mount_point + file;
            UniqueFd fd(::open(path.c_str(), O_WRONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
            if (fd.valid()) {
                (void)::write(fd.get(), pid_line.data(), pid_line.size());
                break;
            }
        }
    }
    UniqueFd oom(::open("/proc/self/oom_score_adj", O_WRONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (oom.valid()) {
        constexpr char val[] = "-900\n";
        (void)::write(oom.get(), val, sizeof(val) - 1U);
    }
}

bool daemonize_process() noexcept {
    const pid_t first = ::fork();
    if (first < 0) return false;
    if (first > 0) ::_exit(0);
    if (::setsid() < 0) return false;
    const pid_t second = ::fork();
    if (second < 0) return false;
    if (second > 0) ::_exit(0);
    (void)::umask(0077);
    (void)::chdir("/");
    for (int fd = 3; fd < 256; ++fd) (void)::close(fd);
    const int null_fd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    if (null_fd >= 0) {
        (void)::dup2(null_fd, STDIN_FILENO);
        if (null_fd > STDERR_FILENO) (void)::close(null_fd);
    }
    (void)ensure_secure_state_dir();
    const std::string logs_dir = std::string(kStateDir) + "/logs";
    (void)::mkdir(logs_dir.c_str(), 0700);
    (void)::chmod(logs_dir.c_str(), 0700);
    const std::string log_path = logs_dir + "/coreflowd.log";
    const int log_fd = ::open(log_path.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (log_fd >= 0) {
        (void)::dup2(log_fd, STDOUT_FILENO);
        (void)::dup2(log_fd, STDERR_FILENO);
        if (log_fd > STDERR_FILENO) (void)::close(log_fd);
    }
    return true;
}

class CpuUtilSampler {
public:
    double sample() noexcept {
        UniqueFd fd(::open("/proc/stat", O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
        if (!fd.valid()) return last_pct_;
        char buf[512];
        const ssize_t n = ::read(fd.get(), buf, sizeof(buf));
        if (n <= 0) return last_pct_;
        std::istringstream ss(std::string(buf, static_cast<size_t>(n)));
        std::string label;
        uint64_t u = 0, ni = 0, s = 0, id = 0, iw = 0, irq = 0, sirq = 0, st = 0;
        if (!(ss >> label >> u >> ni >> s >> id >> iw >> irq >> sirq >> st) || label != "cpu") {
            return last_pct_;
        }
        const uint64_t total = u + ni + s + id + iw + irq + sirq + st;
        const uint64_t idle = id + iw;
        if (prev_total_ > 0U && total > prev_total_ && idle >= prev_idle_) {
            const double ratio = 1.0 - static_cast<double>(idle - prev_idle_) /
                                       static_cast<double>(total - prev_total_);
            last_pct_ = std::clamp(ratio * 100.0, 0.0, 100.0);
        }
        prev_total_ = total;
        prev_idle_ = idle;
        return last_pct_;
    }

private:
    uint64_t prev_total_{0};
    uint64_t prev_idle_{0};
    double last_pct_{0.0};
};

// Lightweight closed-loop RenderThread/GameThread CPU saturation sampler (0.0..1.0).
class RenderLoadSampler {
public:
    RenderLoadSampler() noexcept {
        const long hz = ::sysconf(_SC_CLK_TCK);
        clk_tck_ = (hz > 0) ? static_cast<double>(hz) : 100.0;
    }

    double sample(int32_t pid, const std::vector<int32_t>& tids, double elapsed_sec) noexcept {
        if (pid <= 0 || tids.empty() || elapsed_sec <= 0.05) {
            last_pid_ = 0;
            prev_ticks_.clear();
            last_pressure_ = 0.0;
            return 0.0;
        }
        if (pid != last_pid_) {
            last_pid_ = pid;
            prev_ticks_.clear();
        }
        double peak_ratio = 0.0;
        const size_t limit = std::min<size_t>(tids.size(), 4U);
        std::unordered_map<int32_t, uint64_t> next_ticks;
        next_ticks.reserve(limit);
        for (size_t i = 0; i < limit; ++i) {
            const int32_t tid = tids[i];
            if (tid <= 0) continue;
            uint64_t total_ticks = 0U;
            if (!read_tid_cpu_ticks(pid, tid, total_ticks)) continue;
            next_ticks[tid] = total_ticks;
            const auto prev = prev_ticks_.find(tid);
            if (prev != prev_ticks_.end() && total_ticks >= prev->second) {
                const double delta_sec = static_cast<double>(total_ticks - prev->second) / clk_tck_;
                peak_ratio = std::max(peak_ratio, std::clamp(delta_sec / elapsed_sec, 0.0, 1.0));
            }
        }
        prev_ticks_ = std::move(next_ticks);
        // Smooth EMA to avoid single-tick spikes
        last_pressure_ = std::clamp(0.55 * last_pressure_ + 0.45 * peak_ratio, 0.0, 1.0);
        return last_pressure_;
    }

private:
    static bool read_tid_cpu_ticks(int32_t pid, int32_t tid, uint64_t& out_ticks) noexcept {
        const std::string path = "/proc/" + std::to_string(pid) + "/task/" + std::to_string(tid) + "/stat";
        UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
        if (!fd.valid()) return false;
        char buf[512];
        const ssize_t n = ::read(fd.get(), buf, sizeof(buf) - 1U);
        if (n <= 0) return false;
        buf[n] = '\0';
        const std::string_view raw(buf, static_cast<size_t>(n));
        // Skip past "(comm)" because comm can contain spaces or parentheses
        const size_t rparen = raw.rfind(')');
        if (rparen == std::string_view::npos || rparen + 2U >= raw.size()) return false;
        std::istringstream ss{std::string(raw.substr(rparen + 2U))};
        std::string tok;
        uint64_t utime = 0U, stime = 0U;
        // Field 3 is state (index 0 after ')'); field 14 is utime (index 11), field 15 is stime (index 12)
        for (int idx = 0; idx <= 12; ++idx) {
            if (!(ss >> tok)) return false;
            if (idx == 11) {
                const auto r = std::from_chars(tok.data(), tok.data() + tok.size(), utime);
                if (r.ec != std::errc{}) return false;
            } else if (idx == 12) {
                const auto r = std::from_chars(tok.data(), tok.data() + tok.size(), stime);
                if (r.ec != std::errc{}) return false;
            }
        }
        out_ticks = utime + stime;
        return true;
    }

    double clk_tck_{100.0};
    int32_t last_pid_{0};
    std::unordered_map<int32_t, uint64_t> prev_ticks_;
    double last_pressure_{0.0};
};

struct SessionMetrics {
    uint64_t started_steady_ms{0};
    uint64_t uptime_sec{0};
    uint64_t launch_bursts{0};
    uint64_t thermal_guard_events{0};
    uint64_t temp_samples{0};
    double temp_sum_celsius{0.0};
    double peak_soc_temp_celsius{-1.0};
    uint64_t scale_samples{0};
    double scale_sum{0.0};
    bool prev_thermal_guard{false};
};

struct HistoryPoint {
    uint64_t ts_ms{0};
    double soc_temp{-1.0};
    double thermal_pressure{0.0};
    double cpu_util_pct{0.0};
    double capacity_scale{1.0};
    std::string mode{"EFFICIENCY"};
};

struct RuntimeStatusData {
    bool running{false};
    pid_t pid{0};
    DaemonConfig config{};
    HardwareTopology topo{};
    ContextDecision ctx{};
    bool thermal_guard{false};
    bool thermal_emergency{false};
    double soc_temp{-1.0};
    double battery_temp{-1.0};
    double thermal_pressure{0.0};
    std::string thermal_source{"unavailable"};
    double cpu_util_pct{0.0};
    double render_pressure{0.0};
    std::optional<double> gpu_busy_pct{};
    std::optional<double> cpu_psi{};
    std::optional<double> mem_psi{};
    std::optional<double> io_psi{};
    double capacity_scale{1.0};
    double ai_predicted_10s{0.0};
    double ai_confidence{0.0};
    ThreadSchedulerStats thread_stats{};
    StorageMemoryStats storage_stats{};
    ThermalEnvelopeStats envelope_stats{};
    std::vector<std::string> contended_paths{};
    BatteryPowerTelemetry battery{};
    SessionMetrics session{};
    size_t baseline_nodes{0};
    bool recovery_pending{false};
    std::vector<HistoryPoint> history{};
};

std::string serialize_string_array(const std::vector<std::string>& items) {
    std::ostringstream out;
    out << '[';
    for (size_t i = 0; i < items.size(); ++i) {
        if (i != 0U) out << ',';
        out << json_str(items[i]);
    }
    out << ']';
    return out.str();
}

std::string build_status_json(const RuntimeStatusData& s) {
    const auto gaming_list = DaemonConfig::parse_package_list(s.config.gaming_apps);
    const auto smoothness_list = DaemonConfig::parse_package_list(s.config.smoothness_apps);
    const auto battery_list = DaemonConfig::parse_package_list(s.config.battery_apps);
    const auto excluded_list = DaemonConfig::parse_package_list(s.config.excluded_apps);
    const double avg_temp = s.session.temp_samples > 0U
        ? (s.session.temp_sum_celsius / static_cast<double>(s.session.temp_samples))
        : -1.0;
    const double avg_scale = s.session.scale_samples > 0U
        ? (s.session.scale_sum / static_cast<double>(s.session.scale_samples))
        : s.capacity_scale;

    std::ostringstream out;
    out << "{\"version\":" << json_str(kVersion)
        << ",\"running\":" << (s.running ? "true" : "false")
        << ",\"pid\":" << s.pid
        << ",\"profile\":" << json_str(s.config.profile)
        << ",\"effectiveProfile\":" << json_str(ContextEngine::profile_name(s.ctx.effective_profile))
        << ",\"inspectionMode\":" << (s.config.inspection_mode ? "true" : "false")
        << ",\"mode\":" << json_str(s.thermal_emergency ? "THERMAL_EMERGENCY"
                                    : s.thermal_guard ? "THERMAL_GUARD"
                                    : ContextEngine::mode_name(s.ctx.mode))
        << ",\"foregroundPackage\":" << json_str(s.ctx.foreground_package)
        << ",\"foregroundPid\":" << s.ctx.foreground_pid
        << ",\"capacityScale\":" << safe_num(s.capacity_scale, 1.0)
        << ",\"thermalGuard\":" << (s.thermal_guard ? "true" : "false")
        << ",\"socTempCelsius\":" << (std::isfinite(s.soc_temp) && s.soc_temp >= 0.0 ? std::to_string(s.soc_temp) : "null")
        << ",\"batteryTempCelsius\":" << (std::isfinite(s.battery_temp) && s.battery_temp >= 0.0 ? std::to_string(s.battery_temp) : "null")
        << ",\"thermalPressure\":" << safe_num(s.thermal_pressure)
        << ",\"thermalSource\":" << json_str(s.thermal_source)
        << ",\"cpuUtilPct\":" << safe_num(s.cpu_util_pct)
        << ",\"renderPressure\":" << safe_num(s.render_pressure)
        << ",\"gpuBusyPct\":" << (s.gpu_busy_pct ? std::to_string(safe_num(*s.gpu_busy_pct)) : "null")
        << ",\"cpuPsi\":" << (s.cpu_psi ? std::to_string(safe_num(*s.cpu_psi)) : "null")
        << ",\"memPsi\":" << (s.mem_psi ? std::to_string(safe_num(*s.mem_psi)) : "null")
        << ",\"ioPsi\":" << (s.io_psi ? std::to_string(safe_num(*s.io_psi)) : "null")
        << ",\"aiPredicted10s\":" << safe_num(s.ai_predicted_10s)
        << ",\"aiConfidence\":" << safe_num(s.ai_confidence)
        << ",\"baselineNodesSnapshotted\":" << s.baseline_nodes
        << ",\"recoveryPending\":" << (s.recovery_pending ? "true" : "false")
        << ",\"features\":{"
        << "\"frameSmoothness\":" << (s.config.enable_frame_smoothness ? "true" : "false")
        << ",\"storageBurst\":" << (s.config.enable_storage_burst ? "true" : "false")
        << ",\"memoryGuard\":" << (s.config.enable_memory_guard ? "true" : "false")
        << ",\"thermalPacing\":" << (s.config.enable_thermal_pacing ? "true" : "false")
        << ",\"batteryStepCharging\":" << (s.config.enable_battery_step_charging ? "true" : "false")
        << "}"
        << ",\"apps\":{"
        << "\"gaming\":" << serialize_string_array(gaming_list)
        << ",\"smoothness\":" << serialize_string_array(smoothness_list)
        << ",\"battery\":" << serialize_string_array(battery_list)
        << ",\"excluded\":" << serialize_string_array(excluded_list)
        << "}"
        << ",\"actuators\":{"
        << "\"perThreadUclampSupported\":" << (s.thread_stats.supported ? "true" : "false")
        << ",\"boostedTids\":" << s.thread_stats.active_boosted_tids
        << ",\"threadUclampMin\":" << s.thread_stats.last_util_min
        << ",\"threadUclampMax\":" << s.thread_stats.last_util_max
        << ",\"storageBurstActive\":" << (s.storage_stats.burst_active ? "true" : "false")
        << ",\"readAheadKb\":" << s.storage_stats.active_read_ahead_kb
        << ",\"backgroundIsolated\":" << (s.storage_stats.background_isolated ? "true" : "false")
        << ",\"prereadFiles\":" << s.storage_stats.preread_files_total
        << ",\"prereadBytes\":" << s.storage_stats.preread_bytes_total
        << ",\"managedEnvelopeNodes\":" << s.envelope_stats.managed_nodes
        << ",\"contendedNodes\":" << (s.storage_stats.contended_nodes + s.envelope_stats.contended_nodes)
        << ",\"contendedPaths\":" << serialize_string_array(s.contended_paths)
        << "}"
        << ",\"hardware\":{"
        << "\"socModel\":" << json_str(s.topo.soc_model)
        << ",\"kernelVersion\":" << json_str(s.topo.kernel_version)
        << ",\"storageType\":" << json_str(HardwareProbe::storage_type_name(s.topo.primary_storage))
        << ",\"gpuModel\":" << json_str(s.topo.gpu.model)
        << ",\"ramBytes\":" << s.topo.total_ram_bytes
        << ",\"zramBytes\":" << s.topo.zram_bytes
        << ",\"refreshHz\":" << s.topo.refresh_hz
        << ",\"cpuClusters\":" << s.topo.cpu_clusters.size()
        << ",\"mglruAvailable\":" << (!s.topo.vm.mglru_min_ttl_path.empty() ? "true" : "false")
        << ",\"cgroupV2Available\":" << (!s.topo.cgroups.top_app_procs_path.empty() ? "true" : "false")
        << "}"
        << ",\"batteryPower\":{"
        << "\"available\":" << (s.battery.available ? "true" : "false")
        << ",\"charging\":" << (s.battery.charging ? "true" : "false")
        << ",\"fastCharging\":" << (s.battery.fast_charging ? "true" : "false")
        << ",\"thermalStepActive\":" << (s.battery.thermal_step_active ? "true" : "false")
        << ",\"temperatureCelsius\":" << (s.battery.temperature_celsius >= 0.0 ? std::to_string(safe_num(s.battery.temperature_celsius)) : "null")
        << ",\"currentMa\":" << safe_num(s.battery.current_ma)
        << ",\"voltageMv\":" << safe_num(s.battery.voltage_mv)
        << ",\"powerMw\":" << safe_num(s.battery.power_mw)
        << ",\"chargeLimitUa\":" << s.battery.charge_limit_ua
        << ",\"oemBaselineUa\":" << s.battery.oem_baseline_ua
        << ",\"status\":" << json_str(s.battery.status)
        << "}"
        << ",\"session\":{"
        << "\"uptimeSec\":" << s.session.uptime_sec
        << ",\"launchBursts\":" << s.session.launch_bursts
        << ",\"thermalGuardEvents\":" << s.session.thermal_guard_events
        << ",\"avgSocTempCelsius\":" << (avg_temp >= 0.0 ? std::to_string(safe_num(avg_temp)) : "null")
        << ",\"peakSocTempCelsius\":" << (s.session.peak_soc_temp_celsius >= 0.0 ? std::to_string(safe_num(s.session.peak_soc_temp_celsius)) : "null")
        << ",\"avgCapacityScale\":" << safe_num(avg_scale, 1.0)
        << "}"
        << ",\"history\":[";
    for (size_t i = 0; i < s.history.size(); ++i) {
        if (i != 0U) out << ',';
        const auto& h = s.history[i];
        out << "{\"ts\":" << h.ts_ms
            << ",\"temp\":" << (std::isfinite(h.soc_temp) && h.soc_temp >= 0.0 ? std::to_string(h.soc_temp) : "null")
            << ",\"pressure\":" << safe_num(h.thermal_pressure)
            << ",\"cpu\":" << safe_num(h.cpu_util_pct)
            << ",\"scale\":" << safe_num(h.capacity_scale, 1.0)
            << ",\"mode\":" << json_str(h.mode) << '}';
    }
    out << "]}";
    return out.str();
}

std::string build_readonly_status_json() {
    HardwareProbe probe;
    ThermalMonitor thermal_mon;
    RuntimeStatusData data{};
    data.running = false;
    data.pid = 0;
    data.config = DaemonConfig::load_best_available(false);
    data.ctx.effective_profile = ContextEngine::parse_profile(data.config.profile);
    data.topo = probe.discover();
    const auto tb = thermal_mon.sample_bundle(false);
    data.soc_temp = tb.composite_celsius;
    data.battery_temp = tb.battery_celsius;
    data.thermal_pressure = tb.assessment.pressure;
    data.thermal_source = tb.assessment.source;
    data.gpu_busy_pct = probe.read_gpu_busy_pct(data.topo);
    data.cpu_psi = probe.read_psi_avg10("cpu");
    data.mem_psi = probe.read_psi_avg10("memory");
    data.io_psi = probe.read_psi_avg10("io");
    data.battery = HardwareLongevityManager::probe_battery_telemetry();
    data.baseline_nodes = count_journal_entries();
    data.recovery_pending = !journal_clear();
    return build_status_json(data);
}

void print_readonly_status() {
    std::cout << build_readonly_status_json() << "\n" << std::flush;
}

std::string build_capabilities_json() {
    HardwareProbe probe;
    const auto topo = probe.discover();
    ThreadSchedulerActuator probe_sched;
    const bool per_thread_uclamp = probe_sched.probe_support();
    std::ostringstream out;
    out << "{\"version\":" << json_str(kVersion)
        << ",\"socModel\":" << json_str(topo.soc_model)
        << ",\"kernelVersion\":" << json_str(topo.kernel_version)
        << ",\"storageType\":" << json_str(HardwareProbe::storage_type_name(topo.primary_storage))
        << ",\"gpuModel\":" << json_str(topo.gpu.model)
        << ",\"ramBytes\":" << topo.total_ram_bytes
        << ",\"zramBytes\":" << topo.zram_bytes
        << ",\"refreshHz\":" << topo.refresh_hz
        << ",\"cpuClusters\":" << topo.cpu_clusters.size()
        << ",\"rustPidEngine\":true"
        << ",\"perThreadUclampSupported\":" << (per_thread_uclamp ? "true" : "false")
        << ",\"mglruAvailable\":" << (!topo.vm.mglru_min_ttl_path.empty() ? "true" : "false")
        << ",\"cgroupV2Available\":" << (!topo.cgroups.top_app_procs_path.empty() ? "true" : "false")
        << ",\"oledDisplayPowerNodes\":" << (topo.drm_dpms_paths.size() + topo.fb_blank_paths.size())
        << "}";
    return out.str();
}

void print_capabilities() {
    std::cout << build_capabilities_json() << "\n" << std::flush;
}

std::string hex_decode_journal_field(std::string_view hex) {
    if ((hex.size() % 2U) != 0U) return {};
    std::string out;
    out.reserve(hex.size() / 2U);
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        return -1;
    };
    for (size_t i = 0; i < hex.size(); i += 2U) {
        const int hi = nibble(hex[i]);
        const int lo = nibble(hex[i + 1U]);
        if (hi < 0 || lo < 0) return {};
        out.push_back(static_cast<char>((hi << 4) | lo));
    }
    return out;
}

std::string build_journal_json() {
    struct JournalRow {
        std::string path;
        std::string factory_value;
        std::string active_value;
        bool modified{true};
    };
    std::vector<JournalRow> rows;
    const char* env_dir = std::getenv("COREFLOW_STATE_DIR");
    const std::string state_dir = (env_dir && env_dir[0] != '\0') ? std::string(env_dir) : std::string(kStateDir);
    const std::string jpath = state_dir + "/baseline.journal";
    UniqueFd fd(::open(jpath.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (fd.valid()) {
        struct stat st{};
        if (::fstat(fd.get(), &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0 && st.st_size <= 262144) {
            std::string raw(static_cast<size_t>(st.st_size), '\0');
            size_t off = 0;
            while (off < raw.size()) {
                const ssize_t n = ::read(fd.get(), raw.data() + off, raw.size() - off);
                if (n < 0 && errno == EINTR) continue;
                if (n <= 0) break;
                off += static_cast<size_t>(n);
            }
            raw.resize(off);
            std::istringstream ss(raw);
            std::string header;
            if (std::getline(ss, header) && header.rfind("CFJ", 0) == 0) {
                std::string line;
                while (std::getline(ss, line)) {
                    if (DaemonConfig::trim(line).empty()) continue;
                    std::vector<std::string> cols;
                    size_t start = 0;
                    while (start <= line.size()) {
                        const size_t tab = line.find('\t', start);
                        if (tab == std::string::npos) {
                            cols.push_back(line.substr(start));
                            break;
                        }
                        cols.push_back(line.substr(start, tab - start));
                        start = tab + 1U;
                    }
                    if (cols.size() >= 4U) {
                        JournalRow r{};
                        r.path = cols[0];
                        r.factory_value = hex_decode_journal_field(cols[1]);
                        r.active_value = hex_decode_journal_field(cols[3]);
                        r.modified = true;
                        rows.push_back(std::move(r));
                    } else if (cols.size() >= 2U) {
                        JournalRow r{};
                        r.path = cols[0];
                        r.factory_value = hex_decode_journal_field(cols[1]);
                        r.active_value = r.factory_value;
                        r.modified = true;
                        rows.push_back(std::move(r));
                    }
                }
            }
        }
    }

    std::ostringstream out;
    out << "{\"format\":\"CFJ4\",\"entryCount\":" << rows.size() << ",\"entries\":[";
    for (size_t i = 0; i < rows.size(); ++i) {
        if (i != 0U) out << ',';
        const auto& e = rows[i];
        out << "{\"path\":" << json_str(e.path)
            << ",\"factoryValue\":" << json_str(e.factory_value)
            << ",\"activeValue\":" << json_str(e.active_value)
            << ",\"modified\":" << (e.modified ? "true" : "false") << '}';
    }
    out << "]}";
    return out.str();
}

void print_journal() {
    std::cout << build_journal_json() << "\n" << std::flush;
}

bool read_live_status_snapshot_str(pid_t expected_pid, std::string& out_payload) {
    if (expected_pid <= 0) return false;
    UniqueFd fd(::open("/data/adb/coreflow/status.json", O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return false;
    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode) || st.st_size <= 0 || st.st_size > 512 * 1024) {
        return false;
    }
    const auto now = std::chrono::system_clock::now();
    const auto mod = std::chrono::system_clock::from_time_t(st.st_mtime);
    if (now < mod || now - mod > std::chrono::seconds(4)) return false;
    std::string payload(static_cast<size_t>(st.st_size), '\0');
    size_t off = 0;
    while (off < payload.size()) {
        const ssize_t n = ::read(fd.get(), payload.data() + off, payload.size() - off);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        off += static_cast<size_t>(n);
    }
    const std::string pid_field = "\"pid\":" + std::to_string(expected_pid) + ",";
    if (payload.find("\"running\":true") == std::string::npos ||
        payload.find(pid_field) == std::string::npos ||
        payload.front() != '{' || payload.back() != '}') {
        return false;
    }
    out_payload = std::move(payload);
    return true;
}

void print_diag() {
    std::string status_json;
    const pid_t live = daemon_pid();
    if (!(live > 0 && read_live_status_snapshot_str(live, status_json))) {
        status_json = build_readonly_status_json();
    }
    const std::string caps_json = build_capabilities_json();
    const std::string journal_json = build_journal_json();
#ifndef COREFLOW_BUILD_COMMIT
#define COREFLOW_BUILD_COMMIT "unknown"
#endif
    std::ostringstream out;
    out << "{\"reportFormat\":\"coreflow-diag-v1\""
        << ",\"version\":" << json_str(kVersion)
        << ",\"commit\":" << json_str(COREFLOW_BUILD_COMMIT)
        << ",\"generatedAtMs\":" << epoch_now_ms()
        << ",\"safetyMarkers\":{"
        << "\"safeMode\":" << (marker_exists("/data/adb/coreflow/SAFE_MODE") ? "true" : "false")
        << ",\"disable\":" << (marker_exists("/data/adb/coreflow/DISABLE") ? "true" : "false")
        << "}"
        << ",\"capabilities\":" << caps_json
        << ",\"status\":" << status_json
        << ",\"journal\":" << journal_json
        << "}";
    std::cout << out.str() << "\n" << std::flush;
}

int stop_daemon() {
    const pid_t pid = daemon_pid();
    if (pid > 0) {
        if (::kill(pid, SIGTERM) != 0 && errno != ESRCH) return 1;
        for (int i = 0; i < 80; ++i) {
            if (!process_is_coreflowd(pid)) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (process_is_coreflowd(pid)) return 1;
    }
    if (!BaselineStore::instance().recover_pending()) return 1;
    return journal_clear() ? 0 : 1;
}

} // namespace

int main(int argc, char* argv[]) {
    (void)::umask(0077);
    (void)::setenv("LC_ALL", "C", 1);
    (void)std::setlocale(LC_ALL, "C");

    bool fork_daemon = false;
    if (argc >= 2 && std::string_view(argv[1]) == "--set-config") {
        if (argc < 3) {
            std::cerr << "Usage: coreflowd --set-config key=value [key=value ...]\n";
            return 2;
        }
        std::vector<std::pair<std::string, std::string>> updates;
        for (int i = 2; i < argc; ++i) {
            const std::string_view arg(argv[i]);
            const size_t eq = arg.find('=');
            if (eq == std::string_view::npos || eq == 0U) {
                std::cerr << "Invalid key=value argument: " << arg << "\n";
                return 2;
            }
            updates.emplace_back(
                DaemonConfig::trim(std::string(arg.substr(0, eq))),
                DaemonConfig::trim(std::string(arg.substr(eq + 1U))));
        }
        (void)ensure_secure_state_dir();
        const std::string conf_path = std::string(kStateDir) + "/coreflow.conf";
        if (!DaemonConfig::update_config_file(conf_path, updates)) {
            std::cerr << "Rejected or failed configuration update.\n";
            return 1;
        }
        const pid_t pid = daemon_pid();
        if (pid > 0) (void)::kill(pid, SIGHUP);
        return 0;
    }

    if (argc == 2) {
        const std::string_view cmd(argv[1]);
        if (cmd == "--status") {
            const pid_t live = daemon_pid();
            if (live > 0 && print_live_status_snapshot(live)) return 0;
            print_readonly_status();
            return 0;
        }
        if (cmd == "--capabilities") {
            print_capabilities();
            return 0;
        }
        if (cmd == "--journal") {
            print_journal();
            return 0;
        }
        if (cmd == "--diag") {
            print_diag();
            return 0;
        }
        if (cmd == "--stop" || cmd == "--rollback-baseline") {
            return stop_daemon();
        }
        if (cmd == "--daemon" || cmd == "-d") {
            fork_daemon = true;
        } else {
            std::cerr << "Usage: coreflowd [--daemon|--status|--capabilities|--journal|--diag|--stop|--rollback-baseline|--set-config key=value ...]\n";
            return 2;
        }
    } else if (argc != 1) {
        std::cerr << "Usage: coreflowd [--daemon|--status|--capabilities|--journal|--diag|--stop|--rollback-baseline|--set-config key=value ...]\n";
        return 2;
    }

    if (safety_halt_requested()) {
        emit_log(true, "[CoreFlow] Safety marker active; refusing runtime start.");
        return 2;
    }
    if (daemon_pid() > 0) return 0;
    if (!service_launcher_authorized()) {
        emit_log(true, "[CoreFlow] Launch rejected: service.sh must hold service.lock.");
        return 2;
    }

    if (fork_daemon) {
        if (!daemonize_process()) return 1;
    } else {
        (void)::setsid();
        for (int fd = 3; fd < 256; ++fd) (void)::close(fd);
    }

    escape_app_cgroups_and_protect_oom();
    install_signal_hardening();

    UniqueFd pid_lock;
    if (!write_pid(pid_lock)) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) return 0;
        emit_log(true, "[CoreFlow] Unable to acquire PID singleton lock.");
        return 1;
    }

    auto& baseline = BaselineStore::instance();
    if (!baseline.recover_pending()) {
        emit_log(true, "[CoreFlow] Durable CFJ4 journal recovery failed; refusing new mutations.");
        clear_pid(pid_lock);
        return 1;
    }

    // CoreFlow v5 4-Pillar Runtime Composition
    DaemonConfig config = DaemonConfig::load_best_available();
    HardwareProbe probe;
    HardwareTopology topo = probe.discover();
    ThermalMonitor thermal_mon;
    NeuralPredictor neural;
    neural.initialize("/data/adb/coreflow/models");
    CentralMutationCoordinator coordinator;
    HardwareLongevityManager longevity(coordinator);

    ContextEngine context_engine;
    ThreadSchedulerActuator thread_scheduler;
    (void)thread_scheduler.probe_support();
    StorageMemoryActuator storage_memory(baseline);
    ThermalEnvelopeActuator thermal_envelope(baseline);

    const auto apply_config_to_subsystems = [&]() {
        context_engine.set_profile(ContextEngine::parse_profile(config.profile));
        context_engine.set_app_rules(
            DaemonConfig::parse_package_list(config.gaming_apps),
            DaemonConfig::parse_package_list(config.excluded_apps),
            DaemonConfig::parse_package_list(config.smoothness_apps),
            DaemonConfig::parse_package_list(config.battery_apps));
        longevity.initialize({
            config.enable_battery_step_charging,
            config.enable_storage_burst,
            config.battery_step_temp_threshold,
            config.stepped_charge_current_ma,
            config.top_app_io_weight,
            config.background_io_weight});
    };
    apply_config_to_subsystems();

    const auto pid_target_pressure = [&config](bool generic_fallback) noexcept {
        if (!generic_fallback) return 70.0;
        const double rec = static_cast<double>(config.thermal_recover_celsius);
        const double trip = static_cast<double>(config.thermal_trip_celsius);
        const double span = std::max(1.0, trip - rec);
        const double norm = std::clamp((static_cast<double>(config.pid_target_celsius) - rec) / span, 0.0, 1.0);
        return std::clamp(65.0 + norm * 20.0, 0.0, 85.0);
    };
    bool pid_target_fallback = false;
    bool pid_initialized = false;
    auto pid_controller = engine::new_controller({
        config.pid_kp, config.pid_ki, config.pid_kd, pid_target_pressure(false)});

    EpollReactor reactor;
    NativeInputEventSource input_source(reactor);
    CpuUtilSampler cpu_sampler;
    RenderLoadSampler render_sampler;

    RuntimeStatusData live_status{};
    live_status.running = true;
    live_status.pid = ::getpid();
    live_status.config = config;
    live_status.ctx.effective_profile = context_engine.profile();
    live_status.topo = topo;
    live_status.thread_stats = thread_scheduler.stats();
    live_status.battery = longevity.sample_battery_telemetry();
    live_status.session.started_steady_ms = steady_now_ms();
    bool last_screen_off = false;
    ForegroundAppInfo last_fg{};
    MemorySnapshot last_mem_snap = probe.read_memory_snapshot();
    uint64_t last_sysfs_active_ms = 0U;
    auto last_thermal_tick = std::chrono::steady_clock::now();

    const auto build_sysfs_ctx = [&](const ContextDecision& raw, uint64_t now_ms, const ForegroundAppInfo& fg) noexcept {
        ContextDecision out = raw;
        // Only prefetch assets for real visible UI apps that own a render thread.
        if (fg.critical_tids.size() <= 1U) {
            out.newly_launched_app = false;
        }
        // Anti-flap hysteresis for disk-journaled sysfs actuators: hold Interactive for 4s
        // before relaxing to Efficiency so typing/scrolling does not thrash 25 CFJ4 nodes/sec.
        if (raw.mode == ContextMode::Interactive || raw.mode == ContextMode::LaunchBurst ||
            raw.mode == ContextMode::Gaming) {
            last_sysfs_active_ms = now_ms;
        } else if (raw.mode == ContextMode::Efficiency && last_sysfs_active_ms > 0U &&
                   now_ms >= last_sysfs_active_ms && now_ms - last_sysfs_active_ms < 4000ULL) {
            out.mode = ContextMode::Interactive;
        }
        return out;
    };

    const auto build_thread_demand = [&]() noexcept {
        ThreadDemandContext d{};
        d.little_capacity = topo.little_capacity;
        d.mid_capacity = topo.mid_capacity;
        d.big_capacity = topo.big_capacity;
        d.refresh_hz = topo.refresh_hz;
        d.cpu_psi = live_status.cpu_psi.value_or(0.0);
        d.gpu_busy_pct = live_status.gpu_busy_pct.value_or(0.0);
        d.render_pressure = live_status.render_pressure;
        return d;
    };
    const auto build_storage_telemetry = [&](const MemorySnapshot& mem) noexcept {
        StorageMemoryTelemetry t{};
        t.io_psi = live_status.io_psi.value_or(0.0);
        t.mem_psi = live_status.mem_psi.value_or(0.0);
        t.cpu_psi = live_status.cpu_psi.value_or(0.0);
        t.memory = mem;
        return t;
    };
    const auto build_envelope_telemetry = [&]() noexcept {
        EnvelopeTelemetry e{};
        e.cpu_psi = live_status.cpu_psi.value_or(0.0);
        e.gpu_busy_pct = live_status.gpu_busy_pct.value_or(0.0);
        return e;
    };

    const auto collect_contended_paths = [&]() {
        std::vector<std::string> merged = storage_memory.contended_paths();
        const auto env_paths = thermal_envelope.contended_paths();
        merged.insert(merged.end(), env_paths.begin(), env_paths.end());
        std::sort(merged.begin(), merged.end());
        merged.erase(std::unique(merged.begin(), merged.end()), merged.end());
        return merged;
    };

    // Fast path 1 (< 5 ms): Touchscreen event -> instant per-TID UI/RenderThread boost
    if (!input_source.start([&](const FrameEvent&) {
            if (config.inspection_mode || live_status.thermal_emergency || last_screen_off) return;
            const uint64_t now_ms = steady_now_ms();
            context_engine.notify_touch(now_ms);
            if (live_status.ctx.mode == ContextMode::Efficiency && !live_status.ctx.app_excluded) {
                ContextSignalInput sig{};
                sig.now_ms = now_ms;
                sig.screen_off = false;
                sig.thermal_guard = live_status.thermal_guard;
                sig.cpu_util_pct = live_status.cpu_util_pct;
                sig.gpu_busy_pct = live_status.gpu_busy_pct;
                sig.foreground_pid = last_fg.pid;
                sig.foreground_package = last_fg.package_name;
                sig.has_game_thread = last_fg.has_game_thread;
                live_status.ctx = context_engine.evaluate(sig);
                const auto sf_tids = context_engine.resolve_surfaceflinger_tids();
                thread_scheduler.apply(
                    live_status.ctx.mode, live_status.ctx.effective_profile,
                    live_status.capacity_scale, live_status.thermal_guard,
                    config.enable_frame_smoothness, sf_tids, last_fg.critical_tids,
                    build_thread_demand());
                live_status.thread_stats = thread_scheduler.stats();
            }
        })) {
        emit_log(false, "[CoreFlow] Touchscreen evdev unavailable; running timer-driven context path.");
    }

    // Signals: SIGINT, SIGTERM, SIGHUP
    sigset_t mask{};
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);
    if (::pthread_sigmask(SIG_BLOCK, &mask, nullptr) != 0) {
        clear_pid(pid_lock);
        return 1;
    }

    int thermal_timer_fd = -1;
    UniqueFd signal_fd(::signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC));
    if (!signal_fd.valid() ||
        !reactor.register_fd(signal_fd.get(), EPOLLIN, [&](uint32_t) {
            signalfd_siginfo info{};
            while (::read(signal_fd.get(), &info, sizeof(info)) == static_cast<ssize_t>(sizeof(info))) {
                if (info.ssi_signo == SIGHUP) {
                    thread_scheduler.reset_all();
                    longevity.restore_oem_baseline();
                    (void)baseline.rollback_all();
                    storage_memory.reset_contention();
                    thermal_envelope.reset_contention();
                    config = DaemonConfig::load_best_available();
                    topo = probe.discover();
                    apply_config_to_subsystems();
                    pid_initialized = false;
                    pid_controller = engine::new_controller({
                        config.pid_kp, config.pid_ki, config.pid_kd, pid_target_pressure(pid_target_fallback)});
                    live_status.config = config;
                    live_status.topo = topo;
                    live_status.ctx.effective_profile = context_engine.profile();
                    const uint32_t ms = std::clamp(config.temp_monitor_interval_ms, 1000u, 60000u);
                    itimerspec ts{};
                    ts.it_interval.tv_sec = ms / 1000;
                    ts.it_interval.tv_nsec = (ms % 1000) * 1000000L;
                    ts.it_value = ts.it_interval;
                    if (thermal_timer_fd >= 0) (void)::timerfd_settime(thermal_timer_fd, 0, &ts, nullptr);
                    emit_log(false, "[CoreFlow] Configuration reloaded (profile=" + config.profile +
                             ", paused=" + (config.inspection_mode ? "yes" : "no") + ").");
                } else {
                    reactor.stop();
                }
            }
        })) {
        clear_pid(pid_lock);
        return 1;
    }

    // Fast path 2 (200 ms while screen ON): Rapid foreground app change & launch burst trigger
    (void)reactor.create_timerfd(std::chrono::milliseconds(200), [&](uint32_t) {
        if (last_screen_off || config.inspection_mode || live_status.thermal_emergency) return;
        const auto fg = context_engine.resolve_foreground_app(topo.cgroups.top_app_procs_path);
        const bool changed = fg.pid != last_fg.pid || fg.package_name != last_fg.package_name;
        if (!changed && live_status.ctx.mode != ContextMode::LaunchBurst &&
            live_status.ctx.mode != ContextMode::Interactive) {
            return;
        }
        last_fg = fg;
        const uint64_t now_ms = steady_now_ms();
        ContextSignalInput sig{};
        sig.now_ms = now_ms;
        sig.screen_off = false;
        sig.thermal_guard = live_status.thermal_guard;
        sig.cpu_util_pct = live_status.cpu_util_pct;
        sig.gpu_busy_pct = live_status.gpu_busy_pct;
        sig.foreground_pid = fg.pid;
        sig.foreground_package = fg.package_name;
        sig.has_game_thread = fg.has_game_thread;
        const ContextMode prev_mode = live_status.ctx.mode;
        live_status.ctx = context_engine.evaluate(sig);
        if (live_status.ctx.newly_launched_app) {
            ++live_status.session.launch_bursts;
        }
        if (changed || live_status.ctx.mode != prev_mode) {
            const auto sf_tids = context_engine.resolve_surfaceflinger_tids();
            thread_scheduler.apply(
                live_status.ctx.mode, live_status.ctx.effective_profile,
                live_status.capacity_scale, live_status.thermal_guard,
                config.enable_frame_smoothness, sf_tids, fg.critical_tids,
                build_thread_demand());
            const auto sysfs_ctx = build_sysfs_ctx(live_status.ctx, now_ms, fg);
            storage_memory.apply(
                topo, sysfs_ctx, live_status.ctx.effective_profile,
                config.enable_storage_burst, config.enable_memory_guard, now_ms,
                build_storage_telemetry(last_mem_snap));
            thermal_envelope.apply(
                topo, sysfs_ctx.mode, live_status.ctx.effective_profile,
                live_status.capacity_scale, live_status.thermal_guard,
                config.enable_thermal_pacing,
                build_envelope_telemetry(), now_ms);
            live_status.thread_stats = thread_scheduler.stats();
            live_status.storage_stats = storage_memory.stats();
            live_status.envelope_stats = thermal_envelope.stats();
            live_status.contended_paths = collect_contended_paths();
        }
    });

    // Main control path (default 1000 ms): Thermal + PSI + Online RLS/MLP + Rust PID + All 4 Pillars + Status JSON
    const auto interval = std::chrono::milliseconds(std::clamp(config.temp_monitor_interval_ms, 1000u, 60000u));
    thermal_timer_fd = reactor.create_timerfd(interval, [&](uint32_t) {
        if (safety_halt_requested()) {
            reactor.stop();
            return;
        }

        const auto now_tp = std::chrono::steady_clock::now();
        const double elapsed = std::clamp(
            std::chrono::duration<double>(now_tp - last_thermal_tick).count(), 0.001, 60.0);
        last_thermal_tick = now_tp;
        const uint64_t now_ms = steady_now_ms();
        if (now_ms >= live_status.session.started_steady_ms) {
            live_status.session.uptime_sec = (now_ms - live_status.session.started_steady_ms) / 1000ULL;
        }

        const auto tb = thermal_mon.sample_bundle(true);
        auto thermal = tb.assessment;
        const float soc_temp = tb.composite_celsius;
        const float batt_temp = tb.battery_celsius;

        if (!thermal.available && (tb.profile.total_valid_sensors > 0 || soc_temp >= 0.0f)) {
            const double rec = static_cast<double>(config.thermal_recover_celsius);
            const double trip = static_cast<double>(config.thermal_trip_celsius);
            const double norm = std::clamp((static_cast<double>(soc_temp) - rec) / std::max(1.0, trip - rec), 0.0, 3.0);
            thermal.available = true;
            thermal.fallback_active = true;
            thermal.pressure = std::clamp(65.0 + norm * 20.0, 0.0, 130.0);
            thermal.source = "generic_fallback";
            if (soc_temp >= config.thermal_trip_celsius + config.thermal_emergency_margin_celsius) {
                thermal.emergency = true;
            }
        }
        if (!pid_initialized || pid_target_fallback != thermal.fallback_active) {
            pid_target_fallback = thermal.fallback_active;
            pid_controller = engine::new_controller({
                config.pid_kp, config.pid_ki, config.pid_kd, pid_target_pressure(pid_target_fallback)});
            pid_initialized = true;
        }

        const bool screen_off = probe.is_screen_off(topo);
        last_screen_off = screen_off;
        const double cpu_util = cpu_sampler.sample();
        const auto gpu_busy = probe.read_gpu_busy_pct(topo);
        const auto cpu_psi = probe.read_psi_avg10("cpu");
        const auto mem_psi = probe.read_psi_avg10("memory");
        const auto io_psi = probe.read_psi_avg10("io");
        last_mem_snap = probe.read_memory_snapshot();

        live_status.soc_temp = soc_temp;
        live_status.battery_temp = batt_temp;
        live_status.thermal_pressure = thermal.pressure;
        live_status.thermal_source = thermal.source;
        live_status.cpu_util_pct = cpu_util;
        live_status.gpu_busy_pct = gpu_busy;
        live_status.cpu_psi = cpu_psi;
        live_status.mem_psi = mem_psi;
        live_status.io_psi = io_psi;

        if (std::isfinite(soc_temp) && soc_temp > 0.0f) {
            ++live_status.session.temp_samples;
            live_status.session.temp_sum_celsius += static_cast<double>(soc_temp);
            if (live_status.session.peak_soc_temp_celsius < 0.0 ||
                static_cast<double>(soc_temp) > live_status.session.peak_soc_temp_celsius) {
                live_status.session.peak_soc_temp_celsius = static_cast<double>(soc_temp);
            }
        }

        const auto record_history_and_publish = [&]() {
            live_status.battery = longevity.sample_battery_telemetry();
            live_status.contended_paths = collect_contended_paths();
            size_t active_nodes = 0U;
            for (const auto& entry : baseline.get_audit_trail()) {
                if (entry.is_modified) ++active_nodes;
            }
            live_status.baseline_nodes = active_nodes;
            live_status.recovery_pending = active_nodes > 0U;
            ++live_status.session.scale_samples;
            live_status.session.scale_sum += live_status.capacity_scale;
            live_status.history.push_back(HistoryPoint{
                .ts_ms = epoch_now_ms(),
                .soc_temp = soc_temp >= 0.0f ? static_cast<double>(soc_temp) : -1.0,
                .thermal_pressure = thermal.pressure,
                .cpu_util_pct = cpu_util,
                .capacity_scale = live_status.capacity_scale,
                .mode = live_status.thermal_emergency ? "THERMAL_EMERGENCY"
                        : live_status.thermal_guard ? "THERMAL_GUARD"
                        : ContextEngine::mode_name(live_status.ctx.mode),
            });
            if (live_status.history.size() > 60U) {
                live_status.history.erase(live_status.history.begin());
            }
            (void)write_status_snapshot(build_status_json(live_status));
        };

        // Hard hardware safety cutoff (battery >= 58C or SoC >= 95C)
        if ((batt_temp > 0.0f && batt_temp >= 58.0f) || soc_temp >= 95.0f) {
            live_status.thermal_emergency = true;
            live_status.thermal_guard = true;
            if (!live_status.session.prev_thermal_guard) {
                ++live_status.session.thermal_guard_events;
                live_status.session.prev_thermal_guard = true;
            }
            live_status.capacity_scale = 0.25;
            thread_scheduler.reset_all();
            longevity.restore_oem_baseline();
            (void)baseline.rollback_all();
            emit_log(true, "[CoreFlow][Safety] Extreme thermal threshold reached; rolled back to OEM baseline.");
            record_history_and_publish();
            return;
        }
        live_status.thermal_emergency = false;

        const bool thermal_guard = !thermal.fallback_active
            ? (thermal.emergency || thermal.severity >= 3 || thermal.pressure >= 85.0 ||
               (live_status.thermal_guard && thermal.pressure > 65.0))
            : (thermal.emergency || soc_temp >= config.thermal_trip_celsius ||
               (live_status.thermal_guard && soc_temp > config.thermal_recover_celsius));
        if (thermal_guard && !live_status.session.prev_thermal_guard) {
            ++live_status.session.thermal_guard_events;
        }
        live_status.session.prev_thermal_guard = thermal_guard;
        live_status.thermal_guard = thermal_guard;

        // 10s Online RLS + Micro-MLP thermal lookahead + Rust PID capacity scale
        const auto ai = neural.infer_and_learn({
            .thermal_pressure = std::clamp(thermal.pressure, 0.0, 130.0),
            .cpu_psi = std::clamp(cpu_psi.value_or(0.0), 0.0, 100.0),
            .memory_psi = std::clamp(mem_psi.value_or(0.0), 0.0, 100.0),
            .io_psi = std::clamp(io_psi.value_or(0.0), 0.0, 100.0),
            .cpu_psi_valid = cpu_psi.has_value(),
            .memory_psi_valid = mem_psi.has_value(),
            .io_psi_valid = io_psi.has_value(),
            .current_scale = live_status.capacity_scale,
            .elapsed_seconds = elapsed,
        });
        live_status.ai_predicted_10s = ai.valid ? ai.predicted_pressure_10s : 0.0;
        live_status.ai_confidence = ai.valid ? ai.confidence : 0.0;

        double eff_pressure = std::clamp(thermal.pressure, 0.0, 130.0);
        if (ai.valid && ai.confidence >= 0.55 && ai.predicted_pressure_10s > eff_pressure) {
            eff_pressure = std::clamp(0.70 * eff_pressure + 0.30 * ai.predicted_pressure_10s, 0.0, 130.0);
        }

        const auto pid_dec = pid_controller->step({
            eff_pressure, elapsed,
            std::clamp(cpu_psi.value_or(0.0), 0.0, 100.0),
            std::clamp(mem_psi.value_or(0.0), 0.0, 100.0),
            std::clamp(io_psi.value_or(0.0), 0.0, 100.0),
            cpu_psi.has_value(), mem_psi.has_value(), io_psi.has_value(),
            85.0, 65.0, 15.0, config.cpu_pressure_boost_pct,
            config.memory_pressure_guard_pct, config.io_pressure_guard_pct,
            config.enable_pid_equilibrium});
        if (pid_dec.valid) {
            double scale = pid_dec.capacity_scale;
            if (ai.valid && ai.confidence >= 0.60 && ai.predicted_pressure_10s >= 80.0) {
                scale = std::min(scale, ai.recommended_scale);
            }
            if (thermal_guard) scale = std::min(scale, 0.75);
            live_status.capacity_scale = std::clamp(scale, 0.25, 1.0);
        } else {
            pid_controller->reset();
        }

        if (!screen_off) {
            last_fg = context_engine.resolve_foreground_app(topo.cgroups.top_app_procs_path);
            live_status.render_pressure = render_sampler.sample(last_fg.pid, last_fg.critical_tids, elapsed);
        } else {
            live_status.render_pressure = render_sampler.sample(0, {}, elapsed);
        }

        ContextSignalInput sig{};
        sig.now_ms = now_ms;
        sig.screen_off = screen_off;
        sig.thermal_guard = thermal_guard;
        sig.cpu_util_pct = cpu_util;
        sig.gpu_busy_pct = gpu_busy;
        sig.foreground_pid = screen_off ? 0 : last_fg.pid;
        sig.foreground_package = screen_off ? std::string{} : last_fg.package_name;
        sig.has_game_thread = !screen_off && last_fg.has_game_thread;
        live_status.ctx = context_engine.evaluate(sig);
        if (live_status.ctx.newly_launched_app) {
            ++live_status.session.launch_bursts;
        }

        if (!config.inspection_mode) {
            const auto sf_tids = screen_off ? std::vector<int32_t>{} : context_engine.resolve_surfaceflinger_tids();
            thread_scheduler.apply(
                live_status.ctx.mode, live_status.ctx.effective_profile,
                live_status.capacity_scale, thermal_guard,
                config.enable_frame_smoothness, sf_tids, last_fg.critical_tids,
                build_thread_demand());
            const auto sysfs_ctx = build_sysfs_ctx(live_status.ctx, now_ms, last_fg);
            storage_memory.apply(
                topo, sysfs_ctx, live_status.ctx.effective_profile,
                config.enable_storage_burst, config.enable_memory_guard, now_ms,
                build_storage_telemetry(last_mem_snap));
            thermal_envelope.apply(
                topo, sysfs_ctx.mode, live_status.ctx.effective_profile,
                live_status.capacity_scale, thermal_guard,
                config.enable_thermal_pacing,
                build_envelope_telemetry(), now_ms);
            longevity.monitor_battery_thermals(soc_temp);
        } else {
            thread_scheduler.reset_all();
        }

        live_status.thread_stats = thread_scheduler.stats();
        live_status.storage_stats = storage_memory.stats();
        live_status.envelope_stats = thermal_envelope.stats();
        record_history_and_publish();
    });

    (void)write_status_snapshot(build_status_json(live_status));
    emit_log(false, "[CoreFlow] v5.1.0 Autonomous 4-Pillar Daemon started (profile=" + config.profile +
             ", storage=" + HardwareProbe::storage_type_name(topo.primary_storage) +
             ", perThreadUclamp=" + (thread_scheduler.stats().supported ? "yes" : "no") + ").");

    reactor.run();

    // Guaranteed clean shutdown & OEM baseline restoration
    input_source.stop();
    thread_scheduler.reset_all();
    longevity.restore_oem_baseline();
    (void)baseline.rollback_all();
    live_status.running = false;
    live_status.pid = 0;
    (void)write_status_snapshot(build_status_json(live_status));
    clear_pid(pid_lock);
    emit_log(false, "[CoreFlow] v5.1.0 Daemon stopped; OEM baseline restored.");
    return 0;
}
