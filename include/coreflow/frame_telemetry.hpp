#pragma once

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <string>
#include <vector>

#include "coreflow/integer_parse.hpp"
#include "coreflow/bounded_process.hpp"
#include "coreflow/presentation_window.hpp"

namespace coreflow::observe {

// Legacy parser entry point. Return actual-present to actual-present intervals;
// AOSP's third column is frame-ready time, not presentation completion.
[[nodiscard]] inline std::vector<std::uint64_t> parseLatencyDurations(std::string_view dump) {
    PresentationWindow window;
    return window.consume(dump);
}

// Extracts the display refresh period in nanoseconds from the single-field
// header row emitted by `dumpsys SurfaceFlinger --latency` (e.g. 8333333 for
// 120 Hz, 11111111 for 90 Hz, 16666667 for 60 Hz). Falls back to 60 Hz when
// absent or out of plausible display range [4 ms, 34 ms].
[[nodiscard]] inline std::uint64_t parseLatencyRefreshPeriodNs(
    std::string_view dump, bool allow_fallback = true) noexcept {
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
        if (i != line.size()) break;  // first meaningful line must be a header

        std::uint64_t period_ns = 0;
        if (integer_parse::parseUint64(line.substr(start, token_end - start), period_ns) &&
            period_ns >= kMinRefreshNs && period_ns <= kMaxRefreshNs) {
            return period_ns;
        }
        break;
    }
    return allow_fallback ? kDefaultRefreshNs : 0U;
}

// Bounded, adaptive poller for SurfaceFlinger frame telemetry. The
// collection half is device-only (runs dumpsys); the parser above is
// host-testable. Unavailable telemetry yields empty results; the frame
// analyzer is only fed with returned durations.
class FrameTelemetrySource final {
public:
    using Runner = process::BoundedResult (*)(const std::vector<std::string>&,
                                              std::uint32_t, std::size_t) noexcept;
    explicit FrameTelemetrySource(Runner runner = process::runBounded) noexcept : runner_(runner != nullptr ? runner : process::runBounded) {}
    // Each poll starts a bounded child process, so the rate is capped. The cap is
    // a conservative starting point and must be measured on device (ADR 0005).
    static constexpr unsigned kMaxPollHz = 5U;
    void setForeground(std::string_view process);
    [[nodiscard]] const std::string& layer() const noexcept { return layer_; }
    [[nodiscard]] const char* availability() const noexcept { return availability_; }
    void reset() noexcept {
        presentations_.reset(); next_poll_ms_ = 0;
        layer_.clear(); foreground_.clear(); next_discovery_ms_ = 0;
        primed_ = false; availability_ = "foreground_unavailable";
        last_refresh_period_ns_ = 16666667ULL;
    }
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
    Runner runner_;
    std::string foreground_;
    std::string layer_;
    const char* availability_{"foreground_unavailable"};
    std::uint64_t next_discovery_ms_{0};
    bool primed_{false};
    PresentationWindow presentations_;
    std::uint64_t next_poll_ms_{0};
    std::uint64_t last_refresh_period_ns_{16666667ULL};
    unsigned idle_hz_{1};
    unsigned active_hz_{5};
};

} // namespace coreflow::observe
