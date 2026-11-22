/**
 * @file frame_pacing_sensor.hpp
 * @brief CoreFlow v6.2: Zero-fork Closed-Loop Display Frame Pacing Sensor.
 *
 * Reads live kernel display presentation cadence (Qualcomm SDE measured_fps,
 * Xiaomi mi_display fps, MediaTek fpsgo/disp_fps, Exynos/Pixel panel fps) without
 * spawning subprocesses in the hot loop, emitting smoothed jank ratios into
 * AndroidFrameEventBridge and the Rust closed-loop UClamp controller.
 * @license Apache-2.0
 */
#pragma once

#include "coreflow/frame_bridge.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace coreflow::v6 {

struct FramePacingSample {
    bool available{false};
    double measured_fps{0.0};
    double target_fps{60.0};
    double jank_ratio{-1.0}; // -1.0 when idle/unavailable; 0.0..1.0 when active
};

class FramePacingSensor {
public:
    FramePacingSensor() noexcept = default;

    // Discovers readable display FPS / measured_fps sysfs paths once at startup,
    // falling back to a cached /proc/interrupts display VBlank IRQ counter if absent.
    void discover(const std::string& sysfs_root = "/sys",
                  const std::string& procfs_root = "/proc",
                  const std::string& cached_vblank_irq_token = "") noexcept;

    // Samples current presentation cadence and computes an EWMA-smoothed jank ratio
    // incorporating cadence jitter variance. Returns jank_ratio = -1.0 when panel is
    // static/idle (measured_fps < 12) so static reading never fabricates false jank.
    [[nodiscard]] FramePacingSample sample(
        uint32_t target_refresh_hz,
        uint64_t now_ms,
        bool active_workload,
        AndroidFrameEventBridge* bridge = nullptr) noexcept;

    void reset() noexcept;

    [[nodiscard]] bool supported() const noexcept {
        return !fps_paths_.empty() || !vblank_irq_token_.empty();
    }
    [[nodiscard]] const std::string& active_source() const noexcept { return active_source_; }

private:
    std::vector<std::string> fps_paths_;
    std::string proc_interrupts_path_;
    std::string vblank_irq_token_;
    std::string active_source_;
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
