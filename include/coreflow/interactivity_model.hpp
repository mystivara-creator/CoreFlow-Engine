#pragma once

#include <cstdint>

#include "coreflow/frame_performance.hpp"

namespace coreflow::understand {

// Interaction quality observed by the engine: launch windows, active frames,
// and sustained jank. Deliberately separate from RuntimeState, which tracks
// system/thermal pressure.
enum class InteractionState : std::uint8_t {
    Idle,
    Launching,    // app-launch window after a foreground change
    Interactive,  // active frames or IO without jank
    Stuttering,   // sustained frame deadline misses
};

const char* interactionStateName(InteractionState state) noexcept;

struct InteractivityThresholds {
    std::uint64_t launch_window_ms{2500};
    double jank_miss_rate{0.05};
    std::uint32_t stutter_enter_ticks{2};
    std::uint32_t stutter_exit_clean_ticks{8};
};

struct InteractivitySample {
    std::uint64_t now_ms{0};
    bool frame_fresh{true};
    bool gpu_busy_available{false};
    double gpu_busy_ratio{0.0};
    double io_full_avg10{0.0};     // PSI io full avg10, fraction 0..1
    double memory_some_avg10{0.0}; // PSI memory some avg10, fraction 0..1
    bool cpu_pressure_available{false};
    double cpu_pressure_ratio{0.0};
    bool thermal_guard{false};
};

struct InteractivitySnapshot {
    InteractionState state{InteractionState::Idle};
    graphics::FramePerformanceAnalyzer::Snapshot frame{};
    bool gpu_busy_available{false};
    double gpu_busy_ratio{0.0};
    double io_full_avg10{0.0};
    double memory_some_avg10{0.0};
    bool cpu_pressure_available{false};
    double cpu_pressure_ratio{0.0};
    std::uint64_t state_entered_ms{0};
    std::uint64_t launch_deadline_ms{0};
};

// Fixed-memory, deterministic state machine. No filesystem, no policy
// authority: evaluation only classifies what the producers reported.
class InteractivityModel final {
public:
    void setThresholds(const InteractivityThresholds& thresholds) noexcept {
        thresholds_ = thresholds;
    }
    void ingestFrame(std::uint64_t duration_ns,
                     std::uint64_t target_interval_ns) noexcept;
    // Starts a new bounded app-launch window.
    void onForegroundChanged(std::uint64_t now_ms) noexcept;
    [[nodiscard]] InteractivitySnapshot evaluate(
        const InteractivitySample& sample) noexcept;
    void clearFrames() noexcept;
    void reset() noexcept;

private:
    graphics::FramePerformanceAnalyzer analyzer_;
    InteractivityThresholds thresholds_{};
    InteractionState state_{InteractionState::Idle};
    std::uint64_t state_entered_ms_{0};
    std::uint64_t launch_deadline_ms_{0};
    std::uint32_t stutter_ticks_{0};
    std::uint32_t clean_ticks_{0};
    bool launch_active_{false};
};

} // namespace coreflow::understand
