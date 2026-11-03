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
#include "coreflow/thermal_monitor.hpp"
#include "coreflow/pid_governor.hpp"
#include "coreflow/hardware_longevity.hpp"

#include <iostream>
#include <csignal>
#include <cstdlib>
#include <sys/signalfd.h>

using namespace coreflow;

namespace {

// Emergency Crash Signal Trap: Guaranteed OEM baseline restore on fatal fault
void emergency_crash_handler(int sig) {
    std::cerr << "\n[CoreFlow][EMERGENCY] Fatal signal " << sig 
              << " caught. Restoring OEM factory baseline before termination..." << std::endl;
    BaselineStore::instance().rollback_all();
    std::_Exit(128 + sig);
}

} // namespace

int main(int argc, char* argv[]) {
    // 0. Handle command-line options
    if (argc > 1) {
        const std::string_view arg(argv[1]);
        if (arg == "--rollback-baseline" || arg == "--rollback") {
            size_t restored = BaselineStore::instance().rollback_all();
            std::cout << "[CoreFlow] Restored " << restored << " modified nodes to OEM baseline.\n";
            return 0;
        }
        if (arg == "--status") {
            bool modified = BaselineStore::instance().has_active_mutations();
            std::cout << "[CoreFlow] Status: " << (modified ? "Active mutations present" : "Clean (OEM baseline)") << "\n";
            return 0;
        }
        if (arg == "--recover") {
            return BaselineStore::instance().recover_pending() ? 0 : 1;
        }
    }

    // 1. Register emergency crash traps for unexpected fatal faults
    std::signal(SIGSEGV, emergency_crash_handler);
    std::signal(SIGBUS,  emergency_crash_handler);
    std::signal(SIGABRT, emergency_crash_handler);

    // 2. Recover any pending uncommitted mutations from previous daemon session
    BaselineStore::instance().recover_pending();

    // 3. Load Dynamic Configuration (Zero Hardcoded Values)
    auto config = DaemonConfig::load_best_available();

    std::cout << "========================================================\n"
              << " CoreFlow Engine Daemon (coreflowd) v" << config.version << "\n"
              << " Target: Android 15+ | Zero-Polling | Zero-Hardcode\n"
              << " Hardening: RAII UniqueFd | FailSafe Rollback | PID Equilibrium\n"
              << " Author: mystivara-creator\n"
              << "========================================================\n"
              << std::endl;

    // 4. Hardware & System Probe
    auto topo = DeviceProfiler::instance().probe();
    std::cout << "[Init] Detected Hardware: " << topo.soc_name 
              << " (CPUs: " << topo.online_cpus_mask << ", Kernel: " << topo.kernel_version << ")" << std::endl;

    // 5. Initialize Controllers with dynamic configuration
    CgroupV2Controller cgroup_ctrl;
    UclampGovernor uclamp_gov;
    RenderBooster render_booster;

    if (!cgroup_ctrl.is_supported()) {
        std::cerr << "[Init][Warning] cgroups v2 not active. Falling back to hybrid mode." << std::endl;
    }

    // 6. Initialize Hardware Longevity (Battery Step-Charging & UFS Storage Pacing)
    LongevityConfig longevity_cfg{
        .enable_battery_step_charging = config.enable_battery_step_charging,
        .enable_ufs_burst_pacing = config.enable_ufs_burst_pacing,
        .battery_step_temp_threshold = config.battery_step_temp_threshold,
        .stepped_charge_current_ma = config.stepped_charge_current_ma,
        .top_app_io_weight = config.top_app_io_weight,
        .background_io_weight = config.background_io_weight
    };
    HardwareLongevityManager::instance().initialize(longevity_cfg);

    // 7. Initialize Predictive PID Equilibrium Governor
    PidParameters pid_params{
        .kp = config.pid_kp,
        .ki = config.pid_ki,
        .kd = config.pid_kd,
        .target_temp = config.pid_target_celsius
    };
    PidEquilibriumGovernor pid_gov(pid_params);

    // 8. Dynamic Vendor & Rendering Unlock
    if (config.unlock_vendor_clamping) {
        cgroup_ctrl.unlock_vendor_restrictions();
    }
    if (config.force_120hz_rendering) {
        render_booster.unlock_display_refresh_limits();
    }

    // 9. Epoll Reactor Setup (Zero-Polling)
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

    // 10. Dynamic Predictive PID Heartbeat (timerfd)
    auto interval = std::chrono::milliseconds(config.temp_monitor_interval_ms);
    reactor.create_timerfd(
        interval,
        [&uclamp_gov, &pid_gov, &config, interval](uint32_t) {
            float temp = DeviceProfiler::instance().get_soc_temperature();
            float monitor_temp = ThermalMonitor::instance().hottest_celsius();
            if (monitor_temp > 0.0f) {
                temp = std::max(temp, monitor_temp);
            }

            // Evaluate battery step-charging longevity
            HardwareLongevityManager::instance().monitor_battery_thermals(temp);

            if (config.enable_pid_equilibrium) {
                // Continuous PID Dynamic Equilibrium: micro-adjusts capacity smoothly
                float budget = pid_gov.compute_thermal_budget(temp, interval);
                if (budget < 0.65f) {
                    uclamp_gov.apply_state(WorkloadState::ThermalGuard,
                                           config.top_app_interactive_min,
                                           config.top_app_interactive_max);
                } else {
                    uclamp_gov.apply_state(WorkloadState::Balanced,
                                           config.top_app_interactive_min,
                                           config.top_app_interactive_max);
                }
            } else {
                // Threshold mode
                if (temp >= config.thermal_trip_celsius) {
                    uclamp_gov.apply_state(WorkloadState::ThermalGuard,
                                           config.top_app_interactive_min,
                                           config.top_app_interactive_max);
                } else {
                    uclamp_gov.apply_state(WorkloadState::Balanced,
                                           config.top_app_interactive_min,
                                           config.top_app_interactive_max);
                }
            }
        }
    );

    // 11. Run Event Loop (Zero CPU idle wakeups)
    std::cout << "[CoreFlow] Daemon running. Epoll zero-polling & PID equilibrium active." << std::endl;
    reactor.run();

    // 12. Shutdown: Verified Rollback to Factory OEM Baseline
    std::cout << "[CoreFlow] Restoring OEM factory baseline nodes..." << std::endl;
    size_t restored = BaselineStore::instance().rollback_all();
    std::cout << "[CoreFlow] Clean exit completed. (" << restored << " nodes verified)." << std::endl;

    return 0;
}
