#include "coreflow/launch_detector.hpp"

#include <cmath>

namespace coreflow::observe {

LaunchPhase LaunchBurstDetector::observe(const LaunchObservation& observation) noexcept {
    const std::uint64_t now = observation.timestamp_monotonic_ns;
    if (now == 0U) return phase_;
    if (have_last_ && now <= last_timestamp_ns_) return phase_;

    // While idle, track the lowest membership count. The old app can exit
    // before the new app joins, so comparing only adjacent samples misses it.
    if (!have_last_ || phase_ == LaunchPhase::Idle) {
        if (!have_last_ || observation.foreground_pids < floor_foreground_pids_) {
            floor_foreground_pids_ = observation.foreground_pids;
        }
    }
    // Subtraction avoids overflow in floor + kMinNewPids.
    const bool joined = have_last_ &&
        observation.foreground_pids > floor_foreground_pids_ &&
        observation.foreground_pids - floor_foreground_pids_ >= kMinNewPids;
    const bool io_active = std::isfinite(observation.io_pressure_some) &&
                           observation.io_pressure_some >= kIoPressureActivePct;

    if (joined) {
        floor_foreground_pids_ = observation.foreground_pids;
        if (phase_ == LaunchPhase::Idle) {
            phase_ = LaunchPhase::LaunchWindow;
            window_opened_ns_ = now;
        }
        last_activity_ns_ = now;
    } else if (phase_ == LaunchPhase::LaunchWindow && io_active) {
        // Storage is still busy with the launch: that is activity, which extends
        // the window, but never beyond the hard cap.
        last_activity_ns_ = now;
    }

    if (phase_ == LaunchPhase::LaunchWindow) {
        const bool quiet = now - last_activity_ns_ >= kWindowNs;
        const bool capped = now - window_opened_ns_ >= kMaxWindowNs;
        if (quiet || capped) {
            phase_ = LaunchPhase::Idle;
            // The next idle sample starts from the current settled membership;
            // otherwise a child spawned during the window can reopen it.
            floor_foreground_pids_ = observation.foreground_pids;
        }
    }

    last_timestamp_ns_ = now;
    last_foreground_pids_ = observation.foreground_pids;
    have_last_ = true;
    return phase_;
}

void LaunchBurstDetector::reset() noexcept {
    phase_ = LaunchPhase::Idle;
    window_opened_ns_ = 0;
    last_activity_ns_ = 0;
    last_timestamp_ns_ = 0;
    last_foreground_pids_ = 0;
    floor_foreground_pids_ = 0;
    have_last_ = false;
}

} // namespace coreflow::observe
