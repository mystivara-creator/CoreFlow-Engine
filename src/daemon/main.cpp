#include "coreflow/types.hpp"
#include "coreflow/config.hpp"
#include "coreflow/stability_policy.hpp"
#include "coreflow/reactor.hpp"
#include "coreflow/uclamp.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/thermal_monitor.hpp"
#include "coreflow/unique_fd.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace coreflow;
namespace {
constexpr char kStateDir[] = "/data/adb/coreflow";
constexpr char kPidPath[] = "/data/adb/coreflow/coreflowd.pid";

// Fatal signal handlers cannot lock mutexes, perform stream I/O or allocate.
void on_fatal_signal(int signum) {
    constexpr char line[] = "[CoreFlow] Fatal signal. Next start will recover journal.\n";
    const ssize_t emitted = ::write(STDERR_FILENO, line, sizeof(line) - 1);
    (void)emitted;
    ::_exit(128 + signum);
}
bool marker(const char* path) {
    return ::access(path, F_OK) == 0;
}

double read_psi_avg10(const char* path) {
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("some ", 0) != 0) continue;
        const auto at = line.find("avg10=");
        if (at == std::string::npos) continue;
        const char* start = line.c_str() + at + 6;
        char* end = nullptr;
        errno = 0;
        const double value = std::strtod(start, &end);
        if (errno == 0 && end != start && std::isfinite(value) && value >= 0.0)
            return value;
    }
    return 0.0;
}

const char* state_name(WorkloadState state) {
    switch (state) {
        case WorkloadState::Balanced: return "balanced";
        case WorkloadState::SustainedLoad: return "sustained-load";
        case WorkloadState::PressureGuard: return "pressure-guard";
        case WorkloadState::ThermalGuard: return "thermal-guard";
    }
    return "unknown";
}

bool process_matches(pid_t pid) {
    if (pid <= 0 || ::kill(pid, 0) != 0) return false;
    std::ifstream cmdline("/proc/" + std::to_string(pid) + "/cmdline", std::ios::binary);
    std::string executable;
    std::getline(cmdline, executable, '\0');
    return executable.find("coreflowd") != std::string::npos;
}
pid_t daemon_pid() {
    std::ifstream file(kPidPath);
    long value = 0;
    if (!(file >> value) || value < 1 || value > 2147483647) return 0;
    const auto pid = static_cast<pid_t>(value);
    return process_matches(pid) ? pid : 0;
}
bool journal_clear() {
    std::ifstream journal(std::string(kStateDir) + "/baseline.journal");
    if (!journal) return true;
    std::string header, line;
    if (!std::getline(journal, header) || header.rfind("CFJ1\t", 0) != 0) return false;
    return !std::getline(journal, line);
}
int stop_daemon() {
    const pid_t pid = daemon_pid();
    if (pid) {
        if (::kill(pid, SIGTERM) != 0) return 1;
        for (int attempt = 0; attempt < 80; ++attempt) {
            if (!process_matches(pid)) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (process_matches(pid)) return 1;
    }
    // If the daemon died before receiving SIGTERM, recover its durable journal.
    if (!BaselineStore::instance().recover_pending()) return 1;
    return journal_clear() ? 0 : 1;
}
void print_status() {
    const pid_t pid = daemon_pid();
    const float temp = ThermalMonitor::instance().hottest_celsius();
    UclampGovernor governor;
    std::cout << "{\"running\":" << (pid ? "true" : "false")
              << ",\"pid\":" << pid
              << ",\"uclampMin\":" << governor.get_top_app_min()
              << ",\"uclampMax\":" << governor.get_top_app_max()
              << ",\"uclampSupported\":" << (governor.is_supported() ? "true" : "false")
              << ",\"cgroupV2\":" << (std::filesystem::exists("/sys/fs/cgroup/cgroup.controllers") ? "true" : "false")
              << ",\"socTempCelsius\":";
    if (temp >= 0.0f) std::cout << temp;
    else std::cout << "null";
    std::cout << ",\"recoveryPending\":" << (journal_clear() ? "false" : "true") << "}\n";
}
bool write_pid(UniqueFd& lock) {
    if (::mkdir(kStateDir, 0700) != 0 && errno != EEXIST) return false;
    lock.reset(::open(kPidPath, O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600));
    if (!lock.valid() || ::flock(lock.get(), LOCK_EX | LOCK_NB) != 0) return false;
    const auto pid = std::to_string(::getpid());
    if (::ftruncate(lock.get(), 0) != 0) return false;
    if (::lseek(lock.get(), 0, SEEK_SET) < 0) return false;
    if (::write(lock.get(), pid.data(), pid.size()) != static_cast<ssize_t>(pid.size())) return false;
    return ::fsync(lock.get()) == 0;
}
}

int main(int argc, char* argv[]) {
    if (argc == 2) {
        const std::string command(argv[1]);
        if (command == "--status") { print_status(); return 0; }
        if (command == "--stop" || command == "--rollback-baseline") return stop_daemon();
        std::cerr << "Usage: coreflowd [--status|--stop|--rollback-baseline]\n";
        return 2;
    }
    if (argc != 1) return 2;

    std::signal(SIGSEGV, on_fatal_signal);
    std::signal(SIGBUS, on_fatal_signal);
    std::signal(SIGABRT, on_fatal_signal);

    if (marker("/data/adb/coreflow/SAFE_MODE") ||
        marker("/data/adb/coreflow/DISABLE") ||
        marker("/data/adb/modules/coreflow_engine/disable")) {
        std::cerr << "[CoreFlow] Safety marker active; refusing runtime mutation.\n";
        return 2;
    }
    UniqueFd pid_lock;
    if (!write_pid(pid_lock)) {
        std::cerr << "[CoreFlow] Unable to acquire daemon singleton lock.\n";
        return 1;
    }
    auto& baseline = BaselineStore::instance();
    if (!baseline.recover_pending()) {
        std::cerr << "[CoreFlow] Incomplete crash recovery. Refusing new mutations.\n";
        ::unlink(kPidPath);
        return 1;
    }

    const DaemonConfig config = DaemonConfig::load_best_available();
    UclampGovernor governor;
    EpollReactor reactor;

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);
    if (::pthread_sigmask(SIG_BLOCK, &mask, nullptr) != 0) return 1;
    UniqueFd signal_fd(::signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC));
    if (!signal_fd.valid() ||
        !reactor.register_fd(signal_fd.get(), EPOLLIN, [&](uint32_t) {
            signalfd_siginfo info{};
            const ssize_t consumed = ::read(signal_fd.get(), &info, sizeof(info));
            if (consumed == static_cast<ssize_t>(sizeof(info)))
                reactor.stop();
        })) return 1;

    bool applied = false;
    WorkloadState current = WorkloadState::Balanced;
    uint32_t consecutive_failures = 0;
    auto retry_after = std::chrono::steady_clock::time_point::min();
    const auto interval = std::chrono::milliseconds(
        std::clamp(config.temp_monitor_interval_ms, 1000u, 60000u));
    if (reactor.create_timerfd(interval, [&](uint32_t) {
        if (!config.enable_uclamp || !governor.is_supported()) return;
        if (marker("/data/adb/coreflow/SAFE_MODE") ||
            marker("/data/adb/coreflow/DISABLE") ||
            marker("/data/adb/modules/coreflow_engine/disable")) {
            reactor.stop();
            return;
        }
        const float temp = ThermalMonitor::instance().hottest_celsius();
        if (temp < 0.0f) {
            // No trusted thermal sensor: do not apply speculative performance boost.
            if (applied) {
                baseline.rollback_all();
                applied = false;
                if (baseline.has_active_mutations()) reactor.stop();
            }
            return;
        }
        const PressureSnapshot pressure{
            .cpu = read_psi_avg10("/proc/pressure/cpu"),
            .memory = read_psi_avg10("/proc/pressure/memory"),
            .io = read_psi_avg10("/proc/pressure/io"),
        };

        // Severe heat means CoreFlow gets out of the way and returns ownership
        // to the OEM thermal stack instead of fighting it with another profile.
        if (emergency_thermal_exit(temp, config)) {
            std::cerr << "[CoreFlow][Stability] emergency thermal exit at " << temp << "C\n";
            baseline.rollback_all();
            applied = false;
            reactor.stop();
            return;
        }

        const WorkloadState desired =
            select_stability_state(temp, pressure, current, applied, config);

        if (!applied || current != desired) {
            const auto now = std::chrono::steady_clock::now();
            if (now < retry_after) return;
            if (governor.apply_state(desired,
                                     config.top_app_interactive_min, config.top_app_interactive_max)) {
                current = desired;
                applied = true;
                consecutive_failures = 0;
                std::cerr << "[CoreFlow][Stability] state=" << state_name(current)
                          << " temp=" << temp
                          << " cpu_psi=" << pressure.cpu
                          << " mem_psi=" << pressure.memory
                          << " io_psi=" << pressure.io << "\n";
            } else {
                baseline.rollback_all();
                applied = false;
                if (baseline.has_active_mutations()) {
                    reactor.stop(); // Fail closed when recovery was incomplete.
                    return;
                }
                consecutive_failures = std::min(consecutive_failures + 1, 8u);
                const auto delay_sec = std::min(300u, 1u << consecutive_failures);
                retry_after = now + std::chrono::seconds(delay_sec);
                std::cerr << "[CoreFlow] UClamp write failed; retry no sooner than "
                          << delay_sec << " seconds.\n";
            }
        }
    }) < 0) {
        ::unlink(kPidPath);
        return 1;
    }

    std::cout << "[CoreFlow] Modern v2 event reactor active; baseline recovery armed.\n";
    reactor.run();
    baseline.rollback_all();
    const bool restored = !baseline.has_active_mutations() && journal_clear();
    ::unlink(kPidPath);
    return restored ? 0 : 1;
}
