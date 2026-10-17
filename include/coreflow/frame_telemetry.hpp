#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace coreflow::observe {

// Parses `dumpsys SurfaceFlinger --latency` rows
// ("<vsync> <submission> <presentation>"). A frame duration is
// presentation - submission; rows with reversed or zero durations (ERROR
// rows) are dropped. Parsing is pure so host tests cover it without a display.
[[nodiscard]] inline std::vector<std::uint64_t> parseLatencyDurations(
    std::string_view dump) {
    std::vector<std::uint64_t> out;
    std::size_t pos = 0;
    while (pos < dump.size()) {
        std::size_t end = dump.find('\n', pos);
        if (end == std::string_view::npos) end = dump.size();
        const std::string_view line = dump.substr(pos, end - pos);
        pos = end + 1;
        std::uint64_t v[3] = {0U, 0U, 0U};
        int count = 0;
        std::size_t i = 0;
        while (i < line.size() && count < 3) {
            while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
            if (i >= line.size() || line[i] < '0' || line[i] > '9') break;
            std::uint64_t value = 0;
            while (i < line.size() && line[i] >= '0' && line[i] <= '9') {
                value = value * 10U + static_cast<std::uint64_t>(line[i] - '0');
                ++i;
            }
            v[count++] = value;
        }
        if (count == 3 && v[2] >= v[1]) {
            const std::uint64_t duration = v[2] - v[1];
            if (duration > 0U && duration < 1000000000U) out.push_back(duration);
        }
    }
    return out;
}

// Bounded, adaptive poller for SurfaceFlinger frame telemetry. The
// collection half is device-only (runs dumpsys); the parser above is
// host-testable. Unavailable telemetry yields empty results; the frame
// analyzer is only fed with returned durations.
class FrameTelemetrySource final {
public:
    void setPollRatesHz(unsigned idle_hz, unsigned active_hz) noexcept {
        idle_hz_ = idle_hz;
        active_hz_ = active_hz;
    }
    [[nodiscard]] std::vector<std::uint64_t> poll(std::uint64_t now_ms,
                                                  bool jank_recent) noexcept;

private:
    std::uint64_t next_poll_ms_{0};
    unsigned idle_hz_{1};
    unsigned active_hz_{30};
};

} // namespace coreflow::observe
