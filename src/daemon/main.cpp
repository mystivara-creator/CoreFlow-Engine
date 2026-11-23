/**
 * @file main.cpp
 * @brief CFAE System Framework / CoreFlow Engine v6.1.0-R — Unified Zero-Profile Autonomous Daemon.
 *
 * Pillars:
 *  1. Sovereign Tamper Guard (0444 lease lock, Reclaim-First + 3x/5s circuit breaker, IPC broadcast)
 *  2. Coupled Multi-Domain Orchestration (Compute<->Memory Bus, Rendering<->Scheduler, Headroom<->Thermal)
 *  3. Zero-Wear On-Device Context Learning (CFDNA2 cost-to-gain RAM learning & significant-drift ScreenOff flush)
 *  4. Clean JSON status/config/diag/tamper contract for the Companion App & Telemetry Dialogue Engine.
 * @license Apache-2.0
 */
#include "coreflow/baseline_store.hpp"
#include "coreflow/config.hpp"
#include "coreflow/cooling_feedback.hpp"
#include "coreflow/frame_bridge.hpp"
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
#include "coreflow/v6/app_dna_store.hpp"
#include "coreflow/v6/boot_calm_sanctuary.hpp"
#include "coreflow/v6/decision_journal.hpp"
#include "coreflow/v6/frame_pacing_sensor.hpp"
#include "coreflow/v6/oem_collaboration_map.hpp"
#include "coreflow/v6/semantic_knob_engine.hpp"
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
#include <sys/wait.h>
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
constexpr char kVersion[] = "6.1.0-R";

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
    if (::write(STDERR_FILENO, msg, sizeof(msg) - 1U) < 0) {}
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

bool save_preloader_state(const engine::AppPreloader& preloader,
                          const std::unordered_map<uint64_t, std::string>& pkg_map) noexcept {
    if (!ensure_secure_state_dir()) return false;
    const rust::String exported = preloader.export_state();
    std::ostringstream out;
    out << std::string_view(exported.data(), exported.size());
    for (const auto& [hash, pkg] : pkg_map) {
        if (hash == 0ULL || pkg.empty() || pkg.find('\n') != std::string::npos || pkg.find('\t') != std::string::npos) {
            continue;
        }
        out << "P\t" << hash << '\t' << pkg << '\n';
    }
    const std::string payload = out.str();
    if (payload.empty() || payload.size() > 64U * 1024U) return false;

    UniqueFd dir(::open(kStateDir, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    if (!dir.valid()) return false;
    const std::string tmp = "preloader.state.tmp." + std::to_string(::getpid());
    UniqueFd fd(::openat(dir.get(), tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW, 0600));
    if (!fd.valid()) return false;
    size_t off = 0;
    while (off < payload.size()) {
        const ssize_t n = ::write(fd.get(), payload.data() + off, payload.size() - off);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { (void)::unlinkat(dir.get(), tmp.c_str(), 0); return false; }
        off += static_cast<size_t>(n);
    }
    (void)::fsync(fd.get());
    if (::renameat(dir.get(), tmp.c_str(), dir.get(), "preloader.state") != 0) {
        (void)::unlinkat(dir.get(), tmp.c_str(), 0);
        return false;
    }
    return true;
}

void load_preloader_state(engine::AppPreloader& preloader,
                          std::unordered_map<uint64_t, std::string>& pkg_map) noexcept {
    const std::string path = std::string(kStateDir) + "/preloader.state";
    UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return;
    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode) || st.st_size <= 0 || st.st_size > 65536) {
        return;
    }
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
    std::ostringstream rust_payload;
    std::string line;
    while (std::getline(ss, line)) {
        if (line.rfind("P\t", 0) == 0) {
            const size_t tab2 = line.find('\t', 2U);
            if (tab2 != std::string::npos && tab2 > 2U && tab2 + 1U < line.size()) {
                uint64_t h = 0ULL;
                const auto r = std::from_chars(line.data() + 2U, line.data() + tab2, h);
                if (r.ec == std::errc{} && r.ptr == line.data() + tab2 && h != 0ULL && pkg_map.size() < 64U) {
                    pkg_map[h] = line.substr(tab2 + 1U);
                }
            }
        } else {
            rust_payload << line << '\n';
        }
    }
    const std::string payload = rust_payload.str();
    if (!payload.empty()) {
        (void)preloader.import_state(rust::Str(payload.data(), payload.size()));
    }
}

bool read_live_status_snapshot_str(pid_t expected_pid, std::string& out_payload) {
    if (expected_pid <= 0) return false;
    UniqueFd fd(::open("/data/adb/coreflow/status.json", O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return false;
    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode) || st.st_size <= 0 || st.st_size > 512 * 1024) {
        return false;
    }
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

bool print_live_status_snapshot(pid_t expected_pid) {
    std::string payload;
    if (!read_live_status_snapshot_str(expected_pid, payload)) return false;
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
                if (::write(fd.get(), pid_line.data(), pid_line.size()) > 0) {}
                break;
            }
        }
    }
    UniqueFd oom(::open("/proc/self/oom_score_adj", O_WRONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (oom.valid()) {
        constexpr char val[] = "-900\n";
        if (::write(oom.get(), val, sizeof(val) - 1U) > 0) {}
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
    if (::chdir("/") != 0) {}
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

struct ThermalZoneSnapshot {
    std::string id;
    std::string type;
    std::string category;
    double celsius{-1.0};
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
    double thermal_velocity_cpm{0.0};
    // Observe-only online RC chassis model (see rust/src/thermal_model.rs). Never actuates.
    engine::ThermalModelResult thermal_model{};
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
    v6::SemanticKnobStats semantic_stats{};
    std::vector<std::string> semantic_knob_leaves{};
    std::vector<std::string> contended_paths{};
    std::vector<std::string> oem_yielded_paths{};
    BatteryPowerTelemetry battery{};
    MemorySnapshot memory{};
    std::vector<uint64_t> cluster_cur_khz{};
    std::vector<ThermalZoneSnapshot> thermal_zones{};
    std::vector<v6::DecisionEpisode> episodes{};
    SessionMetrics session{};
    size_t baseline_nodes{0};
    bool recovery_pending{false};
    std::string last_predicted_pkg{};
    bool frame_pacing_available{false};
    double measured_fps{0.0};
    double jank_ratio{-1.0};
    size_t app_dna_learned{0};
    double active_app_bias{1.0};
    double active_app_efficiency_weight{1.0};
    uint64_t app_dna_flush_count{0};
    bool charging_damped{false};
    bool boot_settle_active{false};
    double boot_settle_duration_sec{0.0};
    std::string boot_settle_exit_reason{"warm_daemon_restart"};
    double boot_ramp_progress{1.0};
    // Pillar 1: Sovereign Tamper Guard
    size_t locked_nodes{0};
    size_t quarantined_nodes{0};
    uint64_t total_tamper_events{0};
    std::vector<TamperEvent> tamper_events{};
    std::string conflicting_modules_json{"[]"};
    // Pillar 2: Coupled Multi-Domain Orchestration
    uint8_t symbiotic_regime{0};
    double memory_bus_scale{1.0};
    double scheduler_responsiveness_scale{1.0};
    double thermal_stepdown_lead{0.0};
    uint8_t coupling_mask{0};
    // Hybrid CFW2 + ONNX Neural Engine telemetry
    double ai_jank_risk{0.0};
    std::string ai_workload_class{"balanced"};
    std::string ai_backend{"embedded-rls-mlp"};
    // Unified Zero-Profile Autonomous Engine telemetry
    bool low_battery_eco{false};
    std::string autonomous_regime{"idle_cool"};
    std::vector<v6::AppDnaEntry> app_dna_entries{};
    std::vector<HistoryPoint> history{};
};

bool compute_low_battery_eco(const BatteryPowerTelemetry& b) noexcept {
    return !b.charging && b.battery_capacity_pct > 0 && b.battery_capacity_pct <= 20;
}

// Chassis (battery/skin node) temperature the user wants sustained; used only to report the
// sustainable-power budget from the identified RC model. Observe-only until validated on device.
constexpr double kChassisTargetCelsius = 38.0;

std::string compute_autonomous_regime(const RuntimeStatusData& s) {
    if (s.config.inspection_mode) return "paused_oem";
    if (s.thermal_emergency) return "thermal_emergency";
    if (s.thermal_guard) return "thermal_guard";
    if (s.boot_settle_active) return "boot_settle";
    if (s.ctx.mode == ContextMode::Excluded) return "excluded_oem";
    if (s.ctx.mode == ContextMode::ScreenOff) return "screen_off";
    if (s.ctx.mode == ContextMode::Gaming) return "gaming_3d";
    if (s.ctx.mode == ContextMode::LaunchBurst) return "launch_burst";
    if (s.low_battery_eco) return "low_battery_eco";
    if ((std::isfinite(s.battery_temp) && s.battery_temp >= 38.5) ||
        s.capacity_scale < 0.88) {
        return "thermal_pacing";
    }
    if (s.ctx.mode == ContextMode::Interactive) return "interactive_scroll";
    return "idle_cool";
}

std::string serialize_app_dna_entries(const std::vector<v6::AppDnaEntry>& entries) {
    std::ostringstream out;
    out << '[';
    const size_t limit = std::min<size_t>(entries.size(), 48U);
    for (size_t i = 0; i < limit; ++i) {
        if (i != 0U) out << ',';
        const auto& e = entries[i];
        out << "{\"pkg\":" << json_str(e.package_name)
            << ",\"uclampBias\":" << safe_num(e.uclamp_bias, 1.0)
            << ",\"efficiencyWeight\":" << safe_num(e.efficiency_weight, 1.0)
            << ",\"samples\":" << e.sample_count
            << ",\"launches\":" << e.launch_count << '}';
    }
    out << ']';
    return out.str();
}

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

std::string serialize_tamper_event_json(const TamperEvent& ev) {
    std::ostringstream out;
    out << "{\"id\":" << ev.ts_ms
        << ",\"tsMs\":" << ev.ts_ms
        << ",\"nodePath\":" << json_str(ev.node_path)
        << ",\"domain\":" << json_str(CentralMutationCoordinator::domain_name(ev.domain))
        << ",\"expectedValue\":" << json_str(ev.expected_value)
        << ",\"observedValue\":" << json_str(ev.observed_value)
        << ",\"reason\":" << json_str(ev.reason)
        << ",\"reclaimed\":" << (ev.reclaimed ? "true" : "false")
        << ",\"circuitBreakerOpen\":" << (ev.circuit_open ? "true" : "false")
        << ",\"attemptsInWindow\":" << ev.attempts_in_window << '}';
    return out.str();
}

std::string serialize_tamper_events_array(const std::vector<TamperEvent>& events) {
    std::ostringstream out;
    out << '[';
    for (size_t i = 0; i < events.size(); ++i) {
        if (i != 0U) out << ',';
        out << serialize_tamper_event_json(events[i]);
    }
    out << ']';
    return out.str();
}

std::string read_conflicting_modules_json() {
    const char* env_dir = std::getenv("COREFLOW_STATE_DIR");
    const std::string state_dir = (env_dir && env_dir[0] != '\0') ? std::string(env_dir) : std::string(kStateDir);
    const std::string path = state_dir + "/detected_conflicts.json";
    UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return "[]";
    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode) || st.st_size <= 0 || st.st_size > 16384) {
        return "[]";
    }
    std::string raw(static_cast<size_t>(st.st_size), '\0');
    size_t off = 0;
    while (off < raw.size()) {
        const ssize_t n = ::read(fd.get(), raw.data() + off, raw.size() - off);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        off += static_cast<size_t>(n);
    }
    raw.resize(off);
    const size_t key = raw.find("\"modules\":[");
    if (key == std::string::npos) return "[]";
    const size_t start = raw.find('[', key);
    const size_t end = raw.find(']', start);
    if (start == std::string::npos || end == std::string::npos || end < start) return "[]";
    return raw.substr(start, end - start + 1U);
}

void dispatch_tamper_broadcast(const TamperEvent& ev) noexcept {
    // Only dispatch IPC broadcast on Android environments with /system/bin/am present.
    if (::access("/system/bin/am", X_OK) != 0) return;
    const std::string payload = serialize_tamper_event_json(ev);
    constexpr const char* kPackages[] = {"io.coreflow.companion", "io.coreflow.companion.debug"};
    for (const char* pkg : kPackages) {
        const pid_t child = ::fork();
        if (child < 0) continue;
        if (child == 0) {
            // Double-fork so coreflowd never blocks or leaks zombie processes.
            const pid_t grand = ::fork();
            if (grand == 0) {
                const int devnull = ::open("/dev/null", O_RDWR | O_CLOEXEC);
                if (devnull >= 0) {
                    (void)::dup2(devnull, STDIN_FILENO);
                    (void)::dup2(devnull, STDOUT_FILENO);
                    (void)::dup2(devnull, STDERR_FILENO);
                    if (devnull > STDERR_FILENO) (void)::close(devnull);
                }
                ::execl("/system/bin/am", "am", "broadcast",
                        "-a", "io.coreflow.ACTION_NODE_TAMPER",
                        "-p", pkg,
                        "--es", "payload", payload.c_str(),
                        static_cast<char*>(nullptr));
                ::_exit(127);
            }
            ::_exit(0);
        }
        int status = 0;
        (void)::waitpid(child, &status, 0);
    }
}

const char* thermal_category_name(int cat) noexcept {
    switch (cat) {
        case 1: return "compute";
        case 2: return "battery";
        case 3: return "skin";
        case 4: return "board";
        default: return "sensor";
    }
}

std::vector<ThermalZoneSnapshot> collect_thermal_zones(const coreflow::ThermalBundle& tb) {
    std::vector<ThermalZoneSnapshot> out;
    for (const auto& s : tb.assessment.readings) {
        if (!s.valid || !std::isfinite(s.celsius) || s.celsius <= 0.0f || s.celsius > 125.0f) continue;
        ThermalZoneSnapshot z{};
        z.id = s.id;
        z.type = s.type;
        z.category = thermal_category_name(s.category);
        z.celsius = static_cast<double>(s.celsius);
        out.push_back(std::move(z));
        if (out.size() >= 10U) break;
    }
    return out;
}

std::string serialize_episodes_array(const std::vector<v6::DecisionEpisode>& episodes) {
    std::ostringstream out;
    out << '[';
    for (size_t i = 0; i < episodes.size(); ++i) {
        if (i != 0U) out << ',';
        const auto& ep = episodes[i];
        out << "{\"id\":" << ep.id
            << ",\"tsMs\":" << ep.ts_ms
            << ",\"domain\":" << json_str(ep.domain)
            << ",\"trigger\":" << json_str(ep.trigger)
            << ",\"action\":" << json_str(ep.action)
            << ",\"outcome\":" << json_str(ep.outcome)
            << ",\"socTempC\":" << (std::isfinite(ep.soc_temp_c) && ep.soc_temp_c >= 0.0 ? std::to_string(safe_num(ep.soc_temp_c)) : "null")
            << ",\"renderPressure\":" << safe_num(ep.render_pressure)
            << ",\"capacityScale\":" << safe_num(ep.capacity_scale, 1.0) << '}';
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
        << ",\"autonomousRegime\":" << json_str(s.autonomous_regime.empty() ? compute_autonomous_regime(s) : s.autonomous_regime)
        << ",\"lowBatteryEco\":" << (s.low_battery_eco ? "true" : "false")
        << ",\"inspectionMode\":" << (s.config.inspection_mode ? "true" : "false")
        << ",\"mode\":" << json_str(s.thermal_emergency ? "THERMAL_EMERGENCY"
                                    : s.thermal_guard ? "THERMAL_GUARD"
                                    : s.boot_settle_active ? "BOOT_SETTLE"
                                    : ContextEngine::mode_name(s.ctx.mode))
        << ",\"bootSettleActive\":" << (s.boot_settle_active ? "true" : "false")
        << ",\"bootSettleDurationSec\":" << safe_num(s.boot_settle_duration_sec)
        << ",\"bootSettleExitReason\":" << json_str(s.boot_settle_exit_reason)
        << ",\"bootRampProgress\":" << safe_num(s.boot_ramp_progress, 1.0)
        << ",\"foregroundPackage\":" << json_str(s.ctx.foreground_package)
        << ",\"foregroundPid\":" << s.ctx.foreground_pid
        << ",\"capacityScale\":" << safe_num(s.capacity_scale, 1.0)
        << ",\"thermalGuard\":" << (s.thermal_guard ? "true" : "false")
        << ",\"socTempCelsius\":" << (std::isfinite(s.soc_temp) && s.soc_temp >= 0.0 ? std::to_string(s.soc_temp) : "null")
        << ",\"batteryTempCelsius\":" << (std::isfinite(s.battery_temp) && s.battery_temp >= 0.0 ? std::to_string(s.battery_temp) : "null")
        << ",\"thermalPressure\":" << safe_num(s.thermal_pressure)
        << ",\"thermalVelocityCpm\":" << safe_num(s.thermal_velocity_cpm)
        << ",\"thermalModel\":{"
        << "\"valid\":" << (s.thermal_model.valid ? "true" : "false")
        << ",\"confidence\":" << s.thermal_model.confidence
        << ",\"steps\":" << s.thermal_model.steps
        << ",\"targetCelsius\":" << safe_num(kChassisTargetCelsius)
        << ",\"rThetaCPerW\":" << (s.thermal_model.valid ? std::to_string(safe_num(s.thermal_model.r_theta_c_per_w)) : "null")
        << ",\"tauSec\":" << (s.thermal_model.valid ? std::to_string(safe_num(s.thermal_model.tau_s)) : "null")
        << ",\"ambientCelsius\":" << (s.thermal_model.valid ? std::to_string(safe_num(s.thermal_model.ambient_c)) : "null")
        << ",\"sustainablePowerMw\":" << (s.thermal_model.valid ? std::to_string(safe_num(s.thermal_model.sustainable_w * 1000.0)) : "null")
        << ",\"avgPowerMw\":" << safe_num(s.thermal_model.avg_power_w * 1000.0)
        << ",\"headroomMw\":" << (s.thermal_model.valid ? std::to_string(safe_num(s.thermal_model.headroom_w * 1000.0)) : "null")
        << "}"
        << ",\"thermalSource\":" << json_str(s.thermal_source)
        << ",\"cpuUtilPct\":" << safe_num(s.cpu_util_pct)
        << ",\"renderPressure\":" << safe_num(s.render_pressure)
        << ",\"gpuBusyPct\":" << (s.gpu_busy_pct ? std::to_string(safe_num(*s.gpu_busy_pct)) : "null")
        << ",\"cpuPsi\":" << (s.cpu_psi ? std::to_string(safe_num(*s.cpu_psi)) : "null")
        << ",\"memPsi\":" << (s.mem_psi ? std::to_string(safe_num(*s.mem_psi)) : "null")
        << ",\"ioPsi\":" << (s.io_psi ? std::to_string(safe_num(*s.io_psi)) : "null")
        << ",\"aiPredicted10s\":" << safe_num(s.ai_predicted_10s)
        << ",\"aiConfidence\":" << safe_num(s.ai_confidence)
        << ",\"aiJankRisk\":" << safe_num(s.ai_jank_risk)
        << ",\"aiWorkloadClass\":" << json_str(s.ai_workload_class)
        << ",\"aiBackend\":" << json_str(s.ai_backend)
        << ",\"baselineNodesSnapshotted\":" << s.baseline_nodes
        << ",\"recoveryPending\":" << (s.recovery_pending ? "true" : "false")
        << ",\"features\":{"
        << "\"frameSmoothness\":" << (s.config.enable_frame_smoothness ? "true" : "false")
        << ",\"storageBurst\":" << (s.config.enable_storage_burst ? "true" : "false")
        << ",\"memoryGuard\":" << (s.config.enable_memory_guard ? "true" : "false")
        << ",\"thermalPacing\":" << (s.config.enable_thermal_pacing ? "true" : "false")
        << ",\"batteryStepCharging\":" << (s.config.enable_battery_step_charging ? "true" : "false")
        << ",\"idleCompaction\":" << (s.config.enable_idle_compaction ? "true" : "false")
        << ",\"predictivePreload\":" << (s.config.enable_predictive_preload ? "true" : "false")
        << ",\"cameraBoost\":" << (s.config.enable_camera_boost ? "true" : "false")
        << "}"
        << ",\"hyperFluid\":{"
        << "\"idleCompactions\":" << s.storage_stats.idle_compactions_total
        << ",\"predictivePreloads\":" << s.storage_stats.predictive_preloads_total
        << ",\"lastPredictedPackage\":" << json_str(s.last_predicted_pkg)
        << ",\"cameraChainActive\":" << (s.ctx.camera_boost_active ? "true" : "false")
        << ",\"mediaPipelineActive\":" << (s.ctx.media_pipeline_active ? "true" : "false")
        << ",\"memoryHealthPct\":" << s.storage_stats.memory_health_pct
        << ",\"framePacingAvailable\":" << (s.frame_pacing_available ? "true" : "false")
        << ",\"measuredFps\":" << safe_num(s.measured_fps)
        << ",\"jankRatio\":" << (s.jank_ratio >= 0.0 ? std::to_string(safe_num(s.jank_ratio)) : "null")
        << ",\"appDnaLearned\":" << s.app_dna_learned
        << ",\"activeAppBias\":" << safe_num(s.active_app_bias, 1.0)
        << ",\"activeAppEfficiencyWeight\":" << safe_num(s.active_app_efficiency_weight, 1.0)
        << ",\"appDnaFlushCount\":" << s.app_dna_flush_count
        << ",\"chargingDamped\":" << (s.charging_damped ? "true" : "false")
        << "}"
        << ",\"powerConfig\":{"
        << "\"bypassCharging\":" << (s.config.bypass_charging ? "true" : "false")
        << ",\"bypassOnGaming\":" << (s.config.bypass_on_gaming ? "true" : "false")
        << ",\"chargeLimitPct\":" << s.config.charge_limit_pct
        << "}"
        << ",\"apps\":{"
        << "\"gaming\":" << serialize_string_array(gaming_list)
        << ",\"smoothness\":" << serialize_string_array(smoothness_list)
        << ",\"battery\":" << serialize_string_array(battery_list)
        << ",\"excluded\":" << serialize_string_array(excluded_list)
        << "}"
        << ",\"appDnaEntries\":" << serialize_app_dna_entries(s.app_dna_entries)
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
        << ",\"easTopAppCapPct\":" << s.envelope_stats.eas_top_app_cap_pct
        << ",\"easCappedGroups\":" << s.envelope_stats.eas_capped_groups
        << ",\"semanticKnobsDiscovered\":" << s.semantic_stats.discovered_total
        << ",\"semanticKnobsActive\":" << s.semantic_stats.active_managed
        << ",\"memoryBusKnobs\":" << s.semantic_stats.memory_bus_knobs
        << ",\"semanticKnobLeaves\":" << serialize_string_array(s.semantic_knob_leaves)
        << ",\"contendedNodes\":" << s.contended_paths.size()
        << ",\"contendedPaths\":" << serialize_string_array(s.contended_paths)
        << ",\"oemYieldedNodes\":" << s.oem_yielded_paths.size()
        << ",\"oemYieldedPaths\":" << serialize_string_array(s.oem_yielded_paths)
        << "}"
        << ",\"tamperGuard\":{"
        << "\"enabled\":true"
        << ",\"lockedNodes\":" << s.locked_nodes
        << ",\"quarantinedNodes\":" << s.quarantined_nodes
        << ",\"totalEvents\":" << s.total_tamper_events
        << "}"
        << ",\"tamperEvents\":" << serialize_tamper_events_array(s.tamper_events)
        << ",\"coupledOrchestration\":{"
        << "\"regime\":" << static_cast<unsigned>(s.symbiotic_regime)
        << ",\"memoryBusScale\":" << safe_num(s.memory_bus_scale, 1.0)
        << ",\"schedulerResponsivenessScale\":" << safe_num(s.scheduler_responsiveness_scale, 1.0)
        << ",\"thermalStepdownLead\":" << safe_num(s.thermal_stepdown_lead, 0.0)
        << ",\"couplingMask\":" << static_cast<unsigned>(s.coupling_mask)
        << ",\"memoryBusNodes\":" << s.topo.memory_bus_nodes.size()
        << "}"
        << ",\"conflictingModules\":" << (s.conflicting_modules_json.empty() ? "[]" : s.conflicting_modules_json)
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
        << ",\"memory\":{"
        << "\"totalBytes\":" << s.memory.total_bytes
        << ",\"availableBytes\":" << s.memory.available_bytes
        << ",\"swapTotalBytes\":" << s.memory.swap_total_bytes
        << ",\"swapFreeBytes\":" << s.memory.swap_free_bytes
        << "}"
        << ",\"cpuClusters\":[";
    for (size_t i = 0; i < s.topo.cpu_clusters.size(); ++i) {
        if (i != 0U) out << ',';
        const auto& c = s.topo.cpu_clusters[i];
        const uint64_t cur_khz = (i < s.cluster_cur_khz.size()) ? s.cluster_cur_khz[i] : 0U;
        const uint64_t min_khz = c.opps_khz.empty() ? 0U : c.opps_khz.front();
        const uint64_t max_khz = c.opps_khz.empty() ? 0U : c.opps_khz.back();
        out << "{\"role\":" << json_str(HardwareProbe::cluster_role_name(c.role))
            << ",\"governor\":" << json_str(c.governor)
            << ",\"curKhz\":" << cur_khz
            << ",\"minKhz\":" << min_khz
            << ",\"maxKhz\":" << max_khz
            << ",\"capacity\":" << c.capacity
            << ",\"availableGovernors\":" << serialize_string_array(c.available_governors) << '}';
    }
    out << "]"
        << ",\"thermalZones\":[";
    for (size_t i = 0; i < s.thermal_zones.size(); ++i) {
        if (i != 0U) out << ',';
        const auto& z = s.thermal_zones[i];
        out << "{\"id\":" << json_str(z.id)
            << ",\"type\":" << json_str(z.type)
            << ",\"category\":" << json_str(z.category)
            << ",\"celsius\":" << safe_num(z.celsius) << '}';
    }
    out << "]"
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
        << ",\"capacityPct\":" << (s.battery.battery_capacity_pct >= 0 ? std::to_string(s.battery.battery_capacity_pct) : "null")
        << ",\"bypassSupported\":" << (s.battery.bypass_supported ? "true" : "false")
        << ",\"bypassActive\":" << (s.battery.bypass_active ? "true" : "false")
        << ",\"bypassReason\":" << json_str(s.battery.bypass_reason)
        << ",\"chargeLimitPct\":" << s.battery.charge_limit_pct
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
        << ",\"episodes\":" << serialize_episodes_array(s.episodes)
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
    data.memory = probe.read_memory_snapshot();
    for (const auto& c : data.topo.cpu_clusters) {
        data.cluster_cur_khz.push_back(probe.read_cluster_cur_freq_khz(c).value_or(0U));
    }
    const auto tb = thermal_mon.sample_bundle(false);
    data.soc_temp = tb.composite_celsius;
    data.battery_temp = tb.battery_celsius;
    data.thermal_pressure = tb.assessment.pressure;
    data.thermal_source = tb.assessment.source;
    data.thermal_zones = collect_thermal_zones(tb);
    data.gpu_busy_pct = probe.read_gpu_busy_pct(data.topo);
    data.cpu_psi = probe.read_psi_avg10("cpu");
    data.mem_psi = probe.read_psi_avg10("memory");
    data.io_psi = probe.read_psi_avg10("io");
    data.battery = HardwareLongevityManager::probe_battery_telemetry();
    data.low_battery_eco = compute_low_battery_eco(data.battery);
    data.autonomous_regime = compute_autonomous_regime(data);
    data.baseline_nodes = count_journal_entries();
    data.recovery_pending = !journal_clear();
    data.conflicting_modules_json = read_conflicting_modules_json();
    return build_status_json(data);
}

void print_readonly_status() {
    std::cout << build_readonly_status_json() << "\n" << std::flush;
}

std::string build_atlas_json() {
    HardwareProbe probe;
    ThermalMonitor thermal_mon;
    const auto topo = probe.discover();
    const auto tb = thermal_mon.sample_bundle(false);
    const auto zones = collect_thermal_zones(tb);
    const auto batt = HardwareLongevityManager::probe_battery_telemetry();
    ThreadSchedulerActuator probe_sched;
    const bool per_thread_uclamp = probe_sched.probe_support();

    std::ostringstream out;
    out << "{\"atlasFormat\":\"coreflow-atlas-v6\""
        << ",\"version\":" << json_str(kVersion)
        << ",\"socModel\":" << json_str(topo.soc_model)
        << ",\"kernelVersion\":" << json_str(topo.kernel_version)
        << ",\"refreshHz\":" << topo.refresh_hz
        << ",\"ramBytes\":" << topo.total_ram_bytes
        << ",\"zramBytes\":" << topo.zram_bytes
        << ",\"perThreadUclampSupported\":" << (per_thread_uclamp ? "true" : "false")
        << ",\"cgroupV2Available\":" << (!topo.cgroups.top_app_procs_path.empty() ? "true" : "false")
        << ",\"mglruAvailable\":" << (!topo.vm.mglru_min_ttl_path.empty() ? "true" : "false")
        << ",\"oledDisplayPowerNodes\":" << (topo.drm_dpms_paths.size() + topo.fb_blank_paths.size())
        << ",\"gpu\":{\"available\":" << (topo.gpu.available ? "true" : "false")
        << ",\"model\":" << json_str(topo.gpu.model)
        << ",\"isKgsl\":" << (topo.gpu.is_kgsl ? "true" : "false")
        << ",\"isMtkGed\":" << (topo.gpu.is_mtk_ged ? "true" : "false")
        << ",\"hasBusySensor\":" << (!topo.gpu.busy_path.empty() ? "true" : "false")
        << ",\"oppCount\":" << (topo.gpu.is_kgsl ? topo.gpu.kgsl_num_pwrlevels : topo.gpu.devfreq_opps_hz.size())
        << "}"
        << ",\"powerControl\":{\"available\":" << (batt.available ? "true" : "false")
        << ",\"bypassSupported\":" << (batt.bypass_supported ? "true" : "false")
        << ",\"bypassNode\":" << json_str(batt.bypass_node_path)
        << ",\"capacitySupported\":" << (batt.battery_capacity_pct >= 0 ? "true" : "false")
        << ",\"stepChargingSupported\":" << (batt.charge_limit_ua > 0U ? "true" : "false")
        << "}"
        << ",\"cpuClusters\":[";
    for (size_t i = 0; i < topo.cpu_clusters.size(); ++i) {
        if (i != 0U) out << ',';
        const auto& c = topo.cpu_clusters[i];
        const uint64_t cur_khz = probe.read_cluster_cur_freq_khz(c).value_or(0U);
        out << "{\"role\":" << json_str(HardwareProbe::cluster_role_name(c.role))
            << ",\"governor\":" << json_str(c.governor)
            << ",\"curKhz\":" << cur_khz
            << ",\"minKhz\":" << (c.opps_khz.empty() ? 0U : c.opps_khz.front())
            << ",\"maxKhz\":" << (c.opps_khz.empty() ? 0U : c.opps_khz.back())
            << ",\"capacity\":" << c.capacity
            << ",\"availableGovernors\":" << serialize_string_array(c.available_governors) << '}';
    }
    out << "],\"storageQueues\":[";
    for (size_t i = 0; i < topo.storage_queues.size(); ++i) {
        if (i != 0U) out << ',';
        const auto& q = topo.storage_queues[i];
        out << "{\"device\":" << json_str(q.device_name)
            << ",\"type\":" << json_str(HardwareProbe::storage_type_name(q.type))
            << ",\"baselineReadAheadKb\":" << q.baseline_read_ahead_kb
            << ",\"nrRequests\":" << q.nr_requests << '}';
    }
    out << "],\"thermalZones\":[";
    for (size_t i = 0; i < zones.size(); ++i) {
        if (i != 0U) out << ',';
        const auto& z = zones[i];
        out << "{\"id\":" << json_str(z.id)
            << ",\"type\":" << json_str(z.type)
            << ",\"category\":" << json_str(z.category)
            << ",\"celsius\":" << safe_num(z.celsius) << '}';
    }
    out << "]}";
    return out.str();
}

void print_atlas() {
    std::cout << build_atlas_json() << "\n" << std::flush;
}

std::string build_capabilities_json() {
    HardwareProbe probe;
    const auto topo = probe.discover();
    const auto batt = HardwareLongevityManager::probe_battery_telemetry();
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
        << ",\"bypassChargingSupported\":" << (batt.bypass_supported ? "true" : "false")
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

void print_episodes() {
    std::string status_json;
    const pid_t live = daemon_pid();
    if (live > 0 && read_live_status_snapshot_str(live, status_json)) {
        const size_t pos = status_json.find("\"episodes\":[");
        if (pos != std::string::npos) {
            const size_t arr_start = status_json.find('[', pos);
            int depth = 0;
            bool in_str = false;
            bool esc = false;
            size_t arr_end = std::string::npos;
            for (size_t i = arr_start; i < status_json.size(); ++i) {
                const char c = status_json[i];
                if (in_str) {
                    if (esc) { esc = false; }
                    else if (c == '\\') { esc = true; }
                    else if (c == '"') { in_str = false; }
                    continue;
                }
                if (c == '"') { in_str = true; }
                else if (c == '[') { ++depth; }
                else if (c == ']') {
                    --depth;
                    if (depth == 0) { arr_end = i; break; }
                }
            }
            if (arr_end != std::string::npos) {
                std::cout << "{\"version\":" << json_str(kVersion)
                          << ",\"running\":true,\"episodes\":"
                          << status_json.substr(arr_start, arr_end - arr_start + 1U)
                          << "}\n" << std::flush;
                return;
            }
        }
    }
    std::cout << "{\"version\":" << json_str(kVersion) << ",\"running\":false,\"episodes\":[]}\n" << std::flush;
}

void print_diag() {
    std::string status_json;
    const pid_t live = daemon_pid();
    if (!(live > 0 && read_live_status_snapshot_str(live, status_json))) {
        status_json = build_readonly_status_json();
    }
    const std::string caps_json = build_capabilities_json();
    const std::string atlas_json = build_atlas_json();
    const std::string journal_json = build_journal_json();
#ifndef COREFLOW_BUILD_COMMIT
#define COREFLOW_BUILD_COMMIT "unknown"
#endif
    std::ostringstream out;
    out << "{\"reportFormat\":\"coreflow-diag-v6\""
        << ",\"version\":" << json_str(kVersion)
        << ",\"commit\":" << json_str(COREFLOW_BUILD_COMMIT)
        << ",\"generatedAtMs\":" << epoch_now_ms()
        << ",\"safetyMarkers\":{"
        << "\"safeMode\":" << (marker_exists("/data/adb/coreflow/SAFE_MODE") ? "true" : "false")
        << ",\"disable\":" << (marker_exists("/data/adb/coreflow/DISABLE") ? "true" : "false")
        << "}"
        << ",\"capabilities\":" << caps_json
        << ",\"atlas\":" << atlas_json
        << ",\"status\":" << status_json
        << ",\"journal\":" << journal_json
        << "}";
    std::cout << out.str() << "\n" << std::flush;
}

int handle_resolve_tamper_cli(std::string_view node_path, std::string_view code) {
    if (node_path.empty() || node_path.size() > 512U) {
        std::cerr << "Invalid node path for --resolve-tamper.\n";
        return 2;
    }
    for (unsigned char c : node_path) {
        if (c < 0x20 || c >= 0x7F || c == '\t' || c == '\n' || c == '\r') {
            std::cerr << "Invalid character in node path.\n";
            return 2;
        }
    }
    std::string upper_code(code);
    std::transform(upper_code.begin(), upper_code.end(), upper_code.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    if (upper_code != "RESYNC" && upper_code != "ENFORCE") {
        std::cerr << "Unsupported tamper action code (expected RESYNC or ENFORCE): " << code << "\n";
        return 2;
    }
    if (!ensure_secure_state_dir()) {
        std::cerr << "Unable to prepare CFAE state directory.\n";
        return 1;
    }
    const char* env_dir = std::getenv("COREFLOW_STATE_DIR");
    const std::string state_dir = (env_dir && env_dir[0] != '\0') ? std::string(env_dir) : std::string(kStateDir);
    UniqueFd dir(::open(state_dir.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    if (!dir.valid()) return 1;
    const std::string tmp = "tamper_cmd.req.tmp." + std::to_string(::getpid());
    UniqueFd fd(::openat(dir.get(), tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW, 0600));
    if (!fd.valid()) return 1;
    const std::string payload = upper_code + "\t" + std::string(node_path) + "\n";
    size_t off = 0;
    while (off < payload.size()) {
        const ssize_t n = ::write(fd.get(), payload.data() + off, payload.size() - off);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) {
            (void)::unlinkat(dir.get(), tmp.c_str(), 0);
            return 1;
        }
        off += static_cast<size_t>(n);
    }
    (void)::fsync(fd.get());
    if (::renameat(dir.get(), tmp.c_str(), dir.get(), "tamper_cmd.req") != 0) {
        (void)::unlinkat(dir.get(), tmp.c_str(), 0);
        return 1;
    }
    const pid_t pid = daemon_pid();
    bool signaled = false;
    if (pid > 0) {
        signaled = (::kill(pid, SIGUSR1) == 0);
    }
    std::cout << "{\"ok\":true,\"action\":" << json_str(upper_code)
              << ",\"node\":" << json_str(node_path)
              << ",\"daemonSignaled\":" << (signaled ? "true" : "false") << "}\n" << std::flush;
    return 0;
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
    if (argc >= 2 && std::string_view(argv[1]) == "--resolve-tamper") {
        if (argc != 4) {
            std::cerr << "Usage: coreflowd --resolve-tamper <node_path> <RESYNC|ENFORCE>\n";
            return 2;
        }
        return handle_resolve_tamper_cli(argv[2], argv[3]);
    }
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
        if (cmd == "--atlas") {
            print_atlas();
            return 0;
        }
        if (cmd == "--episodes") {
            print_episodes();
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
        if (cmd == "--recover-only") {
            if (daemon_pid() > 0) return 0;
            return BaselineStore::instance().recover_pending() ? 0 : 1;
        }
        if (cmd == "--stop" || cmd == "--rollback-baseline") {
            return stop_daemon();
        }
        if (cmd == "--daemon" || cmd == "-d") {
            fork_daemon = true;
        } else {
            std::cerr << "Usage: coreflowd [--daemon|--status|--capabilities|--atlas|--episodes|--journal|--diag|--recover-only|--stop|--rollback-baseline|--set-config key=value ...|--resolve-tamper <node> <RESYNC|ENFORCE>]\n";
            return 2;
        }
    } else if (argc != 1) {
        std::cerr << "Usage: coreflowd [--daemon|--status|--capabilities|--atlas|--episodes|--journal|--diag|--recover-only|--stop|--rollback-baseline|--set-config key=value ...|--resolve-tamper <node> <RESYNC|ENFORCE>]\n";
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

    // CFAE System Framework / CoreFlow Engine v6.0.0-R Runtime Composition
    DaemonConfig config = DaemonConfig::load_best_available();
    HardwareProbe probe;
    HardwareTopology topo = probe.discover();
    ThermalMonitor thermal_mon;
    NeuralPredictor neural("/data/adb/coreflow/models");
    CentralMutationCoordinator coordinator;
    HardwareLongevityManager longevity(coordinator);

    ContextEngine context_engine;
    ThreadSchedulerActuator thread_scheduler;
    (void)thread_scheduler.probe_support();
    v6::OemCollaborationMap oem_map;
    oem_map.initialize(kStateDir, topo.kernel_version,
                       v6::OemCollaborationMap::discover_rom_fingerprint());
    StorageMemoryActuator storage_memory(baseline, "/proc", &oem_map);
    ThermalEnvelopeActuator thermal_envelope(baseline, &oem_map);
    v6::SemanticKnobEngine semantic_knobs(baseline, oem_map, &coordinator);
    semantic_knobs.discover(topo);
    v6::FramePacingSensor frame_pacing;
    frame_pacing.discover("/sys", "/proc", topo.vblank_irq_token);
    AndroidFrameEventBridge frame_bridge;
    v6::AppDnaStore app_dna;
    app_dna.initialize(kStateDir);

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
    v6::DecisionJournal decision_journal{};
    live_status.running = true;
    live_status.pid = ::getpid();
    live_status.config = config;
    live_status.ctx.effective_profile = context_engine.profile();
    live_status.topo = topo;
    live_status.thread_stats = thread_scheduler.stats();
    live_status.battery = longevity.sample_battery_telemetry();
    live_status.low_battery_eco = compute_low_battery_eco(live_status.battery);
    live_status.frame_pacing_available = frame_pacing.supported();
    live_status.app_dna_learned = app_dna.learned_count();
    live_status.app_dna_flush_count = app_dna.flush_count();
    live_status.app_dna_entries = app_dna.snapshot_entries();
    live_status.ai_backend = std::string(neural.last_prediction().backend);
    live_status.conflicting_modules_json = read_conflicting_modules_json();
    live_status.session.started_steady_ms = steady_now_ms();

    // Pillar 1: Sovereign Tamper Guard event listener (DecisionJournal + circuit-breaker IPC broadcast)
    // One-off writes from OEM system daemons (e.g. perf-hal during app launch) are logged quietly in
    // DecisionJournal without spamming Android notifications; IPC alerts fire only on repeated
    // write-loop contention (circuit breaker open).
    std::unordered_map<std::string, uint64_t> last_tamper_ipc_ms;
    coordinator.set_tamper_listener([&](const TamperEvent& ev) {
        const auto slash = ev.node_path.find_last_of('/');
        const std::string leaf = slash != std::string::npos ? ev.node_path.substr(slash + 1U) : ev.node_path;
        decision_journal.record(
            epoch_now_ms(),
            "sovereign",
            ev.circuit_open ? "TamperCircuitOpen" : "TamperBlocked",
            "Perubahan eksternal pada " + leaf + " (nilai=" + ev.observed_value +
                ", target=" + ev.expected_value + ")" +
                (ev.circuit_open ? " - Circuit breaker 30d aktif" : " - Dicatat & dikembalikan"),
            ev.circuit_open ? "quarantined" : (ev.reclaimed ? "reclaimed" : "failed"),
            live_status.soc_temp, live_status.render_pressure, live_status.capacity_scale);
        emit_log(true, "[CFAE][SovereignGuard] External write on " + ev.node_path +
                 " (observed=" + ev.observed_value + ", enforced=" + ev.expected_value +
                 ", reclaimed=" + (ev.reclaimed ? "yes" : "no") +
                 ", breaker=" + (ev.circuit_open ? "OPEN" : "closed") + ")");
        if (!ev.circuit_open) {
            return;
        }
        const uint64_t now_ipc = steady_now_ms();
        const auto it = last_tamper_ipc_ms.find(ev.node_path);
        if (it == last_tamper_ipc_ms.end() || now_ipc >= it->second + 30'000ULL) {
            last_tamper_ipc_ms[ev.node_path] = now_ipc;
            dispatch_tamper_broadcast(ev);
        }
    });

    v6::BootCalmSanctuary boot_calm;
    const double init_uptime_sec = v6::BootCalmSanctuary::read_kernel_uptime_sec("/proc");
    boot_calm.initialize(init_uptime_sec, live_status.session.started_steady_ms);
    bool last_screen_off = probe.is_screen_off(topo);
    v6::BootCalmDecision boot_dec = boot_calm.evaluate({
        .kernel_uptime_sec = init_uptime_sec,
        .now_ms = live_status.session.started_steady_ms,
        .screen_off = last_screen_off,
        .thermal_guard = false,
        .cpu_util_pct = 0.0,
        .io_psi_pct = 0.0,
        .soc_temp_c = -1.0,
        .thermal_velocity_cpm = 0.0,
        .touch_active = false,
        .real_foreground_app = false,
    });
    live_status.boot_settle_active = boot_dec.boot_settle_active;
    live_status.boot_settle_duration_sec = boot_dec.settle_duration_sec;
    live_status.boot_settle_exit_reason = boot_dec.exit_reason;
    live_status.boot_ramp_progress = boot_dec.ramp_alpha;
    live_status.autonomous_regime = compute_autonomous_regime(live_status);
    bool prev_bypass_active = false;
    size_t prev_contended_count = 0U;
    ForegroundAppInfo last_fg{};
    MemorySnapshot last_mem_snap = probe.read_memory_snapshot();
    live_status.memory = last_mem_snap;
    decision_journal.record(
        epoch_now_ms(), "context", "Boot",
        boot_dec.boot_settle_active
            ? "CFAE System Framework v6.1.0-R initialized in BOOT_SETTLE sanctuary (Zero-Profile Autonomous)"
            : "CFAE System Framework v6.1.0-R initialized (Zero-Profile Autonomous)",
        "verified", -1.0, 0.0, 1.0);
    live_status.episodes = decision_journal.recent(16U);
    uint64_t last_sysfs_active_ms = 0U;
    uint64_t last_state_persist_ms = 0U;
    uint64_t last_jank_episode_ms = 0U;
    (void)frame_bridge.start([&](const FrameEvent& ev) {
        if (ev.kind != FrameEventKind::FrameTimelineWindow || ev.frame_count == 0U || ev.missed_frames == 0U) {
            return;
        }
        if (ev.missed_frames * 100ULL >= ev.frame_count * 8ULL) {
            const uint64_t now_ms = steady_now_ms();
            if (last_jank_episode_ms == 0U || now_ms >= last_jank_episode_ms + 15'000ULL) {
                last_jank_episode_ms = now_ms;
                const uint64_t pct = (ev.missed_frames * 100ULL) / ev.frame_count;
                decision_journal.record(
                    epoch_now_ms(), "uclamp", "FrameJankRescue",
                    "Detected " + std::to_string(pct) + "% frame drop (" +
                        (frame_pacing.active_source().empty() ? std::string("display") : frame_pacing.active_source()) +
                        "); elevating per-TID floor",
                    "verified", live_status.soc_temp, live_status.render_pressure, live_status.capacity_scale);
            }
        }
    });
    auto last_thermal_tick = std::chrono::steady_clock::now();

    const auto build_sysfs_ctx = [&](const ContextDecision& raw, uint64_t now_ms, const ForegroundAppInfo& fg) noexcept {
        ContextDecision out = raw;
        // Only prefetch assets for real visible UI apps (never system launchers already resident in RAM).
        if (fg.critical_tids.size() <= 1U || ContextEngine::is_system_launcher_package(fg.package_name)) {
            out.newly_launched_app = false;
        }
        // Anti-flap hysteresis for disk-journaled sysfs actuators: hold Interactive for 3s
        // before relaxing to Efficiency so typing/scrolling does not thrash CFJ4 nodes.
        if (raw.mode == ContextMode::Interactive || raw.mode == ContextMode::LaunchBurst ||
            raw.mode == ContextMode::Gaming) {
            last_sysfs_active_ms = now_ms;
        } else if (raw.mode == ContextMode::Efficiency && last_sysfs_active_ms > 0U &&
                   now_ms >= last_sysfs_active_ms && now_ms - last_sysfs_active_ms < 3000ULL) {
            out.mode = ContextMode::Interactive;
        }
        return out;
    };

    auto thermal_trend = engine::new_thermal_trend_analyzer();
    auto thermal_model = engine::new_thermal_model_estimator();
    double thermal_trend_ewma_c = -1.0;
    auto preloader = engine::new_app_preloader();
    std::unordered_map<uint64_t, std::string> preloader_pkg_map;
    load_preloader_state(*preloader, preloader_pkg_map);
    bool preloader_dirty = false;
    const auto hash_pkg_fnv1a = [](std::string_view pkg) noexcept -> uint64_t {
        if (pkg.empty()) return 0ULL;
        uint64_t h = 14695981039346656037ULL;
        for (const unsigned char c : pkg) {
            h ^= static_cast<uint64_t>(c);
            h *= 1099511628211ULL;
        }
        return h == 0ULL ? 1ULL : h;
    };
    const auto record_preloader_launch = [&](const std::string& pkg) {
        if (pkg.empty()) return;
        const uint64_t h = hash_pkg_fnv1a(pkg);
        if (preloader_pkg_map.size() >= 64U && !preloader_pkg_map.contains(h)) {
            preloader_pkg_map.erase(preloader_pkg_map.begin());
        }
        preloader_pkg_map[h] = pkg;
        preloader->record_launch(h);
        preloader_dirty = true;
    };
    const auto build_fg_chain_tids = [&](const ForegroundAppInfo& fg, bool camera_active) {
        auto tids = fg.critical_tids;
        if (camera_active && config.enable_camera_boost) {
            const auto cam_tids = context_engine.resolve_camera_hal_tids();
            tids.insert(tids.end(), cam_tids.begin(), cam_tids.end());
        }
        return tids;
    };

    auto to_dna_signature = [](AppEngineClass cls) noexcept -> v6::EngineSignature {
        return cls == AppEngineClass::HeavyGame3D
            ? v6::EngineSignature::HeavyGame3D
            : v6::EngineSignature::StandardUi;
    };

    // Feed-Forward Pre-Emptive Cancel & Coupled Multi-Domain Symbiotic Orchestrator state
    uint64_t feedforward_freeze_until_ms = 0U;
    double prev_gpu_busy_ff = -1.0;
    double prev_cpu_psi_ff = -1.0;
    double prev_io_psi_ff = -1.0;
    engine::SymbioticPlan symbiotic_plan{0, 0.0, 1.0, 0.65, false, false, false, 1.0, 1.0, 0.0, 0};
    uint8_t prev_symbiotic_regime = 0U;
    uint64_t last_symbiotic_episode_ms = 0U;

    const auto build_thread_demand = [&](uint64_t now_ms) noexcept {
        ThreadDemandContext d{};
        d.little_capacity = topo.little_capacity;
        d.mid_capacity = topo.mid_capacity;
        d.big_capacity = topo.big_capacity;
        d.refresh_hz = topo.refresh_hz;
        d.cpu_psi = live_status.cpu_psi.value_or(0.0);
        d.gpu_busy_pct = live_status.gpu_busy_pct.value_or(0.0);
        d.render_pressure = live_status.render_pressure;
        d.thermal_velocity_cpm = live_status.thermal_velocity_cpm;
        d.jank_ratio = live_status.jank_ratio;
        d.app_uclamp_bias = live_status.active_app_bias;
        d.feedforward_freeze = (now_ms <= feedforward_freeze_until_ms) ||
                               (live_status.ctx.touch_hold_remaining_ms > 0U);
        d.symbiotic_cpu_scale = std::clamp(symbiotic_plan.cpu_uclamp_scale * boot_dec.ramp_uclamp_scale, 0.88, 1.10);
        d.helper_uclamp_ratio = symbiotic_plan.helper_uclamp_ratio;
        return d;
    };
    const auto build_storage_telemetry = [&](const MemorySnapshot& mem) noexcept {
        StorageMemoryTelemetry t{};
        t.io_psi = live_status.io_psi.value_or(0.0);
        t.mem_psi = live_status.mem_psi.value_or(0.0);
        t.cpu_psi = live_status.cpu_psi.value_or(0.0);
        t.memory = mem;
        t.io_supply_boost = symbiotic_plan.io_supply_boost || symbiotic_plan.boost_gpu_memory_supply;
        t.capacity_scale = live_status.capacity_scale;
        t.thermal_guard = live_status.thermal_guard;
        t.low_battery_eco = live_status.low_battery_eco;
        return t;
    };
    const auto build_symbiotic_hints = [&]() noexcept {
        v6::SymbioticHints h{};
        h.boost_gpu_memory_supply = symbiotic_plan.boost_gpu_memory_supply;
        h.encourage_cluster_spread = symbiotic_plan.encourage_cluster_spread;
        h.memory_bus_scale = symbiotic_plan.memory_bus_scale;
        h.scheduler_responsiveness_scale = symbiotic_plan.scheduler_responsiveness_scale;
        h.thermal_stepdown_lead = symbiotic_plan.thermal_stepdown_lead;
        return h;
    };
    const auto is_passive_media_only = [&](const ContextDecision& ctx) noexcept {
        return ctx.mode == ContextMode::Interactive &&
               ctx.media_pipeline_active &&
               !ctx.camera_boost_active &&
               ctx.touch_hold_remaining_ms == 0U &&
               live_status.render_pressure < 0.35 &&
               live_status.cpu_psi.value_or(0.0) < 12.0 &&
               live_status.gpu_busy_pct.value_or(0.0) < 25.0;
    };
    const auto build_envelope_telemetry = [&]() noexcept {
        EnvelopeTelemetry e{};
        e.cpu_psi = live_status.cpu_psi.value_or(0.0);
        e.gpu_busy_pct = live_status.gpu_busy_pct.value_or(0.0);
        e.render_pressure = live_status.render_pressure;
        e.passive_media_only = is_passive_media_only(live_status.ctx);
        e.scheduler_responsiveness_scale = symbiotic_plan.scheduler_responsiveness_scale;
        e.thermal_stepdown_lead = symbiotic_plan.thermal_stepdown_lead;
        e.jank_ratio = live_status.jank_ratio;
        e.low_battery_eco = live_status.low_battery_eco;
        return e;
    };

    const auto collect_semantic_leaves = [&]() {
        std::vector<std::string> leaves;
        for (const auto& k : semantic_knobs.knobs()) {
            if (std::find(leaves.begin(), leaves.end(), k.leaf) == leaves.end()) {
                leaves.push_back(k.leaf);
            }
        }
        return leaves;
    };
    live_status.semantic_stats = semantic_knobs.stats();
    live_status.semantic_knob_leaves = collect_semantic_leaves();
    live_status.oem_yielded_paths = oem_map.permanently_yielded_paths();
    uint64_t last_refresh_check_ms = 0U;

    const auto collect_contended_paths = [&]() {
        return oem_map.active_contended_paths(steady_now_ms());
    };

    // Fast path 1 (< 5 ms): Touchscreen event -> instant per-TID UI/RenderThread boost + 700ms feed-forward guard
    if (!input_source.start([&](const FrameEvent&) {
            if (config.inspection_mode || live_status.thermal_emergency) return;
            if (last_screen_off) {
                last_screen_off = probe.is_screen_off(topo);
                if (last_screen_off) return;
            }
            const uint64_t now_ms = steady_now_ms();
            feedforward_freeze_until_ms = std::max<uint64_t>(feedforward_freeze_until_ms, now_ms + uint64_t{700});
            context_engine.notify_touch(now_ms);
            if (boot_dec.boot_settle_active) {
                // Touch-Aware Soft Bypass during BOOT_SETTLE: light per-TID UI/SF UClamp ONLY for real apps
                // (never system launchers), keeping 0 sysfs writes and 0 storage preread.
                const bool real_app = !last_fg.package_name.empty() &&
                                      !ContextEngine::is_system_launcher_package(last_fg.package_name);
                if (real_app && !live_status.thermal_guard && !live_status.ctx.app_excluded) {
                    const auto sf_tids = context_engine.resolve_surfaceflinger_tids();
                    ThreadDemandContext soft_demand = build_thread_demand(now_ms);
                    soft_demand.symbiotic_cpu_scale = 0.88;
                    thread_scheduler.apply(
                        ContextMode::Interactive, live_status.ctx.effective_profile,
                        live_status.capacity_scale, live_status.thermal_guard,
                        config.enable_frame_smoothness, sf_tids, last_fg.critical_tids,
                        soft_demand, {});
                    live_status.thread_stats = thread_scheduler.stats();
                }
                return;
            }
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
                sig.is_camera_app = config.enable_camera_boost && last_fg.is_camera_app;
                sig.has_media_thread = last_fg.has_media_thread;
                sig.refresh_hz = topo.refresh_hz;
                sig.low_battery_eco = live_status.low_battery_eco;
                live_status.ctx = context_engine.evaluate(sig);
                live_status.autonomous_regime = compute_autonomous_regime(live_status);
                const auto sf_tids = context_engine.resolve_surfaceflinger_tids();
                const auto chain_tids = build_fg_chain_tids(last_fg, live_status.ctx.camera_boost_active);
                thread_scheduler.apply(
                    live_status.ctx.mode, live_status.ctx.effective_profile,
                    live_status.capacity_scale, live_status.thermal_guard,
                    config.enable_frame_smoothness, sf_tids, chain_tids,
                    build_thread_demand(now_ms), last_fg.helper_tids);
                live_status.thread_stats = thread_scheduler.stats();
            }
        })) {
        emit_log(false, "[CoreFlow] Touchscreen evdev unavailable; running timer-driven context path.");
    }

    // Signals: SIGINT, SIGTERM, SIGHUP, SIGUSR1 (Sovereign Tamper RESYNC/ENFORCE)
    sigset_t mask{};
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGUSR1);
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
                if (info.ssi_signo == SIGUSR1) {
                    const char* sig_env_dir = std::getenv("COREFLOW_STATE_DIR");
                    const std::string sig_state_dir = (sig_env_dir && sig_env_dir[0] != '\0')
                        ? std::string(sig_env_dir) : std::string(kStateDir);
                    const std::string req_path = sig_state_dir + "/tamper_cmd.req";
                    UniqueFd rfd(::open(req_path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
                    if (rfd.valid()) {
                        char rbuf[1024];
                        const ssize_t rn = ::read(rfd.get(), rbuf, sizeof(rbuf) - 1U);
                        rfd.reset();
                        (void)::unlink(req_path.c_str());
                        if (rn > 0) {
                            rbuf[rn] = '\0';
                            std::string line(rbuf, static_cast<size_t>(rn));
                            const auto nl = line.find('\n');
                            if (nl != std::string::npos) line.resize(nl);
                            const auto tab = line.find('\t');
                            if (tab != std::string::npos && tab + 1U < line.size()) {
                                const std::string action = line.substr(0, tab);
                                const std::string node = line.substr(tab + 1U);
                                const uint64_t now_sig_ms = steady_now_ms();
                                bool resynced = false;
                                if (node == "*" || node == "ALL") {
                                    for (const auto& k : semantic_knobs.knobs()) {
                                        if (semantic_knobs.force_resync_node(k.path, now_sig_ms)) {
                                            resynced = true;
                                        }
                                    }
                                } else {
                                    resynced = semantic_knobs.force_resync_node(node, now_sig_ms);
                                    coordinator.clear_node_quarantine(node);
                                }
                                decision_journal.record(
                                    epoch_now_ms(), "sovereign", "TamperResync",
                                    "Perintah " + action + " pada " + node +
                                        (resynced ? " · Sinkronisasi & kunci 0444 dipulihkan"
                                                  : " · Karantina dibersihkan"),
                                    "verified", live_status.soc_temp, live_status.render_pressure,
                                    live_status.capacity_scale);
                                live_status.locked_nodes = baseline.locked_node_count();
                                live_status.quarantined_nodes = coordinator.quarantined_node_count(now_sig_ms);
                                live_status.total_tamper_events = coordinator.total_tamper_events();
                                live_status.tamper_events = coordinator.recent_tamper_events(12U);
                                live_status.episodes = decision_journal.recent(16U);
                                (void)write_status_snapshot(build_status_json(live_status));
                            }
                        }
                    }
                } else if (info.ssi_signo == SIGHUP) {
                    thread_scheduler.reset_all();
                    semantic_knobs.rollback_all();
                    longevity.restore_oem_baseline();
                    prev_bypass_active = false;
                    (void)baseline.rollback_all();
                    oem_map.reset_transient_cooldowns();
                    storage_memory.reset_contention();
                    thermal_envelope.reset_contention();
                    config = DaemonConfig::load_best_available();
                    topo = probe.discover();
                    frame_pacing.discover("/sys", "/proc", topo.vblank_irq_token);
                    semantic_knobs.discover(topo);
                    apply_config_to_subsystems();
                    pid_initialized = false;
                    pid_controller = engine::new_controller({
                        config.pid_kp, config.pid_ki, config.pid_kd, pid_target_pressure(pid_target_fallback)});
                    live_status.config = config;
                    live_status.topo = topo;
                    live_status.frame_pacing_available = frame_pacing.supported();
                    live_status.semantic_stats = semantic_knobs.stats();
                    live_status.semantic_knob_leaves = collect_semantic_leaves();
                    live_status.oem_yielded_paths = oem_map.permanently_yielded_paths();
                    live_status.ctx.effective_profile = context_engine.profile();
                    live_status.autonomous_regime = compute_autonomous_regime(live_status);
                    const uint32_t ms = std::clamp(config.temp_monitor_interval_ms, 1000u, 60000u);
                    itimerspec ts{};
                    ts.it_interval.tv_sec = ms / 1000;
                    ts.it_interval.tv_nsec = (ms % 1000) * 1000000L;
                    // Fire the first post-reload actuation tick promptly (50 ms) so new settings apply immediately.
                    ts.it_value.tv_sec = 0;
                    ts.it_value.tv_nsec = 50'000'000L;
                    if (thermal_timer_fd >= 0) (void)::timerfd_settime(thermal_timer_fd, 0, &ts, nullptr);
                    emit_log(false, std::string("[CoreFlow] Configuration reloaded (mode=autonomous, paused=") +
                             (config.inspection_mode ? "yes" : "no") + ").");
                    decision_journal.record(
                        epoch_now_ms(), "context", "ConfigReload",
                        config.inspection_mode ? "Paused (OEM A/B inspection mode)"
                                               : (std::string("Zero-Profile Autonomous") +
                                                  (config.bypass_charging ? " · Bypass ON" : "")),
                        "verified", live_status.soc_temp, live_status.render_pressure, live_status.capacity_scale);
                    live_status.episodes = decision_journal.recent(16U);
                    (void)write_status_snapshot(build_status_json(live_status));
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
        if (boot_dec.boot_settle_active) {
            // Phase 1 (BOOT_SETTLE): observe foreground package only; 0 sysfs writes, 0 preread, 0 learning.
            live_status.ctx.foreground_pid = fg.pid;
            live_status.ctx.foreground_package = fg.package_name;
            return;
        }
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
        sig.is_camera_app = config.enable_camera_boost && fg.is_camera_app;
        sig.has_media_thread = fg.has_media_thread;
        sig.refresh_hz = topo.refresh_hz;
        sig.low_battery_eco = live_status.low_battery_eco;
        const ContextMode prev_mode = live_status.ctx.mode;
        live_status.ctx = context_engine.evaluate(sig);
        live_status.autonomous_regime = compute_autonomous_regime(live_status);
        const bool is_launcher = ContextEngine::is_system_launcher_package(fg.package_name);
        if (boot_dec.allow_learning && !is_launcher && live_status.ctx.newly_launched_app &&
            (fg.has_render_thread || fg.has_game_thread)) {
            ++live_status.session.launch_bursts;
            if (config.enable_predictive_preload) {
                record_preloader_launch(fg.package_name);
            }
            app_dna.record_launch(fg.package_name, now_ms, to_dna_signature(fg.engine_class));
            live_status.app_dna_learned = app_dna.learned_count();
            const bool cam_launch = live_status.ctx.camera_boost_active && config.enable_camera_boost;
            decision_journal.record(
                epoch_now_ms(),
                cam_launch ? "uclamp" : "storage",
                cam_launch ? "CameraBoost" : "LaunchBurst",
                (cam_launch ? "Camera HAL + UI fast-launch: " : "Boosted ") +
                    (fg.package_name.empty() ? std::string("foreground app") : fg.package_name),
                "verified", live_status.soc_temp, live_status.render_pressure, live_status.capacity_scale);
        }
        if (changed) {
            live_status.active_app_bias = app_dna.uclamp_bias_for(fg.package_name);
        }
        if (changed || live_status.ctx.mode != prev_mode) {
            if (boot_dec.allow_uclamp) {
                const auto sf_tids = context_engine.resolve_surfaceflinger_tids();
                const auto chain_tids = build_fg_chain_tids(fg, live_status.ctx.camera_boost_active);
                thread_scheduler.apply(
                    live_status.ctx.mode, live_status.ctx.effective_profile,
                    live_status.capacity_scale, live_status.thermal_guard,
                    config.enable_frame_smoothness, sf_tids, chain_tids,
                    build_thread_demand(now_ms), fg.helper_tids);
            }
            const auto sysfs_ctx = build_sysfs_ctx(live_status.ctx, now_ms, fg);
            if (boot_dec.allow_storage_and_symbiotic) {
                storage_memory.apply(
                    topo, sysfs_ctx, live_status.ctx.effective_profile,
                    config.enable_storage_burst, config.enable_memory_guard, now_ms,
                    build_storage_telemetry(last_mem_snap));
            }
            if (boot_dec.allow_envelope_and_semantic) {
                thermal_envelope.apply(
                    topo, sysfs_ctx.mode, live_status.ctx.effective_profile,
                    live_status.capacity_scale, live_status.thermal_guard,
                    config.enable_thermal_pacing,
                    build_envelope_telemetry(), now_ms);
                semantic_knobs.apply(
                    sysfs_ctx.mode, live_status.ctx.effective_profile,
                    live_status.capacity_scale, live_status.thermal_guard,
                    config.enable_thermal_pacing || config.enable_storage_burst,
                    last_mem_snap.valid ? last_mem_snap.available_ratio() : 0.45,
                    live_status.mem_psi.value_or(0.0), now_ms,
                    is_passive_media_only(live_status.ctx),
                    build_symbiotic_hints());
            }
            if (config.bypass_on_gaming) {
                (void)longevity.apply_power_control(
                    config.bypass_charging,
                    live_status.ctx.mode == ContextMode::Gaming,
                    config.bypass_on_gaming,
                    config.charge_limit_pct);
            }
            live_status.thread_stats = thread_scheduler.stats();
            live_status.storage_stats = storage_memory.stats();
            live_status.envelope_stats = thermal_envelope.stats();
            live_status.semantic_stats = semantic_knobs.stats();
            live_status.contended_paths = collect_contended_paths();
            live_status.oem_yielded_paths = oem_map.permanently_yielded_paths();
        }
    });

    // Main control path (default 1000 ms): Thermal + PSI + Online RLS/MLP + Rust PID + Dual-Timescale Equilibrium + Orthogonal Symbiotic Orchestrator
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
        double skin_temp = -1.0;
        for (const auto& r : tb.assessment.readings) {
            if (r.valid && r.category == 3 && std::isfinite(r.celsius) && r.celsius > 0.0f && r.celsius < 100.0f) {
                skin_temp = std::max(skin_temp, static_cast<double>(r.celsius));
            }
        }

        if (!thermal.available && (tb.profile.total_valid_sensors > 0 || soc_temp >= 0.0f || skin_temp > 0.0 || batt_temp > 0.0f)) {
            const auto fallback = evaluate_generic_thermal_fallback({
                .soc_temp_celsius = soc_temp,
                .skin_temp_celsius = skin_temp > 0.0 ? static_cast<float>(skin_temp) : tb.skin_celsius,
                .battery_temp_celsius = batt_temp,
                .chassis_recover_celsius = config.thermal_recover_celsius,
                .chassis_trip_celsius = config.thermal_trip_celsius,
                .emergency_margin_celsius = config.thermal_emergency_margin_celsius,
            });
            if (fallback.available) {
                thermal.available = fallback.available;
                thermal.fallback_active = fallback.fallback_active;
                thermal.pressure = fallback.pressure;
                thermal.source = fallback.source;
                thermal.sensor = fallback.sensor;
                thermal.emergency = fallback.emergency;
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
        if (!screen_off && (last_refresh_check_ms == 0U || now_ms >= last_refresh_check_ms + 5000ULL)) {
            last_refresh_check_ms = now_ms;
            if (const auto live_hz = probe.read_live_refresh_hz(topo)) {
                topo.refresh_hz = *live_hz;
                live_status.topo.refresh_hz = *live_hz;
            }
        }
        const double cpu_util = cpu_sampler.sample();
        const auto gpu_busy = probe.read_gpu_busy_pct(topo);
        const auto cpu_psi = probe.read_psi_avg10("cpu");
        const auto mem_psi = probe.read_psi_avg10("memory");
        const auto io_psi = probe.read_psi_avg10("io");
        last_mem_snap = probe.read_memory_snapshot();

        // Feed-Forward Pre-Emptive Cancel detector:
        // Arm a 700ms UClamp-trim freeze when d(GPU)/dt >= 25%/s or sudden Linux PSI stall spike occurs.
        if (!screen_off) {
            bool transient_spike = false;
            if (gpu_busy.has_value() && std::isfinite(*gpu_busy)) {
                if (prev_gpu_busy_ff >= 0.0) {
                    const double d_gpu_dt = (*gpu_busy - prev_gpu_busy_ff) / elapsed;
                    if (d_gpu_dt >= 25.0 || (*gpu_busy - prev_gpu_busy_ff) >= 18.0) {
                        transient_spike = true;
                    }
                }
                prev_gpu_busy_ff = *gpu_busy;
            }
            if (cpu_psi.has_value() && std::isfinite(*cpu_psi)) {
                if (prev_cpu_psi_ff >= 0.0 && (*cpu_psi - prev_cpu_psi_ff) >= 6.0) {
                    transient_spike = true;
                }
                prev_cpu_psi_ff = *cpu_psi;
            }
            if (io_psi.has_value() && std::isfinite(*io_psi)) {
                if (prev_io_psi_ff >= 0.0 && (*io_psi - prev_io_psi_ff) >= 8.0) {
                    transient_spike = true;
                }
                prev_io_psi_ff = *io_psi;
            }
            if (transient_spike) {
                feedforward_freeze_until_ms = std::max<uint64_t>(feedforward_freeze_until_ms, now_ms + uint64_t{700});
            }
        } else {
            prev_gpu_busy_ff = -1.0;
            prev_cpu_psi_ff = -1.0;
            prev_io_psi_ff = -1.0;
        }

        live_status.soc_temp = soc_temp;
        live_status.battery_temp = batt_temp;
        live_status.thermal_pressure = thermal.pressure;
        live_status.thermal_source = thermal.source;
        live_status.thermal_zones = collect_thermal_zones(tb);
        live_status.cpu_util_pct = cpu_util;
        live_status.gpu_busy_pct = gpu_busy;
        live_status.cpu_psi = cpu_psi;
        live_status.mem_psi = mem_psi;
        live_status.io_psi = io_psi;
        live_status.memory = last_mem_snap;
        live_status.cluster_cur_khz.clear();
        live_status.cluster_cur_khz.reserve(topo.cpu_clusters.size());
        for (const auto& c : topo.cpu_clusters) {
            live_status.cluster_cur_khz.push_back(probe.read_cluster_cur_freq_khz(c).value_or(0U));
        }

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
            live_status.low_battery_eco = compute_low_battery_eco(live_status.battery);
            live_status.autonomous_regime = compute_autonomous_regime(live_status);
            live_status.app_dna_entries = app_dna.snapshot_entries();
            live_status.contended_paths = collect_contended_paths();
            live_status.oem_yielded_paths = oem_map.permanently_yielded_paths();
            if (live_status.contended_paths.size() > prev_contended_count) {
                std::string node_label = std::to_string(live_status.contended_paths.size()) + " sysfs node(s)";
                if (!live_status.contended_paths.empty()) {
                    const auto& first_path = live_status.contended_paths.front();
                    const auto slash = first_path.find_last_of('/');
                    const std::string leaf = slash != std::string::npos ? first_path.substr(slash + 1U) : first_path;
                    if (!leaf.empty()) {
                        node_label = live_status.contended_paths.size() == 1U
                            ? leaf
                            : (leaf + " +" + std::to_string(live_status.contended_paths.size() - 1U));
                    }
                }
                decision_journal.record(
                    epoch_now_ms(), "thermal", "OEMContention",
                    "Yielded " + node_label + " to OEM cooldown",
                    "oem_yield", live_status.soc_temp, live_status.render_pressure, live_status.capacity_scale);
            } else if (live_status.contended_paths.empty() && prev_contended_count > 0U) {
                decision_journal.record(
                    epoch_now_ms(), "thermal", "OEMRecovery",
                    "Re-acquired full sysfs ownership after cooldown",
                    "verified", live_status.soc_temp, live_status.render_pressure, live_status.capacity_scale);
            }
            prev_contended_count = live_status.contended_paths.size();
            size_t active_nodes = 0U;
            for (const auto& entry : baseline.get_audit_trail()) {
                if (entry.is_modified) ++active_nodes;
            }
            live_status.baseline_nodes = active_nodes;
            live_status.recovery_pending = active_nodes > 0U;
            live_status.locked_nodes = baseline.locked_node_count();
            live_status.quarantined_nodes = coordinator.quarantined_node_count(now_ms);
            live_status.total_tamper_events = coordinator.total_tamper_events();
            live_status.tamper_events = coordinator.recent_tamper_events(12U);
            live_status.symbiotic_regime = symbiotic_plan.regime;
            live_status.memory_bus_scale = symbiotic_plan.memory_bus_scale;
            live_status.scheduler_responsiveness_scale = symbiotic_plan.scheduler_responsiveness_scale;
            live_status.thermal_stepdown_lead = symbiotic_plan.thermal_stepdown_lead;
            live_status.coupling_mask = symbiotic_plan.coupling_mask;
            live_status.app_dna_flush_count = app_dna.flush_count();
            live_status.episodes = decision_journal.recent(16U);
            ++live_status.session.scale_samples;
            live_status.session.scale_sum += live_status.capacity_scale;
            const uint64_t sample_ts = epoch_now_ms();
            live_status.history.push_back(HistoryPoint{
                .ts_ms = sample_ts,
                .soc_temp = soc_temp >= 0.0f ? static_cast<double>(soc_temp) : -1.0,
                .thermal_pressure = thermal.pressure,
                .cpu_util_pct = cpu_util,
                .capacity_scale = live_status.capacity_scale,
                .mode = live_status.thermal_emergency ? "THERMAL_EMERGENCY"
                        : live_status.thermal_guard ? "THERMAL_GUARD"
                        : live_status.boot_settle_active ? "BOOT_SETTLE"
                        : ContextEngine::mode_name(live_status.ctx.mode),
            });
            if (live_status.history.size() > 60U) {
                live_status.history.erase(live_status.history.begin());
            }
            // Compute jitter-free thermal rise/cool velocity (°C/min) via Rust OLS ring buffer
            // using monotonic steady_clock timestamps and an EWMA-damped composite chassis+die
            // temperature (tau = 15s) so 1-second CPU die bursts never trigger fake ±25°/m swings.
            double raw_trend_c = -1.0;
            const double slow_chassis_c =
                (std::isfinite(batt_temp) && batt_temp > 0.0f) ? static_cast<double>(batt_temp)
                : (std::isfinite(skin_temp) && skin_temp > 0.0) ? skin_temp
                : -1.0;
            if (slow_chassis_c > 0.0 && std::isfinite(soc_temp) && soc_temp > 0.0f) {
                raw_trend_c = 0.65 * slow_chassis_c + 0.35 * static_cast<double>(soc_temp);
            } else if (slow_chassis_c > 0.0) {
                raw_trend_c = slow_chassis_c;
            } else if (std::isfinite(soc_temp) && soc_temp > 0.0f) {
                raw_trend_c = static_cast<double>(soc_temp);
            }
            if (raw_trend_c > 0.0 && std::isfinite(raw_trend_c)) {
                if (thermal_trend_ewma_c <= 0.0 || !std::isfinite(thermal_trend_ewma_c)) {
                    thermal_trend_ewma_c = raw_trend_c;
                } else {
                    const double alpha = 1.0 - std::exp(-std::clamp(elapsed, 0.1, 30.0) / 15.0);
                    thermal_trend_ewma_c += alpha * (raw_trend_c - thermal_trend_ewma_c);
                }
            }
            const double trip_c = slow_chassis_c > 0.0
                ? std::clamp(static_cast<double>(config.thermal_trip_celsius), 40.0, 52.0)
                : std::clamp(static_cast<double>(config.thermal_trip_celsius) + 30.0, 72.0, 82.0);
            const auto trend = thermal_trend->push_sample(
                now_ms,
                thermal_trend_ewma_c,
                trip_c);
            live_status.thermal_velocity_cpm = trend.velocity_cpm;
            // Observe-only RC chassis identification: total system power (discharging only) vs the
            // slow chassis node. Charging/unknown power poisons the step instead of fabricating data.
            {
                const bool power_ok = live_status.battery.available && !live_status.battery.charging &&
                                      std::isfinite(live_status.battery.power_mw) &&
                                      live_status.battery.power_mw > 0.0;
                const double model_temp = slow_chassis_c > 0.0 ? slow_chassis_c : -1.0;
                live_status.thermal_model = thermal_model->push_sample(
                    now_ms, model_temp,
                    power_ok ? live_status.battery.power_mw / 1000.0 : -1.0,
                    kChassisTargetCelsius);
            }
            (void)write_status_snapshot(build_status_json(live_status));
        };

        // Hard hardware safety cutoff (battery >= 58C or SoC >= 95C)
        if ((batt_temp > 0.0f && batt_temp >= 58.0f) || soc_temp >= 95.0f) {
            const bool entering_emergency = !live_status.thermal_emergency;
            live_status.thermal_emergency = true;
            live_status.thermal_guard = true;
            if (!live_status.session.prev_thermal_guard) {
                ++live_status.session.thermal_guard_events;
                live_status.session.prev_thermal_guard = true;
            }
            live_status.capacity_scale = 0.25;
            if (entering_emergency) {
                thread_scheduler.reset_all();
                semantic_knobs.rollback_all();
                longevity.restore_oem_baseline();
                prev_bypass_active = false;
                (void)baseline.rollback_all();
                decision_journal.record(
                    epoch_now_ms(), "thermal", "EmergencyCutoff",
                    "Extreme thermal limit reached; restored OEM baseline",
                    "rolled_back", live_status.soc_temp, live_status.render_pressure, live_status.capacity_scale);
                emit_log(true, "[CoreFlow][Safety] Extreme thermal threshold reached; rolled back to OEM baseline.");
            }
            record_history_and_publish();
            return;
        }
        live_status.thermal_emergency = false;

        // Both OEM thermal policy and dual-domain generic_fallback emit normalized 0..130 pressure
        // (65 = recovery threshold, 85 = thermal guard trip across chassis OR CPU silicon die).
        const bool thermal_guard =
            thermal.emergency || thermal.severity >= 3 || thermal.pressure >= 85.0 ||
            (live_status.thermal_guard && thermal.pressure > 65.0);
        if (thermal_guard && !live_status.session.prev_thermal_guard) {
            ++live_status.session.thermal_guard_events;
            decision_journal.record(
                epoch_now_ms(), "thermal", "ThermalGuard",
                "Thermal Guard engaged; smoothly capping headroom",
                "verified", live_status.soc_temp, live_status.render_pressure, live_status.capacity_scale);
        } else if (!thermal_guard && live_status.session.prev_thermal_guard) {
            decision_journal.record(
                epoch_now_ms(), "thermal", "ThermalRecovered",
                "Thermal Guard released; full headroom restored",
                "verified", live_status.soc_temp, live_status.render_pressure, live_status.capacity_scale);
        }
        live_status.session.prev_thermal_guard = thermal_guard;
        live_status.thermal_guard = thermal_guard;

        // 10s Hybrid CFW2/ONNX + Online RLS thermal & jank-risk lookahead
        const double charging_mw = live_status.battery.charging
            ? std::clamp(live_status.battery.power_mw, 0.0, 25000.0)
            : 0.0;
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
            .gpu_busy_pct = std::clamp(gpu_busy.value_or(0.0), 0.0, 100.0),
            .render_pressure = std::clamp(live_status.render_pressure, 0.0, 1.0),
            .jank_ratio = live_status.jank_ratio >= 0.0 ? std::clamp(live_status.jank_ratio, 0.0, 1.0) : 0.0,
            .charging_power_mw = charging_mw,
        });
        live_status.ai_predicted_10s = ai.valid ? ai.predicted_pressure_10s : 0.0;
        live_status.ai_confidence = ai.valid ? ai.confidence : 0.0;
        live_status.ai_jank_risk = ai.valid ? ai.jank_risk : 0.0;
        live_status.ai_workload_class = ai.valid ? std::string(ai.workload_class) : "balanced";
        live_status.ai_backend = ai.valid ? std::string(ai.backend) : "embedded-rls-mlp";

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
        bool equilibrium_guard = false;
        if (pid_dec.valid) {
            double scale = pid_dec.capacity_scale;
            if (ai.valid && ai.confidence >= 0.60 && ai.predicted_pressure_10s >= 80.0) {
                // Keep advisory pre-damp bounded above 0.65 unless Thermal Guard is active so
                // 10s Android thermal headroom refresh steps cannot slam headroom down to 0.44.
                const double ai_floor = thermal_guard ? 0.25 : 0.65;
                scale = std::min(scale, std::max(ai.recommended_scale, ai_floor));
            }
            if (thermal_guard) scale = std::min(scale, 0.75);
            const auto eq = engine::compute_thermal_equilibrium({
                static_cast<double>(soc_temp),
                static_cast<double>(batt_temp),
                skin_temp,
                live_status.thermal_velocity_cpm,
                scale,
                live_status.battery.charging,
                longevity.is_bypass_active(),
                live_status.ctx.mode == ContextMode::LaunchBurst});
            live_status.charging_damped = eq.charging_damped;
            equilibrium_guard = eq.equilibrium_guard;
            // Slew-rate limit post-PID AI/equilibrium scale changes (max 0.08/s down, 0.06/s up)
            // so 1-tick telemetry/AI blips cannot produce a 40% headroom cliff.
            const double target_scale = std::clamp(eq.effective_scale, 0.25, 1.0);
            const double max_drop = std::clamp(0.08 * elapsed, 0.02, 0.20);
            const double max_rise = std::clamp(0.06 * elapsed, 0.02, 0.15);
            const double prev_scale = std::isfinite(live_status.capacity_scale)
                ? std::clamp(live_status.capacity_scale, 0.25, 1.0)
                : 1.0;
            live_status.capacity_scale = std::clamp(
                std::clamp(target_scale, prev_scale - max_drop, prev_scale + max_rise),
                0.25, 1.0);
        } else {
            pid_controller->reset();
            live_status.charging_damped = false;
        }
        const bool effective_thermal_guard = thermal_guard || equilibrium_guard;
        live_status.thermal_guard = effective_thermal_guard;

        if (!screen_off) {
            last_fg = context_engine.resolve_foreground_app(topo.cgroups.top_app_procs_path);
            double rp = render_sampler.sample(last_fg.pid, last_fg.critical_tids, elapsed);
            // Blend GPU busy / CPU PSI floor so Vulkan/GPU-bound scenes always report live render pressure
            const double gpu_floor = gpu_busy.has_value() && *gpu_busy >= 8.0
                ? std::clamp((*gpu_busy / 100.0) * 0.78, 0.0, 0.92)
                : 0.0;
            const double psi_floor = cpu_psi.has_value() && *cpu_psi >= 3.0
                ? std::clamp(*cpu_psi / 65.0, 0.0, 0.65)
                : 0.0;
            live_status.render_pressure = std::clamp(std::max({rp, gpu_floor, psi_floor}), 0.0, 1.0);
        } else {
            live_status.render_pressure = render_sampler.sample(0, {}, elapsed);
        }

        // Adaptive Post-Boot Thermal Tranquility Protocol (Boot-Calm Sanctuary)
        const bool real_fg_app = !screen_off && !last_fg.package_name.empty() &&
                                 !ContextEngine::is_system_launcher_package(last_fg.package_name);
        const bool touch_active_now = (now_ms <= feedforward_freeze_until_ms) ||
                                      (live_status.ctx.touch_hold_remaining_ms > 0U);
        boot_dec = boot_calm.evaluate({
            .kernel_uptime_sec = v6::BootCalmSanctuary::read_kernel_uptime_sec("/proc"),
            .now_ms = now_ms,
            .screen_off = screen_off,
            .thermal_guard = effective_thermal_guard,
            .cpu_util_pct = cpu_util,
            .io_psi_pct = io_psi.value_or(0.0),
            .soc_temp_c = static_cast<double>(soc_temp),
            .thermal_velocity_cpm = live_status.thermal_velocity_cpm,
            .touch_active = touch_active_now,
            .real_foreground_app = real_fg_app,
        });
        live_status.boot_settle_active = boot_dec.boot_settle_active;
        live_status.boot_settle_duration_sec = boot_dec.settle_duration_sec;
        live_status.boot_settle_exit_reason = boot_dec.exit_reason;
        live_status.boot_ramp_progress = boot_dec.ramp_alpha;

        if (boot_dec.needs_late_baseline_snapshot) {
            // Phase 3 Entry (Screen-ON): Re-discover & capture true post-boot OEM baselines AFTER
            // HyperOS perf-hal & mi_thermald finished boot init, preventing false OEM yields.
            topo = probe.discover();
            frame_pacing.discover("/sys", "/proc", topo.vblank_irq_token);
            semantic_knobs.discover(topo);
            live_status.topo = topo;
            live_status.frame_pacing_available = frame_pacing.supported();
            live_status.semantic_stats = semantic_knobs.stats();
            live_status.semantic_knob_leaves = collect_semantic_leaves();
            decision_journal.record(
                epoch_now_ms(), "context", "BootSettleComplete",
                "Exited BOOT_SETTLE (" + boot_dec.exit_reason + ", " +
                    std::to_string(static_cast<unsigned>(std::round(boot_dec.settle_duration_sec))) +
                    "s); captured late OEM baseline & started 30s soft-start ramp",
                "verified", live_status.soc_temp, live_status.render_pressure, live_status.capacity_scale);
        }

        // Coupled Multi-Domain Symbiotic Orchestrator: cross-cluster saturation + Compute<->Bus, Render<->Sched, Headroom<->Thermal
        if (!screen_off && boot_dec.allow_storage_and_symbiotic) {
            double little_r = 0.0, mid_r = 0.0, big_r = 0.0;
            for (size_t i = 0; i < topo.cpu_clusters.size() && i < live_status.cluster_cur_khz.size(); ++i) {
                const auto& c = topo.cpu_clusters[i];
                const uint64_t max_khz = c.opps_khz.empty() ? 0U : c.opps_khz.back();
                if (max_khz == 0U) continue;
                const double ratio = std::clamp(
                    static_cast<double>(live_status.cluster_cur_khz[i]) / static_cast<double>(max_khz),
                    0.0, 1.0);
                if (c.role == ClusterRole::Little) little_r = std::max(little_r, ratio);
                else if (c.role == ClusterRole::Mid) mid_r = std::max(mid_r, ratio);
                else if (c.role == ClusterRole::Big) big_r = std::max(big_r, ratio);
            }
            symbiotic_plan = engine::compute_symbiotic_plan({
                little_r, mid_r, big_r,
                cpu_util,
                live_status.render_pressure,
                gpu_busy.value_or(0.0),
                cpu_psi.value_or(0.0),
                mem_psi.value_or(0.0),
                io_psi.value_or(0.0),
                last_mem_snap.valid ? last_mem_snap.available_ratio() : 0.45,
                live_status.capacity_scale,
                effective_thermal_guard});
            if (symbiotic_plan.regime != 0U && symbiotic_plan.regime != prev_symbiotic_regime &&
                (last_symbiotic_episode_ms == 0U || now_ms >= last_symbiotic_episode_ms + 30'000ULL)) {
                last_symbiotic_episode_ms = now_ms;
                const std::string desc = symbiotic_plan.regime == 4U
                    ? ("Spread single-cluster hotspot (" +
                       std::to_string(static_cast<unsigned>(std::round(symbiotic_plan.cluster_imbalance * 100.0))) +
                       "% skew) across companion clusters")
                    : symbiotic_plan.regime == 1U
                        ? "GPU-bound co-pacing: shifted headroom & bus supply to GPU"
                        : symbiotic_plan.regime == 2U
                            ? "CPU draw-call bound: lifted parallel worker headroom"
                            : "I/O supply co-pacing: boosted asset read-ahead";
                decision_journal.record(
                    epoch_now_ms(), "uclamp", "SymbioticBalance", desc,
                    "verified", live_status.soc_temp, live_status.render_pressure, live_status.capacity_scale);
            }
            prev_symbiotic_regime = symbiotic_plan.regime;
        } else {
            symbiotic_plan = {0, 0.0, 1.0, 0.65, false, false, false, 1.0, 1.0, 0.0, 0};
            prev_symbiotic_regime = 0U;
        }

        ContextSignalInput sig{};
        sig.now_ms = now_ms;
        sig.screen_off = screen_off;
        sig.thermal_guard = effective_thermal_guard;
        sig.cpu_util_pct = cpu_util;
        sig.gpu_busy_pct = gpu_busy;
        sig.foreground_pid = screen_off ? 0 : last_fg.pid;
        sig.foreground_package = screen_off ? std::string{} : last_fg.package_name;
        sig.has_game_thread = !screen_off && last_fg.has_game_thread;
        sig.is_camera_app = !screen_off && config.enable_camera_boost && last_fg.is_camera_app;
        sig.has_media_thread = !screen_off && last_fg.has_media_thread;
        sig.refresh_hz = topo.refresh_hz;
        sig.low_battery_eco = live_status.low_battery_eco;
        const ContextMode prev_mode = live_status.ctx.mode;
        live_status.ctx = context_engine.evaluate(sig);
        live_status.autonomous_regime = compute_autonomous_regime(live_status);
        const bool fg_is_launcher = ContextEngine::is_system_launcher_package(last_fg.package_name);
        if (boot_dec.allow_learning && !fg_is_launcher && live_status.ctx.newly_launched_app &&
            (last_fg.has_render_thread || last_fg.has_game_thread)) {
            ++live_status.session.launch_bursts;
            if (config.enable_predictive_preload) {
                record_preloader_launch(last_fg.package_name);
            }
            app_dna.record_launch(last_fg.package_name, now_ms, to_dna_signature(last_fg.engine_class));
        }

        // Pillar 3: Zero-Wear On-Device Context Learning (RAM cost-to-gain; ScreenOff significant-drift flush)
        if (!screen_off) {
            const bool active_workload =
                !boot_dec.boot_settle_active &&
                (live_status.ctx.mode == ContextMode::Interactive ||
                 live_status.ctx.mode == ContextMode::LaunchBurst ||
                 live_status.ctx.mode == ContextMode::Gaming);
            const auto fps_sample = frame_pacing.sample(
                topo.refresh_hz, now_ms, active_workload, &frame_bridge);
            live_status.frame_pacing_available = fps_sample.available || frame_pacing.supported();
            live_status.measured_fps = fps_sample.measured_fps;
            live_status.jank_ratio = fps_sample.jank_ratio;
            if (!last_fg.package_name.empty()) {
                if (active_workload && boot_dec.allow_learning) {
                    v6::ContextLearningSample ctx_sample{};
                    ctx_sample.jank_ratio = live_status.jank_ratio;
                    ctx_sample.measured_fps = live_status.measured_fps;
                    ctx_sample.target_fps = static_cast<double>(topo.refresh_hz > 0U ? topo.refresh_hz : 60U);
                    ctx_sample.thermal_velocity_cpm = live_status.thermal_velocity_cpm;
                    ctx_sample.power_mw = live_status.battery.power_mw;
                    ctx_sample.charging = live_status.battery.charging;
                    ctx_sample.low_battery = (live_status.battery.battery_capacity_pct >= 0 &&
                                              live_status.battery.battery_capacity_pct <= 20);
                    ctx_sample.in_wake_ramp = (boot_dec.ramp_alpha < 0.99);
                    ctx_sample.capacity_scale = live_status.capacity_scale;
                    app_dna.observe_context(last_fg.package_name, ctx_sample, now_ms,
                                            to_dna_signature(last_fg.engine_class));
                }
                live_status.active_app_bias = app_dna.uclamp_bias_for(last_fg.package_name);
                live_status.active_app_efficiency_weight = app_dna.efficiency_weight_for(last_fg.package_name);
                live_status.app_dna_learned = app_dna.learned_count();
            } else {
                live_status.active_app_bias = 1.0;
                live_status.active_app_efficiency_weight = 1.0;
            }
        } else {
            frame_pacing.reset();
            live_status.measured_fps = 0.0;
            live_status.jank_ratio = -1.0;
            live_status.active_app_bias = 1.0;
            live_status.active_app_efficiency_weight = 1.0;
            // Zero-Wear persistence during ScreenOff ONLY (throttled <= 1x / 15 min & gated on significant drift)
            if (boot_dec.allow_learning &&
                (last_state_persist_ms == 0U || now_ms >= last_state_persist_ms + 900'000ULL)) {
                last_state_persist_ms = now_ms;
                (void)app_dna.flush_if_significant();
                if (preloader_dirty && save_preloader_state(*preloader, preloader_pkg_map)) {
                    preloader_dirty = false;
                }
            }
        }

        if (!boot_dec.boot_settle_active && live_status.ctx.mode != prev_mode) {
            decision_journal.record(
                epoch_now_ms(), "context", "ModeShift",
                std::string(ContextEngine::mode_name(prev_mode)) + " -> " +
                    ContextEngine::mode_name(live_status.ctx.mode) +
                    (live_status.ctx.foreground_package.empty() ? "" : (" (" + live_status.ctx.foreground_package + ")")),
                "verified", live_status.soc_temp, live_status.render_pressure, live_status.capacity_scale);
        }

        if (!config.inspection_mode) {
            if (boot_dec.boot_settle_active) {
                // Phase 1 (BOOT_SETTLE — Zero-Write Passive Sanctuary):
                // 0 sysfs mutations, 0 LaunchBurst/preread I/O, 0 boost UClamp.
                // Exception 1: Touch-Aware Soft Bypass on real non-launcher apps (light per-TID UClamp only).
                // Exception 2: Emergency thermal guard cooling path remains active if boot overheats.
                if (boot_dec.allow_touch_soft_uclamp && !live_status.ctx.app_excluded) {
                    const auto sf_tids = context_engine.resolve_surfaceflinger_tids();
                    ThreadDemandContext soft_demand = build_thread_demand(now_ms);
                    soft_demand.symbiotic_cpu_scale = 0.88;
                    thread_scheduler.apply(
                        ContextMode::Interactive, live_status.ctx.effective_profile,
                        live_status.capacity_scale, effective_thermal_guard,
                        config.enable_frame_smoothness, sf_tids, last_fg.critical_tids,
                        soft_demand, {});
                } else {
                    thread_scheduler.reset_all();
                }
                if (effective_thermal_guard) {
                    thermal_envelope.apply(
                        topo, ContextMode::Efficiency, live_status.ctx.effective_profile,
                        live_status.capacity_scale, true,
                        config.enable_thermal_pacing,
                        build_envelope_telemetry(), now_ms);
                    longevity.monitor_battery_thermals(soc_temp);
                }
            } else {
                // Phase 3 (Exponential Soft-Start Ramp) & Steady Active Operation:
                // 0-10s: UClamp only -> 10-20s: +ThermalEnvelope & SemanticKnobs -> 20-30s+: +Storage & Symbiotic
                const auto sf_tids = screen_off ? std::vector<int32_t>{} : context_engine.resolve_surfaceflinger_tids();
                const auto chain_tids = screen_off
                    ? std::vector<int32_t>{}
                    : build_fg_chain_tids(last_fg, live_status.ctx.camera_boost_active);
                const auto helper_tids = screen_off ? std::vector<int32_t>{} : last_fg.helper_tids;
                if (boot_dec.allow_uclamp) {
                    thread_scheduler.apply(
                        live_status.ctx.mode, live_status.ctx.effective_profile,
                        live_status.capacity_scale, effective_thermal_guard,
                        config.enable_frame_smoothness, sf_tids, chain_tids,
                        build_thread_demand(now_ms), helper_tids);
                }
                const auto sysfs_ctx = build_sysfs_ctx(live_status.ctx, now_ms, last_fg);
                const auto storage_tel = build_storage_telemetry(last_mem_snap);
                if (boot_dec.allow_storage_and_symbiotic) {
                    storage_memory.apply(
                        topo, sysfs_ctx, live_status.ctx.effective_profile,
                        config.enable_storage_burst, config.enable_memory_guard, now_ms,
                        storage_tel);

                    // HyperFluid 9-Hour Anti-Aging Guard: proactive idle memory compaction during cool ScreenOff
                    if (screen_off && config.enable_memory_guard && config.enable_idle_compaction) {
                        if (storage_memory.maybe_run_idle_compaction(topo, storage_tel, soc_temp, now_ms)) {
                            decision_journal.record(
                                epoch_now_ms(), "memory", "IdleCompaction",
                                "Defragmented RAM during cool ScreenOff idle (anti-aging guard)",
                                "verified", live_status.soc_temp, live_status.render_pressure, live_status.capacity_scale);
                        }
                    }

                    // HyperFluid Rust Predictive Preloader: warm top predicted next app during calm cool Efficiency
                    if (!screen_off && config.enable_memory_guard && config.enable_predictive_preload &&
                        live_status.ctx.mode == ContextMode::Efficiency &&
                        !last_fg.package_name.empty() &&
                        (!std::isfinite(soc_temp) || soc_temp < 42.0f) &&
                        storage_tel.mem_psi < 8.0 &&
                        (!last_mem_snap.valid || last_mem_snap.available_ratio() >= 0.25)) {
                        const auto pred = preloader->predict_next(hash_pkg_fnv1a(last_fg.package_name));
                        if (pred.has_prediction) {
                            const auto pit = preloader_pkg_map.find(pred.target_hash);
                            if (pit != preloader_pkg_map.end() && pit->second != last_fg.package_name) {
                                const uint64_t basis = last_mem_snap.valid
                                    ? last_mem_snap.available_bytes
                                    : topo.total_ram_bytes;
                                const uint64_t warmed = storage_memory.prefetch_cached_package(pit->second, basis, now_ms);
                                if (warmed > 0U) {
                                    live_status.last_predicted_pkg = pit->second;
                                    decision_journal.record(
                                        epoch_now_ms(), "storage", "PredictivePreload",
                                        "Pre-warmed " + pit->second + " (" +
                                            std::to_string(static_cast<unsigned>(pred.confidence * 100.0)) + "% match)",
                                        "verified", live_status.soc_temp, live_status.render_pressure, live_status.capacity_scale);
                                }
                            }
                        }
                    }
                }

                if (boot_dec.allow_envelope_and_semantic || effective_thermal_guard) {
                    thermal_envelope.apply(
                        topo, sysfs_ctx.mode, live_status.ctx.effective_profile,
                        live_status.capacity_scale, effective_thermal_guard,
                        config.enable_thermal_pacing,
                        build_envelope_telemetry(), now_ms);
                }
                if (boot_dec.allow_envelope_and_semantic) {
                    semantic_knobs.apply(
                        sysfs_ctx.mode, live_status.ctx.effective_profile,
                        live_status.capacity_scale, effective_thermal_guard,
                        config.enable_thermal_pacing || config.enable_storage_burst,
                        last_mem_snap.valid ? last_mem_snap.available_ratio() : 0.45,
                        storage_tel.mem_psi, now_ms,
                        is_passive_media_only(live_status.ctx),
                        build_symbiotic_hints());
                }
                longevity.monitor_battery_thermals(soc_temp);
                (void)longevity.apply_power_control(
                    config.bypass_charging,
                    live_status.ctx.mode == ContextMode::Gaming,
                    config.bypass_on_gaming,
                    config.charge_limit_pct);
                const bool now_bypass = longevity.is_bypass_active();
                if (now_bypass != prev_bypass_active) {
                    const auto btel = longevity.sample_battery_telemetry();
                    decision_journal.record(
                        epoch_now_ms(), "power",
                        now_bypass ? "BypassEngaged" : "BypassReleased",
                        now_bypass ? ("Charging bypassed (" + btel.bypass_reason + ")") : "Restored OEM charging path",
                        "verified", live_status.soc_temp, live_status.render_pressure, live_status.capacity_scale);
                    prev_bypass_active = now_bypass;
                }
            }
        } else {
            thread_scheduler.reset_all();
            semantic_knobs.rollback_all();
        }

        live_status.thread_stats = thread_scheduler.stats();
        live_status.storage_stats = storage_memory.stats();
        live_status.envelope_stats = thermal_envelope.stats();
        live_status.semantic_stats = semantic_knobs.stats();
        record_history_and_publish();
    });

    (void)write_status_snapshot(build_status_json(live_status));
    emit_log(false, "[CFAE] CoreFlow Engine v6.1.0-R Unified Autonomous Daemon started (mode=autonomous" +
             std::string(", storage=") + HardwareProbe::storage_type_name(topo.primary_storage) +
             ", hz=" + std::to_string(topo.refresh_hz) +
             ", framePacing=" + (frame_pacing.supported() ? "yes" : "no") +
             ", appDna=" + std::to_string(app_dna.learned_count()) +
             ", semanticKnobs=" + std::to_string(semantic_knobs.stats().discovered_total) +
             ", memBusNodes=" + std::to_string(topo.memory_bus_nodes.size()) +
             ", oemYielded=" + std::to_string(oem_map.yielded_count()) +
             ", perThreadUclamp=" + (thread_scheduler.stats().supported ? "yes" : "no") +
             ", bypassSupported=" + (longevity.is_bypass_supported() ? "yes" : "no") +
             ", aiBackend=" + live_status.ai_backend + ").");

    reactor.run();

    // Guaranteed clean shutdown, cross-reboot state persistence & OEM baseline restoration
    input_source.stop();
    frame_bridge.stop();
    (void)app_dna.flush_if_dirty();
    if (preloader_dirty) {
        (void)save_preloader_state(*preloader, preloader_pkg_map);
    }
    thread_scheduler.reset_all();
    semantic_knobs.rollback_all();
    longevity.restore_oem_baseline();
    (void)baseline.rollback_all();
    live_status.running = false;
    live_status.pid = 0;
    live_status.locked_nodes = baseline.locked_node_count();
    (void)write_status_snapshot(build_status_json(live_status));
    clear_pid(pid_lock);
    emit_log(false, "[CFAE] CoreFlow Engine v6.1.0-R Unified Autonomous Daemon stopped; OEM baseline restored.");
    return 0;
}
