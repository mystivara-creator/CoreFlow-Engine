#pragma once

#include <charconv>
#include <cstddef>
#include <cmath>
#include <string_view>
#include <system_error>

namespace coreflow::observation::pressure {

inline constexpr std::size_t kMaximumPressureTextBytes = 4096;

// Parse the PSI "some avg10=..." percentage from a bounded /proc/pressure
// snapshot. Unknown key/value fields are tolerated for forward compatibility,
// while duplicate/malformed avg10 values and percentages outside [0, 100] fail
// closed. The output argument is unchanged on failure.
[[nodiscard]] inline bool parseSomeAvg10(std::string_view snapshot,
                                         double& output) noexcept {
    if (snapshot.empty() || snapshot.size() > kMaximumPressureTextBytes) {
        return false;
    }

    bool found_some_line = false;
    bool found_avg10 = false;
    double parsed_avg10 = 0.0;

    std::size_t offset = 0;
    while (offset < snapshot.size()) {
        const auto newline = snapshot.find('\n', offset);
        const auto end = newline == std::string_view::npos ? snapshot.size() : newline;
        std::string_view line = snapshot.substr(offset, end - offset);
        offset = newline == std::string_view::npos ? snapshot.size() : newline + 1;

        while (!line.empty() && (line.front() == ' ' || line.front() == '\t' || line.front() == '\r'))
            line.remove_prefix(1);
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r'))
            line.remove_suffix(1);
        if (line.empty()) continue;

        const auto first_space = line.find_first_of(" \t");
        const auto kind = line.substr(0, first_space);
        if (kind != "some") continue;
        if (found_some_line) return false;
        found_some_line = true;
        if (first_space == std::string_view::npos) return false;

        std::string_view fields = line.substr(first_space + 1);
        while (!fields.empty()) {
            while (!fields.empty() && (fields.front() == ' ' || fields.front() == '\t'))
                fields.remove_prefix(1);
            if (fields.empty()) break;

            const auto separator = fields.find_first_of(" \t");
            const auto token = fields.substr(0, separator);
            fields = separator == std::string_view::npos
                ? std::string_view{} : fields.substr(separator + 1);

            const auto equal = token.find('=');
            if (equal == std::string_view::npos || equal == 0 || equal + 1 >= token.size())
                return false;
            if (token.substr(0, equal) != "avg10") continue;
            if (found_avg10) return false;

            const auto value_text = token.substr(equal + 1);
            double candidate = 0.0;
            const auto parsed = std::from_chars(
                value_text.data(), value_text.data() + value_text.size(),
                candidate, std::chars_format::general);
            if (parsed.ec != std::errc{} ||
                parsed.ptr != value_text.data() + value_text.size() ||
                !std::isfinite(candidate) || candidate < 0.0 || candidate > 100.0) {
                return false;
            }
            parsed_avg10 = candidate;
            found_avg10 = true;
        }
    }

    if (!found_some_line || !found_avg10) return false;
    output = parsed_avg10;
    return true;
}

} // namespace coreflow::observation::pressure
