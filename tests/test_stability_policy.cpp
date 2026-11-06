#include "coreflow/stability_policy.hpp"
#include <cassert>
#include <iostream>

using namespace coreflow;

int main() {
    DaemonConfig cfg;
    cfg.thermal_trip_celsius = 44.0f;
    cfg.thermal_recover_celsius = 40.0f;
    cfg.thermal_headroom_celsius = 3.0f;
    cfg.thermal_emergency_margin_celsius = 4.0f;
    cfg.cpu_pressure_boost_pct = 8.0f;
    cfg.memory_pressure_guard_pct = 3.0f;
    cfg.io_pressure_guard_pct = 5.0f;

    PressureSnapshot quiet{.cpu = 1.0, .memory = 0.0, .io = 0.0};
    assert(select_stability_state(35.0f, quiet, WorkloadState::Balanced, true, cfg) ==
           WorkloadState::Balanced);

    PressureSnapshot cpu_load{.cpu = 12.0, .memory = 0.5, .io = 1.0};
    assert(select_stability_state(38.0f, cpu_load, WorkloadState::Balanced, true, cfg) ==
           WorkloadState::SustainedLoad);

    PressureSnapshot memory_load{.cpu = 15.0, .memory = 4.0, .io = 1.0};
    assert(select_stability_state(38.0f, memory_load, WorkloadState::Balanced, true, cfg) ==
           WorkloadState::PressureGuard);

    PressureSnapshot io_load{.cpu = 15.0, .memory = 0.0, .io = 7.0};
    assert(select_stability_state(38.0f, io_load, WorkloadState::Balanced, true, cfg) ==
           WorkloadState::PressureGuard);

    assert(select_stability_state(44.0f, quiet, WorkloadState::Balanced, true, cfg) ==
           WorkloadState::ThermalGuard);
    assert(select_stability_state(42.0f, quiet, WorkloadState::ThermalGuard, true, cfg) ==
           WorkloadState::ThermalGuard);
    assert(select_stability_state(39.5f, quiet, WorkloadState::ThermalGuard, true, cfg) ==
           WorkloadState::Balanced);

    PressureSnapshot unavailable{.cpu = 0.0, .memory = 0.0, .io = 0.0,
                                 .cpu_valid = false, .memory_valid = false, .io_valid = false};
    assert(select_stability_state(38.0f, unavailable, WorkloadState::Balanced, false, cfg) ==
           WorkloadState::Balanced);
    unavailable.memory = 99.0;
    assert(select_stability_state(38.0f, unavailable, WorkloadState::Balanced, false, cfg) ==
           WorkloadState::Balanced);
    unavailable.memory_valid = true;
    assert(select_stability_state(38.0f, unavailable, WorkloadState::Balanced, false, cfg) ==
           WorkloadState::PressureGuard);

    assert(!emergency_thermal_exit(47.9f, cfg));
    assert(emergency_thermal_exit(48.0f, cfg));

    std::cout << "Four-state stability policy PASS\n";
    return 0;
}
