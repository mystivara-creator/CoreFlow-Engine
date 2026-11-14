/**
 * @file frame_bridge.hpp
 * @brief Vendor-neutral, fail-closed ingress for normalized Android frame/input events.
 * @license Apache-2.0
 */
#pragma once
#include <cstdint>
#include <functional>
#include <mutex>
namespace coreflow {
enum class FrameEventKind : uint8_t {
    FramePresented = 0, FrameDropped = 1, TouchBurst = 2, FrameTimelineWindow = 3
};
struct FrameEvent final {
    FrameEventKind kind{FrameEventKind::FramePresented};
    uint64_t vsync_timestamp_ns{0};
    uint64_t frame_duration_ns{0};
    uint64_t jank_threshold_ns{0};
    // Optional touch-driven UClamp window; zero selects the RenderBooster default.
    uint32_t burst_duration_ms{0};
    // Aggregated, parser-validated Perfetto actual-display-frame window.
    uint64_t frame_count{0};
    uint64_t missed_frames{0};
};
using FrameEventListener = std::function<void(const FrameEvent&)>;
class FrameEventSource {
public:
    virtual ~FrameEventSource() = default;
    FrameEventSource(const FrameEventSource&) = delete;
    FrameEventSource& operator=(const FrameEventSource&) = delete;
    [[nodiscard]] virtual bool start(FrameEventListener listener) noexcept = 0;
    virtual void stop() noexcept = 0;
protected:
    FrameEventSource() = default;
};
// Public-kernel/Android adapters normalize SurfaceFlinger FrameTimeline and
// input events, then submit them here. No vendor ABI, guessed node, or mutation
// permission is used. Missing upstream events remain unavailable, never inferred.
class AndroidFrameEventBridge final : public FrameEventSource {
public:
    AndroidFrameEventBridge() = default;
    ~AndroidFrameEventBridge() override { stop(); }
    [[nodiscard]] bool start(FrameEventListener listener) noexcept override;
    void stop() noexcept override;
    [[nodiscard]] bool submit(const FrameEvent& event) noexcept;
private:
    std::mutex mutex_;
    FrameEventListener listener_{};
    bool active_{false};
};
} // namespace coreflow
