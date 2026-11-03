#pragma once
#include "coreflow/config.hpp"
#include "coreflow/types.hpp"

namespace coreflow {

struct PressureSnapshot {
    double cpu{0.0};
    double memory{0.0};
    double io{0.0};
};

[[nodiscard]] inline bool emergency_thermal_exit(float temp, const DaemonConfig& cfg) noexcept {
    return cfg.emergency_rollback_on_overheat &&
           temp >= cfg.thermal_trip_celsius + cfg.thermal_emergency_margin_celsius;
}

[[nodiscard]] inline WorkloadState select_stability_state(
    float temp,
    const PressureSnapshot& pressure,
    WorkloadState current,
    bool state_applied,
    const DaemonConfig& cfg) noexcept {

    if (temp >= cfg.thermal_trip_celsius ||
        (state_applied && current == WorkloadState::ThermalGuard &&
         temp > cfg.thermal_recover_celsius)) {
        return WorkloadState::ThermalGuard;
    }

    if (pressure.memory >= cfg.memory_pressure_guard_pct ||
        pressure.io >= cfg.io_pressure_guard_pct) {
        return WorkloadState::PressureGuard;
    }

    if (pressure.cpu >= cfg.cpu_pressure_boost_pct &&
        temp <= cfg.thermal_trip_celsius - cfg.thermal_headroom_celsius) {
        return WorkloadState::SustainedLoad;
    }

    return WorkloadState::Balanced;
}

} // namespace coreflow
