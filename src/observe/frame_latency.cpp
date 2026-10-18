#include "coreflow/frame_latency.hpp"

#include <charconv>
#include <cstdint>
#include <string_view>
#include <system_error>

namespace coreflow::graphics {
namespace {

// SurfaceFlinger marks a not-yet-presented frame with INT64_MAX.
constexpr std::uint64_t kPendingSentinel = 9223372036854775807ULL;

bool parseU64(std::string_view token, std::uint64_t& out) noexcept {
    if (token.empty()) return false;
    const char* first = token.data();
    const char* last = token.data() + token.size();
    const auto result = std::from_chars(first, last, out);
    return result.ec == std::errc{} && result.ptr == last;
}

// Splits on spaces and tabs, ignoring trailing carriage return.
std::size_t splitFields(std::string_view line, std::string_view (&fields)[3]) noexcept {
    std::size_t count = 0;
    std::size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r')) ++i;
        if (i >= line.size()) break;
        const std::size_t start = i;
        while (i < line.size() && line[i] != ' ' && line[i] != '\t' && line[i] != '\r') ++i;
        if (count == 3U) return 4U;  // too many fields: reject the row
        fields[count++] = line.substr(start, i - start);
    }
    return count;
}

} // namespace

FrameLatencyTrace parseSurfaceFlingerLatency(const std::string& text) noexcept {
    FrameLatencyTrace trace;
    if (text.size() > FrameLatencyTrace::kMaxInputBytes) return trace;

    const std::string_view view(text);
    std::size_t pos = 0;
    bool header_seen = false;
    std::uint64_t previous_actual = 0;
    bool have_previous = false;

    while (pos < view.size()) {
        const std::size_t eol = view.find('\n', pos);
        const std::string_view line =
            view.substr(pos, eol == std::string_view::npos ? std::string_view::npos : eol - pos);
        pos = eol == std::string_view::npos ? view.size() : eol + 1U;

        std::string_view fields[3];
        const std::size_t count = splitFields(line, fields);
        if (count == 0U) continue;  // blank line

        if (!header_seen) {
            // The first meaningful line is the refresh period. Anything else is not
            // this format, so the whole trace is rejected.
            if (count != 1U || !parseU64(fields[0], trace.refresh_period_ns) ||
                trace.refresh_period_ns == 0U) {
                return FrameLatencyTrace{};
            }
            header_seen = true;
            continue;
        }

        if (trace.rows_total >= FrameLatencyTrace::kMaxRows) break;
        ++trace.rows_total;

        std::uint64_t desired = 0;
        std::uint64_t actual = 0;
        std::uint64_t ready = 0;
        if (count != 3U || !parseU64(fields[0], desired) || !parseU64(fields[1], actual) ||
            !parseU64(fields[2], ready)) {
            ++trace.rows_rejected;
            continue;
        }
        if (actual == kPendingSentinel) {
            ++trace.rows_pending;
            continue;
        }
        if (actual == 0U) {
            ++trace.rows_rejected;
            continue;
        }
        if (have_previous) {
            if (actual <= previous_actual) {
                ++trace.rows_rejected;  // non-monotonic: never a negative interval
                continue;
            }
            const std::uint64_t interval = actual - previous_actual;
            if (interval > FrameLatencyTrace::kMaxIntervalNs) {
                ++trace.rows_rejected;
                previous_actual = actual;
                continue;
            }
            trace.frame_intervals_ns.push_back(interval);
        }
        previous_actual = actual;
        have_previous = true;
    }

    trace.valid = header_seen && !trace.frame_intervals_ns.empty();
    if (!trace.valid) {
        trace.frame_intervals_ns.clear();
    }
    return trace;
}

} // namespace coreflow::graphics
