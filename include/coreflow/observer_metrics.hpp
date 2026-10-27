#pragma once

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>

namespace coreflow::observer_metrics {

inline bool parsePsiPercentage(std::string_view text,
                               double& result) noexcept {
    if (text.empty() || text.size() >= 31U) return false;

    std::size_t i = 0U;
    while (i < text.size() &&
           text[i] >= '0' && text[i] <= '9') {
        ++i;
    }

    if (i == 0U) return false;

    if (i < text.size() && text[i] == '.') {
        ++i;
        const std::size_t fraction_begin = i;

        while (i < text.size() &&
               text[i] >= '0' && text[i] <= '9') {
            ++i;
        }

        if (i == fraction_begin) return false;
    }

    if (i != text.size()) return false;

    std::array<char, 32> buf{};
    std::memcpy(buf.data(), text.data(), text.size());
    buf[text.size()] = '\0';

    char* end = nullptr;
    errno = 0;
    const double value = std::strtod(buf.data(), &end);

    if (end != buf.data() + text.size() ||
        errno == ERANGE ||
        !std::isfinite(value) ||
        value < 0.0 || value > 100.0) {
        return false;
    }

    result = value;
    return true;
}

inline bool parsePsiLine(std::string_view line,
                         double& some_avg10,
                         double& full_avg10) noexcept {
    // Keep the production path allocation-free. Regex is used only as a
    // grammar oracle in tests, never in the runtime sampling path.
    const auto is_space = [](char c) noexcept {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    };
    const bool is_some = line.size() >= 4U && line.compare(0U, 4U, "some") == 0;
    const bool is_full = line.size() >= 4U && line.compare(0U, 4U, "full") == 0;
    if (!is_some && !is_full) return false;

    constexpr std::size_t prefix_size = 4U;
    if (line.size() <= prefix_size || !is_space(line[prefix_size]))
        return false;

    bool found_avg10 = false;
    bool found_avg60 = false;
    bool found_avg300 = false;
    bool found_total = false;
    double parsed_value = 0.0;
    std::size_t cursor = prefix_size;

    while (cursor < line.size()) {
        while (cursor < line.size() && is_space(line[cursor])) ++cursor;
        if (cursor == line.size()) break;

        const std::size_t token_begin = cursor;
        while (cursor < line.size() && !is_space(line[cursor])) ++cursor;
        const std::string_view token = line.substr(token_begin, cursor - token_begin);
        const auto equals = token.find('=');
        if (equals == std::string_view::npos || equals == 0U ||
            equals + 1U >= token.size()) return false;

        const std::string_view key = token.substr(0U, equals);
        const std::string_view value_text = token.substr(equals + 1U);
        if (key == "avg10") {
            if (found_avg10) return false;
            found_avg10 = true;

            if (!parsePsiPercentage(value_text, parsed_value))
                return false;
        } else if (key == "avg60" || key == "avg300") {
            bool& found = key == "avg60" ? found_avg60 : found_avg300;
            if (found) return false;
            found = true;
            double value = 0.0;
            if (!parsePsiPercentage(value_text, value))
                return false;
        } else if (key == "total") {
            if (found_total) return false;
            found_total = true;
            for (const char c : value_text) {
                if (c < '0' || c > '9') return false;
            }
        } else {
            return false;
        }
    }

    // A kernel PSI record is valid only when all four standard fields exist.
    if (!found_avg10 || !found_avg60 || !found_avg300 || !found_total)
        return false;

    if (is_some)
        some_avg10 = parsed_value;
    else
        full_avg10 = parsed_value;
    return true;
}

inline double normalizePsiPercent(double value) {
    if (!std::isfinite(value) || value < 0.0) return 0.0;
    return std::clamp(value / 100.0, 0.0, 1.0);
}


struct DiskCounters {
    std::uint64_t read_sectors{0};
    std::uint64_t write_sectors{0};
    std::uint64_t reads_completed{0};
    std::uint64_t writes_completed{0};
    std::uint64_t read_time_ms{0};
    std::uint64_t write_time_ms{0};
};

using DiskCounterMap = std::unordered_map<std::string, DiskCounters>;

struct IoDelta {
    double read_kb_per_sec{0.0};
    double write_kb_per_sec{0.0};
    double read_latency_ms{0.0};
    double write_latency_ms{0.0};
    double avg_latency_ms{0.0};
    bool activity_available{false};
    bool latency_available{false};
};

inline IoDelta calculateIoDelta(
    const DiskCounterMap& current,
    DiskCounterMap& previous,
    double interval_seconds) {
    IoDelta result{};

    const double interval = std::isfinite(interval_seconds)
        ? std::clamp(interval_seconds, 0.25, 3600.0)
        : 5.0;

    long double read_sectors_delta = 0.0L;
    long double write_sectors_delta = 0.0L;
    long double read_ops_total = 0.0L;
    long double write_ops_total = 0.0L;
    long double read_ms_total = 0.0L;
    long double write_ms_total = 0.0L;

    for (const auto& entry : current) {
        const auto old = previous.find(entry.first);
        if (old == previous.end()) continue;

        const auto& now = entry.second;
        const auto& before = old->second;

        if (now.read_sectors >= before.read_sectors) {
            read_sectors_delta += static_cast<long double>(
                now.read_sectors - before.read_sectors);
            result.activity_available = true;
        }

        if (now.write_sectors >= before.write_sectors) {
            write_sectors_delta += static_cast<long double>(
                now.write_sectors - before.write_sectors);
            result.activity_available = true;
        }

        const bool read_valid =
            now.reads_completed >= before.reads_completed &&
            now.read_time_ms >= before.read_time_ms;

        if (read_valid) {
            const auto ops = now.reads_completed - before.reads_completed;
            const auto elapsed = now.read_time_ms - before.read_time_ms;

            if (ops > 0U) {
                read_ops_total += static_cast<long double>(ops);
                read_ms_total += static_cast<long double>(elapsed);
            }
        }

        const bool write_valid =
            now.writes_completed >= before.writes_completed &&
            now.write_time_ms >= before.write_time_ms;

        if (write_valid) {
            const auto ops = now.writes_completed - before.writes_completed;
            const auto elapsed = now.write_time_ms - before.write_time_ms;

            if (ops > 0U) {
                write_ops_total += static_cast<long double>(ops);
                write_ms_total += static_cast<long double>(elapsed);
            }
        }
    }

    result.read_kb_per_sec = static_cast<double>(
        read_sectors_delta * 0.5L / interval);
    result.write_kb_per_sec = static_cast<double>(
        write_sectors_delta * 0.5L / interval);

    if (read_ops_total > 0.0L) {
        result.read_latency_ms = static_cast<double>(
            read_ms_total / read_ops_total);
    }

    if (write_ops_total > 0.0L) {
        result.write_latency_ms = static_cast<double>(
            write_ms_total / write_ops_total);
    }

    const long double total_ops = read_ops_total + write_ops_total;
    if (total_ops > 0.0L) {
        result.avg_latency_ms = static_cast<double>(
            (read_ms_total + write_ms_total) / total_ops);
        result.latency_available = true;
    }

    // Newly observed devices establish a baseline. Removed devices are
    // dropped, and reset counters do not contribute negative deltas.
    previous = current;
    return result;
}

} // namespace coreflow::observer_metrics
