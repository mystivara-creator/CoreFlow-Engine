/**
 * @file frame_pacing_sensor.hpp
 * @brief CoreFlow v6.2+: Zero-fork, Zero-heap Closed-Loop Display Frame Pacing Sensor
 *        with Non-LTPO Fixed-Hz Panel-Clock Discriminator (Strict Fail-Closed Probation).
 *
 * Separates verified kernel presentation-commit counters (Tier A: Qualcomm SDE measured_fps,
 * Xiaomi mi_display measured_fps, MediaTek FPSGO) from unverified scanout/refresh candidates
 * (Tier B: generic *\/fps sysfs and /proc/interrupts VBlank IRQs).
 *
 * Tier B sources stay fail-closed (jank_ratio = -1.0) during probation until static-screen
 * quiescence proves they drop with frame commits (< 18 Hz or < 50% target_hz). If a Tier B
 * source stays pinned >= 92% of target_hz across 3 consecutive static-idle probes, it is
 * permanently quarantined for the boot session so fixed-Hz non-LTPO panels never emit fake
 * 0% jank that would trim UClamp or decay App DNA.
 * @license Apache-2.0
 */
#pragma once

#include "coreflow/frame_bridge.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace coreflow::v6 {

enum class FrameSourceTier : uint8_t {
    None = 0,
    VerifiedCommit = 1,
    UnverifiedScanout = 2,
};

enum class TierBVerificationState : uint8_t {
    Unproven = 0,
    ProvenCommit = 1,
    QuarantinedFixedHz = 2,
};

struct FramePacingSample {
    bool available{false};
    double measured_fps{0.0};
    double target_fps{60.0};
    double jank_ratio{-1.0}; // -1.0 when idle/unproven/unavailable; 0.0..1.0 when active & verified
    bool verified_commit_source{false};
    bool quarantined_fixed_hz{false};
};

class FramePacingSensor {
public:
    static constexpr uint32_t kFixedHzQuarantineStrikes = 3U;

    FramePacingSensor() noexcept = default;

    // Discovers readable display FPS / measured_fps sysfs paths once at startup,
    // separating Tier A (verified commit counters) from Tier B (unverified scanout/VBlank IRQ).
    void discover(const std::string& sysfs_root = "/sys",
                  const std::string& procfs_root = "/proc",
                  const std::string& cached_vblank_irq_token = "") noexcept;

    // Samples current presentation cadence and computes an EWMA-smoothed jank ratio
    // incorporating cadence jitter variance (strictly 0 heap allocations in the hot path).
    //
    // Pass `screen_idle_hint = true` during calm screen-ON Efficiency windows so Tier B
    // sources can be proven commit-driven (drops < 18 Hz / < 50% target_hz) or quarantined
    // as fixed-Hz non-LTPO scanout clocks (>= 92% target_hz for 3 consecutive idle probes).
    [[nodiscard]] FramePacingSample sample(
        uint32_t target_refresh_hz,
        uint64_t now_ms,
        bool active_workload,
        AndroidFrameEventBridge* bridge = nullptr,
        bool screen_idle_hint = false) noexcept;

    // Resets transient EWMA / IRQ baselines (preserves per-boot Tier-B verification/quarantine state).
    void reset() noexcept;

    [[nodiscard]] bool supported() const noexcept {
        if (source_tier_ == FrameSourceTier::VerifiedCommit) return true;
        if (source_tier_ == FrameSourceTier::UnverifiedScanout) {
            return tier_b_state_ != TierBVerificationState::QuarantinedFixedHz;
        }
        return false;
    }

    [[nodiscard]] bool verified() const noexcept {
        return source_tier_ == FrameSourceTier::VerifiedCommit ||
               (source_tier_ == FrameSourceTier::UnverifiedScanout &&
                tier_b_state_ == TierBVerificationState::ProvenCommit);
    }

    [[nodiscard]] bool quarantined_fixed_hz() const noexcept {
        return tier_b_state_ == TierBVerificationState::QuarantinedFixedHz;
    }

    [[nodiscard]] FrameSourceTier source_tier() const noexcept { return source_tier_; }
    [[nodiscard]] TierBVerificationState tier_b_state() const noexcept { return tier_b_state_; }
    [[nodiscard]] const std::string& active_source() const noexcept { return active_source_; }

private:
    std::vector<std::string> verified_fps_paths_;
    std::vector<std::string> unverified_fps_paths_;
    std::string proc_interrupts_path_;
    std::string vblank_irq_token_;
    std::string active_source_;
    FrameSourceTier source_tier_{FrameSourceTier::None};
    TierBVerificationState tier_b_state_{TierBVerificationState::Unproven};
    uint32_t static_pinned_streak_{0};
    FramePacingSample last_sample_{};
    double ewma_jank_{-1.0};
    double prev_measured_fps_{-1.0};
    double ewma_jitter_{0.0};
    uint64_t prev_vblank_count_{0};
    uint64_t prev_vblank_ms_{0};
    bool has_vblank_baseline_{false};
    uint64_t last_sample_ms_{0};
};

} // namespace coreflow::v6
