/**
 * @file main.cpp
 * @brief CoreFlow Engine Daemon (coreflowd) Native Entry Point
 * @author mystivara-creator
 * @license Apache-2.0
 */

#include "coreflow/types.hpp"
#include "coreflow/reactor.hpp"
#include "coreflow/cgroup_v2.hpp"
#include "coreflow/uclamp.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/render_booster.hpp"
#include "coreflow/device_profiler.hpp"

#include <iostream>
#include <csignal>
#include <sys/signalfd.h>

using namespace coreflow;

int main(int argc, char* argv[]) {
    std::cout << "========================================================\n"
              << " CoreFlow Engine Daemon (coreflowd) v2.0-MODERN\n"
              << " Target: Android 15+ | cgroups v2 | EAS Uclamp\n"
              << " Author: mystivara-creator\n"
              << "========================================================\n"
              << std::endl;

    // 1. Hardware & System Probe
    auto topo = DeviceProfiler::instance().probe();
    std::cout << "[Init] Detected SoC: " << topo.soc_name 
              << " (CPUs: " << topo.total_cpus << ")" << std::endl;

    // 2. Initialize Controllers
    CgroupV2Controller cgroup_ctrl;
    UclampGovernor uclamp_gov;
    RenderBooster render_booster;

    if (!cgroup_ctrl.is_supported()) {
        std::cerr << "[Init][Warning] cgroups v2 not active. Falling back to hybrid mode." << std::endl;
    }

    // 3. Unlock OEM Throttling Restrictions
    cgroup_ctrl.unlock_vendor_restrictions();
    render_booster.unlock_display_refresh_limits();

    // 4. Epoll Reactor Setup (Zero-Polling)
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

    // 5. Register 1000ms Safety Heartbeat (timerfd) to monitor thermals without busy looping
    reactor.create_timerfd(std::chrono::milliseconds(1000), [&uclamp_gov, &topo](uint32_t) {
        float temp = DeviceProfiler::instance().get_soc_temperature();
        if (temp >= 44.0f) {
            std::cerr << "[Thermal Alert] Temp: " << temp << "°C! Shifting to ThermalThrottled state." << std::endl;
            uclamp_gov.apply_state(WorkloadState::ThermalThrottled, topo.tier);
        } else {
            uclamp_gov.apply_state(WorkloadState::Interactive, topo.tier);
        }
    });

    // 6. Run Event Loop (Zero CPU idle wakeups)
    reactor.run();

    // 7. Shutdown: Atomic Rollback to Factory OEM Baseline
    std::cout << "[CoreFlow] Restoring OEM factory baseline nodes..." << std::endl;
    size_t restored = BaselineStore::instance().rollback_all();
    std::cout << "[CoreFlow] Clean exit completed. (" << restored << " nodes verified)." << std::endl;

    return 0;
}
