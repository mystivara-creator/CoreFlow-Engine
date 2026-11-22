/**
 * @file boot_calm_sanctuary.hpp
 * @brief CoreFlow v6.2: Adaptive Post-Boot Thermal Tranquility Protocol (Boot-Calm Sanctuary).
 *
 * Eliminates post-boot thermal spikes and OEM init contention via a 3-phase kernel-physics gate:
 *   Phase 1 (BOOT_SETTLE): Zero sysfs mutations, zero LaunchBurst/preread I/O, frozen learning.
 *   Phase 2 (Dynamic Quiescence Gate): Exits only when kernel /proc/uptime >= 35s, CPU < 35% &
 *     IO PSI < 8% hold >= 5s, SoC < 52C & slope <= +1.0C/min, AND screen is ON (max 120s).
 *   Phase 3 (Late-Baseline & Exponential Soft-Start): Re-snapshots true post-init OEM baselines,
 *     then smoothly ramps actuation authority via alpha(t) = 1 - exp(-t / 15s) across 30s.
 *
 * @license Apache-2.0
 */
#pragma once

#include <cstdint>
#include <string>

namespace coreflow::v6 {

struct BootCalmInput {
    double kernel_uptime_sec{0.0};
    uint64_t now_ms{0};
    bool screen_off{false};
    bool thermal_guard{false};
    double cpu_util_pct{0.0};
    double io_psi_pct{0.0};
    double soc_temp_c{-1.0};
    double thermal_velocity_cpm{0.0};
    bool touch_active{false};
    bool real_foreground_app{false}; // True only for non-launcher foreground app
};

struct BootCalmDecision {
    bool boot_settle_active{false};
    bool needs_late_baseline_snapshot{false}; // True on the single transition tick exiting BOOT_SETTLE
    bool allow_learning{true};                // Freezes AppPreloader + AppDnaStore during BOOT_SETTLE
    bool allow_touch_soft_uclamp{false};      // Light per-TID UI/SF UClamp only on real-app touch during BOOT_SETTLE
    bool allow_uclamp{true};                  // Full UClamp enabled (Phase 3 t >= 0s)
    bool allow_envelope_and_semantic{true};   // ThermalEnvelope + SemanticKnobs (Phase 3 t >= 10s)
    bool allow_storage_and_symbiotic{true};   // Storage preread + SymbioticOrchestrator (Phase 3 t >= 20s)
    double ramp_alpha{1.0};                   // Exponential soft-start alpha(t) = 1 - exp(-t/15s), 1.0 when complete
    double ramp_uclamp_scale{1.0};            // Smooth UClamp floor multiplier (0.88..1.00 during 30s ramp)
    double settle_duration_sec{0.0};          // Time spent in BOOT_SETTLE (seconds)
    std::string exit_reason{"active"};        // "settling" | "quiescence_met" | "max_uptime_timeout" | "warm_daemon_restart"
};

class BootCalmSanctuary {
public:
    static constexpr double kMinUptimeSec = 35.0;
    static constexpr double kMaxUptimeSec = 120.0;
    static constexpr double kCalmCpuUtilPct = 35.0;
    static constexpr double kCalmIoPsiPct = 8.0;
    static constexpr double kCalmSocTempC = 52.0;
    static constexpr double kCalmThermalVelCpm = 1.0;
    static constexpr uint64_t kQuiescenceHoldMs = 5000ULL;
    static constexpr double kRampTauSec = 15.0;
    static constexpr double kRampStageEnvelopeSec = 10.0;
    static constexpr double kRampStageFullSec = 20.0;
    static constexpr double kRampCompleteSec = 30.0;

    /**
     * @brief Reads monotonic kernel uptime in seconds from <procfs_root>/uptime.
     * Returns -1.0 if unreadable.
     */
    [[nodiscard]] static double read_kernel_uptime_sec(const std::string& procfs_root = "/proc") noexcept;

    /**
     * @brief Initializes sanctuary state at daemon startup using kernel /proc/uptime.
     * If kernel_uptime_sec >= kMaxUptimeSec (or unreadable), treats startup as a warm daemon restart
     * with zero boot delay.
     */
    void initialize(double initial_kernel_uptime_sec, uint64_t now_ms) noexcept;

    /**
     * @brief Evaluates one control tick and returns orthogonal phase gates + ramp progress.
     */
    [[nodiscard]] BootCalmDecision evaluate(const BootCalmInput& input) noexcept;

    [[nodiscard]] bool is_settling() const noexcept { return settling_; }
    [[nodiscard]] double settle_duration_sec() const noexcept { return settle_duration_sec_; }
    [[nodiscard]] const std::string& exit_reason() const noexcept { return exit_reason_; }
    [[nodiscard]] double last_ramp_alpha() const noexcept { return last_ramp_alpha_; }

private:
    bool initialized_{false};
    bool settling_{false};
    uint64_t settle_start_ms_{0};
    uint64_t settle_exit_ms_{0};
    uint64_t calm_since_ms_{0};
    double settle_duration_sec_{0.0};
    double last_ramp_alpha_{1.0};
    std::string exit_reason_{"warm_daemon_restart"};
};

} // namespace coreflow::v6
