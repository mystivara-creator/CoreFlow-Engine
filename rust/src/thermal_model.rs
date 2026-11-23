use crate::bridge::ffi::ThermalModelResult;

/// Decimated model step. Chassis thermal time constants are minutes, so identifying
/// them from 1 s samples is ill-conditioned (a = exp(-dt/tau) ~ 0.999); 10 s steps are not.
const STEP_MS: u64 = 10_000;
const STEP_S: f64 = 10.0;
const RING: usize = 60;
const MIN_STEPS: u32 = 60;
const FORGET_STEADY: f64 = 0.996;
const FORGET_ADAPT: f64 = 0.925;
const MAX_GAP_MS: u64 = 120_000;

/// Online first-order lumped-RC chassis identification with innovation-triggered adaptive forgetting.
///
/// Physical model (battery/skin temperature as the chassis node, total system power as input):
///   C dT/dt = P - (T - Ta) / R_theta
/// Discretised at STEP_S with a = exp(-STEP_S / tau):
///   x[k+1] = a * x[k] + b * P[k] + c      (x = T - T0, b = R_theta (1-a), c = (1-a)(Ta - T0))
/// Parameters (a, b, c) are tracked with recursive least squares and an adaptive forgetting factor
/// that tightens temporarily when consistent 1-step prediction residuals indicate an environment
/// shift (e.g. external Peltier cooler attached/removed or moving between AC and hot outdoors).
/// From them: tau = -STEP_S / ln(a), R_theta = b / (1-a), Ta = T0 + c / (1-a), and the
/// sustainable power for a target chassis temperature P_sus = (T_target - Ta) / R_theta.
///
/// Everything is gated by plausibility and persistent excitation; otherwise `valid=false`
/// so the daemon never reports a number it cannot justify.
pub struct ThermalModelEstimator {
    // Step accumulation
    step_start_ms: u64,
    have_step: bool,
    step_power_sum: f64,
    step_power_n: u32,
    step_poisoned: bool,
    last_temp_c: f64,
    // Chain state
    have_prev: bool,
    prev_x: f64,
    have_t0: bool,
    t0: f64,
    // RLS + environment-shift residual tracker
    theta: [f64; 3],
    cov: [[f64; 3]; 3],
    steps: u32,
    innovation_ewma: f64,
    shift_streak: u8,
    adapt_steps_remaining: u8,
    // Excitation ring
    ring_p: [f64; RING],
    ring_t: [f64; RING],
    ring_len: usize,
    ring_head: usize,
    avg_power_w: f64,
}

pub fn new_thermal_model_estimator() -> Box<ThermalModelEstimator> {
    let mut m = ThermalModelEstimator {
        step_start_ms: 0,
        have_step: false,
        step_power_sum: 0.0,
        step_power_n: 0,
        step_poisoned: false,
        last_temp_c: 0.0,
        have_prev: false,
        prev_x: 0.0,
        have_t0: false,
        t0: 0.0,
        theta: [0.0; 3],
        cov: [[0.0; 3]; 3],
        steps: 0,
        innovation_ewma: 0.0,
        shift_streak: 0,
        adapt_steps_remaining: 0,
        ring_p: [0.0; RING],
        ring_t: [0.0; RING],
        ring_len: 0,
        ring_head: 0,
        avg_power_w: 0.0,
    };
    m.reset();
    Box::new(m)
}

fn invalid(steps: u32, avg_power_w: f64) -> ThermalModelResult {
    ThermalModelResult {
        valid: false,
        confidence: 0,
        r_theta_c_per_w: -1.0,
        tau_s: -1.0,
        ambient_c: -1.0,
        sustainable_w: -1.0,
        avg_power_w,
        headroom_w: 0.0,
        steps,
    }
}

impl ThermalModelEstimator {
    pub fn reset(&mut self) {
        self.have_step = false;
        self.step_power_sum = 0.0;
        self.step_power_n = 0;
        self.step_poisoned = false;
        self.have_prev = false;
        self.have_t0 = false;
        self.theta = [0.9, 0.0, 0.0];
        self.cov = [[0.0; 3]; 3];
        for (i, row) in self.cov.iter_mut().enumerate() {
            row[i] = 1.0e3;
        }
        self.steps = 0;
        self.innovation_ewma = 0.0;
        self.shift_streak = 0;
        self.adapt_steps_remaining = 0;
        self.ring_len = 0;
        self.ring_head = 0;
        self.avg_power_w = 0.0;
    }

    /// `power_w` must be total system power while discharging; pass a negative/NaN value when
    /// charging or unknown (the current step is then discarded, never fabricated).
    pub fn push_sample(
        &mut self,
        ts_ms: u64,
        temp_c: f64,
        power_w: f64,
        target_c: f64,
    ) -> ThermalModelResult {
        if !temp_c.is_finite() || temp_c <= 0.0 || temp_c > 100.0 {
            return self.snapshot(target_c);
        }
        if self.have_step
            && (ts_ms <= self.step_start_ms
                || ts_ms.saturating_sub(self.step_start_ms) > MAX_GAP_MS)
        {
            self.reset();
        }
        if !self.have_step {
            self.have_step = true;
            self.step_start_ms = ts_ms;
            self.step_power_sum = 0.0;
            self.step_power_n = 0;
            self.step_poisoned = false;
        }
        self.last_temp_c = temp_c;
        if power_w.is_finite() && (0.0..=30.0).contains(&power_w) {
            self.step_power_sum += power_w;
            self.step_power_n += 1;
        } else {
            self.step_poisoned = true;
        }

        if ts_ms.saturating_sub(self.step_start_ms) >= STEP_MS {
            self.close_step(ts_ms);
        }
        self.snapshot(target_c)
    }

    fn close_step(&mut self, ts_ms: u64) {
        let poisoned = self.step_poisoned || self.step_power_n == 0;
        let p = if self.step_power_n > 0 {
            self.step_power_sum / f64::from(self.step_power_n)
        } else {
            0.0
        };
        let temp = self.last_temp_c;
        self.step_start_ms = ts_ms;
        self.step_power_sum = 0.0;
        self.step_power_n = 0;
        self.step_poisoned = false;

        if poisoned {
            // Break the regression chain: never bridge across charging/unknown power.
            self.have_prev = false;
            return;
        }
        if !self.have_t0 {
            self.have_t0 = true;
            self.t0 = temp;
        }
        let x = temp - self.t0;
        self.avg_power_w = if self.steps == 0 && !self.have_prev {
            p
        } else {
            self.avg_power_w + 0.05 * (p - self.avg_power_w)
        };

        if self.have_prev {
            // The transition x_prev -> x is driven by the power averaged over the step just closed.
            self.rls_update(self.prev_x, p, x);
            self.steps += 1;
            self.ring_p[self.ring_head] = p;
            self.ring_t[self.ring_head] = temp;
            self.ring_head = (self.ring_head + 1) % RING;
            if self.ring_len < RING {
                self.ring_len += 1;
            }
        }
        self.have_prev = true;
        self.prev_x = x;
    }

    fn rls_update(&mut self, x_prev: f64, p_prev: f64, x_next: f64) {
        let phi = [x_prev, p_prev, 1.0];
        let pred = self
            .theta
            .iter()
            .zip(phi.iter())
            .map(|(a, b)| a * b)
            .sum::<f64>();
        let err = x_next - pred;
        if self.steps >= 12 && err.is_finite() {
            self.innovation_ewma = 0.75 * self.innovation_ewma + 0.25 * err;
            if self.innovation_ewma.abs() > 0.22 {
                self.shift_streak = self.shift_streak.saturating_add(1);
                // On initial regime-shift detection (e.g. external cooler attached or moving outdoors),
                // inject physically scaled diagonal covariance (dominantly into ambient intercept c)
                // and hold FORGET_ADAPT for 60 steps (~10 min) to cleanly flush pre-shift memory.
                if self.shift_streak == 4 {
                    // Add strictly PSD diagonal matrix D (NEVER clamp a diagonal below its current
                    // value while off-diagonals exist, which breaks positive-definiteness!).
                    self.cov[0][0] += 0.0003;
                    self.cov[1][1] += 0.008;
                    self.cov[2][2] += 0.40;
                    self.adapt_steps_remaining = 60;
                }
            } else if self.innovation_ewma.abs() <= 0.10 {
                self.shift_streak = 0;
            }
        }
        let cov_trace = self.cov[0][0] + self.cov[1][1] + self.cov[2][2];
        let adapting = self.shift_streak >= 4 || self.adapt_steps_remaining > 0;
        if self.adapt_steps_remaining > 0 {
            self.adapt_steps_remaining -= 1;
        }
        let lambda = if adapting && cov_trace < 250.0 {
            FORGET_ADAPT
        } else {
            FORGET_STEADY
        };

        // k = P phi / (lambda + phi' P phi)
        let mut p_phi = [0.0; 3];
        for i in 0..3 {
            for j in 0..3 {
                p_phi[i] += self.cov[i][j] * phi[j];
            }
        }
        let denom = lambda
            + phi
                .iter()
                .zip(p_phi.iter())
                .map(|(a, b)| a * b)
                .sum::<f64>();
        if !denom.is_finite() || denom <= 1e-12 {
            return;
        }
        let mut k = [0.0; 3];
        for i in 0..3 {
            k[i] = p_phi[i] / denom;
            self.theta[i] += k[i] * err;
        }
        // Joseph-stabilized symmetric PSD update:
        //   P_new = ((I - k phi') P (I - k phi')' + lambda * k k') / lambda
        let mut a_mat = [[0.0; 3]; 3];
        for i in 0..3 {
            for j in 0..3 {
                a_mat[i][j] = (if i == j { 1.0 } else { 0.0 }) - k[i] * phi[j];
            }
        }
        let mut ap = [[0.0; 3]; 3];
        for i in 0..3 {
            for j in 0..3 {
                let mut s = 0.0;
                for m in 0..3 {
                    s += a_mat[i][m] * self.cov[m][j];
                }
                ap[i][j] = s;
            }
        }
        let mut next_cov = [[0.0; 3]; 3];
        for i in 0..3 {
            for j in 0..3 {
                let mut s = lambda * k[i] * k[j];
                for m in 0..3 {
                    s += ap[i][m] * a_mat[j][m];
                }
                next_cov[i][j] = s / lambda;
            }
        }
        // Enforce exact floating-point symmetry
        for i in 0..3 {
            for j in (i + 1)..3 {
                let sym = 0.5 * (next_cov[i][j] + next_cov[j][i]);
                next_cov[i][j] = sym;
                next_cov[j][i] = sym;
            }
        }
        self.cov = next_cov;
        // Wind-up & PSD guard: re-arm if any diagonal becomes non-positive, non-finite, or blows up.
        if self
            .cov
            .iter()
            .enumerate()
            .any(|(i, row)| !row[i].is_finite() || row[i] <= 0.0 || row[i] > 1.0e7)
        {
            self.reset();
        }
    }

    fn snapshot(&self, target_c: f64) -> ThermalModelResult {
        if self.steps < MIN_STEPS || self.ring_len < MIN_STEPS as usize {
            return invalid(self.steps, self.avg_power_w);
        }
        // Persistent-excitation gates over the recent window.
        let n = self.ring_len as f64;
        let mean_p = self.ring_p[..self.ring_len].iter().sum::<f64>() / n;
        let var_p = self.ring_p[..self.ring_len]
            .iter()
            .map(|v| (v - mean_p) * (v - mean_p))
            .sum::<f64>()
            / n;
        let std_p = var_p.sqrt();
        let (mut tmin, mut tmax) = (f64::MAX, f64::MIN);
        for &t in &self.ring_t[..self.ring_len] {
            tmin = tmin.min(t);
            tmax = tmax.max(t);
        }
        let span = tmax - tmin;
        if std_p < 0.25 || span < 0.8 {
            return invalid(self.steps, self.avg_power_w);
        }

        let [a, b, c] = self.theta;
        if !(a.is_finite() && b.is_finite() && c.is_finite()) || a <= 0.0 || a >= 1.0 {
            return invalid(self.steps, self.avg_power_w);
        }
        let one_minus_a = 1.0 - a;
        let tau = -STEP_S / a.ln();
        let r_theta = b / one_minus_a;
        let ambient = self.t0 + c / one_minus_a;
        if !(60.0..=3600.0).contains(&tau)
            || !(0.5..=30.0).contains(&r_theta)
            || !(5.0..=45.0).contains(&ambient)
        {
            return invalid(self.steps, self.avg_power_w);
        }

        let residual_damp = if self.shift_streak >= 6 {
            (1.0 - (self.innovation_ewma.abs() - 0.12).max(0.0) / 0.55).clamp(0.15, 1.0)
        } else {
            1.0
        };
        let conf = (f64::from(self.steps) / 180.0).min(1.0)
            * (std_p / 1.0).min(1.0)
            * (span / 3.0).min(1.0)
            * residual_damp
            * 100.0;
        let sustainable = if target_c.is_finite() {
            ((target_c - ambient) / r_theta).max(0.0)
        } else {
            -1.0
        };
        ThermalModelResult {
            valid: true,
            confidence: conf.round().clamp(1.0, 100.0) as u32,
            r_theta_c_per_w: r_theta,
            tau_s: tau,
            ambient_c: ambient,
            sustainable_w: sustainable,
            avg_power_w: self.avg_power_w,
            headroom_w: if sustainable >= 0.0 {
                sustainable - self.avg_power_w
            } else {
                0.0
            },
            steps: self.steps,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Deterministic LCG so tests need no RNG crate.
    struct Lcg(u64);
    impl Lcg {
        fn next(&mut self) -> f64 {
            self.0 = self.0.wrapping_mul(6364136223846793005).wrapping_add(1442695040888963407);
            ((self.0 >> 33) as f64) / ((1u64 << 31) as f64)
        }
    }

    fn simulate(r: f64, tau: f64, ta: f64, minutes: u64, quantum: f64) -> ThermalModelResult {
        let mut est = new_thermal_model_estimator();
        let mut rng = Lcg(7);
        let c = tau / r;
        let mut t = ta + 4.0;
        let mut p = 2.0;
        let mut last = invalid(0, 0.0);
        for sec in 0..(minutes * 60) {
            if sec % 45 == 0 {
                // Bursty real-world power: 0.6 .. 6.5 W in ~45 s dwells.
                p = 0.6 + rng.next() * 5.9;
            }
            t += (p - (t - ta) / r) / c; // 1 s Euler step
            let measured = (t / quantum).round() * quantum;
            last = est.push_sample(1_000 + sec * 1_000, measured, p, 38.0);
        }
        last
    }

    #[test]
    fn recovers_rc_parameters_with_sensor_quantisation() {
        let res = simulate(6.0, 500.0, 28.0, 60, 0.1);
        assert!(res.valid, "model should be valid after 60 min of excitation");
        assert!(
            (res.r_theta_c_per_w - 6.0).abs() / 6.0 < 0.10,
            "R={} tau={} Ta={} steps={}",
            res.r_theta_c_per_w,
            res.tau_s,
            res.ambient_c,
            res.steps
        );
        assert!((res.tau_s - 500.0).abs() / 500.0 < 0.15, "tau={}", res.tau_s);
        // Sustainable power for 38 C must match the analytic (38-28)/6.
        assert!((res.sustainable_w - 10.0 / 6.0).abs() < 0.3, "Psus={}", res.sustainable_w);
        assert!(res.confidence >= 40);
    }

    #[test]
    fn adapts_to_sudden_cooler_attach_and_ambient_shift() {
        let mut est = new_thermal_model_estimator();
        let mut rng = Lcg(19);
        let mut t = 34.0;
        let mut p = 2.5;
        let mut before = invalid(0, 0.0);
        let mut after = invalid(0, 0.0);

        // Phase 1: 30 min in warm room (R=6.0 C/W, tau=480 s, Ta=30.0 C) -> Psus ~ 1.33 W
        for sec in 0..(30 * 60) {
            if sec % 40 == 0 {
                p = 0.8 + rng.next() * 5.2;
            }
            let c = 480.0 / 6.0;
            t += (p - (t - 30.0) / 6.0) / c;
            let measured = (t / 0.1).round() * 0.1;
            before = est.push_sample(1_000 + sec * 1_000, measured, p, 38.0);
        }
        assert!(before.valid);
        assert!(
            (before.ambient_c - 30.0).abs() < 2.5,
            "before Ta={}",
            before.ambient_c
        );

        // Phase 2: External Peltier cooler attached (R=3.5 C/W, tau=300 s, Ta=18.0 C) -> Psus ~ 5.7 W
        for sec in (30 * 60)..(55 * 60) {
            if sec % 40 == 0 {
                p = 1.0 + rng.next() * 6.0;
            }
            let c = 300.0 / 3.5;
            t += (p - (t - 18.0) / 3.5) / c;
            let measured = (t / 0.1).round() * 0.1;
            after = est.push_sample(1_000 + sec * 1_000, measured, p, 38.0);
        }
        assert!(after.valid, "model must stay valid after cooler attach");
        assert!(
            after.ambient_c < before.ambient_c - 6.0,
            "ambient should drop significantly after cooler attach: before={} after={}",
            before.ambient_c,
            after.ambient_c
        );
        assert!(
            after.sustainable_w > before.sustainable_w + 2.0,
            "sustainable power should rise sharply with cooler: before={} after={}",
            before.sustainable_w,
            after.sustainable_w
        );
    }

    #[test]
    fn refuses_to_report_without_excitation() {
        let mut est = new_thermal_model_estimator();
        let mut last = invalid(0, 0.0);
        for sec in 0..3600u64 {
            last = est.push_sample(1_000 + sec * 1_000, 36.0, 2.0, 38.0);
        }
        assert!(!last.valid);
        assert_eq!(last.confidence, 0);
        assert!(last.sustainable_w < 0.0);
    }

    #[test]
    fn charging_never_pollutes_the_fit_and_gap_resets() {
        let mut est = new_thermal_model_estimator();
        let mut last = invalid(0, 0.0);
        for sec in 0..1800u64 {
            // Power unknown/charging the whole time: no steps may ever be accepted.
            last = est.push_sample(1_000 + sec * 1_000, 35.0 + (sec as f64) * 0.001, -1.0, 38.0);
        }
        assert_eq!(last.steps, 0);
        assert!(!last.valid);
        // A >120 s gap resets accumulated state.
        let after = est.push_sample(10_000_000, 35.0, 2.0, 38.0);
        assert_eq!(after.steps, 0);
    }

    #[test]
    fn rejects_garbage_inputs() {
        let mut est = new_thermal_model_estimator();
        let r = est.push_sample(1_000, f64::NAN, 2.0, 38.0);
        assert!(!r.valid);
        let r = est.push_sample(2_000, 500.0, 2.0, 38.0);
        assert!(!r.valid);
    }

    #[test]
    fn adapts_to_tropical_ambient_shift_without_r_theta_explosion() {
        let mut est = new_thermal_model_estimator();
        let mut t: f64 = 30.0;
        let mut last = invalid(0, 0.0);

        // Phase 1: 18 min indoor AC room (Ta=26 C, R=5.5 C/W, tau=180 s)
        for sec in 0..1080u64 {
            let p = if (sec / 60) % 2 == 0 { 3.6 } else { 1.1 };
            let c = 180.0 / 5.5;
            t += (p - (t - 26.0) / 5.5) / c;
            let measured = (t / 0.1).round() * 0.1;
            last = est.push_sample(1_000 + sec * 1_000, measured, p, 38.0);
        }
        assert!(last.valid);
        assert!((last.ambient_c - 26.0).abs() < 2.0, "phase1 Ta={}", last.ambient_c);

        // Phase 2: 13 min tropical outdoor ambient (Ta=34 C, R=5.5 C/W, tau=180 s)
        for sec in 1080..1860u64 {
            let p = if ((sec - 1080) / 90) % 2 == 0 { 2.4 } else { 0.7 };
            let c = 180.0 / 5.5;
            t += (p - (t - 34.0) / 5.5) / c;
            let measured = (t / 0.1).round() * 0.1;
            last = est.push_sample(1_000 + sec * 1_000, measured, p, 38.0);
        }
        assert!(last.valid, "model must stay valid after tropical ambient shift");
        assert!(
            last.ambient_c > 31.5 && last.ambient_c < 36.5,
            "ambient_c must track 34 C tropical shift, got Ta={} R={}",
            last.ambient_c,
            last.r_theta_c_per_w
        );
        assert!(
            last.r_theta_c_per_w > 3.5 && last.r_theta_c_per_w < 8.0,
            "R_theta must remain physical (~5.5 C/W) without explosion, got R={}",
            last.r_theta_c_per_w
        );

        // Phase 3: 10 min sudden Peltier cooler attach (Ta=16 C, R=3.2 C/W, tau=112 s)
        for sec in 1860..2460u64 {
            let p = if ((sec - 1860) / 60) % 2 == 0 { 5.6 } else { 2.8 };
            let c = 112.0 / 3.2;
            t += (p - (t - 16.0) / 3.2) / c;
            let measured = (t / 0.1).round() * 0.1;
            last = est.push_sample(1_000 + sec * 1_000, measured, p, 38.0);
        }
        assert!(
            last.valid,
            "model must remain valid after Phase 3 Peltier cooler attach (steps={} theta={:?} cov_diag=[{},{},{}])",
            last.steps, est.theta, est.cov[0][0], est.cov[1][1], est.cov[2][2]
        );
        assert!(
            last.ambient_c < 22.0,
            "ambient_c must drop sharply on Peltier cooler, got Ta={} R={}",
            last.ambient_c,
            last.r_theta_c_per_w
        );
        assert!(
            last.sustainable_w > 5.0,
            "sustainable_w must surge on Peltier cooler, got Psus={}",
            last.sustainable_w
        );
    }
}
