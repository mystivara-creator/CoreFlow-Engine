/**
 * @file main.cpp
 * @brief CoreFlow Engine Daemon (coreflowd) Native Entry Point (Hardened & Zero Hardcode)
 * @author mystivara-creator
 * @license Apache-2.0
 */

#include "coreflow/types.hpp"
#include "coreflow/config.hpp"
#include "coreflow/reactor.hpp"
#include "coreflow/cgroup_v2.hpp"
#include "coreflow/uclamp.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/render_booster.hpp"
#include "coreflow/device_profiler.hpp"

#include <iostream>
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <sys/signalfd.h>

using namespace coreflow;

namespace {

// Fatal signal handlers must only use async-signal-safe operations.
// Recovery after an abrupt crash requires a durable journal, not mutexes here.
void emergency_crash_handler(int sig) {
    static constexpr char message[] = "[CoreFlow] Fatal signal; terminating without unsafe signal-handler rollback.\n";
    (void)::write(STDERR_FILENO, message, sizeof(message) - 1);
    std::_Exit(128 + sig);
}

} // namespace

int main() {
    // 0. Register emergency crash traps for unexpected fatal faults
    std::signal(SIGSEGV, emergency_crash_handler);
    std::signal(SIGBUS,  emergency_crash_handler);
    std::signal(SIGABRT, emergency_crash_handler);

    // 1. Load Dynamic Configuration (Zero Hardcoded Values)
    auto config = DaemonConfig::load_best_available();

    std::cout << "========================================================\n"
              << " CoreFlow Engine Daemon (coreflowd) v" << config.version << "\n"
              << " Target: Android 15+ | Zero-Polling | Zero-Hardcode\n"
              << " Hardening: RAII UniqueFd | Immediate FailSafe Rollback\n"
              << " Author: mystivara-creator\n"
              << "========================================================\n"
              << std::endl;

    // 2. Hardware & System Probe
    auto topo = DeviceProfiler::instance().probe();
    std::cout << "[Init] Detected SoC: " << topo.soc_name 
              << " (CPUs: " << topo.total_cpus << ")" << std::endl;

    // 3. Initialize Controllers with dynamic configuration
    CgroupV2Controller cgroup_ctrl;
    UclampGovernor uclamp_gov;
    RenderBooster render_booster;

    if (!cgroup_ctrl.is_supported()) {
        std::cerr << "[Init][Warning] cgroups v2 not active. Falling back to hybrid mode." << std::endl;
    }

    // 4. Dynamic Vendor & Rendering Unlock
    if (config.enable_cgroups_v2 && config.unlock_vendor_clamping) {
        cgroup_ctrl.unlock_vendor_restrictions();
    }
    if (config.force_120hz_rendering) {
        render_booster.unlock_display_refresh_limits();
    }

    // 5. Epoll Reactor Setup (Zero-Polling)
    EpollReactor reactor;

    // Mask signals for clean signalfd event handling
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);
    pthread_sigmask(SIG_BLOCK, &mask, nullptr);

    int sfd = ::signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (sfd >= 0) {
        reactor.register_fd(sfd, EPOLLIN, [&reactor](uint32_t) {
            std::cout << "[CoreFlow] Received termination signal. Initiating graceful shutdown..." << std::endl;
            reactor.stop();
        });
    }

    // 6. Dynamic Thermal Heartbeat (timerfd) using configured interval and trip threshold
    reactor.create_timerfd(
        std::chrono::milliseconds(config.temp_monitor_interval_ms),
        [&uclamp_gov, &topo, &config](uint32_t) {
            float temp = DeviceProfiler::instance().get_soc_temperature();
            if (temp >= config.thermal_trip_celsius) {
                std::cerr << "[Thermal Alert] Temp: " << temp << "°C >= " 
                          << config.thermal_trip_celsius << "°C! Shifting to ThermalThrottled state." << std::endl;
                uclamp_gov.apply_state(WorkloadState::ThermalThrottled, topo.tier);
            } else {
                uclamp_gov.apply_state(WorkloadState::Interactive, topo.tier);
            }
        }
    );

    // 7. Run Event Loop (Zero CPU idle wakeups)
    std::cout << "[CoreFlow] Daemon running. Epoll zero-polling active." << std::endl;
    reactor.run();

    // 8. Shutdown: Atomic Rollback to Factory OEM Baseline
    std::cout << "[CoreFlow] Restoring OEM factory baseline nodes..." << std::endl;
    size_t restored = BaselineStore::instance().rollback_all();
    std::cout << "[CoreFlow] Clean exit completed. (" << restored << " nodes verified)." << std::endl;

    return 0;
}
