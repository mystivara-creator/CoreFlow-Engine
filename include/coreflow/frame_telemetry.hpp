#pragma once

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <vector>

#include "coreflow/integer_parse.hpp"

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
        pos = end + 1U;

        // A row must hold exactly three unsigned fields. Each field is parsed
        // strictly, so an overflowing or non-numeric field drops the whole row
        // (ERROR rows, truncated rows, and corrupt rows all take this path).
        std::uint64_t v[3] = {0U, 0U, 0U};
        int count = 0;
        bool ok = true;
        std::size_t i = 0;
        while (i < line.size()) {
            while (i < line.size() && integer_parse::isSpace(line[i])) ++i;
            if (i >= line.size()) break;
            const std::size_t start = i;
            while (i < line.size() && !integer_parse::isSpace(line[i])) ++i;
            if (count == 3) {
                ok = false;
                break;
            }
            if (!integer_parse::parseUint64(line.substr(start, i - start), v[count])) {
                ok = false;
                break;
            }
            ++count;
        }
        if (!ok || count != 3) continue;
        if (v[2] < v[1]) continue;  // presentation before submission: reversed row
        const std::uint64_t duration = v[2] - v[1];
        if (duration > 0U && duration < 1000000000U) out.push_back(duration);
    }
    return out;
}

// Extracts the display refresh period in nanoseconds from the single-field
// header row emitted by `dumpsys SurfaceFlinger --latency` (e.g. 8333333 for
// 120 Hz, 11111111 for 90 Hz, 16666667 for 60 Hz). Falls back to 60 Hz when
// absent or out of plausible display range [4 ms, 34 ms].
[[nodiscard]] inline std::uint64_t parseLatencyRefreshPeriodNs(
    std::string_view dump) noexcept {
    constexpr std::uint64_t kDefaultRefreshNs = 16666667ULL;
    constexpr std::uint64_t kMinRefreshNs = 4000000ULL;
    constexpr std::uint64_t kMaxRefreshNs = 34000000ULL;
    std::size_t pos = 0;
    while (pos < dump.size()) {
        std::size_t end = dump.find('\n', pos);
        if (end == std::string_view::npos) end = dump.size();
        const std::string_view line = dump.substr(pos, end - pos);
        pos = end + 1U;

        std::size_t i = 0;
        while (i < line.size() && integer_parse::isSpace(line[i])) ++i;
        if (i >= line.size()) continue;
        const std::size_t start = i;
        while (i < line.size() && !integer_parse::isSpace(line[i])) ++i;
        const std::size_t token_end = i;
        while (i < line.size() && integer_parse::isSpace(line[i])) ++i;
        if (i != line.size()) continue;  // 3-column timestamp rows are not headers

        std::uint64_t period_ns = 0;
        if (integer_parse::parseUint64(line.substr(start, token_end - start), period_ns) &&
            period_ns >= kMinRefreshNs && period_ns <= kMaxRefreshNs) {
            return period_ns;
        }
    }
    return kDefaultRefreshNs;
}

// Bounded, adaptive poller for SurfaceFlinger frame telemetry. The
// collection half is device-only (runs dumpsys); the parser above is
// host-testable. Unavailable telemetry yields empty results; the frame
// analyzer is only fed with returned durations.
class FrameTelemetrySource final {
public:
    // Each poll starts a bounded child process, so the rate is capped. The cap is
    // a conservative starting point and must be measured on device (ADR 0005).
    static constexpr unsigned kMaxPollHz = 5U;
    void setPollRatesHz(unsigned idle_hz, unsigned active_hz) noexcept {
        idle_hz_ = std::min(idle_hz, kMaxPollHz);
        active_hz_ = std::min(active_hz, kMaxPollHz);
    }
    [[nodiscard]] std::vector<std::uint64_t> poll(std::uint64_t now_ms,
                                                  bool jank_recent) noexcept;
    [[nodiscard]] std::uint64_t lastRefreshPeriodNs() const noexcept {
        return last_refresh_period_ns_;
    }

private:
    std::uint64_t next_poll_ms_{0};
    std::uint64_t last_refresh_period_ns_{16666667ULL};
    unsigned idle_hz_{1};
    unsigned active_hz_{5};
};

} // namespace coreflow::observe
