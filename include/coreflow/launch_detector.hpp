#pragma once

#include <cstddef>
#include <cstdint>

namespace coreflow::observe {

// Detects the window in which the foreground application is starting, from
// signals the daemon can read without framework hooks:
//   - the population of the foreground scheduling group (new processes joining
//     it is the switch to a new app), and
//   - I/O pressure (the launch reads a burst of storage).
//
// The detector is a pure state machine. It has no clock and no I/O, and it takes
// monotonic timestamps from its caller. Its thresholds are heuristics that must
// be validated on device, so they are constants here and not user tunables.
enum class LaunchPhase : std::uint8_t {
    Idle,
    LaunchWindow,  // a launch is in progress: prefer latency over efficiency
};

struct LaunchObservation final {
    std::uint64_t timestamp_monotonic_ns{0};
    std::size_t foreground_pids{0};   // members of the foreground group
    double io_pressure_some{-1.0};    // PSI "some avg10" in percent; <0 = unavailable
};

class LaunchBurstDetector final {
public:
    static constexpr std::uint64_t kWindowNs = 3'000'000'000ULL;      // quiet time to close
    static constexpr std::uint64_t kMaxWindowNs = 8'000'000'000ULL;   // hard cap on a window
    static constexpr std::size_t kMinNewPids = 1U;
    // PSI avg10 is a 10-second average; a short launch burst can be diluted.
    // Keep the threshold low enough for window extension to observe that burst.
    static constexpr double kIoPressureActivePct = 1.0;

    // Returns the phase after this observation. Invalid timestamps (zero or not
    // increasing) are ignored and leave the state unchanged.
    LaunchPhase observe(const LaunchObservation& observation) noexcept;

    LaunchPhase phase() const noexcept { return phase_; }
    void reset() noexcept;

private:
    LaunchPhase phase_{LaunchPhase::Idle};
    std::uint64_t window_opened_ns_{0};
    std::uint64_t last_activity_ns_{0};
    std::uint64_t last_timestamp_ns_{0};
    std::size_t last_foreground_pids_{0};
    // Lowest membership seen while Idle. This catches a new app spawning after
    // the previous app's foreground processes have already exited.
    std::size_t floor_foreground_pids_{0};
    bool have_last_{false};
};

} // namespace coreflow::observe
