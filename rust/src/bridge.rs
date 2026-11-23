use crate::controller::{Controller, new_controller};
use crate::orchestrator::compute_symbiotic_plan;
use crate::preloader::{AppPreloader, new_app_preloader};
use crate::thermal_model::{ThermalModelEstimator, new_thermal_model_estimator};
use crate::thermal_trend::{
    ThermalTrendAnalyzer, compute_thermal_equilibrium, new_thermal_trend_analyzer,
};
use crate::uclamp::compute_uclamp_target;

#[cxx::bridge(namespace = "coreflow::engine")]
pub mod ffi {
    struct Parameters {
        kp: f64,
        ki: f64,
        kd: f64,
        target: f64,
    }
    struct Snapshot {
        thermal_pressure: f64,
        elapsed_seconds: f64,
        cpu_pressure: f64,
        memory_pressure: f64,
        io_pressure: f64,
        cpu_pressure_valid: bool,
        memory_pressure_valid: bool,
        io_pressure_valid: bool,
        trip: f64,
        recover: f64,
        headroom: f64,
        cpu_threshold: f64,
        memory_threshold: f64,
        io_threshold: f64,
        pid_enabled: bool,
    }
    struct Decision {
        valid: bool,
        state: u8,
        capacity_scale: f64,
    }
    struct UclampInput {
        mode: u8,
        profile: u8,
        capacity_scale: f64,
        thermal_guard: bool,
        little_capacity: u32,
        mid_capacity: u32,
        big_capacity: u32,
        refresh_hz: u32,
        cpu_psi: f64,
        gpu_busy_pct: f64,
        render_pressure: f64,
        thermal_velocity_cpm: f64,
        jank_ratio: f64,
        app_uclamp_bias: f64,
        feedforward_freeze: bool,
        symbiotic_cpu_scale: f64,
        starvation_pressure: f64,
    }
    struct UclampTarget {
        util_min: u32,
        util_max: u32,
    }
    struct SymbioticInput {
        little_freq_ratio: f64,
        mid_freq_ratio: f64,
        big_freq_ratio: f64,
        cpu_util_pct: f64,
        render_pressure: f64,
        gpu_busy_pct: f64,
        cpu_psi: f64,
        mem_psi: f64,
        io_psi: f64,
        avail_ram_ratio: f64,
        capacity_scale: f64,
        thermal_guard: bool,
        starvation_pressure: f64,
    }
    struct SymbioticPlan {
        regime: u8,
        cluster_imbalance: f64,
        cpu_uclamp_scale: f64,
        helper_uclamp_ratio: f64,
        boost_gpu_memory_supply: bool,
        io_supply_boost: bool,
        encourage_cluster_spread: bool,
        memory_bus_scale: f64,
        scheduler_responsiveness_scale: f64,
        thermal_stepdown_lead: f64,
        coupling_mask: u8,
    }
    struct ThermalTrendResult {
        velocity_cpm: f64,
        stability_score: u32,
        seconds_to_trip: f64,
    }
    struct ThermalModelResult {
        valid: bool,
        confidence: u32,
        r_theta_c_per_w: f64,
        tau_s: f64,
        ambient_c: f64,
        sustainable_w: f64,
        avg_power_w: f64,
        headroom_w: f64,
        steps: u32,
    }
    struct ThermalEquilibriumInput {
        soc_temp_c: f64,
        battery_temp_c: f64,
        skin_temp_c: f64,
        thermal_velocity_cpm: f64,
        pid_capacity_scale: f64,
        charging: bool,
        bypass_active: bool,
        is_launch_burst: bool,
        model_valid: bool,
        model_confidence: u32,
        model_r_theta: f64,
        model_tau_s: f64,
        model_ambient_c: f64,
        model_avg_power_w: f64,
        base_target_c: f64,
    }
    struct ThermalEquilibriumOutput {
        effective_scale: f64,
        charging_damped: bool,
        equilibrium_guard: bool,
        effective_target_c: f64,
        power_budget_w: f64,
        model_closed_loop_active: bool,
    }
    struct PreloadPrediction {
        has_prediction: bool,
        target_hash: u64,
        confidence: f64,
    }
    extern "Rust" {
        type Controller;
        fn new_controller(parameters: Parameters) -> Box<Controller>;
        fn step(self: &mut Controller, input: Snapshot) -> Decision;
        fn reset(self: &mut Controller);

        fn compute_uclamp_target(input: UclampInput) -> UclampTarget;
        fn compute_symbiotic_plan(input: SymbioticInput) -> SymbioticPlan;
        fn compute_thermal_equilibrium(input: ThermalEquilibriumInput) -> ThermalEquilibriumOutput;

        type ThermalTrendAnalyzer;
        fn new_thermal_trend_analyzer() -> Box<ThermalTrendAnalyzer>;
        fn push_sample(
            self: &mut ThermalTrendAnalyzer,
            ts_ms: u64,
            temp_c: f64,
            trip_c: f64,
        ) -> ThermalTrendResult;
        fn reset(self: &mut ThermalTrendAnalyzer);

        type ThermalModelEstimator;
        fn new_thermal_model_estimator() -> Box<ThermalModelEstimator>;
        fn push_sample(
            self: &mut ThermalModelEstimator,
            ts_ms: u64,
            temp_c: f64,
            power_w: f64,
            target_c: f64,
        ) -> ThermalModelResult;
        fn reset(self: &mut ThermalModelEstimator);

        type AppPreloader;
        fn new_app_preloader() -> Box<AppPreloader>;
        fn record_launch(self: &mut AppPreloader, pkg_hash: u64);
        fn predict_next(self: &AppPreloader, current_pkg_hash: u64) -> PreloadPrediction;
        fn export_state(self: &AppPreloader) -> String;
        fn import_state(self: &mut AppPreloader, payload: &str) -> bool;
        fn reset(self: &mut AppPreloader);
    }
}
