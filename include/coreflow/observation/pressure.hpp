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
                                         double& output,
                                         double* measurement_resolution = nullptr) noexcept {
    if (snapshot.empty() || snapshot.size() > kMaximumPressureTextBytes) {
        return false;
    }

    bool found_some_line = false;
    bool found_avg10 = false;
    double parsed_avg10 = 0.0;
    double parsed_resolution = 0.0;

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
            // Preserve the precision of the published decimal instead of
            // inferring sensor resolution from a workload transition. In PSI
            // files this is commonly 0.01 for the avg10 fields.
            const auto exponent_pos = value_text.find_first_of("eE");
            const auto mantissa = value_text.substr(0, exponent_pos);
            int exponent = 0;
            if (exponent_pos != std::string_view::npos) {
                auto exponent_text = value_text.substr(exponent_pos + 1);
                // std::from_chars for integers intentionally does not accept a
                // leading '+', although scientific notation does.
                if (!exponent_text.empty() && exponent_text.front() == '+') {
                    exponent_text.remove_prefix(1);
                }
                if (exponent_text.empty()) return false;
                const auto exp_parse = std::from_chars(
                    exponent_text.data(), exponent_text.data() + exponent_text.size(), exponent);
                if (exp_parse.ec != std::errc{} ||
                    exp_parse.ptr != exponent_text.data() + exponent_text.size()) return false;
            }
            const auto decimal_pos = mantissa.find('.');
            const int decimals = decimal_pos == std::string_view::npos
                ? 0 : static_cast<int>(mantissa.size() - decimal_pos - 1);
            const double quantum = std::pow(10.0, (static_cast<double>(exponent) - static_cast<double>(decimals)));
            if (!std::isfinite(quantum) || quantum <= 0.0) return false;
            parsed_avg10 = candidate;
            parsed_resolution = quantum;
            found_avg10 = true;
        }
    }

    if (!found_some_line || !found_avg10) return false;
    output = parsed_avg10;
    if (measurement_resolution) *measurement_resolution = parsed_resolution;
    return true;
}

} // namespace coreflow::observation::pressure
