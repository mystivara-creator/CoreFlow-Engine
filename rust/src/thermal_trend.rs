use crate::bridge::ffi::ThermalTrendResult;

const WINDOW_CAPACITY: usize = 16;

#[derive(Clone, Copy, Default)]
struct ThermalPoint {
    ts_ms: u64,
    temp_c: f64,
}

/// Fixed-size zero-allocation ring buffer performing OLS linear regression over
/// recent SoC temperature samples to produce jitter-free thermal velocity (°C/min).
pub struct ThermalTrendAnalyzer {
    buf: [ThermalPoint; WINDOW_CAPACITY],
    head: usize,
    len: usize,
}

pub fn new_thermal_trend_analyzer() -> Box<ThermalTrendAnalyzer> {
    Box::new(ThermalTrendAnalyzer {
        buf: [ThermalPoint::default(); WINDOW_CAPACITY],
        head: 0,
        len: 0,
    })
}

impl ThermalTrendAnalyzer {
    pub fn reset(&mut self) {
        self.head = 0;
        self.len = 0;
    }

    pub fn push_sample(&mut self, ts_ms: u64, temp_c: f64, trip_c: f64) -> ThermalTrendResult {
        if !temp_c.is_finite() || temp_c <= 0.0 || temp_c > 130.0 {
            return ThermalTrendResult {
                velocity_cpm: 0.0,
                stability_score: 100,
                seconds_to_trip: -1.0,
            };
        }

        if self.len > 0 {
            let last_idx = (self.head + WINDOW_CAPACITY - 1) % WINDOW_CAPACITY;
            let last_ts = self.buf[last_idx].ts_ms;
            if ts_ms <= last_ts || ts_ms.saturating_sub(last_ts) > 120_000 {
                self.reset();
            }
        }

        self.buf[self.head] = ThermalPoint { ts_ms, temp_c };
        self.head = (self.head + 1) % WINDOW_CAPACITY;
        if self.len < WINDOW_CAPACITY {
            self.len += 1;
        }

        if self.len < 4 {
            return ThermalTrendResult {
                velocity_cpm: 0.0,
                stability_score: 100,
                seconds_to_trip: -1.0,
            };
        }

        let start = if self.len == WINDOW_CAPACITY {
            self.head
        } else {
            0
        };
        let t0 = self.buf[start].ts_ms;
        let n = self.len as f64;

        // Ordinary Least Squares slope (x = elapsed minutes, y = Celsius)
        let mut sum_x = 0.0;
        let mut sum_y = 0.0;
        let mut sum_xx = 0.0;
        let mut sum_xy = 0.0;
        for i in 0..self.len {
            let pt = self.buf[(start + i) % WINDOW_CAPACITY];
            let x = (pt.ts_ms.saturating_sub(t0) as f64) / 60_000.0;
            let y = pt.temp_c;
            sum_x += x;
            sum_y += y;
            sum_xx += x * x;
            sum_xy += x * y;
        }

        let denom = n * sum_xx - sum_x * sum_x;
        let velocity_cpm = if denom > 1e-9 {
            ((n * sum_xy - sum_x * sum_y) / denom).clamp(-25.0, 25.0)
        } else {
            0.0
        };

        // Stability score (100 = flat/cooling, drops smoothly as heating velocity increases)
        let penalty = if velocity_cpm > 0.4 {
            ((velocity_cpm - 0.4) * 14.0).clamp(0.0, 75.0)
        } else {
            0.0
        };
        let stability_score = (100.0 - penalty).round().clamp(15.0, 100.0) as u32;

        // Estimated seconds until reaching trip_c if heating steadily
        let seconds_to_trip =
            if trip_c.is_finite() && trip_c > temp_c && velocity_cpm >= 0.5 {
                (((trip_c - temp_c) / velocity_cpm) * 60.0).clamp(1.0, 3600.0)
            } else {
                -1.0
            };

        ThermalTrendResult {
            velocity_cpm,
            stability_score,
            seconds_to_trip,
        }
    }
}

/// Dual-timescale thermal equilibrium planner separating fast CPU die transients
/// from slow battery/chassis heat accumulation, charging/bypass thermal budgets,
/// and closed-loop online RC power-budget pacing (adaptive to room ambient & coolers).
pub fn compute_thermal_equilibrium(
    input: crate::bridge::ffi::ThermalEquilibriumInput,
) -> crate::bridge::ffi::ThermalEquilibriumOutput {
    let base_scale = if input.pid_capacity_scale.is_finite() {
        input.pid_capacity_scale.clamp(0.25, 1.0)
    } else {
        1.0
    };

    let soc = if input.soc_temp_c.is_finite() && input.soc_temp_c > 0.0 {
        input.soc_temp_c
    } else {
        -1.0
    };
    let batt = if input.battery_temp_c.is_finite() && input.battery_temp_c > 0.0 {
        input.battery_temp_c
    } else {
        -1.0
    };
    let skin = if input.skin_temp_c.is_finite() && input.skin_temp_c > 0.0 {
        input.skin_temp_c
    } else {
        -1.0
    };
    let vel = if input.thermal_velocity_cpm.is_finite() {
        input.thermal_velocity_cpm
    } else {
        0.0
    };

    let base_target =
        if input.base_target_c.is_finite() && (34.0..=42.0).contains(&input.base_target_c) {
            input.base_target_c
        } else {
            38.0
        };

    let ambient_valid = input.model_valid
        && input.model_confidence >= 45
        && input.model_ambient_c.is_finite()
        && (5.0..=45.0).contains(&input.model_ambient_c);
    // Ambient-adaptive chassis target (C0-continuous across the confidence=45 gate):
    // - Cool AC room / external cooler (Ta <= 31 C): stays locked at base_target (38.0 C) while
    //   higher sustainable power P_sus unlocks extra Watt headroom.
    // - Hot tropical ambient (Ta in 31..34.5 C): lifts target gently up to 41.5 C (strictly below
    //   the 44.0 C THERMAL_GUARD ceiling) so idle phones never falsely enter Cool-Glide.
    let target_blend = ((f64::from(input.model_confidence) - 45.0) / 20.0).clamp(0.0, 1.0);
    let effective_target_c = if ambient_valid {
        let raw_target = base_target.max(input.model_ambient_c + 7.0).clamp(base_target, 41.5);
        base_target + (raw_target - base_target) * target_blend
    } else {
        base_target
    };
    let onset_c = (effective_target_c - 0.5).clamp(37.5, 41.0);
    let plateau_c = (effective_target_c + 0.5).clamp(38.5, 42.0);

    let model_closed_loop = ambient_valid
        && !input.charging
        && input.model_r_theta.is_finite()
        && (0.5..=30.0).contains(&input.model_r_theta)
        && input.model_tau_s.is_finite()
        && (60.0..=3600.0).contains(&input.model_tau_s)
        && input.model_avg_power_w.is_finite()
        && input.model_avg_power_w > 0.35;

    let mut scale = base_scale;
    let mut charging_damped = false;
    let mut equilibrium_guard = false;
    let mut power_budget_w = -1.0;
    let mut closed_loop_active = false;

    // 1. Slow chassis / battery thermal-power equilibrium loop:
    // Proactively ease headroom smoothly when battery/chassis warms above adaptive onset_c so
    // Multi-Cgroup EAS cpu.uclamp.max pacing sheds excess C*V^2*f Watts well before
    // hitting harsh THERMAL_GUARD (which remains unconditionally at 44.0 C).
    let chassis = if skin > 0.0 && batt > 0.0 {
        skin.max(batt)
    } else if skin > 0.0 {
        skin
    } else {
        batt
    };

    let mut eq_cap = 1.0;
    if chassis >= onset_c {
        let excess = (chassis - onset_c).clamp(0.0, 9.0);
        eq_cap = (1.0 - excess * 0.055).clamp(0.55, 1.0);
        // Sustained warm-plateau nudge: smoothly ease power draw as chassis approaches/exceeds
        // plateau_c while not actively cooling (C0-continuous ramp prevents step-cliff chatter).
        let plateau_frac = ((chassis - (plateau_c - 0.3)) / 0.8).clamp(0.0, 1.0);
        let vel_frac = ((vel + 0.4) / 0.6).clamp(0.0, 1.0);
        eq_cap = (eq_cap - 0.05 * plateau_frac * vel_frac).clamp(0.55, 1.0);
    }

    if model_closed_loop && chassis > 0.0 {
        // Exact 1-node RC receding-horizon power budget:
        //   P_budget(t) = (T_target - Ta) / R_theta + (tau * (T_target - T_chassis)) / (R_theta * H)
        let horizon_s = (0.40 * input.model_tau_s).clamp(120.0, 300.0);
        let p_sus = ((effective_target_c - input.model_ambient_c) / input.model_r_theta).max(0.40);
        let p_cap =
            (input.model_tau_s * (effective_target_c - chassis)) / (input.model_r_theta * horizon_s);
        let p_budget = (p_sus + p_cap).clamp(0.40, 18.0);
        power_budget_w = p_budget;
        closed_loop_active = true;

        // Silicon dynamic power law P ~ s^gamma converts Watt ratio to capacity_scale.
        // Uses online-calibrated C0-blended silicon exponent (1.35..2.80, fallback 1.8) matching
        // daemon P_demand reconstruction so s_power = s_ewma * (P_budget / P_ewma)^(1/gamma)
        // preserves an exact overdamped fixed point at P_ewma == P_budget on every SoC.
        let power_gamma = if input.edp_power_exponent.is_finite()
            && input.edp_power_exponent >= 1.35
        {
            input.edp_power_exponent.clamp(1.35, 2.80)
        } else {
            1.8
        };
        let power_ratio = (p_budget / input.model_avg_power_w.max(0.50)).clamp(0.05, 4.0);
        let s_power = power_ratio.powf(1.0 / power_gamma).clamp(0.52, 1.0);
        // Zero-step C0 handoff at model_confidence == 45 prevents limit-cycle chatter when confidence hovers at 44..46.
        let conf_weight = ((f64::from(input.model_confidence) - 45.0) / 40.0).clamp(0.0, 1.0);

        if chassis >= onset_c {
            // Above onset: blend temperature curve with physical power budget.
            // With an external cooler (s_power == 1.0), unnecessary temperature trimming relaxes;
            // under heavy over-budget draw (s_power < eq_cap), pacing tightens cleanly by physics.
            let blended = (1.0 - conf_weight) * eq_cap + conf_weight * s_power;
            scale = scale.min(blended.clamp(0.52, 1.0));
        } else if !input.is_launch_burst && s_power < 1.0 {
            // Approaching equilibrium (within 3.0 C below onset_c): smoothly engage proactive Watt pacing
            // with exact C0 continuity at chassis == onset_c (approach -> 1.0 matches blended at onset_c).
            let approach = ((chassis - (onset_c - 3.0)) / 3.0).clamp(0.0, 1.0);
            if approach > 0.0 {
                let proactive_cap = 1.0 - (1.0 - s_power) * approach * conf_weight;
                scale = scale.min(proactive_cap.clamp(0.52, 1.0));
            }
        }
    } else if chassis >= onset_c {
        scale = scale.min(eq_cap);
    }

    if chassis >= 44.0 {
        equilibrium_guard = true;
        scale = scale.min(0.65);
    }

    // 2. Charging-aware vs Bypass-Charging-aware budget
    // Hot active charging adds charging-IC heat to the battery & PMIC simultaneously; apply a
    // proportional penalty below the chassis curve. Bypass Charging removes IC heat cleanly.
    if input.charging && !input.bypass_active && batt >= 38.5 {
        let charge_penalty = (0.04 + (batt - 38.5).clamp(0.0, 6.0) * 0.025).clamp(0.04, 0.19);
        let charge_cap = (scale - charge_penalty).clamp(0.50, 0.92);
        if charge_cap < scale {
            scale = charge_cap;
            charging_damped = true;
        }
    }

    // 3. Fast Die Guard: tolerate brief (< 2.5s) launch-burst die spikes up to 68 C when chassis
    // is cool (< plateau_c), but immediately damp sustained hot die (> 70 C or rapid heating).
    if soc > 0.0 {
        let cool_chassis_burst = input.is_launch_burst && (chassis < 0.0 || chassis < plateau_c);
        if !cool_chassis_burst && soc >= 70.0 {
            let die_excess = (soc - 70.0).clamp(0.0, 15.0);
            let die_cap = (0.94 - die_excess * 0.030).clamp(0.45, 0.94);
            scale = scale.min(die_cap);
        }
        if soc >= 76.0 || (soc >= 70.0 && vel >= 6.0) {
            equilibrium_guard = true;
        }
    }

    crate::bridge::ffi::ThermalEquilibriumOutput {
        effective_scale: scale.clamp(0.25, 1.0),
        charging_damped,
        equilibrium_guard,
        effective_target_c,
        power_budget_w,
        model_closed_loop_active: closed_loop_active,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::bridge::ffi::ThermalEquilibriumInput;

    fn fallback_eq_input(soc: f64, batt: f64, skin: f64, vel: f64) -> ThermalEquilibriumInput {
        ThermalEquilibriumInput {
            soc_temp_c: soc,
            battery_temp_c: batt,
            skin_temp_c: skin,
            thermal_velocity_cpm: vel,
            pid_capacity_scale: 1.0,
            charging: false,
            bypass_active: false,
            is_launch_burst: false,
            model_valid: false,
            model_confidence: 0,
            model_r_theta: -1.0,
            model_tau_s: -1.0,
            model_ambient_c: -1.0,
            model_avg_power_w: 0.0,
            base_target_c: 38.0,
            edp_power_exponent: 0.0,
            edp_knee_scale: 0.0,
            edp_blend: 0.0,
        }
    }

    #[test]
    fn ols_detects_steady_heating_and_cooling_accurately() {
        let mut analyzer = new_thermal_trend_analyzer();
        // Heating at +6.0 C/min (+0.1 C per second)
        let mut last = ThermalTrendResult {
            velocity_cpm: 0.0,
            stability_score: 100,
            seconds_to_trip: -1.0,
        };
        for i in 0..10u64 {
            last = analyzer.push_sample(1_000 + i * 1_000, 40.0 + (i as f64) * 0.1, 55.0);
        }
        assert!((last.velocity_cpm - 6.0).abs() < 0.05);
        assert!(last.stability_score < 50);
        assert!(last.seconds_to_trip > 100.0 && last.seconds_to_trip < 200.0);

        analyzer.reset();
        // Cooling at -3.0 C/min (-0.05 C per second)
        for i in 0..8u64 {
            last = analyzer.push_sample(50_000 + i * 1_000, 45.0 - (i as f64) * 0.05, 55.0);
        }
        assert!((last.velocity_cpm + 3.0).abs() < 0.05);
        assert_eq!(last.stability_score, 100);
        assert!(last.seconds_to_trip < 0.0);
    }

    #[test]
    fn rejects_nan_and_non_monotonic_timestamps_gracefully() {
        let mut analyzer = new_thermal_trend_analyzer();
        let bad = analyzer.push_sample(1000, f64::NAN, 55.0);
        assert_eq!(bad.velocity_cpm, 0.0);
        assert_eq!(bad.stability_score, 100);
    }

    #[test]
    fn resets_window_across_long_sleep_gap() {
        let mut analyzer = new_thermal_trend_analyzer();
        for i in 0..6u64 {
            analyzer.push_sample(1_000 + i * 1_000, 40.0 + (i as f64) * 0.2, 55.0);
        }
        // Jump forward by 5 minutes (deep sleep): window must reset instead of computing a stale slope.
        let after_wake = analyzer.push_sample(310_000, 32.0, 55.0);
        assert_eq!(after_wake.velocity_cpm, 0.0);
        assert_eq!(after_wake.stability_score, 100);
    }

    #[test]
    fn dual_timescale_equilibrium_handles_burst_charging_and_bypass() {
        let mut burst_in = fallback_eq_input(64.0, 36.0, 36.5, 4.0);
        burst_in.is_launch_burst = true;
        let burst = compute_thermal_equilibrium(burst_in);
        assert!((burst.effective_scale - 1.0).abs() < 1e-9);
        assert!(!burst.equilibrium_guard);
        assert!(!burst.model_closed_loop_active);

        let mut chg_in = fallback_eq_input(55.0, 41.0, 40.0, 1.0);
        chg_in.charging = true;
        let hot_chg = compute_thermal_equilibrium(chg_in);
        assert!(hot_chg.charging_damped);
        assert!(hot_chg.effective_scale < 0.90);

        let mut byp_in = fallback_eq_input(55.0, 41.0, 40.0, 1.0);
        byp_in.charging = true;
        byp_in.bypass_active = true;
        let bypass = compute_thermal_equilibrium(byp_in);
        assert!(!bypass.charging_damped);
        assert!(bypass.effective_scale > hot_chg.effective_scale);
    }

    #[test]
    fn proactive_cool_glide_steers_41c_plateau_toward_38c_without_guard() {
        let cool_37 = compute_thermal_equilibrium(fallback_eq_input(44.0, 37.0, 37.0, 0.0));
        assert!((cool_37.effective_scale - 1.0).abs() < 1e-9);
        assert!(!cool_37.equilibrium_guard);

        let plateau_41 = compute_thermal_equilibrium(fallback_eq_input(47.0, 41.0, 41.0, 0.1));
        // Actively trims headroom (~0.76) to cool 41 C plateau back toward ~38 C without false THERMAL_GUARD
        assert!(!plateau_41.equilibrium_guard);
        assert!(plateau_41.effective_scale <= 0.78 && plateau_41.effective_scale >= 0.70);

        let cooling_39 = compute_thermal_equilibrium(fallback_eq_input(45.0, 39.0, 39.0, -0.6));
        assert!(!cooling_39.equilibrium_guard);
        assert!(cooling_39.effective_scale > plateau_41.effective_scale + 0.10);
    }

    #[test]
    fn closed_loop_model_adapts_to_tropical_ambient_cooler_and_power_budget() {
        let make_model = |soc, batt, vel, conf, r, tau, ta, pwr| ThermalEquilibriumInput {
            soc_temp_c: soc,
            battery_temp_c: batt,
            skin_temp_c: batt,
            thermal_velocity_cpm: vel,
            pid_capacity_scale: 1.0,
            charging: false,
            bypass_active: false,
            is_launch_burst: false,
            model_valid: true,
            model_confidence: conf,
            model_r_theta: r,
            model_tau_s: tau,
            model_ambient_c: ta,
            model_avg_power_w: pwr,
            base_target_c: 38.0,
            edp_power_exponent: 0.0,
            edp_knee_scale: 0.0,
            edp_blend: 0.0,
        };

        // 1. Tropical 34 C ambient: target lifts to 41.0 C (<= 41.5 C cap), so 38.5 C idle
        //    with modest 1.1 W draw stays unthrottled (instead of falsely throttling at 37.5 C).
        let out_trop = compute_thermal_equilibrium(make_model(45.0, 38.5, 0.0, 85, 5.5, 480.0, 34.0, 1.1));
        assert!(out_trop.model_closed_loop_active);
        assert!((out_trop.effective_target_c - 41.0).abs() < 1e-6);
        assert!((out_trop.effective_scale - 1.0).abs() < 1e-6);
        assert!(!out_trop.equilibrium_guard);

        // Extreme 38 C ambient must stay strictly clamped at 41.5 C ceiling (< 44 C guard).
        let out_ext = compute_thermal_equilibrium(make_model(45.0, 38.5, 0.0, 85, 5.5, 480.0, 38.0, 1.1));
        assert!((out_ext.effective_target_c - 41.5).abs() < 1e-6);

        // 2. External cooler (Ta = 18 C, R = 3.5 C/W): target locked at 38.0 C, huge P_budget (~5.6 W),
        //    so a 4.0 W gaming draw at 38.5 C gets full unthrottled scale!
        let out_cooler = compute_thermal_equilibrium(make_model(56.0, 38.5, -0.3, 90, 3.5, 320.0, 18.0, 4.0));
        assert!(out_cooler.model_closed_loop_active);
        assert!((out_cooler.effective_target_c - 38.0).abs() < 1e-6);
        assert!(out_cooler.power_budget_w > 5.0);
        assert!(out_cooler.effective_scale >= 0.99);

        // 3. Over-budget draw in normal 28 C room (R = 6.0 -> Psus = 1.67 W) with 4.5 W draw
        //    at 37.2 C (approaching 38 C): proactively paces smoothly BEFORE hitting 41 C.
        let out_over = compute_thermal_equilibrium(make_model(58.0, 37.2, 0.8, 85, 6.0, 500.0, 28.0, 4.5));
        assert!(out_over.model_closed_loop_active);
        assert!(out_over.power_budget_w > 1.5 && out_over.power_budget_w < 3.0);
        assert!(out_over.effective_scale < 0.85 && out_over.effective_scale >= 0.58);

        // 4. Low confidence (< 45) falls back strictly to legacy curve (model_closed_loop_active = false).
        let out_low = compute_thermal_equilibrium(make_model(58.0, 37.2, 0.8, 40, 6.0, 500.0, 28.0, 4.5));
        assert!(!out_low.model_closed_loop_active);
        assert!((out_low.effective_scale - 1.0).abs() < 1e-9);

        // 5. Hard 44 C thermal guard ALWAYS wins regardless of cooler or model.
        let out_guard = compute_thermal_equilibrium(make_model(56.0, 44.5, -0.3, 90, 3.5, 320.0, 18.0, 4.0));
        assert!(out_guard.equilibrium_guard);
        assert!(out_guard.effective_scale <= 0.65);
    }

    #[test]
    fn equilibrium_is_c0_continuous_and_fixed_point_stable() {
        let make = |chassis, vel, p_demand| ThermalEquilibriumInput {
            soc_temp_c: chassis + 15.0,
            battery_temp_c: chassis,
            skin_temp_c: chassis,
            thermal_velocity_cpm: vel,
            pid_capacity_scale: 1.0,
            charging: false,
            bypass_active: false,
            is_launch_burst: false,
            model_valid: true,
            model_confidence: 90,
            model_r_theta: 5.5,
            model_tau_s: 180.0,
            model_ambient_c: 26.0,
            model_avg_power_w: p_demand,
            base_target_c: 38.0,
            edp_power_exponent: 0.0,
            edp_knee_scale: 0.0,
            edp_blend: 0.0,
        };

        // C0 continuity across onset_c = 37.5 C
        let below = compute_thermal_equilibrium(make(37.499, 0.2, 4.2));
        let above = compute_thermal_equilibrium(make(37.501, 0.2, 4.2));
        assert!(
            (below.effective_scale - above.effective_scale).abs() < 1e-3,
            "discontinuity at onset_c: below={} above={}",
            below.effective_scale,
            above.effective_scale
        );

        // C0 continuity across vel = -0.2 C/min at plateau_c = 38.5 C (legacy curve)
        let o1 = compute_thermal_equilibrium(fallback_eq_input(52.0, 38.8, 38.8, -0.22));
        let o2 = compute_thermal_equilibrium(fallback_eq_input(52.0, 38.8, 38.8, -0.18));
        assert!(
            (o1.effective_scale - o2.effective_scale).abs() < 0.01,
            "step cliff at plateau vel=-0.2: o1={} o2={}",
            o1.effective_scale,
            o2.effective_scale
        );
        let p1 = compute_thermal_equilibrium(fallback_eq_input(52.0, 38.49, 38.49, -0.18));
        let p2 = compute_thermal_equilibrium(fallback_eq_input(52.0, 38.51, 38.51, -0.18));
        assert!((p1.effective_scale - p2.effective_scale).abs() < 0.01);

        // Fixed-point stability: at T_chassis = T_target (38.0 C), P_budget = P_sus = (38 - 26) / 5.5 = 2.1818 W.
        // If current scale is s = 0.72 and measured power equals P_budget, reconstructing unthrottled demand
        // P_demand = P_meas / s^1.8 must hold exact fixed point effective_scale == 0.72 (no bounce to 1.0!).
        let s_eq: f64 = 0.72;
        let p_sus = (38.0 - 26.0) / 5.5;
        let p_demand = p_sus / s_eq.powf(1.8);
        let eq = compute_thermal_equilibrium(make(38.0, 0.0, p_demand));
        assert!(
            (eq.effective_scale - s_eq).abs() < 1e-4,
            "fixed point drifted: expected {} got {}",
            s_eq,
            eq.effective_scale
        );

        // C0 continuity across model_confidence = 44 -> 45 gate (both scale and effective_target_c)
        let mut c44 = make(40.0, 0.1, 4.2);
        c44.model_ambient_c = 34.0;
        c44.model_confidence = 44;
        let mut c45 = make(40.0, 0.1, 4.2);
        c45.model_ambient_c = 34.0;
        c45.model_confidence = 45;
        let o44 = compute_thermal_equilibrium(c44);
        let o45 = compute_thermal_equilibrium(c45);
        assert!(!o44.model_closed_loop_active && o45.model_closed_loop_active);
        assert!((o44.effective_target_c - o45.effective_target_c).abs() < 1e-6);
        assert!((o44.effective_scale - o45.effective_scale).abs() < 1e-6);
    }

    #[test]
    fn calibrated_edp_exponent_preserves_fixed_point_and_c0_continuity() {
        let p_sus = (38.0 - 26.0) / 5.5; // 2.1818 W
        let s_eq: f64 = 0.76;

        let make = |alpha: f64, blend: f64, p_demand: f64| {
            let mut inp = fallback_eq_input(55.0, 38.0, 38.0, 0.0);
            inp.model_valid = true;
            inp.model_confidence = 90;
            inp.model_r_theta = 5.5;
            inp.model_tau_s = 180.0;
            inp.model_ambient_c = 26.0;
            inp.model_avg_power_w = p_demand;
            inp.edp_power_exponent = alpha;
            inp.edp_knee_scale = 0.68;
            inp.edp_blend = blend;
            inp
        };

        // Steep-voltage silicon (alpha = 2.4) needs less frequency cut for the same Watt ratio than flat silicon (1.5)
        let o_steep = compute_thermal_equilibrium(make(2.4, 1.0, 4.2));
        let o_flat = compute_thermal_equilibrium(make(1.5, 1.0, 4.2));
        assert!(o_steep.effective_scale > o_flat.effective_scale + 0.05);

        // Synchronized P_demand = P_meas / s^alpha preserves exact fixed point for ANY calibrated alpha!
        for alpha in [1.45_f64, 1.80, 2.15, 2.45, 2.75] {
            let out = compute_thermal_equilibrium(make(alpha, 1.0, p_sus / s_eq.powf(alpha)));
            assert!(
                (out.effective_scale - s_eq).abs() < 1e-4,
                "fixed point broke for alpha={}: got {}",
                alpha,
                out.effective_scale
            );
        }

        // Smooth C0 blend across confidence 50..70 (alpha 1.80 -> 2.40) has zero step cliff
        let mut prev = 0.0;
        for step in 0..=20 {
            let blend = f64::from(step) / 20.0;
            let eff_alpha = 1.8 + blend * (2.4 - 1.8);
            let cur = compute_thermal_equilibrium(make(eff_alpha, blend, 4.2)).effective_scale;
            if step > 0 {
                assert!(
                    (cur - prev).abs() < 0.01,
                    "step cliff at step {}: {} -> {}",
                    step,
                    prev,
                    cur
                );
            }
            prev = cur;
        }
    }
}
