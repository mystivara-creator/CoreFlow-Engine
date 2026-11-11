/**
 * @file render_booster.cpp
 * @brief Render Booster Implementation with BaselineStore protection
 * @author mystivara-creator
 * @license Apache-2.0
 */

#include "coreflow/render_booster.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/unique_fd.hpp"
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <unistd.h>

namespace coreflow {

RenderBooster::RenderBooster(UclampGovernor& governor, EpollReactor& reactor)
    : governor_(governor), reactor_(reactor) {
    // No SurfaceFlinger/frame-timeline source is connected; never synthesize 120 FPS.
    metrics_.target_fps = 0;
    metrics_.average_frame_time_ms = 0.0;
    metrics_.dropped_frames = 0;
    metrics_.is_vsync_aligned = false;

    request_fd_.reset(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC));
    expiry_fd_.reset(::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC));
    if (!request_fd_.valid() || !expiry_fd_.valid() ||
        !reactor_.register_fd(request_fd_.get(), EPOLLIN,
            [this](uint32_t events) { handle_request(events); }) ||
        !reactor_.register_fd(expiry_fd_.get(), EPOLLIN,
            [this](uint32_t events) { handle_expiry(events); })) {
        if (request_fd_.valid()) (void)reactor_.unregister_fd(request_fd_.get());
        if (expiry_fd_.valid()) (void)reactor_.unregister_fd(expiry_fd_.get());
        request_fd_.reset();
        expiry_fd_.reset();
    }
}

RenderBooster::~RenderBooster() {
    if (request_fd_.valid()) (void)reactor_.unregister_fd(request_fd_.get());
    if (expiry_fd_.valid()) (void)reactor_.unregister_fd(expiry_fd_.get());
    restore_burst();
}

void RenderBooster::on_touch_down() {
    in_touch_gesture_.store(true, std::memory_order_relaxed);
    trigger_render_burst(380, std::chrono::milliseconds(120));
}

void RenderBooster::on_touch_up() {
    in_touch_gesture_.store(false, std::memory_order_relaxed);
}

void RenderBooster::on_frame_presented(double frame_time_ms) {
    if (metrics_.target_fps == 0 || !std::isfinite(frame_time_ms) || frame_time_ms <= 0.0) return;
    double frame_budget = 1000.0 / metrics_.target_fps;

    if (frame_time_ms > frame_budget * 1.25) {
        metrics_.dropped_frames++;
        uint32_t hitches = ++consecutive_hitches_;

        if (hitches >= 2) {
            trigger_render_burst(580, std::chrono::milliseconds(250));
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

void RenderBooster::trigger_render_burst(
    uint32_t uclamp_min, std::chrono::milliseconds duration) noexcept {
    // The producer path is allocation-free: publish two scalars and wake the
    // reactor. All ownership checks, baseline reads and kernel writes run there.
    if (!request_fd_.valid() || !expiry_fd_.valid() || duration.count() <= 0) return;
    requested_min_.store(std::min(uclamp_min, 1024u), std::memory_order_relaxed);
    requested_duration_ms_.store(duration.count(), std::memory_order_relaxed);
    uint64_t one = 1;
    ssize_t written;
    do { written = ::write(request_fd_.get(), &one, sizeof(one)); }
    while (written < 0 && errno == EINTR);
}

void RenderBooster::handle_request(uint32_t) noexcept {
    uint64_t signals = 0;
    while (::read(request_fd_.get(), &signals, sizeof(signals)) ==
           static_cast<ssize_t>(sizeof(signals))) {}
    const uint32_t requested_min = requested_min_.exchange(0, std::memory_order_acq_rel);
    const int64_t duration_ms = requested_duration_ms_.exchange(0, std::memory_order_acq_rel);
    if (duration_ms <= 0 || requested_min == 0) return;

    // Never reassert a clamp after an OEM or another owner changes either node.
    if (!governor_.owned_values_intact()) {
        itimerspec disarm{};
        (void)::timerfd_settime(expiry_fd_.get(), 0, &disarm, nullptr);
        burst_active_ = false;
        return;
    }

    if (!burst_active_) {
        baseline_min_ = governor_.get_top_app_min();
        baseline_max_ = governor_.get_top_app_max();
    }
    if (!governor_.apply_state(WorkloadState::Balanced, requested_min,
                               governor_.get_top_app_max())) {
        if (burst_active_) revert_burst();
        return;
    }
    burst_active_ = true;

    itimerspec expiry{};
    const auto seconds = duration_ms / 1000;
    const auto nanoseconds = (duration_ms % 1000) * 1000000;
    expiry.it_value.tv_sec = static_cast<time_t>(seconds);
    expiry.it_value.tv_nsec = static_cast<long>(nanoseconds);
    if (::timerfd_settime(expiry_fd_.get(), 0, &expiry, nullptr) != 0) {
        revert_burst();
    }
}

void RenderBooster::handle_expiry(uint32_t) noexcept {
    uint64_t expirations = 0;
    while (::read(expiry_fd_.get(), &expirations, sizeof(expirations)) ==
           static_cast<ssize_t>(sizeof(expirations))) {}
    if (expirations != 0) restore_burst();
}

void RenderBooster::restore_burst() noexcept {
    itimerspec disarm{};
    if (expiry_fd_.valid()) (void)::timerfd_settime(expiry_fd_.get(), 0, &disarm, nullptr);
    revert_burst();
}

void RenderBooster::revert_burst() noexcept {
    if (!burst_active_) return;
    // OWNERSHIP_LOST is a handoff: do not overwrite the value that replaced ours.
    if (governor_.owned_values_intact()) {
        // Restore the exact pre-burst governor state. UclampGovernor uses the
        // captured OEM raw encoding when the pair returns to its factory values.
        (void)governor_.apply_state(
            WorkloadState::Balanced, baseline_min_, baseline_max_);
    }
    burst_active_ = false;
}

} // namespace coreflow

