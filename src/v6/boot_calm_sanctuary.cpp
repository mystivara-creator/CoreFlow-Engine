/**
 * @file boot_calm_sanctuary.cpp
 * @brief CoreFlow v6.2: Adaptive Post-Boot Thermal Tranquility Protocol implementation.
 * @license Apache-2.0
 */
#include "coreflow/v6/boot_calm_sanctuary.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>

namespace coreflow::v6 {

double BootCalmSanctuary::read_kernel_uptime_sec(const std::string& procfs_root) noexcept {
    std::ifstream in(procfs_root + "/uptime");
    if (!in.is_open()) return -1.0;
    double uptime_sec = -1.0;
    if (!(in >> uptime_sec) || !std::isfinite(uptime_sec) || uptime_sec < 0.0) {
        return -1.0;
    }
    return uptime_sec;
}

void BootCalmSanctuary::initialize(double initial_kernel_uptime_sec, uint64_t now_ms) noexcept {
    initialized_ = true;
    settle_start_ms_ = now_ms;
    settle_exit_ms_ = now_ms;
    calm_since_ms_ = 0U;
    settle_duration_sec_ = 0.0;

    if (!std::isfinite(initial_kernel_uptime_sec) || initial_kernel_uptime_sec < 0.0 ||
        initial_kernel_uptime_sec >= kMaxUptimeSec) {
        // Warm daemon restart or long-running device: no post-boot storm present.
        settling_ = false;
        last_ramp_alpha_ = 1.0;
        exit_reason_ = "warm_daemon_restart";
    } else {
        settling_ = true;
        last_ramp_alpha_ = 0.0;
        exit_reason_ = "settling";
    }
}

BootCalmDecision BootCalmSanctuary::evaluate(const BootCalmInput& input) noexcept {
    if (!initialized_) {
        initialize(input.kernel_uptime_sec, input.now_ms);
    }

    BootCalmDecision out{};
    const double uptime = (std::isfinite(input.kernel_uptime_sec) && input.kernel_uptime_sec >= 0.0)
        ? input.kernel_uptime_sec
        : kMaxUptimeSec;

    if (settling_) {
        if (input.now_ms >= settle_start_ms_) {
            settle_duration_sec_ = static_cast<double>(input.now_ms - settle_start_ms_) / 1000.0;
        }

        const bool cpu_calm = std::isfinite(input.cpu_util_pct) && input.cpu_util_pct < kCalmCpuUtilPct;
        const bool io_calm = !std::isfinite(input.io_psi_pct) || input.io_psi_pct < kCalmIoPsiPct;
        const bool temp_calm = !std::isfinite(input.soc_temp_c) || input.soc_temp_c < 0.0 ||
                               input.soc_temp_c < kCalmSocTempC;
        const bool slope_calm = !std::isfinite(input.thermal_velocity_cpm) ||
                                input.thermal_velocity_cpm <= kCalmThermalVelCpm;
        const bool compute_io_calm = cpu_calm && io_calm;

        if (compute_io_calm) {
            if (calm_since_ms_ == 0U || input.now_ms < calm_since_ms_) {
                calm_since_ms_ = input.now_ms;
            }
        } else {
            calm_since_ms_ = 0U;
        }

        const bool held_calm_5s = calm_since_ms_ > 0U && input.now_ms >= calm_since_ms_ &&
                                  (input.now_ms - calm_since_ms_) >= kQuiescenceHoldMs;
        const bool quiescence_met = (uptime >= kMinUptimeSec) && held_calm_5s && temp_calm && slope_calm;
        const bool timeout_met = (uptime >= kMaxUptimeSec);

        // Screen-On Gate: never perform Late-Baseline Snapshot or Soft-Start while ScreenOff,
        // so OEM factory baselines captured are active display baselines, never deep-sleep values.
        if (!input.screen_off && (quiescence_met || timeout_met)) {
            settling_ = false;
            settle_exit_ms_ = input.now_ms;
            exit_reason_ = quiescence_met ? "quiescence_met" : "max_uptime_timeout";
            last_ramp_alpha_ = 0.0;

            out.boot_settle_active = false;
            out.needs_late_baseline_snapshot = true;
            out.allow_learning = true;
            out.allow_uclamp = true;
            out.allow_envelope_and_semantic = false;
            out.allow_storage_and_symbiotic = false;
            out.ramp_alpha = 0.0;
            out.ramp_uclamp_scale = 0.88;
            out.settle_duration_sec = settle_duration_sec_;
            out.exit_reason = exit_reason_;
            return out;
        }

        // Still inside Phase 1 (BOOT_SETTLE — Zero-Write Passive Sanctuary)
        out.boot_settle_active = true;
        out.needs_late_baseline_snapshot = false;
        out.allow_learning = false;
        out.allow_touch_soft_uclamp =
            !input.screen_off && !input.thermal_guard && input.touch_active && input.real_foreground_app;
        out.allow_uclamp = false;
        out.allow_envelope_and_semantic = false;
        out.allow_storage_and_symbiotic = false;
        out.ramp_alpha = 0.0;
        out.ramp_uclamp_scale = 0.88;
        out.settle_duration_sec = settle_duration_sec_;
        out.exit_reason = exit_reason_;
        return out;
    }

    // Warm daemon restart skips Phase 3 ramp immediately.
    if (exit_reason_ == "warm_daemon_restart") {
        last_ramp_alpha_ = 1.0;
        out.boot_settle_active = false;
        out.allow_learning = true;
        out.allow_uclamp = true;
        out.allow_envelope_and_semantic = true;
        out.allow_storage_and_symbiotic = true;
        out.ramp_alpha = 1.0;
        out.ramp_uclamp_scale = 1.0;
        out.settle_duration_sec = settle_duration_sec_;
        out.exit_reason = exit_reason_;
        return out;
    }

    // Phase 3: Exponential Soft-Start Ramp across 30s: alpha(t) = 1 - exp(-t / 15s)
    const double ramp_elapsed_sec = input.now_ms >= settle_exit_ms_
        ? static_cast<double>(input.now_ms - settle_exit_ms_) / 1000.0
        : 0.0;
    const bool ramp_done = ramp_elapsed_sec >= kRampCompleteSec;
    const double alpha = ramp_done
        ? 1.0
        : std::clamp(1.0 - std::exp(-ramp_elapsed_sec / kRampTauSec), 0.0, 1.0);
    last_ramp_alpha_ = alpha;

    out.boot_settle_active = false;
    out.needs_late_baseline_snapshot = false;
    out.allow_learning = true;
    out.allow_uclamp = true;
    out.allow_envelope_and_semantic = (ramp_elapsed_sec >= kRampStageEnvelopeSec);
    out.allow_storage_and_symbiotic = (ramp_elapsed_sec >= kRampStageFullSec);
    out.ramp_alpha = alpha;
    out.ramp_uclamp_scale = std::clamp(0.88 + 0.12 * alpha, 0.88, 1.0);
    out.settle_duration_sec = settle_duration_sec_;
    out.exit_reason = exit_reason_;
    return out;
}

} // namespace coreflow::v6
