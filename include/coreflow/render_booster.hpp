/**
 * @file render_booster.hpp
 * @brief SurfaceFlinger 120Hz Frame Pacing & Display Refresh Controller
 * @author mystivara-creator
 * @license Apache-2.0
 */

#pragma once

#include <chrono>
#include <atomic>
#include <string>
#include "coreflow/feature_accountability.hpp"
#include "coreflow/reactor.hpp"
#include "coreflow/uclamp.hpp"

namespace coreflow {

struct FrameMetrics {
    uint32_t target_fps{120};
    double average_frame_time_ms{8.33};
    uint32_t dropped_frames{0};
    bool is_vsync_aligned{true};
};

class RenderBooster {
public:
 static constexpr FeatureStatus kStatus=kRenderBoosterStatus;
    RenderBooster(UclampGovernor& governor, EpollReactor& reactor);
    ~RenderBooster();
    RenderBooster(const RenderBooster&) = delete;
    RenderBooster& operator=(const RenderBooster&) = delete;

    void on_touch_down();
    void on_touch_up();
    void on_frame_presented(double frame_time_ms);

    bool unlock_display_refresh_limits();
    [[nodiscard]] FrameMetrics get_metrics() const;

private:
    FrameMetrics metrics_;
    std::atomic<bool> in_touch_gesture_{false};
    std::atomic<uint32_t> consecutive_hitches_{0};
    UclampGovernor& governor_;
    EpollReactor& reactor_;
    UniqueFd request_fd_;
    UniqueFd expiry_fd_;
    std::atomic<uint32_t> requested_min_{0};
    std::atomic<int64_t> requested_duration_ms_{0};
    bool burst_active_{false}; // Reactor-thread confined.
    uint32_t baseline_min_{0};
    uint32_t baseline_max_{1024};

    void trigger_render_burst(uint32_t uclamp_min, std::chrono::milliseconds duration) noexcept;
    void handle_request(uint32_t events) noexcept;
    void handle_expiry(uint32_t events) noexcept;
    void revert_burst() noexcept;
};

} // namespace coreflow
