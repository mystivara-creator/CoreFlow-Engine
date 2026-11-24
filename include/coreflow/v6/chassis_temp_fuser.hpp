/**
 * @file chassis_temp_fuser.hpp
 * @brief CoreFlow v6.1 Zero-DC-Bias Complementary Chassis Temperature Fuser.
 *
 * Some Android fuel-gauge / battery thermal zones report integer 1.0°C steps
 * (e.g. Qualcomm thermal_zone52 "battery"), whereas sibling board/skin sensors
 * (skin, xo-therm, ap_therm) report 0.001°C millidegree resolution with a static
 * thermal offset. This zero-heap complementary fuser detects 1°C battery
 * quantization automatically and borrows only bounded zero-mean AC detail
 * (±0.65°C) from the finest available slow auxiliary sensor while preserving
 * exact DC agreement with physical battery temperature.
 *
 * Safety invariant: hard thermal cutoffs (>= 44°C guard, >= 58°C emergency) and
 * batteryTemp telemetry ALWAYS read raw physical battery temperature; the fused
 * temperature is strictly an estimator/velocity signal.
 *
 * @license Apache-2.0
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace coreflow::v6 {

class ChassisTempFuser {
public:
    static constexpr double kFilterTauSec = 45.0;
    static constexpr double kAcGain = 0.75;
    static constexpr double kMaxAcOffsetC = 0.65;
    static constexpr double kMaxBattDeviationC = 0.60;
    static constexpr double kMaxResetGapSec = 30.0;
    static constexpr double kFlatlineGuardSec = 180.0;
    static constexpr double kFlatlineAuxDriftC = 3.0;
    static constexpr double kMaxAuxLpStepC = 8.0;

    ChassisTempFuser() noexcept = default;

    void reset() noexcept {
        have_prev_batt_ = false;
        prev_batt_c_ = -1.0;
        batt_fine_obs_ = 0U;
        batt_coarse_steps_ = 0U;
        batt_int_streak_ = 0U;
        batt_unchanged_sec_ = 0.0;
        have_aux_at_batt_change_ = false;
        aux_at_batt_change_c_ = -1.0;
        battery_coarse_1c_ = false;
        battery_flatlined_ = false;

        skin_tracker_ = {};
        board_tracker_ = {};
        active_aux_source_ = AuxSource::None;
        lp_initialized_ = false;
        batt_lp_c_ = -1.0;
        aux_lp_c_ = -1.0;
        fused_active_ = false;
        last_fused_c_ = -1.0;
    }

    // Updates fuser with current raw physical sensors and returns the chassis
    // temperature to feed ThermalModelEstimator / thermal velocity trend.
    [[nodiscard]] double update(double battery_c,
                                double skin_c,
                                double board_avg_c,
                                double dt_sec) noexcept {
        const bool batt_valid = std::isfinite(battery_c) && battery_c > 5.0 && battery_c < 85.0;
        const bool skin_valid = std::isfinite(skin_c) && skin_c > 5.0 && skin_c < 85.0;
        const bool board_valid = std::isfinite(board_avg_c) && board_avg_c > 5.0 && board_avg_c < 85.0;
        const bool dt_valid = std::isfinite(dt_sec) && dt_sec > 0.0 && dt_sec <= kMaxResetGapSec;

        if (!batt_valid) {
            lp_initialized_ = false;
            fused_active_ = false;
            last_fused_c_ = skin_valid ? skin_c : -1.0;
            return last_fused_c_;
        }

        // 1. Auto-detect coarse (>= 0.5°C integer) battery quantization and flatline stalls.
        const bool batt_changed = observe_battery_precision(battery_c, dt_valid ? dt_sec : 0.0);

        // 2. Track precision of candidate fine slow auxiliary sensors (prefer skin, then board_average).
        if (skin_valid) skin_tracker_.observe(skin_c);
        if (board_valid) board_tracker_.observe(board_avg_c);

        AuxSource chosen = AuxSource::None;
        double aux_c = -1.0;
        if (skin_valid && skin_tracker_.is_fine()) {
            chosen = AuxSource::Skin;
            aux_c = skin_c;
        } else if (board_valid && board_tracker_.is_fine()) {
            chosen = AuxSource::BoardAverage;
            aux_c = board_avg_c;
        }

        if (chosen == AuxSource::None) {
            active_aux_source_ = AuxSource::None;
            lp_initialized_ = false;
            fused_active_ = false;
            last_fused_c_ = battery_c;
            return battery_c;
        }

        if (batt_changed || !have_aux_at_batt_change_ || chosen != active_aux_source_) {
            aux_at_batt_change_c_ = aux_c;
            have_aux_at_batt_change_ = true;
        }

        // Fail-closed flatline guard: if physical battery reading stays completely frozen for
        // >= 180s with either zero historical step transitions OR >= 3.0°C auxiliary drift,
        // stop injecting AC ripple so upstream RLS never mistakes auxiliary motion for battery heat.
        const double aux_drift = std::abs(aux_c - aux_at_batt_change_c_);
        battery_flatlined_ = (batt_unchanged_sec_ >= kFlatlineGuardSec) &&
                             (batt_coarse_steps_ == 0U || aux_drift >= kFlatlineAuxDriftC);

        // 3. Keep complementary low-pass states continuously warmed so enabling fusion is glitch-free.
        if (!lp_initialized_ || chosen != active_aux_source_ || !dt_valid) {
            active_aux_source_ = chosen;
            batt_lp_c_ = battery_c;
            aux_lp_c_ = aux_c;
            lp_initialized_ = true;
        } else {
            const double alpha = 1.0 - std::exp(-dt_sec / kFilterTauSec);
            batt_lp_c_ += alpha * (battery_c - batt_lp_c_);
            const double aux_delta = std::clamp(aux_c - aux_lp_c_, -kMaxAuxLpStepC, kMaxAuxLpStepC);
            aux_lp_c_ += alpha * aux_delta;
        }

        if (!battery_coarse_1c_ || battery_flatlined_) {
            fused_active_ = false;
            last_fused_c_ = battery_c;
            return battery_c;
        }

        // 4. Zero-DC-bias complementary fusion:
        //    At steady state (aux_c - aux_lp_c -> 0), fused converges identically to battery_c.
        //    Between 1°C battery steps, bounded AC detail from skin/board provides smooth resolution.
        const double ac_detail = std::clamp(kAcGain * (aux_c - aux_lp_c_),
                                            -kMaxAcOffsetC, kMaxAcOffsetC);
        const double raw_fused = batt_lp_c_ + ac_detail;
        last_fused_c_ = std::clamp(raw_fused,
                                   battery_c - kMaxBattDeviationC,
                                   battery_c + kMaxBattDeviationC);
        fused_active_ = true;
        return last_fused_c_;
    }

    [[nodiscard]] bool fused_active() const noexcept { return fused_active_; }
    [[nodiscard]] bool battery_coarse_1c() const noexcept { return battery_coarse_1c_; }
    [[nodiscard]] bool battery_flatlined() const noexcept { return battery_flatlined_; }
    [[nodiscard]] double last_fused_celsius() const noexcept { return last_fused_c_; }

private:
    enum class AuxSource : uint8_t {
        None = 0,
        Skin = 1,
        BoardAverage = 2,
    };

    struct AuxPrecisionTracker {
        bool have_prev{false};
        double prev_c{0.0};
        uint32_t fine_obs{0U};

        void observe(double val_c) noexcept {
            const double frac = std::abs(val_c - std::round(val_c));
            if (frac > 0.02) {
                if (fine_obs < 8U) ++fine_obs;
            }
            if (have_prev) {
                const double d = std::abs(val_c - prev_c);
                if (d >= 0.0005 && d < 0.25) {
                    if (fine_obs < 8U) fine_obs += 2U;
                }
            }
            prev_c = val_c;
            have_prev = true;
        }

        [[nodiscard]] bool is_fine() const noexcept { return fine_obs >= 2U; }
    };

    // Returns true when battery_c physically moved by >= 0.05°C.
    bool observe_battery_precision(double battery_c, double dt_sec) noexcept {
        bool changed = !have_prev_batt_;
        const double frac = std::abs(battery_c - std::round(battery_c));
        if (frac > 0.04) {
            if (batt_fine_obs_ < 8U) ++batt_fine_obs_;
            batt_int_streak_ = 0U;
            batt_coarse_steps_ = 0U;
        } else {
            if (batt_int_streak_ < 1000U) ++batt_int_streak_;
            // Allow graceful recovery if a sensor transitions from fine -> coarse 1°C mode.
            if (batt_fine_obs_ > 0U && batt_int_streak_ >= 12U && (batt_int_streak_ % 6U == 0U)) {
                --batt_fine_obs_;
            }
        }
        if (have_prev_batt_) {
            const double d = std::abs(battery_c - prev_batt_c_);
            if (d >= 0.05) {
                changed = true;
                batt_unchanged_sec_ = 0.0;
            } else if (dt_sec > 0.0) {
                batt_unchanged_sec_ = std::min(batt_unchanged_sec_ + dt_sec, 10000.0);
            }
            if (d >= 0.04 && d < 0.45) {
                if (batt_fine_obs_ < 8U) batt_fine_obs_ += 2U;
                batt_coarse_steps_ = 0U;
            } else if (d >= 0.45 && frac <= 0.04) {
                if (batt_coarse_steps_ < 64U) ++batt_coarse_steps_;
                if (batt_fine_obs_ > 0U) --batt_fine_obs_;
            }
        }
        prev_batt_c_ = battery_c;
        have_prev_batt_ = true;
        battery_coarse_1c_ = (batt_fine_obs_ < 2U) &&
                             (batt_coarse_steps_ >= 1U || batt_int_streak_ >= 8U);
        return changed;
    }

    bool have_prev_batt_{false};
    double prev_batt_c_{-1.0};
    uint32_t batt_fine_obs_{0U};
    uint32_t batt_coarse_steps_{0U};
    uint32_t batt_int_streak_{0U};
    double batt_unchanged_sec_{0.0};
    bool have_aux_at_batt_change_{false};
    double aux_at_batt_change_c_{-1.0};
    bool battery_coarse_1c_{false};
    bool battery_flatlined_{false};

    AuxPrecisionTracker skin_tracker_{};
    AuxPrecisionTracker board_tracker_{};
    AuxSource active_aux_source_{AuxSource::None};
    bool lp_initialized_{false};
    double batt_lp_c_{-1.0};
    double aux_lp_c_{-1.0};
    bool fused_active_{false};
    double last_fused_c_{-1.0};
};

} // namespace coreflow::v6
