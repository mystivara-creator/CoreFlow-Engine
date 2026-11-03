/**
 * @file render_booster.cpp
 * @brief Render Booster Implementation with BaselineStore protection
 * @author mystivara-creator
 * @license Apache-2.0
 */

#include "coreflow/render_booster.hpp"
#include "coreflow/baseline_store.hpp"
#include <iostream>
#include <vector>

namespace coreflow {

RenderBooster::RenderBooster() {
    metrics_.target_fps = 120;
    metrics_.average_frame_time_ms = 8.33;
    metrics_.dropped_frames = 0;
    metrics_.is_vsync_aligned = true;
}

void RenderBooster::on_touch_down() {
    in_touch_gesture_.store(true, std::memory_order_relaxed);
    trigger_render_burst(380, std::chrono::milliseconds(120));
}

void RenderBooster::on_touch_up() {
    in_touch_gesture_.store(false, std::memory_order_relaxed);
}

void RenderBooster::on_frame_presented(double frame_time_ms) {
    double frame_budget = 1000.0 / metrics_.target_fps;

    if (frame_time_ms > frame_budget * 1.25) {
        metrics_.dropped_frames++;
        uint32_t hitches = ++consecutive_hitches_;

        if (hitches >= 2) {
            trigger_render_burst(580, std::chrono::milliseconds(250));
            std::cout << "[CoreFlow][Render] Jank detected (" << frame_time_ms 
                      << "ms vs " << frame_budget << "ms budget). No native frame backend attached." << std::endl;
        }
    } else {
        consecutive_hitches_.store(0, std::memory_order_relaxed);
    }

    metrics_.average_frame_time_ms = 0.9 * metrics_.average_frame_time_ms + 0.1 * frame_time_ms;
}

bool RenderBooster::unlock_display_refresh_limits() {
    // No verified display-mode backend: never force an unadvertised panel mode.
    return false;
}

FrameMetrics RenderBooster::get_metrics() const {
    return metrics_;
}

void RenderBooster::trigger_render_burst(uint32_t uclamp_min, std::chrono::milliseconds) {
    // A rendering backend must supply trusted events and an expiry before it
    // can request boosts through the central controller. This helper does not write nodes.
    (void)uclamp_min;
}

} // namespace coreflow

