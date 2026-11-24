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

    // 2. Rust EAS UClamp Planner + Thermal-Velocity Pre-Damping + Closed-Loop Jank, App DNA, Feed-Forward, Starvation & Phase 3 p99/EDP
    UclampInput u_steady{3, 3, 1.0, false, 384, 768, 1024, 120, 8.0, 60.0, 0.45, 0.0, -1.0, 1.0, false, 1.0, -1.0, -1.0, 0.0, 0.0};
    auto t_steady = compute_uclamp_target(u_steady);
    assert(t_steady.util_min > 0 && t_steady.util_min <= t_steady.util_max && t_steady.util_max == 1024);
    UclampInput u_heating = u_steady;
    u_heating.thermal_velocity_cpm = 5.5;
    auto t_damped = compute_uclamp_target(u_heating);
    assert(t_damped.util_min < t_steady.util_min);

    UclampInput u_interactive{1, 0, 1.0, false, 384, 768, 1024, 120, 5.0, 20.0, 0.25, 0.0, -1.0, 1.0, false, 1.0, -1.0, -1.0, 0.0, 0.0};
    auto t_bal = compute_uclamp_target(u_interactive);

    UclampInput u_smooth = u_interactive;
    u_smooth.jank_ratio = 0.0;
    u_smooth.app_uclamp_bias = 0.85;
    auto t_smooth = compute_uclamp_target(u_smooth);
    assert(t_smooth.util_min < t_bal.util_min);

    // Feed-Forward Pre-Emptive Cancel blocks smooth jank-trim while frozen, then resumes cleanly
    UclampInput u_frozen = u_interactive;
    u_frozen.jank_ratio = 0.0;
    u_frozen.feedforward_freeze = true;
    auto t_frozen = compute_uclamp_target(u_frozen);
    UclampInput u_unfrozen = u_frozen;
    u_unfrozen.feedforward_freeze = false;
    auto t_unfrozen = compute_uclamp_target(u_unfrozen);
    assert(t_frozen.util_min == t_bal.util_min);
    assert(t_unfrozen.util_min < t_frozen.util_min);

    // Universal schedstat starvation rescues floor over cxx FFI
    UclampInput u_starved = u_unfrozen;
    u_starved.starvation_pressure = 0.75;
    auto t_starved = compute_uclamp_target(u_starved);
    assert(t_starved.util_min > t_frozen.util_min);

    // Phase 3 p99 deadline rescue & EDP knee cap over cxx FFI
    UclampInput u_p99_rescue = u_interactive;
    u_p99_rescue.p99_deadline_util = 1.20;
    auto t_p99_rescue = compute_uclamp_target(u_p99_rescue);
    assert(t_p99_rescue.util_min > t_bal.util_min + 30U);

    UclampInput u_edp_warm = u_steady;
    u_edp_warm.capacity_scale = 0.80;
    u_edp_warm.thermal_velocity_cpm = 1.2;
    u_edp_warm.p99_deadline_util = 0.70;
    u_edp_warm.edp_knee_scale = 0.72;
    u_edp_warm.edp_blend = 1.0;
    auto t_edp_warm = compute_uclamp_target(u_edp_warm);
    assert(t_edp_warm.util_max <= 740U);

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

    // 4. Rust Dual-Timescale Thermal Equilibrium (fallback + closed-loop RC P_budget & calibrated EDP exponent)
    auto eq_burst = compute_thermal_equilibrium({72.0, 34.0, 35.0, 1.0, 0.96, false, false, true, false, 0, -1.0, -1.0, -1.0, 0.0, 38.0, 0.0, 0.0, 0.0});
    auto eq_sustained = compute_thermal_equilibrium({72.0, 34.0, 35.0, 1.0, 0.96, false, false, false, false, 0, -1.0, -1.0, -1.0, 0.0, 38.0, 0.0, 0.0, 0.0});
    assert(eq_burst.effective_scale > eq_sustained.effective_scale && !eq_burst.charging_damped && !eq_burst.equilibrium_guard);
    assert(!eq_burst.model_closed_loop_active);
    auto eq_chg = compute_thermal_equilibrium({52.0, 41.0, 41.0, 1.2, 0.90, true, false, false, false, 0, -1.0, -1.0, -1.0, 0.0, 38.0, 0.0, 0.0, 0.0});
    auto eq_byp = compute_thermal_equilibrium({52.0, 41.0, 41.0, 1.2, 0.90, true, true, false, false, 0, -1.0, -1.0, -1.0, 0.0, 38.0, 0.0, 0.0, 0.0});
    assert(eq_chg.charging_damped && !eq_byp.charging_damped);
    assert(eq_chg.effective_scale < eq_byp.effective_scale);

    // Closed-loop tropical ambient (Ta=34 C -> target 41 C) vs external cooler (Ta=18 C -> target 38 C, high P_budget)
    auto eq_tropical = compute_thermal_equilibrium({45.0, 38.5, 38.5, 0.0, 1.0, false, false, false, true, 85, 5.5, 480.0, 34.0, 1.1, 38.0, 2.1, 0.76, 1.0});
    assert(eq_tropical.model_closed_loop_active);
    assert(std::abs(eq_tropical.effective_target_c - 41.0) < 1e-6);
    assert(std::abs(eq_tropical.effective_scale - 1.0) < 1e-6);

    auto eq_cooler = compute_thermal_equilibrium({56.0, 38.5, 38.5, -0.3, 1.0, false, false, false, true, 90, 3.5, 320.0, 18.0, 4.0, 38.0, 2.1, 0.76, 1.0});
    assert(eq_cooler.model_closed_loop_active);
    assert(std::abs(eq_cooler.effective_target_c - 38.0) < 1e-6);
    assert(eq_cooler.power_budget_w > 5.0 && eq_cooler.effective_scale >= 0.99);

    // 5. Rust Orthogonal Symbiotic Orchestrator (single-cluster hotspot + Little's Law CPU-GPU-Memory co-pacing)
    auto sym_hotspot = compute_symbiotic_plan({0.30, 0.44, 0.94, 52.0, 0.62, 48.0, 10.0, 2.0, 2.0, 0.45, 1.0, false, -1.0});
    assert(sym_hotspot.regime == 4U);
    assert(sym_hotspot.cluster_imbalance > 0.30);
    assert(sym_hotspot.cpu_uclamp_scale < 1.0);
    assert(sym_hotspot.helper_uclamp_ratio >= 0.75);
    assert(sym_hotspot.encourage_cluster_spread);

    auto sym_gpu = compute_symbiotic_plan({0.35, 0.52, 0.58, 30.0, 0.32, 86.0, 3.0, 2.0, 4.0, 0.48, 1.0, false, -1.0});
    assert(sym_gpu.regime == 1U);
    assert(sym_gpu.boost_gpu_memory_supply && !sym_gpu.io_supply_boost);
    assert(sym_gpu.cpu_uclamp_scale < 1.0);

    auto sym_io = compute_symbiotic_plan({0.35, 0.45, 0.50, 35.0, 0.30, 30.0, 4.0, 2.0, 14.0, 0.45, 1.0, false, -1.0});
    assert(sym_io.regime == 3U);
    assert(sym_io.io_supply_boost && !sym_io.boost_gpu_memory_supply);

    auto sym_cpu = compute_symbiotic_plan({0.55, 0.65, 0.72, 74.0, 0.72, 34.0, 18.0, 2.0, 2.0, 0.50, 1.0, false, -1.0});
    assert(sym_cpu.regime == 2U);
    assert(sym_cpu.cpu_uclamp_scale > 1.0);
    assert(sym_cpu.helper_uclamp_ratio >= 0.72);
    assert(sym_cpu.memory_bus_scale > 1.05 && sym_cpu.memory_bus_scale <= 1.25);
    assert(sym_cpu.scheduler_responsiveness_scale >= 0.70 && sym_cpu.scheduler_responsiveness_scale < 0.90);
    assert(sym_cpu.thermal_stepdown_lead > 0.02 && sym_cpu.thermal_stepdown_lead <= 0.15);
    assert(sym_cpu.coupling_mask == 0x07U);

    // Schedstat starvation alone flags CpuDrawBound when GPU is actively rendering
    auto sym_starv = compute_symbiotic_plan({0.45, 0.50, 0.55, 35.0, 0.45, 30.0, 2.0, 1.0, 1.0, 0.50, 1.0, false, 0.80});
    assert(sym_starv.regime == 2U && sym_starv.cpu_uclamp_scale > 1.04);

    // Background 92% CPU (dex2oat/post-boot storm) with 0% GPU must NEVER trigger CpuDrawBound
    auto sym_bg_storm = compute_symbiotic_plan({0.55, 0.65, 0.72, 92.0, 0.22, 0.0, 24.0, 2.0, 2.0, 0.50, 1.0, false, -1.0});
    assert(sym_bg_storm.regime == 0U);
    assert(sym_bg_storm.cpu_uclamp_scale == 1.0);

    auto sym_hot_guard = compute_symbiotic_plan({0.30, 0.40, 0.95, 65.0, 0.85, 88.0, 22.0, 5.0, 14.0, 0.45, 0.75, true, 0.90});
    assert(!sym_hot_guard.boost_gpu_memory_supply && !sym_hot_guard.io_supply_boost);
    assert(sym_hot_guard.cpu_uclamp_scale <= 1.0);
    assert(sym_hot_guard.memory_bus_scale == 1.0);
    assert(sym_hot_guard.scheduler_responsiveness_scale == 1.0);
    assert(sym_hot_guard.thermal_stepdown_lead == 0.0);
    assert(sym_hot_guard.coupling_mask == 0U);

    // 6. Rust Markov AppPreloader + Cross-Reboot export_state/import_state
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

    std::cout << "Rust bridge integration tests passed (PID + Feed-Forward/Starvation UClamp + Symbiotic + Closed-Loop RC Equilibrium + OLS + Preloader)\n";
    return 0;
}
