/**
 * @file app_dna_store.hpp
 * @brief CoreFlow v6.2: Persistent Per-App Behavioral DNA Store.
 *
 * Silently learns each frequently used app's optimal UClamp bias (lowest floor
 * that stays jank-free), thermal slope, and launch frequency in a compact LRU
 * table (< 8 KB on disk, atomic write-temp-fsync-rename on ScreenOff/shutdown).
 * @license Apache-2.0
 */
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace coreflow::v6 {

enum class EngineSignature : uint8_t {
    StandardUi = 0,
    HeavyGame3D = 1,
};

struct ContextLearningSample {
    double jank_ratio{-1.0};           // 0.0..1.0 (< 0 if frame sensor unavailable)
    double measured_fps{0.0};          // Live measured FPS (0 if unavailable)
    double target_fps{60.0};           // Panel target FPS
    double thermal_velocity_cpm{0.0};  // °C/min thermal rise velocity
    double power_mw{0.0};              // Instantaneous battery power draw in mW (0 if unavailable)
    bool charging{false};              // External power/charger connected
    bool low_battery{false};           // Battery <= 20%
    bool in_wake_ramp{false};          // Boot-Calm / wake ramp still stabilizing
    double capacity_scale{1.0};        // Current thermal PID capacity scale (0.25..1.0)
};

struct AppDnaEntry {
    std::string package_name;
    double uclamp_bias{1.0};                  // 0.78..1.18 learned optimal UClamp multiplier
    double avg_thermal_slope{0.0};            // EWMA °C/min during active use
    double avg_jank_ratio{0.0};               // EWMA missed-frame ratio (0.0..1.0)
    double avg_power_mw{0.0};                 // EWMA power draw in mW
    double efficiency_weight{1.0};            // 0.80..1.15 cost-to-gain efficiency weight
    uint32_t launch_count{0};
    uint32_t sample_count{0};
    uint64_t last_seen_ms{0};
    // Persisted snapshot baselines for zero-wear significant-drift flush gating:
    double persisted_uclamp_bias{1.0};
    double persisted_efficiency_weight{1.0};
    bool persisted_on_disk{false};
};

class AppDnaStore {
public:
    static constexpr size_t kMaxEntries = 64U;
    static constexpr double kBayesianHeavyGamePrior = 1.10;
    static constexpr uint32_t kBayesianPriorSamples = 3U;
    static constexpr double kMinBiasDriftForFlush = 0.025;
    static constexpr double kMinEfficiencyDriftForFlush = 0.05;

    AppDnaStore() noexcept = default;

    // Loads existing DNA table (CFDNA2 or backward-compatible CFDNA1) from <state_dir>/app_dna.map.
    void initialize(const std::string& state_dir) noexcept;

    // Records a foreground app launch event with optional Bayesian engine-signature prior.
    void record_launch(const std::string& package_name,
                       uint64_t now_ms,
                       EngineSignature signature = EngineSignature::StandardUi) noexcept;

    // Updates per-app EWMA behavioral DNA in RAM using full cost-to-gain context physics.
    void observe_context(const std::string& package_name,
                         const ContextLearningSample& sample,
                         uint64_t now_ms,
                         EngineSignature signature = EngineSignature::StandardUi) noexcept;

    // Legacy compatibility wrapper delegating to observe_context().
    void observe(const std::string& package_name,
                 double jank_ratio,
                 double thermal_velocity_cpm,
                 uint64_t now_ms,
                 EngineSignature signature = EngineSignature::StandardUi) noexcept;

    // Returns the learned optimal UClamp multiplier for this package (1.0 if unknown).
    [[nodiscard]] double uclamp_bias_for(const std::string& package_name) const noexcept;

    // Returns the learned cost-to-gain efficiency weight for this package (1.0 if unknown).
    [[nodiscard]] double efficiency_weight_for(const std::string& package_name) const noexcept;

    // True iff a new app was learned or any entry drifted past zero-wear significance thresholds.
    [[nodiscard]] bool has_significant_drift() const noexcept;

    // Zero-wear atomic flush: writes CFDNA2 ONLY when has_significant_drift() is true.
    bool flush_if_significant() noexcept;

    // Unconditional atomic flush when dirty (e.g., clean daemon shutdown).
    bool flush_if_dirty() noexcept;

    [[nodiscard]] size_t learned_count() const noexcept { return entries_.size(); }
    [[nodiscard]] uint64_t flush_count() const noexcept { return flush_count_; }
    [[nodiscard]] std::vector<AppDnaEntry> snapshot_entries() const;

private:
    void evict_lru_if_needed() noexcept;
    bool flush_atomic_now() noexcept;

    std::string store_path_;
    std::unordered_map<std::string, AppDnaEntry> entries_;
    bool dirty_{false};
    uint64_t flush_count_{0};
};

} // namespace coreflow::v6
