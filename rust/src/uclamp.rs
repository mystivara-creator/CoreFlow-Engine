use crate::bridge::ffi::{UclampInput, UclampTarget};

/// ContextMode integer encoding shared across C++ <-> Rust FFI:
/// 0 = Efficiency, 1 = Interactive, 2 = LaunchBurst, 3 = Gaming, 4 = ScreenOff, 5 = Excluded
const MODE_INTERACTIVE: u8 = 1;
const MODE_LAUNCH_BURST: u8 = 2;
const MODE_GAMING: u8 = 3;

/// PerformanceProfile integer encoding shared across C++ <-> Rust FFI:
/// 0 = Balanced, 1 = Smoothness, 2 = Battery, 3 = Gaming
const PROFILE_SMOOTHNESS: u8 = 1;
const PROFILE_BATTERY: u8 = 2;
const PROFILE_GAMING: u8 = 3;

/// Zero-allocation, memory-safe EAS UClamp target planner with thermal-velocity pre-damping.
pub fn compute_uclamp_target(input: UclampInput) -> UclampTarget {
    let bounded_scale = if input.capacity_scale.is_finite() {
        input.capacity_scale.clamp(0.25, 1.0)
    } else {
        1.0
    };

    if input.mode != MODE_INTERACTIVE
        && input.mode != MODE_LAUNCH_BURST
        && input.mode != MODE_GAMING
    {
        return UclampTarget {
            util_min: 0,
            util_max: 1024,
        };
    }

    let big_cap = f64::from(input.big_capacity.clamp(512, 1024));
    let max_little = (big_cap * 0.55) as u32;
    let little_cap = if input.little_capacity > 0 {
        f64::from(input.little_capacity.clamp(128, max_little.max(128)))
    } else {
        big_cap * 0.40
    };
    let little_u32 = little_cap as u32;
    let big_u32 = big_cap as u32;
    let mid_cap = if input.mid_capacity > little_u32 {
        f64::from(input.mid_capacity.clamp(little_u32, big_u32))
    } else {
        (little_cap + big_cap) * 0.50
    };

    let hz = f64::from(input.refresh_hz.clamp(60, 240));
    let hz_factor = (1.0 + (hz / 60.0).log2() * 0.18).clamp(1.0, 1.28);

    let psi_norm = if input.cpu_psi.is_finite() {
        (input.cpu_psi / 100.0).clamp(0.0, 1.0)
    } else {
        0.0
    };
    let gpu_norm = if input.gpu_busy_pct.is_finite() {
        (input.gpu_busy_pct / 100.0).clamp(0.0, 1.0)
    } else {
        0.0
    };
    let render_norm = if input.render_pressure.is_finite() {
        input.render_pressure.clamp(0.0, 1.0)
    } else {
        0.0
    };
    let starvation_norm = if input.starvation_pressure.is_finite() && input.starvation_pressure >= 0.0
    {
        input.starvation_pressure.clamp(0.0, 1.0)
    } else {
        0.0
    };
    let starvation_rescue = starvation_norm >= 0.20;

    // Unified Zero-Profile bias: PROFILE_BATTERY represents automatic Low-Battery Eco (<= 20% discharging)
    // applying a smooth ~15% trim while preserving full jank rescue when frames drop.
    let profile_bias = match input.profile {
        PROFILE_SMOOTHNESS => 1.18,
        PROFILE_GAMING => 1.15,
        PROFILE_BATTERY => 0.85,
        _ => 1.00,
    };

    let anchor_floor = match input.mode {
        MODE_LAUNCH_BURST => {
            (mid_cap * 0.82 + little_cap * 0.18) * (1.0 + psi_norm * 0.15 + render_norm * 0.12)
        }
        MODE_INTERACTIVE => {
            little_cap * (0.68 + psi_norm * 0.35 + render_norm * 0.42) * hz_factor
        }
        MODE_GAMING => {
            let load_blend =
                (0.35 + gpu_norm * 0.45 + psi_norm * 0.25 + render_norm * 0.30).clamp(0.20, 1.0);
            (little_cap + (mid_cap - little_cap) * load_blend) * hz_factor
        }
        _ => {
            return UclampTarget {
                util_min: 0,
                util_max: 1024,
            };
        }
    };

    // Closed-loop frame pacing feedback (ADPF-class) + Feed-Forward Pre-Emptive Cancel:
    // - Verified zero/near-zero jank (< 1.5% missed frames) -> gently trim floor to save power & heat,
    //   UNLESS feedforward_freeze or runqueue starvation_rescue is active,
    //   which freezes trimming pre-emptively before frames can miss T_budget = 1000 / Hz ms.
    // - Measured jank (>= 3% missed frames) -> inject proportional rescue headroom immediately.
    let jank_factor = if input.jank_ratio.is_finite() && input.jank_ratio >= 0.0 {
        if input.jank_ratio < 0.015 && input.mode != MODE_LAUNCH_BURST {
            if input.feedforward_freeze || starvation_rescue {
                1.0
            } else if input.mode == MODE_GAMING {
                0.88
            } else {
                0.84
            }
        } else if input.jank_ratio >= 0.03 {
            let sev = ((input.jank_ratio - 0.03) / 0.25).clamp(0.0, 1.0);
            1.0 + sev * 0.20
        } else {
            1.0
        }
    } else {
        1.0
    };

    // Universal schedstat runqueue starvation rescue (works across all engines & non-LTPO panels):
    // When critical foreground / SurfaceFlinger threads sit runnable waiting for a CPU, lift floor up to +18%.
    let starvation_boost = if starvation_rescue {
        let sev = ((starvation_norm - 0.20) / 0.80).clamp(0.0, 1.0);
        1.0 + sev * 0.18
    } else {
        1.0
    };

    // Learned Per-App Behavioral DNA bias (defaults cleanly to 1.0 when unknown/unset).
    let app_bias = if input.app_uclamp_bias.is_finite() && input.app_uclamp_bias > 0.0 {
        input.app_uclamp_bias.clamp(0.75, 1.20)
    } else {
        1.0
    };

    // Orthogonal Symbiotic Orchestrator cooperative CPU scale (defaults cleanly to 1.0).
    let symbiotic_scale = if input.symbiotic_cpu_scale.is_finite() && input.symbiotic_cpu_scale > 0.0
    {
        input.symbiotic_cpu_scale.clamp(0.88, 1.10)
    } else {
        1.0
    };

    // Thermal-velocity pre-damping: gently ease floor (up to 12%) when heating rapidly (>= +1.8 C/min)
    // so sustained workloads stay smooth instead of slamming into OEM cliff-throttling.
    let velocity_damping = if input.thermal_velocity_cpm.is_finite() && input.thermal_velocity_cpm > 1.8
    {
        let excess = (input.thermal_velocity_cpm - 1.8).clamp(0.0, 6.0);
        1.0 - (excess / 6.0) * 0.12
    } else {
        1.0
    };

    let mut target_max = (big_cap * bounded_scale).round().clamp(0.0, 1024.0) as u32;
    let max_safe_floor = big_cap * 0.75;
    let base_min = (anchor_floor
        * profile_bias
        * jank_factor
        * starvation_boost
        * app_bias
        * symbiotic_scale
        * bounded_scale.clamp(0.50, 1.0))
    .min(max_safe_floor);
    // Warm-zone Little-cluster steering: when chassis is in Cool-Glide (scale < 0.92) during
    // light Interactive scrolling/browsing (render < 0.35, no jank, no runqueue starvation), trim
    // floor smoothly so routine UI work stays on cool Little cores instead of waking hot Big cores.
    let jank_rescue = input.jank_ratio.is_finite() && input.jank_ratio >= 0.03;
    let warm_efficiency = if input.mode == MODE_INTERACTIVE
        && bounded_scale < 0.92
        && render_norm < 0.35
        && !jank_rescue
        && !starvation_rescue
    {
        (bounded_scale / 0.92).clamp(0.65, 1.0)
    } else {
        1.0
    };

    let mut raw_min = base_min * velocity_damping * warm_efficiency;

    if input.thermal_guard {
        let guard_cap = (big_cap * 0.75).round().clamp(0.0, 1024.0) as u32;
        target_max = target_max.min(guard_cap);
        raw_min = raw_min.min(little_cap * 0.70);
    }

    let final_min = (raw_min.round().clamp(0.0, 1024.0) as u32).min(target_max);
    UclampTarget {
        util_min: final_min,
        util_max: final_min.max(target_max),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn base_input(mode: u8) -> UclampInput {
        UclampInput {
            mode,
            profile: 0,
            capacity_scale: 1.0,
            thermal_guard: false,
            little_capacity: 384,
            mid_capacity: 768,
            big_capacity: 1024,
            refresh_hz: 120,
            cpu_psi: 5.0,
            gpu_busy_pct: 40.0,
            render_pressure: 0.3,
            thermal_velocity_cpm: 0.0,
            jank_ratio: -1.0,
            app_uclamp_bias: 1.0,
            feedforward_freeze: false,
            symbiotic_cpu_scale: 1.0,
            starvation_pressure: -1.0,
        }
    }

    #[test]
    fn non_active_modes_release_uclamp_to_kernel_defaults() {
        for mode in [0u8, 4u8, 5u8] {
            let out = compute_uclamp_target(base_input(mode));
            assert_eq!(out.util_min, 0);
            assert_eq!(out.util_max, 1024);
        }
    }

    #[test]
    fn higher_refresh_and_gaming_raise_uclamp_within_bounds() {
        let mut s60 = base_input(MODE_INTERACTIVE);
        s60.refresh_hz = 60;
        let mut s120 = base_input(MODE_INTERACTIVE);
        s120.refresh_hz = 120;
        let t60 = compute_uclamp_target(s60);
        let t120 = compute_uclamp_target(s120);
        assert!(t120.util_min > t60.util_min);
        assert!(t120.util_min <= t120.util_max && t120.util_max <= 1024);

        let game = compute_uclamp_target(base_input(MODE_GAMING));
        assert!(game.util_min > t120.util_min);
    }

    #[test]
    fn closed_loop_jank_trims_when_smooth_and_rescues_on_drops() {
        let neutral = compute_uclamp_target(base_input(MODE_GAMING));
        let mut smooth = base_input(MODE_GAMING);
        smooth.jank_ratio = 0.0;
        let trimmed = compute_uclamp_target(smooth);
        assert!(trimmed.util_min < neutral.util_min);

        let mut janky = base_input(MODE_GAMING);
        janky.jank_ratio = 0.20;
        let rescued = compute_uclamp_target(janky);
        assert!(rescued.util_min > neutral.util_min);
        assert!(rescued.util_min <= 768);
    }

    #[test]
    fn feedforward_freeze_cancels_jank_trim_preemptively_then_resumes() {
        let neutral = compute_uclamp_target(base_input(MODE_INTERACTIVE));
        let mut frozen = base_input(MODE_INTERACTIVE);
        frozen.jank_ratio = 0.002;
        frozen.feedforward_freeze = true;
        let held = compute_uclamp_target(frozen);
        assert_eq!(held.util_min, neutral.util_min);

        let mut steady = base_input(MODE_INTERACTIVE);
        steady.jank_ratio = 0.002;
        steady.feedforward_freeze = false;
        let resumed_trim = compute_uclamp_target(steady);
        assert!(resumed_trim.util_min < held.util_min);
    }

    #[test]
    fn thermal_velocity_predamps_and_guard_clamps_safely() {
        let steady = compute_uclamp_target(base_input(MODE_GAMING));
        let mut heating = base_input(MODE_GAMING);
        heating.thermal_velocity_cpm = 5.0;
        let damped = compute_uclamp_target(heating);
        assert!(damped.util_min < steady.util_min);

        let mut guarded = base_input(MODE_LAUNCH_BURST);
        guarded.thermal_guard = true;
        let g = compute_uclamp_target(guarded);
        assert!(g.util_max <= 768);
        assert!(g.util_min <= 269);
    }

    #[test]
    fn nan_and_infinite_inputs_stay_bounded() {
        let mut bad = base_input(MODE_GAMING);
        bad.capacity_scale = f64::NAN;
        bad.cpu_psi = f64::INFINITY;
        bad.gpu_busy_pct = f64::NEG_INFINITY;
        bad.render_pressure = f64::NAN;
        bad.thermal_velocity_cpm = f64::NAN;
        bad.jank_ratio = f64::NAN;
        bad.app_uclamp_bias = f64::INFINITY;
        bad.symbiotic_cpu_scale = f64::NAN;
        let out = compute_uclamp_target(bad);
        assert!(out.util_min <= out.util_max && out.util_max <= 1024);
    }

    #[test]
    fn launch_burst_never_pins_util_min_to_full_capacity() {
        let mut burst = base_input(MODE_LAUNCH_BURST);
        burst.profile = PROFILE_GAMING;
        burst.little_capacity = 770;
        burst.mid_capacity = 0;
        burst.big_capacity = 1024;
        burst.cpu_psi = 60.0;
        burst.render_pressure = 0.9;
        let out = compute_uclamp_target(burst);
        assert!(out.util_min <= 768, "util_min should stay <= 75% big capacity, got {}", out.util_min);
        assert_eq!(out.util_max, 1024);
    }

    #[test]
    fn warm_zone_interactive_steers_light_scroll_without_hurting_heavy_render() {
        let mut light_cool = base_input(MODE_INTERACTIVE);
        light_cool.capacity_scale = 1.0;
        light_cool.render_pressure = 0.16;

        let mut light_warm = base_input(MODE_INTERACTIVE);
        light_warm.capacity_scale = 0.76; // 41 C Cool-Glide zone
        light_warm.render_pressure = 0.16;
        light_warm.feedforward_freeze = true; // active touch scroll window

        let mut heavy_warm = base_input(MODE_INTERACTIVE);
        heavy_warm.capacity_scale = 0.76;
        heavy_warm.render_pressure = 0.65; // heavy rendering demands full headroom

        let t_cool = compute_uclamp_target(light_cool);
        let t_light_warm = compute_uclamp_target(light_warm);
        let t_heavy_warm = compute_uclamp_target(heavy_warm);

        assert!(t_light_warm.util_min < (t_cool.util_min * 75) / 100);
        assert!(t_heavy_warm.util_min > t_light_warm.util_min + 60);
    }

    #[test]
    fn low_battery_eco_trims_smoothly_and_preserves_jank_rescue() {
        let mut normal = base_input(MODE_INTERACTIVE);
        normal.profile = 0; // unified autonomous
        normal.jank_ratio = -1.0;

        let mut eco = base_input(MODE_INTERACTIVE);
        eco.profile = PROFILE_BATTERY; // dynamic Low-Battery Auto-Eco (<= 20% discharging)
        eco.jank_ratio = -1.0;

        let mut eco_jank = base_input(MODE_INTERACTIVE);
        eco_jank.profile = PROFILE_BATTERY;
        eco_jank.jank_ratio = 0.08; // 8% frame drops -> rescue must still lift floor

        let t_norm = compute_uclamp_target(normal);
        let t_eco = compute_uclamp_target(eco);
        let t_eco_jank = compute_uclamp_target(eco_jank);

        assert!(t_eco.util_min < t_norm.util_min);
        assert!(t_eco.util_min >= (t_norm.util_min * 78) / 100);
        assert!(t_eco_jank.util_min > t_eco.util_min);
    }

    #[test]
    fn schedstat_starvation_cancels_trim_lifts_floor_and_respects_thermal_guard() {
        let make = |starv: f64, guard: bool| {
            let mut inp = base_input(MODE_INTERACTIVE);
            inp.jank_ratio = 0.0;
            inp.capacity_scale = 0.80;
            inp.render_pressure = 0.20;
            inp.starvation_pressure = starv;
            inp.thermal_guard = guard;
            inp
        };
        let t_trimmed = compute_uclamp_target(make(-1.0, false));
        let t_rescued = compute_uclamp_target(make(0.75, false));
        assert!(
            t_rescued.util_min > t_trimmed.util_min + 40,
            "starvation should cancel trim/warm-efficiency and boost floor: trimmed={} rescued={}",
            t_trimmed.util_min,
            t_rescued.util_min
        );

        let t_guarded = compute_uclamp_target(make(0.75, true));
        assert!(t_guarded.util_max <= 768);
        assert!(t_guarded.util_min <= 269);
    }
}
