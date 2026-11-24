/**
 * @file edp_knee_calibrator.hpp
 * @brief CoreFlow v6.2+ Phase 3: Online Silicon EDP Knee Discovery (`silicon_edp.map`, CFEDP2).
 *
 * Calibrates each device's actual C*V^2*f dynamic power curve P(s) = P_base + k * s^alpha
 * directly from live discharging power and normalized CPU/GPU frequency telemetry:
 * - Zero-heap hot path (fixed 8-bin scale array across [0.25..1.00]).
 * - Zero hardcoded per-SoC lookup tables.
 * - Numerically safe log-linear regression (strict positive epsilon before log()) that fails
 *   closed (calibrated = false) on flat, non-monotonic, or wildly noisy curves.
 * - Discovers the silicon EDP knee (highest scale before marginal power/performance slope > 1.65,
 *   clamped safely to [0.55, 0.88]) and provides a C0-continuous activation blend across
 *   confidence 50..70 so thermal power-budget and uclamp.max transitions never step-cliff.
 * - Zero-Wear atomic POSIX persistence (openat + fsync + close + renameat + parent-dir fsync)
 *   only on ScreenOff significant drift or clean shutdown, auto-resetting on kernel/ROM OTA.
 * @license Apache-2.0
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace coreflow::v6 {

struct EdpObservation {
    uint64_t now_ms{0};
    double effective_scale{0.0}; // Normalized silicon freq/load scale in [0.25..1.00]
    double power_mw{0.0};        // Live discharging system power in mW
    bool power_available{false};
    bool charging{false};
    bool screen_off{false};
    double cpu_util_pct{-1.0};   // Optional CPU utilization (0..100, -1 if unavailable)
    double gpu_busy_pct{-1.0};   // Optional GPU busy percentage (0..100, -1 if unavailable)
    bool steady_state_checked{false}; // True when caller enforces multi-tick steady-state settling
    bool steady_state_ok{true};       // False during abrupt transient bursts before fuel-gauge settles
};

struct EdpSnapshot {
    bool calibrated{false};         // True only when confidence >= 50 and all fit gates pass
    uint32_t confidence{0};         // 0..100
    double blend{0.0};              // C0 smooth blend across confidence 50..70 (0.0..1.0)
    double knee_scale{0.78};        // Clamped [0.55, 0.88] when calibrated; 0.78 neutral default
    double power_exponent{1.8};     // Raw calibrated alpha in [1.35, 2.80]; 1.8 default
    double effective_exponent{1.8}; // C0-blended: 1.8 + blend * (power_exponent - 1.8)
    uint32_t total_samples{0};
    uint32_t populated_bins{0};
    uint32_t flush_count{0};
};

class EdpKneeCalibrator {
public:
    static constexpr size_t kNumBins = 8U;
    static constexpr double kMinScale = 0.25;
    static constexpr double kMaxScale = 1.00;
    static constexpr double kDefaultExponent = 1.8;
    static constexpr double kMinExponent = 1.35;
    static constexpr double kMaxExponent = 2.80;
    static constexpr double kKneeSlopeThreshold = 1.65;
    static constexpr double kMinKneeScale = 0.55;
    static constexpr double kMaxKneeScale = 0.88;
    static constexpr double kMinBinEwmaAlpha = 0.025;
    static constexpr uint32_t kMinConfidence = 50U;
    static constexpr uint32_t kFullBlendConfidence = 70U;
    static constexpr uint32_t kMinPopulatedBins = 4U;
    static constexpr uint32_t kMinTotalSamples = 24U;

    EdpKneeCalibrator() noexcept = default;

    // Loads persisted CFEDP2 map if kernel_release and rom_fingerprint match;
    // otherwise auto-resets (including legacy CFEDP1 upgrades and poisoned uncalibrated caches).
    // automatically resets & unlinks on kernel/ROM OTA update.
    void initialize(const std::string& state_dir,
                    const std::string& kernel_release,
                    const std::string& rom_fingerprint) noexcept;

    // Zero-heap hot-path observation ingest and online curve refit.
    void observe(const EdpObservation& obs) noexcept;

    // Zero-Wear atomic persistence on ScreenOff (only if significant calibration drift occurred).
    [[nodiscard]] bool flush_if_significant() noexcept;

    // Unconditional atomic persistence on clean daemon shutdown (only if dirty).
    [[nodiscard]] bool flush_now() noexcept;

    // Resets all learned bins and removes persisted map.
    void clear_all() noexcept;

    [[nodiscard]] EdpSnapshot snapshot() const noexcept;
    [[nodiscard]] bool dirty() const noexcept { return dirty_; }
    [[nodiscard]] uint32_t flush_count() const noexcept { return flush_count_; }

private:
    struct ScaleBin {
        uint32_t count{0};
        double mean_scale{0.0};
        double mean_power_mw{0.0};
        double ewma_abs_dev_mw{0.0};
    };

    void recompute_locked() noexcept;
    void load_locked() noexcept;
    [[nodiscard]] bool persist_locked() noexcept;

    std::string state_dir_;
    std::string map_path_;
    std::string kernel_release_;
    std::string rom_fingerprint_;

    std::array<ScaleBin, kNumBins> bins_{};
    uint32_t total_samples_{0};
    double min_idle_power_mw_{-1.0};
    uint64_t last_obs_ms_{0};
    double last_obs_scale_{-1.0};

    bool calibrated_{false};
    uint32_t confidence_{0};
    double knee_scale_{0.78};
    double power_exponent_{kDefaultExponent};
    uint32_t populated_bins_{0};

    // Zero-Wear persistence drift tracking
    bool persisted_calibrated_{false};
    uint32_t persisted_confidence_{0};
    double persisted_knee_scale_{0.78};
    double persisted_exponent_{kDefaultExponent};
    bool dirty_{false};
    bool significant_dirty_{false};
    uint32_t flush_count_{0};
};

} // namespace coreflow::v6
