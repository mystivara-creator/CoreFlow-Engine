#include "coreflow/types.hpp"
#include "coreflow/config.hpp"
#include "coreflow/stability_policy.hpp"
#include "coreflow/reactor.hpp"
#include "coreflow/uclamp.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/thermal_monitor.hpp"
#include "coreflow/cooling_feedback.hpp"
#include "coreflow/unique_fd.hpp"
#include "coreflow/frequency_controller.hpp"
#include "coreflow/power_bridge.hpp"
#include "coreflow/device_profiler.hpp"
#include "coreflow/hardware_longevity.hpp"
#include "coreflow-engine/src/bridge.rs.h"

#include <algorithm>
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
#include <optional>
#include <pthread.h>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/file.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <thread>
#include <unistd.h>

#ifdef __ANDROID__
#include <android/log.h>
#endif

using namespace coreflow;

namespace {

constexpr char kStateDir[] = "/data/adb/coreflow";
constexpr char kPidPath[] = "/data/adb/coreflow/coreflowd.pid";

void emit_log(bool is_error, const std::string& message) {
    if (is_error) {
        std::cerr << message << "\n";
    } else {
        std::cout << message << "\n";
    }
#ifdef __ANDROID__
    __android_log_write(
        is_error ? ANDROID_LOG_WARN : ANDROID_LOG_INFO,
        "CoreFlow",
        message.c_str());
#endif
}

// Fatal signal handlers cannot lock mutexes, perform stream I/O, or allocate.
void on_fatal_signal(int signum) {
    constexpr char line[] = "[CoreFlow] Fatal signal. Next start will recover journal.\n";
    const ssize_t emitted = ::write(STDERR_FILENO, line, sizeof(line) - 1);
    (void)emitted;
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
    fatal_sa.sa_flags = SA_RESETHAND;
    (void)::sigaction(SIGSEGV, &fatal_sa, nullptr);
    (void)::sigaction(SIGBUS, &fatal_sa, nullptr);
    (void)::sigaction(SIGABRT, &fatal_sa, nullptr);
    (void)::sigaction(SIGILL, &fatal_sa, nullptr);
    (void)::sigaction(SIGFPE, &fatal_sa, nullptr);
}

bool marker(const char* path) noexcept {
    struct stat st{};
    return ::lstat(path, &st) == 0;
}

bool safety_halt_requested() noexcept {
    return marker("/data/adb/coreflow/SAFE_MODE") ||
           marker("/data/adb/coreflow/DISABLE") ||
           marker("/data/adb/modules/coreflow_engine/disable");
}

std::optional<double> read_psi_avg10(const char* path) {
    UniqueFd fd(::open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return std::nullopt;

    char buf[512];
    ssize_t n = 0;
    do {
        n = ::read(fd.get(), buf, sizeof(buf));
    } while (n < 0 && errno == EINTR);
    if (n <= 0) return std::nullopt;

    std::string_view content(buf, static_cast<size_t>(n));
    size_t pos = 0;
    while (pos < content.size()) {
        const size_t eol = content.find('\n', pos);
        const std::string_view line =
            eol == std::string_view::npos ? content.substr(pos) : content.substr(pos, eol - pos);
        pos = eol == std::string_view::npos ? content.size() : eol + 1;

        if (line.rfind("some ", 0) != 0) continue;
        const size_t at = line.find("avg10=");
        if (at == std::string_view::npos) continue;
        const std::string_view rest = line.substr(at + 6);
        const size_t end_tok = rest.find_first_of(" \t\r");
        const std::string_view token =
            end_tok == std::string_view::npos ? rest : rest.substr(0, end_tok);
        double value = 0.0;
        if (parse_ascii_double(token, value) && value >= 0.0 && value <= 100.0) {
            return value;
        }
    }
    return std::nullopt;
}

const char* state_name(WorkloadState state) noexcept {
    switch (state) {
        case WorkloadState::Balanced: return "balanced";
        case WorkloadState::SustainedLoad: return "sustained-load";
        case WorkloadState::PressureGuard: return "pressure-guard";
        case WorkloadState::ThermalGuard: return "thermal-guard";
    }
    return "unknown";
}

bool process_matches(pid_t pid) {
    if (pid <= 0) return false;
    if (::kill(pid, 0) != 0 && errno != EPERM) return false;

    const std::string cmd_path = "/proc/" + std::to_string(pid) + "/cmdline";
    UniqueFd fd(::open(cmd_path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return false;

    char buf[256];
    ssize_t n = 0;
    do {
        n = ::read(fd.get(), buf, sizeof(buf));
    } while (n < 0 && errno == EINTR);
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
    do {
        n = ::read(fd.get(), buf, sizeof(buf));
    } while (n < 0 && errno == EINTR);
    if (n <= 0) return 0;

    const std::string trimmed = DaemonConfig::trim(std::string(buf, static_cast<size_t>(n)));
    if (trimmed.empty()) return 0;

    long value = 0;
    const auto parsed = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != trimmed.data() + trimmed.size()) return 0;
    if (value < 1 || value > 2147483647L) return 0;

    const auto pid = static_cast<pid_t>(value);
    return process_matches(pid) ? pid : 0;
}

bool journal_clear() {
    const std::string jpath = std::string(kStateDir) + "/baseline.journal";
    UniqueFd fd(::open(jpath.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) {
        return errno == ENOENT;
    }

    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0 || st.st_size > 65536) {
        return false;
    }

    char buf[4096];
    std::string raw;
    while (raw.size() <= 65536) {
        const ssize_t n = ::read(fd.get(), buf, sizeof(buf));
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) break;
        raw.append(buf, static_cast<size_t>(n));
    }
    if (raw.size() > 65536) return false;

    std::istringstream journal(raw);
    std::string header;
    if (!std::getline(journal, header) ||
        (header.rfind("CFJ1\t", 0) != 0 && header.rfind("CFJ2\t", 0) != 0 &&
         header.rfind("CFJ3\t", 0) != 0)) return false;
    std::string line;
    while (std::getline(journal, line)) {
        if (!DaemonConfig::trim(line).empty()) return false;
    }
    return true;
}

int stop_daemon() {
    const pid_t pid = daemon_pid();
    if (pid > 0) {
        if (::kill(pid, SIGTERM) != 0 && errno != ESRCH) return 1;
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

std::string json_string(std::string_view text);

void print_status() {
    const pid_t pid = daemon_pid();
    const float temp = ThermalMonitor::instance().hottest_celsius();
    const auto thermal = ThermalMonitor::instance().assess(false);
    UclampGovernor governor;
    std::cout << "{\"running\":" << (pid ? "true" : "false")
              << ",\"pid\":" << pid
              << ",\"uclampMin\":" << governor.get_top_app_min()
              << ",\"uclampMax\":" << governor.get_top_app_max()
              << ",\"uclampSupported\":" << (governor.is_supported() ? "true" : "false")
              << ",\"socTempCelsius\":";
    if (std::isfinite(temp) && temp >= 0.0f) {
        char num[32];
        const int whole = static_cast<int>(temp);
        const int tenths = static_cast<int>(std::lround((temp - static_cast<float>(whole)) * 10.0f));
        if (tenths >= 10) {
            std::snprintf(num, sizeof(num), "%d.0", whole + 1);
        } else {
            std::snprintf(num, sizeof(num), "%d.%d", whole, std::max(0, tenths));
        }
        std::cout << num;
    } else {
        std::cout << "null";
    }
    std::cout << ",\"thermalPolicyAvailable\":" << (thermal.available ? "true" : "false")
              << ",\"thermalSource\":" << json_string(thermal.source)
              << ",\"thermalSensor\":" << json_string(thermal.sensor)
              << ",\"thermalSeverity\":" << thermal.severity
              << ",\"thermalPressure\":" << thermal.pressure;
    std::cout << ",\"thermalSensors\":[";
    bool first_sensor = true;
    for (const auto& reading : thermal.readings) {
        if (!first_sensor) std::cout << ',';
        first_sensor = false;
        std::cout << "{\"id\":" << json_string(reading.id)
                  << ",\"type\":" << json_string(reading.type)
                  << ",\"category\":" << reading.category
                  << ",\"valid\":" << (reading.valid ? "true" : "false")
                  << ",\"celsius\":" << (reading.valid ? std::to_string(reading.celsius) : "null")
                  << ",\"policyAvailable\":" << (reading.policy_available ? "true" : "false") << '}';
    }
    std::cout << "]";
    const auto battery = ThermalMonitor::instance().battery_celsius();
    const auto skin = ThermalMonitor::instance().skin_celsius();
    std::cout << ",\"batteryTempCelsius\":" << (battery >= 0 ? std::to_string(battery) : "null")
              << ",\"skinTempCelsius\":" << (skin >= 0 ? std::to_string(skin) : "null");
    std::cout << ",\"recoveryPending\":" << (journal_clear() ? "false" : "true") << "}\n";
}

std::string json_string(std::string_view text) {
    std::string out = "\"";
    for (unsigned char c : text) {
        if (c == '\"' || c == '\\') { out.push_back('\\'); out.push_back(static_cast<char>(c)); }
        else if (c >= 32 && c < 127) out.push_back(static_cast<char>(c));
        else out.push_back('?');
    }
    return out + "\"";
}

void print_capabilities() {
    const auto topology = DeviceProfiler::instance().probe();
    const auto power = PowerBridge{}.probe();
    std::cout << "{\"socModel\":" << json_string(topology.soc_name)
              << ",\"kernelVersion\":" << json_string(topology.kernel_version)
              << ",\"onlineCpus\":" << json_string(topology.online_cpus_mask)
              << ",\"gpuType\":" << json_string(topology.gpu_type)
              << ",\"rustEngine\":true,\"powerHalDetected\":" << (power.aidl_power_service ? "true" : "false")
              << ",\"binderInterception\":false,\"npuOffload\":false,\"frameTelemetry\":false}\n";
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
    if (::ftruncate(lock.get(), 0) != 0) return false;
    if (::lseek(lock.get(), 0, SEEK_SET) < 0) return false;
    if (::write(lock.get(), pid.data(), pid.size()) != static_cast<ssize_t>(pid.size())) return false;
    return ::fsync(lock.get()) == 0;
}

void clear_pid(const UniqueFd& lock) noexcept {
    if (!lock.valid()) return;
    if (::ftruncate(lock.get(), 0) == 0) {
        (void)::fsync(lock.get());
    }
}

} // namespace

int main(int argc, char* argv[]) {
    (void)::umask(0077);
    (void)::setenv("LC_ALL", "C", 1);
    (void)std::setlocale(LC_ALL, "C");

    if (argc == 2) {
        const std::string_view command(argv[1]);
        if (command == "--capabilities") { print_capabilities(); return 0; }
        if (command == "--status") {
            print_status();
            return 0;
        }
        if (command == "--stop" || command == "--rollback-baseline") {
            return stop_daemon();
        }
        std::cerr << "Usage: coreflowd [--status|--stop|--rollback-baseline]\n";
        return 2;
    }
    if (argc != 1) {
        std::cerr << "Usage: coreflowd [--status|--stop|--rollback-baseline]\n";
        return 2;
    }

    install_signal_hardening();

    if (safety_halt_requested()) {
        emit_log(true, "[CoreFlow] Safety marker active; refusing runtime mutation.");
        return 2;
    }

    UniqueFd pid_lock;
    if (!write_pid(pid_lock)) {
        emit_log(true, "[CoreFlow] Unable to acquire daemon singleton lock.");
        return 1;
    }

    auto& baseline = BaselineStore::instance();
    if (!baseline.recover_pending()) {
        emit_log(true, "[CoreFlow] Incomplete crash recovery. Refusing new mutations.");
        clear_pid(pid_lock);
        return 1;
    }

    DaemonConfig config = DaemonConfig::load_best_available();
    UclampGovernor governor;
    FrequencyController frequencies;
    frequencies.discover(config.enable_cpu_frequency_control, config.enable_gpu_frequency_control);
    auto controller = engine::new_controller({config.pid_kp, config.pid_ki, config.pid_kd, 70.0});
    HardwareLongevityManager::instance().initialize({
        config.enable_battery_step_charging, config.enable_ufs_burst_pacing,
        config.battery_step_temp_threshold, config.stepped_charge_current_ma,
        config.top_app_io_weight, config.background_io_weight});
    const auto power = PowerBridge{}.probe();
    emit_log(false, "[CoreFlow] CPU policies=" + std::to_string(frequencies.cpu_count()) +
        " GPU domains=" + std::to_string(frequencies.gpu_count()) +
        " Power HAL detected=" + std::to_string(power.aidl_power_service) +
        "; Binder interception and NPU offload unavailable without platform integration.");
    EpollReactor reactor;

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);
    if (::pthread_sigmask(SIG_BLOCK, &mask, nullptr) != 0) {
        clear_pid(pid_lock);
        return 1;
    }

    int timer_fd = -1;
    auto last_tick = std::chrono::steady_clock::now();
    bool applied = false;
    CoolingFeedback cooling_feedback;
    bool thermal_ready = false;
    bool unavailable_reported = false;
    WorkloadState current = WorkloadState::Balanced;

    UniqueFd signal_fd(::signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC));
    if (!signal_fd.valid() ||
        !reactor.register_fd(signal_fd.get(), EPOLLIN, [&](uint32_t) {
            signalfd_siginfo info{};
            while (::read(signal_fd.get(), &info, sizeof(info)) ==
                   static_cast<ssize_t>(sizeof(info))) {
                if (info.ssi_signo == SIGHUP) {
                    baseline.rollback_all();
                    if (baseline.has_active_mutations()) { reactor.stop(); continue; }
                    config = DaemonConfig::load_best_available();
                    governor = UclampGovernor{};
                    controller = engine::new_controller({config.pid_kp, config.pid_ki, config.pid_kd, 70.0});
                    frequencies.discover(config.enable_cpu_frequency_control, config.enable_gpu_frequency_control);
                    HardwareLongevityManager::instance().initialize({
                        config.enable_battery_step_charging, config.enable_ufs_burst_pacing,
                        config.battery_step_temp_threshold, config.stepped_charge_current_ma,
                        config.top_app_io_weight, config.background_io_weight});
                    itimerspec timer{};
                    timer.it_interval.tv_sec = config.temp_monitor_interval_ms / 1000;
                    timer.it_interval.tv_nsec = (config.temp_monitor_interval_ms % 1000) * 1000000;
                    timer.it_value = timer.it_interval;
                    if (::timerfd_settime(timer_fd, 0, &timer, nullptr) != 0) reactor.stop();
                    last_tick = std::chrono::steady_clock::now();
                    applied = false; thermal_ready = false; cooling_feedback.reset();
                    emit_log(false, "[CoreFlow] Configuration reloaded on SIGHUP.");
                } else {
                    reactor.stop();
                }
            }
        })) {
        clear_pid(pid_lock);
        return 1;
    }

    const auto interval = std::chrono::milliseconds(
        std::clamp(config.temp_monitor_interval_ms, 1000u, 60000u));
    timer_fd = reactor.create_timerfd(interval, [&](uint32_t) {
        if (safety_halt_requested()) {
            reactor.stop();
            return;
        }
        if (config.enable_uclamp && governor.is_supported() && !governor.owned_values_intact() && applied) {
            baseline.rollback_all();
            applied = false;
            reactor.stop();
            return;
        }

        const auto thermal = ThermalMonitor::instance().assess();
        const float temp = ThermalMonitor::instance().hottest_celsius(); // Telemetry only.
        if (!thermal.available) {
            last_tick = std::chrono::steady_clock::now();
            if (!unavailable_reported) {
                emit_log(true, "[CoreFlow][Thermal] OEM thermal policy unavailable; retaining baseline without synthetic thresholds.");
                unavailable_reported = true;
            }
            thermal_ready = false;
            cooling_feedback.reset();
            if (applied) {
                baseline.rollback_all();
                applied = false;
                controller->reset();
                governor = UclampGovernor{};
                if (baseline.has_active_mutations()) { reactor.stop(); return; }
                // Frequency ownership is intentionally not reacquired until reload.
                frequencies.discover(false, false);
            }
            return; // Remain available to status/stop; never infer safe headroom.
        }
        if (!thermal_ready) {
            frequencies.discover(config.enable_cpu_frequency_control, config.enable_gpu_frequency_control);
            controller->reset();
            thermal_ready = true; unavailable_reported = false;
        }
        if (thermal.emergency) {
            emit_log(true, "[CoreFlow][Stability] OEM thermal emergency source=" +
                     thermal.source + " sensor=" + thermal.sensor);
            baseline.rollback_all(); applied = false; reactor.stop(); return;
        }

        const auto cpu_psi = read_psi_avg10("/proc/pressure/cpu");
        const auto memory_psi = read_psi_avg10("/proc/pressure/memory");
        const auto io_psi = read_psi_avg10("/proc/pressure/io");
        if (!cpu_psi || !memory_psi || !io_psi) {
            if (applied) {
                baseline.rollback_all();
                applied = false;
                reactor.stop();
            }
            return;
        }
        const PressureSnapshot pressure{
            .cpu = *cpu_psi,
            .memory = *memory_psi,
            .io = *io_psi,
        };

        const auto now_tick = std::chrono::steady_clock::now();
        const double elapsed = std::chrono::duration<double>(now_tick - last_tick).count();
        last_tick = now_tick;
        const auto decision = controller->step({
            thermal.pressure, elapsed, pressure.cpu, pressure.memory, pressure.io,
            85.0, 65.0, 15.0, config.cpu_pressure_boost_pct,
            config.memory_pressure_guard_pct, config.io_pressure_guard_pct,
            config.enable_pid_equilibrium});
        if (!decision.valid) {
            baseline.rollback_all();
            reactor.stop();
            return;
        }
        const WorkloadState desired = static_cast<WorkloadState>(decision.state);
        // The Rust budget drives real CPU/GPU caps and the scheduler ceiling.
        if (!frequencies.apply(decision.capacity_scale)) {
            baseline.rollback_all();
            reactor.stop();
            return;
        }
        if (!HardwareLongevityManager::instance().apply_storage_pacing()) {
            baseline.rollback_all(); reactor.stop(); return;
        }
        HardwareLongevityManager::instance().monitor_battery_thermals(temp);
        const auto capacity = static_cast<uint32_t>(std::lround(decision.capacity_scale * 1024.0));
        if (config.enable_uclamp && governor.is_supported() && !governor.apply_budget(desired,
                config.top_app_interactive_min, config.top_app_interactive_max, capacity)) {
            baseline.rollback_all();
            reactor.stop();
            return;
        }
        const auto trend = cooling_feedback.sample(thermal,
            std::chrono::duration<double>(now_tick.time_since_epoch()).count(),
            decision.capacity_scale < 0.999 || desired == WorkloadState::ThermalGuard);
        if (trend) {
            emit_log(true, "[CoreFlow][Cooling] observed=" + *trend +
                " sensors=" + std::to_string(thermal.readings.size()) +
                " pressure=" + std::to_string(thermal.pressure) +
                " capacity=" + std::to_string(decision.capacity_scale));
        }
        if (!applied || current != desired) {
            {
                current = desired;
                applied = true;
                std::ostringstream line;
                line << "[CoreFlow][Stability] state=" << state_name(current)
                     << " compute_temp=" << temp
                     << " thermal_source=" << thermal.source
                     << " thermal_sensor=" << thermal.sensor
                     << " thermal_pressure=" << thermal.pressure
                     << " cpu_psi=" << pressure.cpu
                     << " mem_psi=" << pressure.memory
                     << " io_psi=" << pressure.io;
                emit_log(true, line.str());
            }
        }
    });
    if (timer_fd < 0) {
        clear_pid(pid_lock);
        return 1;
    }

    emit_log(false, "[CoreFlow] C++23 / Rust 2024 equilibrium active; verified baseline recovery armed.");
    reactor.run();
    baseline.rollback_all();
    const bool restored = !baseline.has_active_mutations() && journal_clear();
    clear_pid(pid_lock);
    return restored ? 0 : 1;
}
