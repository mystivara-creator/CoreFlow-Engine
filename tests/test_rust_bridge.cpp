#include "coreflow-engine/src/bridge.rs.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    using namespace coreflow::engine;

    // 1. Rust PID + PSI Controller
    auto controller = new_controller({2.5, 0.15, 1.2, 70});
    Snapshot input{10, 1, 30, 0, 0, true, true, true, 85, 65, 15, 25, 15, 15, true};
    auto decision = controller->step(input);
    assert(decision.valid && decision.state == 1);
    input.thermal_pressure = 95;
    decision = controller->step(input);
    assert(decision.valid && decision.state == 3);
    assert(std::abs(decision.capacity_scale - 0.98) < 1e-9);
    input.thermal_pressure = NAN;
    assert(!controller->step(input).valid);

    // 2. Rust EAS UClamp Planner + Thermal-Velocity Pre-Damping
    UclampInput u_steady{3, 3, 1.0, false, 384, 768, 1024, 120, 8.0, 60.0, 0.45, 0.0};
    auto t_steady = compute_uclamp_target(u_steady);
    assert(t_steady.util_min > 0 && t_steady.util_min <= t_steady.util_max && t_steady.util_max == 1024);
    UclampInput u_heating = u_steady;
    u_heating.thermal_velocity_cpm = 5.5;
    auto t_damped = compute_uclamp_target(u_heating);
    assert(t_damped.util_min < t_steady.util_min);

    // 3. Rust OLS ThermalTrendAnalyzer Ring Buffer
    auto trend = new_thermal_trend_analyzer();
    ThermalTrendResult res{0.0, 100, -1.0};
    for (uint64_t i = 0; i < 8; ++i) {
        res = trend->push_sample(1000ULL + i * 1000ULL, 40.0 + static_cast<double>(i) * 0.1, 55.0);
    }
    assert(std::abs(res.velocity_cpm - 6.0) < 0.05);
    assert(res.stability_score < 60U);
    assert(res.seconds_to_trip > 100.0);
    trend->reset();

    std::cout << "Rust bridge integration tests passed (PID + UClamp + OLS ThermalTrend)\n";
    return 0;
}
