#pragma once

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace coreflow::observer_metrics {

inline bool parsePsiLine(const std::string& line,
                         double& some_avg10,
                         double& full_avg10) {
    const bool is_some = line.rfind("some ", 0) == 0;
    const bool is_full = line.rfind("full ", 0) == 0;

    if (!is_some && !is_full) return false;

    const auto pos = line.find("avg10=");
    if (pos == std::string::npos) return false;

    const char* begin = line.c_str() + pos + 6;
    char* end = nullptr;
    errno = 0;
    const double value = std::strtod(begin, &end);

    if (end == begin || errno == ERANGE || !std::isfinite(value))
        return false;

    // The numeric token must end at whitespace or the end of the line.
    if (*end != '\0' && *end != ' ' && *end != '\t' &&
        *end != '\r' && *end != '\n')
        return false;

    if (value < 0.0 || value > 100.0) return false;

    if (is_some)
        some_avg10 = value;
    else
        full_avg10 = value;

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
