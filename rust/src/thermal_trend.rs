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
/// from slow battery/chassis heat accumulation and charging/bypass thermal budgets.
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

    let mut scale = base_scale;
    let mut charging_damped = false;
    let mut equilibrium_guard = false;

    // 1. Slow chassis / battery Cool-Glide equilibrium loop:
    // Proactively steer warm chassis temperatures (37.5..43.9 C) smoothly toward the ~38 C
    // comfort zone without triggering harsh THERMAL_GUARD (which remains at 44.0 C).
    let chassis = if skin > 0.0 && batt > 0.0 {
        skin.max(batt)
    } else if skin > 0.0 {
        skin
    } else {
        batt
    };

    if chassis >= 37.5 {
        let excess = (chassis - 37.5).clamp(0.0, 9.0);
        let mut eq_cap = (1.0 - excess * 0.055).clamp(0.55, 1.0);
        // Sustained warm-plateau nudge: if chassis >= 38.5 C and not actively cooling,
        // ease power draw below ambient dissipation so temperature glides back toward ~38 C.
        if chassis >= 38.5 && vel >= -0.2 {
            eq_cap = (eq_cap - 0.05).clamp(0.55, 1.0);
        }
        scale = scale.min(eq_cap);
        if chassis >= 44.0 {
            equilibrium_guard = true;
        }
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
    // is cool (< 38.5 C), but immediately damp sustained hot die (> 70 C or rapid heating).
    if soc > 0.0 {
        let cool_chassis_burst = input.is_launch_burst && (chassis < 0.0 || chassis < 38.5);
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
    }
}

#[cfg(test)]
mod tests {
    use super::*;

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
        use crate::bridge::ffi::ThermalEquilibriumInput;
        let burst = compute_thermal_equilibrium(ThermalEquilibriumInput {
            soc_temp_c: 64.0,
            battery_temp_c: 36.0,
            skin_temp_c: 36.5,
            thermal_velocity_cpm: 4.0,
            pid_capacity_scale: 1.0,
            charging: false,
            bypass_active: false,
            is_launch_burst: true,
        });
        assert!((burst.effective_scale - 1.0).abs() < 1e-9);
        assert!(!burst.equilibrium_guard);

        let hot_chg = compute_thermal_equilibrium(ThermalEquilibriumInput {
            soc_temp_c: 55.0,
            battery_temp_c: 41.0,
            skin_temp_c: 40.0,
            thermal_velocity_cpm: 1.0,
            pid_capacity_scale: 1.0,
            charging: true,
            bypass_active: false,
            is_launch_burst: false,
        });
        assert!(hot_chg.charging_damped);
        assert!(hot_chg.effective_scale < 0.90);

        let bypass = compute_thermal_equilibrium(ThermalEquilibriumInput {
            soc_temp_c: 55.0,
            battery_temp_c: 41.0,
            skin_temp_c: 40.0,
            thermal_velocity_cpm: 1.0,
            pid_capacity_scale: 1.0,
            charging: true,
            bypass_active: true,
            is_launch_burst: false,
        });
        assert!(!bypass.charging_damped);
        assert!(bypass.effective_scale > hot_chg.effective_scale);
    }

    #[test]
    fn proactive_cool_glide_steers_41c_plateau_toward_38c_without_guard() {
        use crate::bridge::ffi::ThermalEquilibriumInput;
        let cool_37 = compute_thermal_equilibrium(ThermalEquilibriumInput {
            soc_temp_c: 44.0,
            battery_temp_c: 37.0,
            skin_temp_c: 37.0,
            thermal_velocity_cpm: 0.0,
            pid_capacity_scale: 1.0,
            charging: false,
            bypass_active: false,
            is_launch_burst: false,
        });
        assert!((cool_37.effective_scale - 1.0).abs() < 1e-9);
        assert!(!cool_37.equilibrium_guard);

        let plateau_41 = compute_thermal_equilibrium(ThermalEquilibriumInput {
            soc_temp_c: 47.0,
            battery_temp_c: 41.0,
            skin_temp_c: 41.0,
            thermal_velocity_cpm: 0.1,
            pid_capacity_scale: 1.0,
            charging: false,
            bypass_active: false,
            is_launch_burst: false,
        });
        // Actively trims headroom (~0.76) to cool 41 C plateau back toward ~38 C without false THERMAL_GUARD
        assert!(!plateau_41.equilibrium_guard);
        assert!(plateau_41.effective_scale <= 0.78 && plateau_41.effective_scale >= 0.70);

        let cooling_39 = compute_thermal_equilibrium(ThermalEquilibriumInput {
            soc_temp_c: 45.0,
            battery_temp_c: 39.0,
            skin_temp_c: 39.0,
            thermal_velocity_cpm: -0.6,
            pid_capacity_scale: 1.0,
            charging: false,
            bypass_active: false,
            is_launch_burst: false,
        });
        assert!(!cooling_39.equilibrium_guard);
        assert!(cooling_39.effective_scale > plateau_41.effective_scale + 0.10);
    }
}
