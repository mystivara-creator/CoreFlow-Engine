#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace coreflow::graphics {

// Parser for the text printed by `dumpsys SurfaceFlinger --latency <layer>`.
// That output is a diagnostic format, not a stable API. The parser therefore
// accepts only the documented shape (a refresh period line, then rows of three
// integer timestamps) and fails closed on anything else. Nothing here runs a
// process. The caller collects the text and bounds its size before calling.
struct FrameLatencyTrace final {
    static constexpr std::size_t kMaxInputBytes = 256U * 1024U;
    static constexpr std::size_t kMaxRows = 128U;
    // A present gap above this is treated as an idle layer, not a frame interval.
    static constexpr std::uint64_t kMaxIntervalNs = 10'000'000'000ULL;

    std::uint64_t refresh_period_ns{0};
    std::vector<std::uint64_t> frame_intervals_ns;  // consecutive actual-present deltas
    std::size_t rows_total{0};
    std::size_t rows_pending{0};   // present time not yet known (sentinel)
    std::size_t rows_rejected{0};  // malformed, non-monotonic, or implausible
    bool valid{false};
};

[[nodiscard]] FrameLatencyTrace parseSurfaceFlingerLatency(const std::string& text) noexcept;

} // namespace coreflow::graphics
