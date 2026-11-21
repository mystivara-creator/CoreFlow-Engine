use crate::controller::{Controller, new_controller};
use crate::preloader::{AppPreloader, new_app_preloader};
use crate::thermal_trend::{ThermalTrendAnalyzer, new_thermal_trend_analyzer};
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
    }
    struct UclampTarget {
        util_min: u32,
        util_max: u32,
    }
    struct ThermalTrendResult {
        velocity_cpm: f64,
        stability_score: u32,
        seconds_to_trip: f64,
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

        type ThermalTrendAnalyzer;
        fn new_thermal_trend_analyzer() -> Box<ThermalTrendAnalyzer>;
        fn push_sample(
            self: &mut ThermalTrendAnalyzer,
            ts_ms: u64,
            temp_c: f64,
            trip_c: f64,
        ) -> ThermalTrendResult;
        fn reset(self: &mut ThermalTrendAnalyzer);

        type AppPreloader;
        fn new_app_preloader() -> Box<AppPreloader>;
        fn record_launch(self: &mut AppPreloader, pkg_hash: u64);
        fn predict_next(self: &AppPreloader, current_pkg_hash: u64) -> PreloadPrediction;
        fn reset(self: &mut AppPreloader);
    }
}
