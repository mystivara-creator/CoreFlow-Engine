/**
 * @file render_booster.hpp
 * @brief Verified input-triggered transient top-app UClamp burst actuator.
 * SurfaceFlinger FrameTimeline and display refresh-rate control are not implemented.
 * @author mystivara-creator
 * @license Apache-2.0
 */

#pragma once

#include <chrono>
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include "coreflow/feature_accountability.hpp"
#include "coreflow/frame_bridge.hpp"
#include "coreflow/reactor.hpp"
#include "coreflow/uclamp.hpp"

namespace coreflow {

class ForegroundTaskResolver;

struct FrameMetrics {
    // Zero/false means no trusted frame-timeline producer is connected.
    uint32_t target_fps{0};
    double average_frame_time_ms{0.0};
    uint32_t dropped_frames{0};
    bool is_vsync_aligned{false};
};

class RenderBooster {
    friend struct RenderBoosterTestAccess;
public:
 static constexpr FeatureStatus kStatus=kRenderBoosterStatus;
    RenderBooster(
        UclampGovernor& governor,
        EpollReactor& reactor,
        uint32_t touch_boost_uclamp_min = 380U,
        uint32_t burst_boost_duration_ms = 120U);
    ~RenderBooster();
    RenderBooster(const RenderBooster&) = delete;
    RenderBooster& operator=(const RenderBooster&) = delete;

    void on_touch_down();
    void on_touch_up();
    void on_frame_presented(double frame_time_ms);
    [[nodiscard]] bool connect_frame_source(FrameEventSource& source) noexcept;
    void disconnect_frame_source() noexcept;
    void set_thermal_guard(bool active) noexcept;
    void set_foreground_task_resolver(ForegroundTaskResolver* resolver) noexcept;

    bool unlock_display_refresh_limits();
    [[nodiscard]] FrameMetrics get_metrics() const;

private:
    mutable std::mutex metrics_mutex_;
    FrameMetrics metrics_;
    std::atomic<bool> in_touch_gesture_{false};
    std::atomic<bool> thermal_guard_{false};
    std::atomic<uint32_t> consecutive_hitches_{0};
    std::array<FrameEventSource*, 2> frame_sources_{};
    UclampGovernor& governor_;
    EpollReactor& reactor_;
    ForegroundTaskResolver* foreground_task_resolver_{nullptr};
    bool foreground_tasks_active_{false}; // Reactor-thread confined.
    const uint32_t touch_boost_uclamp_min_;
    const std::chrono::milliseconds burst_boost_duration_;
    UniqueFd request_fd_;
    UniqueFd expiry_fd_;
    std::atomic<uint32_t> requested_min_{0};
    std::atomic<int64_t> requested_duration_ms_{0};
    bool burst_active_{false}; // Reactor-thread confined.
    uint32_t baseline_min_{0};
    uint32_t baseline_max_{1024};

    void on_frame_event(const FrameEvent& event) noexcept;
    void trigger_render_burst(uint32_t uclamp_min, std::chrono::milliseconds duration) noexcept;
    void handle_request(uint32_t events) noexcept;
    void handle_expiry(uint32_t events) noexcept;
    void restore_burst() noexcept;
    void revert_burst() noexcept;
    void restore_foreground_tasks() noexcept;
};

} // namespace coreflow
