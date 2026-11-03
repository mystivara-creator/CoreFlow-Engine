/**
 * @file render_booster.cpp
 * @brief Render Booster Implementation
 * @author mystivara-creator
 * @license Apache-2.0
 */

#include "coreflow/render_booster.hpp"
#include "coreflow/baseline_store.hpp"
#include <iostream>

namespace coreflow {

RenderBooster::RenderBooster() {
    metrics_.target_fps = 0;
    metrics_.average_frame_time_ms = 0;
    metrics_.dropped_frames = 0;
    metrics_.is_vsync_aligned = false;
}

void RenderBooster::on_touch_down() {
    in_touch_gesture_.store(true, std::memory_order_relaxed);
    // Instantaneous EAS uclamp boost for render thread
    trigger_render_burst(380, std::chrono::milliseconds(120));
}

void RenderBooster::on_touch_up() {
    in_touch_gesture_.store(false, std::memory_order_relaxed);
}

void RenderBooster::on_frame_presented(double frame_time_ms) {
    // 120Hz budget = 8.33ms; 60Hz budget = 16.6ms
    if (metrics_.target_fps == 0) return; // Display refresh source not connected.
    double frame_budget = 1000.0 / metrics_.target_fps;

    if (frame_time_ms > frame_budget * 1.25) {
        // Frame missed its VSYNC window (Jank detected)
        metrics_.dropped_frames++;
        uint32_t hitches = ++consecutive_hitches_;

        if (hitches >= 2) {
            // Consecutive hitches: aggressively boost top-app uclamp to resolve bottleneck
            trigger_render_burst(580, std::chrono::milliseconds(250));
            // A frame-event source exists, but timed boost is disabled until it is safe.
        }
    } else {
        consecutive_hitches_.store(0, std::memory_order_relaxed);
    }

    metrics_.average_frame_time_ms = 0.9 * metrics_.average_frame_time_ms + 0.1 * frame_time_ms;
}

bool RenderBooster::unlock_display_refresh_limits() {
    // Refresh-rate policy is owned by Android DisplayManager and vendor HAL.
    // Writing undocumented panel nodes can destabilize the display pipeline.
    return false;
}

FrameMetrics RenderBooster::get_metrics() const {
    return metrics_;
}

void RenderBooster::trigger_render_burst(uint32_t uclamp_min, std::chrono::milliseconds duration) {
    // No timer-backed restoration exists yet. Fail closed instead of leaving
    // a persistent boost that outlives the requested duration.
    (void)uclamp_min;
    (void)duration;
}

} // namespace coreflow
