#include "coreflow/interactivity_model.hpp"

#include <limits>

namespace coreflow::understand {

const char* interactionStateName(InteractionState state) noexcept {
    switch (state) {
        case InteractionState::Idle: return "IDLE";
        case InteractionState::Launching: return "LAUNCHING";
        case InteractionState::Interactive: return "INTERACTIVE";
        case InteractionState::Stuttering: return "STUTTERING";
    }
    return "UNKNOWN";
}

void InteractivityModel::ingestFrame(std::uint64_t duration_ns,
                                     std::uint64_t target_interval_ns) noexcept {
    (void)analyzer_.recordFrame(duration_ns, target_interval_ns);
}

void InteractivityModel::onForegroundChanged(std::uint64_t now_ms) noexcept {
    // Saturating add: a timestamp near UINT64_MAX must not wrap to a small
    // deadline, which would close the launch window at once.
    const std::uint64_t window = thresholds_.launch_window_ms;
    constexpr std::uint64_t kMax = std::numeric_limits<std::uint64_t>::max();
    launch_active_ = true;
    launch_deadline_ms_ = (window > kMax - now_ms) ? kMax : now_ms + window;
}

InteractivitySnapshot InteractivityModel::evaluate(
    const InteractivitySample& sample) noexcept {
    InteractivitySnapshot out;
    out.frame = analyzer_.snapshot();
    out.gpu_busy_available = sample.gpu_busy_available;
    out.gpu_busy_ratio = sample.gpu_busy_ratio;
    out.io_full_avg10 = sample.io_full_avg10;
    out.launch_deadline_ms = launch_deadline_ms_;

    InteractionState want = InteractionState::Idle;
    if (sample.thermal_guard) {
        // Thermal authority outranks interactivity: drop the launch window and
        // the jank streak so no boost intent survives the guard.
        launch_active_ = false;
        stutter_ticks_ = 0;
        clean_ticks_ = 0;
    } else {
        if (launch_active_ && sample.now_ms > launch_deadline_ms_) {
            launch_active_ = false;
        }
        const bool janky = out.frame.valid &&
            out.frame.deadline_miss_rate > thresholds_.jank_miss_rate &&
            out.frame.longest_miss_streak >= 2U;
        if (janky) {
            ++stutter_ticks_;
            clean_ticks_ = 0;
        } else {
            stutter_ticks_ = 0;
            if (out.frame.valid) ++clean_ticks_;
        }
        if (launch_active_) {
            want = InteractionState::Launching;
        } else if (stutter_ticks_ >= thresholds_.stutter_enter_ticks) {
            want = InteractionState::Stuttering;
        } else if (state_ == InteractionState::Stuttering &&
                   clean_ticks_ < thresholds_.stutter_exit_clean_ticks) {
            want = InteractionState::Stuttering; // hysteresis exit
        } else if (out.frame.valid || sample.io_full_avg10 > 0.005) {
            want = InteractionState::Interactive;
        }
    }

    if (want != state_) {
        state_ = want;
        state_entered_ms_ = sample.now_ms;
    }
    out.state = state_;
    out.state_entered_ms = state_entered_ms_;
    return out;
}

void InteractivityModel::reset() noexcept {
    analyzer_.reset();
    state_ = InteractionState::Idle;
    state_entered_ms_ = 0;
    launch_deadline_ms_ = 0;
    stutter_ticks_ = 0;
    clean_ticks_ = 0;
    launch_active_ = false;
}

} // namespace coreflow::understand
