/**
 * @file render_booster.hpp
 * @brief Android 15+ Rendering Pipeline & Frame Delivery Booster
 * @author mystivara-creator
 * @license Apache-2.0
 * 
 * Coordinates with SurfaceFlinger and HWUI to eliminate frame drops without
 * causing battery drain. Unblocks vendor refresh rate clamping (60Hz lock on mid-range).
 */

#pragma once

#include "types.hpp"
#include <chrono>
#include <atomic>

namespace coreflow {

struct FrameMetrics {
    uint32_t target_fps{120};
    double average_frame_time_ms{8.33};
    uint32_t dropped_frames{0};
    bool is_vsync_aligned{true};
};

class RenderBooster {
public:
    RenderBooster();
    ~RenderBooster() = default;

    /// Notify booster of incoming touch fling or UI interaction
    void on_touch_down();
    void on_touch_up();

    /// Notify booster of frame presentation time from HWUI / SurfaceFlinger
    void on_frame_presented(double frame_time_ms);

    /// Unlock vendor refresh rate limiter (e.g. forced 60Hz in YouTube or Google Maps)
    bool unlock_display_refresh_limits();

    /// Retrieve live rendering metrics
    [[nodiscard]] FrameMetrics get_metrics() const;

private:
    std::atomic<bool> in_touch_gesture_{false};
    std::atomic<uint32_t> consecutive_hitches_{0};
    FrameMetrics metrics_;

    void trigger_render_burst(uint32_t uclamp_min, std::chrono::milliseconds duration);
};

} // namespace coreflow
