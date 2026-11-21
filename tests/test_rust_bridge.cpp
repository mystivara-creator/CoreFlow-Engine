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

    // 2. Rust EAS UClamp Planner + Thermal-Velocity Pre-Damping + Closed-Loop Jank & App DNA
    UclampInput u_steady{3, 3, 1.0, false, 384, 768, 1024, 120, 8.0, 60.0, 0.45, 0.0, -1.0, 1.0};
    auto t_steady = compute_uclamp_target(u_steady);
    assert(t_steady.util_min > 0 && t_steady.util_min <= t_steady.util_max && t_steady.util_max == 1024);
    UclampInput u_heating = u_steady;
    u_heating.thermal_velocity_cpm = 5.5;
    auto t_damped = compute_uclamp_target(u_heating);
    assert(t_damped.util_min < t_steady.util_min);

    UclampInput u_interactive{1, 0, 1.0, false, 384, 768, 1024, 120, 5.0, 20.0, 0.25, 0.0, -1.0, 1.0};
    auto t_bal = compute_uclamp_target(u_interactive);

    UclampInput u_smooth = u_interactive;
    u_smooth.jank_ratio = 0.0;
    u_smooth.app_uclamp_bias = 0.85;
    auto t_smooth = compute_uclamp_target(u_smooth);
    assert(t_smooth.util_min < t_bal.util_min);

    UclampInput u_jank = u_interactive;
    u_jank.jank_ratio = 0.14;
    u_jank.app_uclamp_bias = 1.12;
    auto t_jank = compute_uclamp_target(u_jank);
    assert(t_jank.util_min > t_bal.util_min && t_jank.util_min <= 768U);

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

    // 4. Rust Dual-Timescale Thermal Equilibrium (burst tolerance + charging vs bypass)
    auto eq_burst = compute_thermal_equilibrium({72.0, 34.0, 35.0, 1.0, 0.96, false, false, true});
    auto eq_sustained = compute_thermal_equilibrium({72.0, 34.0, 35.0, 1.0, 0.96, false, false, false});
    assert(eq_burst.effective_scale > eq_sustained.effective_scale && !eq_burst.charging_damped && !eq_burst.equilibrium_guard);
    auto eq_chg = compute_thermal_equilibrium({52.0, 41.0, 41.0, 1.2, 0.90, true, false, false});
    auto eq_byp = compute_thermal_equilibrium({52.0, 41.0, 41.0, 1.2, 0.90, true, true, false});
    assert(eq_chg.charging_damped && !eq_byp.charging_damped);
    assert(eq_chg.effective_scale < eq_byp.effective_scale);

    // 5. Rust Markov AppPreloader + Cross-Reboot export_state/import_state
    auto preloader = new_app_preloader();
    for (int i = 0; i < 4; ++i) {
        preloader->record_launch(0x101ULL);
        preloader->record_launch(0x202ULL);
    }
    auto pred = preloader->predict_next(0x101ULL);
    assert(pred.has_prediction && pred.target_hash == 0x202ULL && pred.confidence >= 0.99);
    const auto exported = preloader->export_state();
    assert(exported.size() > 5U);
    auto restored = new_app_preloader();
    assert(restored->import_state(rust::Str(exported.data(), exported.size())));
    auto pred2 = restored->predict_next(0x101ULL);
    assert(pred2.has_prediction && pred2.target_hash == 0x202ULL && pred2.confidence >= 0.99);
    preloader->reset();

    std::cout << "Rust bridge integration tests passed (PID + Closed-Loop UClamp + Dual-Timescale Equilibrium + OLS + Persistent Preloader)\n";
    return 0;
}
