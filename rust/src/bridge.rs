use crate::controller::{Controller, new_controller};

#[cxx::bridge(namespace = "coreflow::engine")]
pub mod ffi {
    struct Parameters {
        kp: f64,
        ki: f64,
        kd: f64,
        target: f64,
    }
    struct Snapshot {
        temperature: f64,
        elapsed_seconds: f64,
        cpu_pressure: f64,
        memory_pressure: f64,
        io_pressure: f64,
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
    extern "Rust" {
        type Controller;
        fn new_controller(parameters: Parameters) -> Box<Controller>;
        fn step(self: &mut Controller, input: Snapshot) -> Decision;
        fn reset(self: &mut Controller);
    }
}
