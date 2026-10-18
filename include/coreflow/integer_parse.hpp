#pragma once

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace coreflow::integer_parse {

// Strict unsigned-decimal parsing for kernel tunables. Every helper rejects an
// input it does not fully understand. None saturates silently, and none wraps.

[[nodiscard]] constexpr bool isSpace(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

[[nodiscard]] constexpr std::string_view trim(std::string_view raw) noexcept {
    while (!raw.empty() && isSpace(raw.front())) raw.remove_prefix(1);
    while (!raw.empty() && isSpace(raw.back())) raw.remove_suffix(1);
    return raw;
}

// The whole token, after surrounding whitespace, must be decimal digits. A sign,
// an internal space, trailing garbage, or a value above UINT64_MAX is rejected.
[[nodiscard]] inline bool parseUint64(std::string_view raw, std::uint64_t& out) noexcept {
    const std::string_view token = trim(raw);
    if (token.empty()) return false;
    std::uint64_t value = 0;
    const char* first = token.data();
    const char* last = token.data() + token.size();
    const auto result = std::from_chars(first, last, value);
    if (result.ec != std::errc{} || result.ptr != last) return false;
    out = value;
    return true;
}

// A whitespace-separated list of unsigned decimals. One invalid token rejects the
// whole list, so a corrupt sysfs read cannot quietly drop an entry.
[[nodiscard]] inline bool parseUint64List(std::string_view raw,
                                          std::vector<std::uint64_t>& out) noexcept {
    out.clear();
    std::size_t i = 0;
    while (i < raw.size()) {
        while (i < raw.size() && isSpace(raw[i])) ++i;
        if (i >= raw.size()) break;
        const std::size_t start = i;
        while (i < raw.size() && !isSpace(raw[i])) ++i;
        std::uint64_t value = 0;
        if (!parseUint64(raw.substr(start, i - start), value)) {
            out.clear();
            return false;
        }
        out.push_back(value);
    }
    return !out.empty();
}

// The frequency reached after `steps` distinct steps above `current` in an
// available_frequencies list. Only comparisons are performed, so nothing can
// overflow. Fails when the list is invalid, steps < 1, or no higher step exists.
[[nodiscard]] inline bool bumpedFrequency(std::string_view available, std::uint64_t current,
                                          int steps, std::string& requested) noexcept {
    if (steps < 1) return false;
    std::vector<std::uint64_t> frequencies;
    if (!parseUint64List(available, frequencies)) return false;
    std::sort(frequencies.begin(), frequencies.end());
    frequencies.erase(std::unique(frequencies.begin(), frequencies.end()), frequencies.end());

    std::uint64_t chosen = current;
    int taken = 0;
    for (const std::uint64_t frequency : frequencies) {
        if (frequency > chosen) {
            chosen = frequency;
            ++taken;
            if (taken >= steps) break;
        }
    }
    if (taken == 0) return false;
    requested = std::to_string(chosen);
    return true;
}

} // namespace coreflow::integer_parse
