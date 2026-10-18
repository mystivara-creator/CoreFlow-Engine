#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace coreflow::graphics {

// Fixed-memory, deterministic analysis of frame durations supplied by a platform
// telemetry adapter. This class does not collect privileged data or change device
// state; unavailable telemetry must be represented by not calling recordFrame().
class FramePerformanceAnalyzer final {
public:
    static constexpr std::size_t kWindowSize = 120;
    static constexpr std::uint64_t kMinFrameIntervalNs = 1'000'000ULL;
    static constexpr std::uint64_t kMaxFrameIntervalNs = 100'000'000ULL;

    struct Snapshot {
        std::size_t samples{0};
        std::size_t deadline_misses{0};
        std::size_t severe_stutters{0};
        std::size_t longest_miss_streak{0};
        double deadline_miss_rate{0.0};
        std::uint64_t p50_frame_ns{0};
        std::uint64_t p95_frame_ns{0};
        std::uint64_t p99_frame_ns{0};
        bool valid{false};
    };

    // Rejects zero durations, invalid target intervals, and implausible samples.
    // A frame misses its deadline when duration > target; severe stutter is > 1.5x.
    bool recordFrame(std::uint64_t duration_ns,
                     std::uint64_t target_interval_ns) noexcept;
    Snapshot snapshot() const noexcept;
    void reset() noexcept;

private:
    std::array<std::uint64_t, kWindowSize> durations_{};
    std::array<std::uint64_t, kWindowSize> targets_{};
    std::size_t size_{0};
    std::size_t next_{0};
};

} // namespace coreflow::graphics
