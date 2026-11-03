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
                      << "ms vs " << frame_budget << "ms budget). Applied burst." << std::endl;
        }
    } else {
        consecutive_hitches_.store(0, std::memory_order_relaxed);
    }

    metrics_.average_frame_time_ms = 0.9 * metrics_.average_frame_time_ms + 0.1 * frame_time_ms;
}

bool RenderBooster::unlock_display_refresh_limits() {
    std::vector<std::string> panel_fps_nodes = {
        "/sys/devices/virtual/graphics/fb0/dynamic_fps",
        "/sys/class/drm/card0-DSI-1/fps",
        "/sys/devices/platform/soc/soc:qcom,dsi-display-primary/fps"
    };

    bool any_success = false;
    for (const auto& node : panel_fps_nodes) {
        if (BaselineStore::instance().capture(node)) {
            if (BaselineStore::instance().apply_mutation(node, "120")) {
                any_success = true;
                std::cout << "[CoreFlow][Render] Unlocked 120Hz on " << node << std::endl;
            }
        }
    }

    return any_success;
}

FrameMetrics RenderBooster::get_metrics() const {
    return metrics_;
}

void RenderBooster::trigger_render_burst(uint32_t uclamp_min, std::chrono::milliseconds) {
    (void)BaselineStore::instance().apply_mutation(
        "/sys/fs/cgroup/top-app/cpu.uclamp.min", 
        std::to_string(uclamp_min)
    );
}

} // namespace coreflow
