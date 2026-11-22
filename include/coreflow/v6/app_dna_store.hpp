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

namespace coreflow::v6 {

struct AppDnaEntry {
    std::string package_name;
    double uclamp_bias{1.0};        // 0.78..1.18 learned optimal UClamp multiplier
    double avg_thermal_slope{0.0};  // EWMA °C/min during active use
    double avg_jank_ratio{0.0};     // EWMA missed-frame ratio (0.0..1.0)
    uint32_t launch_count{0};
    uint32_t sample_count{0};
    uint64_t last_seen_ms{0};
};

class AppDnaStore {
public:
    static constexpr size_t kMaxEntries = 64U;

    AppDnaStore() noexcept = default;

    // Loads existing DNA table from <state_dir>/app_dna.map (safe if missing/corrupt).
    void initialize(const std::string& state_dir) noexcept;

    // Records a foreground app launch event.
    void record_launch(const std::string& package_name, uint64_t now_ms) noexcept;

    // Updates per-app EWMA behavioral DNA from live closed-loop jank & thermal slope.
    void observe(const std::string& package_name,
                 double jank_ratio,
                 double thermal_velocity_cpm,
                 uint64_t now_ms) noexcept;

    // Returns the learned optimal UClamp multiplier for this package (1.0 if unknown).
    [[nodiscard]] double uclamp_bias_for(const std::string& package_name) const noexcept;

    // Persists table atomically to disk if modified since last flush.
    bool flush_if_dirty() noexcept;

    [[nodiscard]] size_t learned_count() const noexcept { return entries_.size(); }

private:
    void evict_lru_if_needed() noexcept;

    std::string store_path_;
    std::unordered_map<std::string, AppDnaEntry> entries_;
    bool dirty_{false};
};

} // namespace coreflow::v6
