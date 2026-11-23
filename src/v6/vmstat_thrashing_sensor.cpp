/**
 * @file vmstat_thrashing_sensor.cpp
 * @brief CoreFlow v6.2+: Universal Zero-Heap, Zero-eBPF Page-Cache Refault & Direct-Reclaim Sensor.
 * @license Apache-2.0
 */
#include "coreflow/v6/vmstat_thrashing_sensor.hpp"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <fcntl.h>
#include <string_view>
#include <unistd.h>

namespace coreflow::v6 {

namespace {

bool parse_vmstat_kv(std::string_view line, std::string_view& key, uint64_t& value) noexcept {
    while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) line.remove_prefix(1U);
    while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
        line.remove_suffix(1U);
    }
    const size_t sp = line.find_first_of(" \t");
    if (sp == std::string_view::npos || sp == 0U) return false;
    key = line.substr(0U, sp);
    std::string_view val_sv = line.substr(sp + 1U);
    while (!val_sv.empty() && (val_sv.front() == ' ' || val_sv.front() == '\t')) {
        val_sv.remove_prefix(1U);
    }
    if (val_sv.empty() || val_sv.size() > 22U) return false;
    const auto r = std::from_chars(val_sv.data(), val_sv.data() + val_sv.size(), value);
    return r.ec == std::errc{} && r.ptr == val_sv.data() + val_sv.size();
}

} // namespace

VmstatThrashingSensor::VmstatThrashingSensor(std::string procfs_root) noexcept
    : vmstat_path_(procfs_root.empty() ? std::string("/proc/vmstat") : (procfs_root + "/vmstat")) {}

void VmstatThrashingSensor::reset() noexcept {
    has_baseline_ = false;
    prev_ = {};
    prev_ms_ = 0U;
    last_sample_ms_ = 0U;
    ewma_pressure_ = 0.0;
    last_sample_ = {};
}

VmstatThrashingSensor::RawCounters VmstatThrashingSensor::read_vmstat_zero_alloc() const noexcept {
    RawCounters out{};
    const int fd = ::open(vmstat_path_.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return out;

    bool seen_refault_file = false;
    uint64_t legacy_refault = 0U;
    bool seen_legacy_refault = false;
    bool seen_direct_unified = false;
    uint64_t direct_split_sum = 0U;
    bool seen_direct_split = false;

    auto process_line = [&](std::string_view line) {
        std::string_view key;
        uint64_t val = 0U;
        if (!parse_vmstat_kv(line, key, val)) return;
        if (key == "workingset_refault_file") {
            out.refault_file = val;
            seen_refault_file = true;
            out.has_any = true;
        } else if (key == "workingset_refault") {
            legacy_refault = val;
            seen_legacy_refault = true;
            out.has_any = true;
        } else if (key == "workingset_refault_anon") {
            out.refault_anon = val;
            out.has_any = true;
        } else if (key == "pgsteal_direct") {
            out.pgsteal_direct = val;
            seen_direct_unified = true;
            out.has_any = true;
        } else if (key.rfind("pgsteal_direct_", 0) == 0) {
            direct_split_sum += val;
            seen_direct_split = true;
            out.has_any = true;
        } else if (key == "pswpin") {
            out.pswpin = val;
            out.has_any = true;
        }
    };

    char chunk[4096];
    char line_buf[128];
    size_t line_len = 0U;
    bool line_overflow = false;
    size_t total_bytes = 0U;
    constexpr size_t kMaxVmstatBytes = 131072U;

    while (total_bytes < kMaxVmstatBytes) {
        const ssize_t n = ::read(fd, chunk, sizeof(chunk));
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        total_bytes += static_cast<size_t>(n);
        for (ssize_t i = 0; i < n; ++i) {
            const char ch = chunk[i];
            if (ch == '\n') {
                if (!line_overflow && line_len > 0U) {
                    process_line(std::string_view(line_buf, line_len));
                }
                line_len = 0U;
                line_overflow = false;
            } else if (!line_overflow) {
                if (line_len < sizeof(line_buf)) {
                    line_buf[line_len++] = ch;
                } else {
                    line_overflow = true;
                }
            }
        }
    }
    if (!line_overflow && line_len > 0U) {
        process_line(std::string_view(line_buf, line_len));
    }
    (void)::close(fd);

    if (!seen_refault_file && seen_legacy_refault) {
        out.refault_file = legacy_refault;
    }
    if (!seen_direct_unified && seen_direct_split) {
        out.pgsteal_direct = direct_split_sum;
    }
    return out;
}

VmstatThrashingSample VmstatThrashingSensor::sample(uint64_t now_ms, bool active) noexcept {
    if (!active) {
        reset();
        return {};
    }
    if (last_sample_ms_ > 0U && now_ms >= last_sample_ms_ &&
        (now_ms - last_sample_ms_) < kMinSampleIntervalMs) {
        return last_sample_;
    }
    last_sample_ms_ = now_ms;

    const RawCounters cur = read_vmstat_zero_alloc();
    if (!cur.has_any) {
        last_sample_ = {};
        return last_sample_;
    }
    supported_ = true;

    if (!has_baseline_ || now_ms <= prev_ms_ || (now_ms - prev_ms_) > 120'000ULL ||
        cur.refault_file < prev_.refault_file ||
        cur.refault_anon < prev_.refault_anon ||
        cur.pgsteal_direct < prev_.pgsteal_direct ||
        cur.pswpin < prev_.pswpin) {
        prev_ = cur;
        prev_ms_ = now_ms;
        has_baseline_ = true;
        VmstatThrashingSample init{};
        init.available = true;
        last_sample_ = init;
        return init;
    }

    const double dt_sec = static_cast<double>(now_ms - prev_ms_) / 1000.0;
    if (dt_sec < 0.25) {
        return last_sample_;
    }

    const double refault_file_ps = static_cast<double>(cur.refault_file - prev_.refault_file) / dt_sec;
    const double refault_anon_ps = static_cast<double>(cur.refault_anon - prev_.refault_anon) / dt_sec;
    const double direct_ps = static_cast<double>(cur.pgsteal_direct - prev_.pgsteal_direct) / dt_sec;
    const double pswpin_ps = static_cast<double>(cur.pswpin - prev_.pswpin) / dt_sec;

    prev_ = cur;
    prev_ms_ = now_ms;

    // File refaults: >= 250 pages/s (~1 MB/s) warns of working-set eviction; >= 2500 pages/s saturates.
    const double file_score = std::clamp((refault_file_ps - 250.0) / 2250.0, 0.0, 1.0);
    // Synchronous direct reclaim stalls: even 16 pages/s indicates foreground allocation blocking.
    const double direct_score = direct_ps >= 16.0
        ? std::clamp(0.30 + (direct_ps - 16.0) / 384.0, 0.0, 1.0)
        : 0.0;
    // ZRAM / swap-in ping-pong pressure.
    const double swap_score = std::clamp((pswpin_ps - 300.0) / 2700.0, 0.0, 0.75);

    const double inst_pressure = std::clamp(std::max({file_score, direct_score, swap_score}), 0.0, 1.0);
    ewma_pressure_ = 0.55 * ewma_pressure_ + 0.45 * inst_pressure;

    VmstatThrashingSample out{};
    out.available = true;
    out.refault_file_per_sec = refault_file_ps;
    out.refault_anon_per_sec = refault_anon_ps;
    out.direct_reclaim_per_sec = direct_ps;
    out.pswpin_per_sec = pswpin_ps;
    out.thrashing_pressure = std::clamp(ewma_pressure_, 0.0, 1.0);
    out.direct_reclaim_storm = (direct_ps >= kDirectReclaimStormThreshold);
    last_sample_ = out;
    return out;
}

} // namespace coreflow::v6
