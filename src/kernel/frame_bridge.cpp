#include "coreflow/frame_bridge.hpp"
#include <utility>

namespace coreflow {

bool AndroidFrameEventBridge::start(FrameEventListener listener) noexcept {
    if (!listener) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    listener_ = std::move(listener);
    active_ = true;
    return true;
}

void AndroidFrameEventBridge::stop() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    active_ = false;
    listener_ = {};
}

bool AndroidFrameEventBridge::submit(const FrameEvent& event) noexcept {
    if (event.vsync_timestamp_ns == 0) return false;
    if (event.kind == FrameEventKind::FrameDropped &&
        (event.frame_duration_ns == 0 || event.jank_threshold_ns == 0 ||
         event.frame_duration_ns <= event.jank_threshold_ns)) {
        return false;
    }
    if (event.kind == FrameEventKind::FramePresented && event.frame_duration_ns == 0) {
        return false;
    }

    FrameEventListener listener;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!active_ || !listener_) return false;
        listener = listener_;
    }

    // User code must run outside mutex_: listeners may stop or recursively submit.
    listener(event);
    return true;
}

} // namespace coreflow
