use crate::bridge::ffi::{Decision, Parameters, Snapshot};

pub struct Controller {
    parameters: Parameters,
    integral: f64,
    previous_pressure: Option<f64>,
    scale: f64,
    thermal_guard: bool,
}

pub fn new_controller(parameters: Parameters) -> Box<Controller> {
    Box::new(Controller {
        parameters, integral: 0.0, previous_pressure: None,
        scale: 1.0, thermal_guard: false,
    })
}

impl Controller {
    pub fn reset(&mut self) {
        self.integral = 0.0;
        self.previous_pressure = None;
        self.scale = 1.0;
        self.thermal_guard = false;
    }

    pub fn step(&mut self, input: Snapshot) -> Decision {
        let p = &self.parameters;
        let finite = [p.kp, p.ki, p.kd, p.target, input.thermal_pressure,
            input.elapsed_seconds, input.trip, input.recover, input.headroom,
            input.cpu_threshold, input.memory_threshold, input.io_threshold]
            .iter().all(|x| x.is_finite())
            && (!input.cpu_pressure_valid || input.cpu_pressure.is_finite())
            && (!input.memory_pressure_valid || input.memory_pressure.is_finite())
            && (!input.io_pressure_valid || input.io_pressure.is_finite());
        let pressures_valid = [input.cpu_threshold, input.memory_threshold, input.io_threshold]
            .iter().all(|x| (0.0..=100.0).contains(x))
            && (!input.cpu_pressure_valid || (0.0..=100.0).contains(&input.cpu_pressure))
            && (!input.memory_pressure_valid || (0.0..=100.0).contains(&input.memory_pressure))
            && (!input.io_pressure_valid || (0.0..=100.0).contains(&input.io_pressure));
        if !finite || !pressures_valid || !(0.0..=130.0).contains(&input.thermal_pressure)
            || input.elapsed_seconds <= 0.0 || input.elapsed_seconds > 120.0
            || input.recover >= input.trip || input.headroom < 0.0
            || !(0.0..=130.0).contains(&p.target) || p.target > input.trip
            || [p.kp, p.ki, p.kd].iter().any(|x| !(0.0..=20.0).contains(x)) {
            self.reset();
            return Decision { valid: false, state: 2, capacity_scale: 1.0 };
        }
        let dt = input.elapsed_seconds.clamp(0.001, 60.0);
        self.thermal_guard = input.thermal_pressure >= input.trip
            || (self.thermal_guard && input.thermal_pressure > input.recover);
        let state = if self.thermal_guard { 3 }
            else if (input.memory_pressure_valid && input.memory_pressure >= input.memory_threshold)
                 || (input.io_pressure_valid && input.io_pressure >= input.io_threshold) { 2 }
            else if input.cpu_pressure_valid && input.cpu_pressure >= input.cpu_threshold
                 && input.thermal_pressure <= input.trip - input.headroom { 1 }
            else { 0 };

        let mut target_scale = 1.0;
        if input.pid_enabled {
            let error = p.target - input.thermal_pressure;
            // Do not bank a large positive integral while the device is cold.
            self.integral = (self.integral + error * dt).clamp(-50.0, 0.0);
            let velocity = self.previous_pressure
                .map_or(0.0, |previous| (input.thermal_pressure - previous) / dt);
            let output = p.kp * error + p.ki * self.integral - p.kd * velocity;
            target_scale = (1.0 + output / 50.0).clamp(0.4, 1.0);
        }
        self.previous_pressure = Some(input.thermal_pressure);
        if self.thermal_guard { target_scale = target_scale.min(0.875); }
        // At most 2 percentage points per elapsed second, never a fixed cadence jump.
        let delta = (0.02 * dt).min(0.1);
        self.scale += (target_scale - self.scale).clamp(-delta, delta);
        self.scale = self.scale.clamp(0.4, 1.0);
        Decision { valid: true, state, capacity_scale: self.scale }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn sample(temp: f64) -> Snapshot {
        Snapshot { thermal_pressure: temp, elapsed_seconds: 1.0, cpu_pressure: 30.0,
            memory_pressure: 0.0, io_pressure: 0.0, cpu_pressure_valid: true,
            memory_pressure_valid: true, io_pressure_valid: true, trip: 44.0, recover: 40.0,
            headroom: 3.0, cpu_threshold: 25.0, memory_threshold: 15.0,
            io_threshold: 15.0, pid_enabled: true }
    }
    fn controller() -> Box<Controller> {
        new_controller(Parameters { kp: 2.5, ki: 0.15, kd: 1.2, target: 42.5 })
    }
    #[test]
    fn thermal_ramp_is_bounded_and_hysteretic() {
        let mut c = controller();
        assert_eq!(c.step(sample(38.0)).state, 1);
        let hot = c.step(sample(46.0));
        assert_eq!(hot.state, 3);
        assert!((hot.capacity_scale - 0.98).abs() < 1e-9);
        assert_eq!(c.step(sample(41.0)).state, 3);
        assert_ne!(c.step(sample(40.0)).state, 3);
    }
    #[test]
    fn invalid_telemetry_never_becomes_a_boost() {
        let mut c = controller();
        assert!(!c.step(sample(f64::NAN)).valid);
        let mut s = sample(35.0); s.memory_pressure = -1.0;
        assert!(!c.step(s).valid);
        let mut s = sample(35.0); s.elapsed_seconds = 200.0;
        assert!(!c.step(s).valid);
    }
    #[test]
    fn unavailable_psi_is_not_interpreted_as_zero_or_as_a_cpu_boost() {
        let mut c = controller();
        let mut s = sample(38.0);
        s.cpu_pressure_valid = false;
        s.memory_pressure_valid = false;
        s.io_pressure_valid = false;
        s.cpu_pressure = 0.0;
        s.memory_pressure = 0.0;
        s.io_pressure = 0.0;
        s.cpu_pressure = f64::NAN;
        s.memory_pressure = f64::NAN;
        s.io_pressure = f64::NAN;
        assert_eq!(c.step(s).state, 0);

        let mut s = sample(38.0);
        s.cpu_pressure_valid = false;
        s.cpu_pressure = 0.0;
        assert_eq!(c.step(s).state, 0);
    }
    #[test]
    fn available_pressure_domains_remain_independently_actionable() {
        let mut c = controller();
        let mut s = sample(38.0);
        s.memory_pressure_valid = false;
        s.io_pressure_valid = false;
        s.memory_pressure = 0.0;
        s.io_pressure = 0.0;
        assert_eq!(c.step(s).state, 1);
    }
    #[test]
    fn pressure_wins_over_cpu_boost_and_scale_stays_bounded() {
        let mut c = controller();
        let mut s = sample(38.0); s.io_pressure = 20.0;
        assert_eq!(c.step(s).state, 2);
        for _ in 0..1000 {
            let d = c.step(sample(47.0));
            assert!((0.4..=1.0).contains(&d.capacity_scale));
        }
    }
}
