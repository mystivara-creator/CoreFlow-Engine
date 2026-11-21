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

    let profile_bias = match input.profile {
        PROFILE_SMOOTHNESS => 1.18,
        PROFILE_GAMING => 1.15,
        PROFILE_BATTERY => 0.78,
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
    let base_min = (anchor_floor * profile_bias * bounded_scale.clamp(0.50, 1.0)).min(max_safe_floor);
    let mut raw_min = base_min * velocity_damping;

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
}
