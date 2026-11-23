/**
 * @file schedstat_sensor.cpp
 * @brief CoreFlow v6.2+: Universal Zero-Heap, Zero-eBPF CPU Runqueue Starvation Sensor.
 * @license Apache-2.0
 */
#include "coreflow/v6/schedstat_sensor.hpp"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <fcntl.h>
#include <string_view>
#include <unistd.h>

namespace coreflow::v6 {

namespace {

bool parse_three_u64(std::string_view raw, uint64_t& a, uint64_t& b, uint64_t& c) noexcept {
    auto next_u64 = [&](size_t& pos, uint64_t& out) -> bool {
        while (pos < raw.size() && (raw[pos] == ' ' || raw[pos] == '\t' || raw[pos] == '\r' || raw[pos] == '\n')) {
            ++pos;
        }
        if (pos >= raw.size()) return false;
        size_t end = pos;
        while (end < raw.size() && raw[end] >= '0' && raw[end] <= '9') ++end;
        if (end == pos || (end - pos) > 22U) return false;
        const auto r = std::from_chars(raw.data() + pos, raw.data() + end, out);
        if (r.ec != std::errc{} || r.ptr != raw.data() + end) return false;
        pos = end;
        return true;
    };
    size_t pos = 0U;
    return next_u64(pos, a) && next_u64(pos, b) && next_u64(pos, c);
}

bool read_schedstat_path(const char* path, uint64_t& exec_ns, uint64_t& delay_ns, uint64_t& pcount) noexcept {
    const int fd = ::open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return false;
    char buf[128];
    ssize_t n = 0;
    do {
        n = ::read(fd, buf, sizeof(buf) - 1U);
    } while (n < 0 && errno == EINTR);
    (void)::close(fd);
    if (n <= 0) return false;
    buf[n] = '\0';
    return parse_three_u64(std::string_view(buf, static_cast<size_t>(n)), exec_ns, delay_ns, pcount);
}

} // namespace

SchedstatSensor::SchedstatSensor(std::string procfs_root) noexcept
    : procfs_root_(std::move(procfs_root)) {}

void SchedstatSensor::reset() noexcept {
    for (auto& s : slots_) {
        s = {};
    }
    ewma_wait_ms_ = 0.0;
    ewma_wait_ratio_ = 0.0;
    ewma_starvation_ = -1.0;
}

bool SchedstatSensor::read_tid_schedstat(
    int32_t pid_hint,
    int32_t tid,
    uint64_t& exec_ns,
    uint64_t& delay_ns,
    uint64_t& pcount) const noexcept {
    if (tid <= 0) return false;
    char path[160];
    if (pid_hint > 0) {
        const int len = std::snprintf(
            path, sizeof(path), "%s/%d/task/%d/schedstat",
            procfs_root_.c_str(), pid_hint, tid);
        if (len > 0 && static_cast<size_t>(len) < sizeof(path)) {
            if (read_schedstat_path(path, exec_ns, delay_ns, pcount)) return true;
        }
    }
    const int len2 = std::snprintf(
        path, sizeof(path), "%s/%d/schedstat",
        procfs_root_.c_str(), tid);
    if (len2 > 0 && static_cast<size_t>(len2) < sizeof(path)) {
        return read_schedstat_path(path, exec_ns, delay_ns, pcount);
    }
    return false;
}

SchedstatSample SchedstatSensor::sample(
    int32_t fg_pid,
    const std::vector<int32_t>& fg_tids,
    const std::vector<int32_t>& sf_tids,
    uint32_t refresh_hz,
    double elapsed_sec) noexcept {
    SchedstatSample out{};
    if (!std::isfinite(elapsed_sec) || elapsed_sec <= 0.05 || elapsed_sec > 60.0) {
        return out;
    }

    // Build bounded unique target list on the stack (strictly 0 heap allocations).
    struct Target {
        int32_t pid_hint{0};
        int32_t tid{0};
    };
    std::array<Target, kMaxTrackedTids> targets{};
    size_t target_count = 0U;

    auto push_unique = [&](int32_t pid_hint, int32_t tid) {
        if (tid <= 0 || target_count >= kMaxTrackedTids) return;
        for (size_t i = 0U; i < target_count; ++i) {
            if (targets[i].tid == tid) return;
        }
        targets[target_count++] = Target{pid_hint, tid};
    };

    // Always include foreground main thread (universal across unknown custom engines/apps),
    // then foreground critical render/game TIDs (up to 8 total), then SurfaceFlinger TIDs (up to 4).
    if (fg_pid > 0) {
        push_unique(fg_pid, fg_pid);
        for (size_t i = 0U; i < fg_tids.size() && target_count < 8U; ++i) {
            push_unique(fg_pid, fg_tids[i]);
        }
    }
    const int32_t sf_pid_hint = sf_tids.empty() ? 0 : sf_tids.front();
    for (size_t i = 0U; i < sf_tids.size() && target_count < kMaxTrackedTids; ++i) {
        push_unique(sf_pid_hint, sf_tids[i]);
    }

    if (target_count == 0U) {
        reset();
        return out;
    }

    std::array<TidSlot, kMaxTrackedTids> next_slots{};
    size_t next_count = 0U;
    size_t readable = 0U;
    size_t delta_samples = 0U;

    const double dt_ns = elapsed_sec * 1e9;
    double peak_wait_ms_inst = 0.0;
    double peak_wait_ratio_inst = 0.0;
    double peak_exec_ratio_inst = 0.0;

    for (size_t i = 0U; i < target_count; ++i) {
        uint64_t exec_ns = 0U, delay_ns = 0U, pcount = 0U;
        if (!read_tid_schedstat(targets[i].pid_hint, targets[i].tid, exec_ns, delay_ns, pcount)) {
            continue;
        }
        ++readable;
        next_slots[next_count++] = TidSlot{targets[i].tid, exec_ns, delay_ns, pcount, true};

        // Match previous slot for the same TID (linear scan over <= 12 stack elements).
        for (const auto& prev : slots_) {
            if (!prev.valid || prev.tid != targets[i].tid) continue;
            if (exec_ns >= prev.exec_ns && delay_ns >= prev.delay_ns && pcount >= prev.pcount) {
                const uint64_t d_delay = delay_ns - prev.delay_ns;
                const uint64_t d_exec = exec_ns - prev.exec_ns;
                const uint64_t d_pcount = pcount - prev.pcount;
                const double wait_ms = d_pcount > 0U
                    ? (static_cast<double>(d_delay) / (static_cast<double>(d_pcount) * 1e6))
                    : (static_cast<double>(d_delay) / 1e6);
                const double w_ratio = std::clamp(static_cast<double>(d_delay) / dt_ns, 0.0, 1.0);
                const double e_ratio = std::clamp(static_cast<double>(d_exec) / dt_ns, 0.0, 1.0);
                if (std::isfinite(wait_ms)) peak_wait_ms_inst = std::max(peak_wait_ms_inst, wait_ms);
                if (std::isfinite(w_ratio)) peak_wait_ratio_inst = std::max(peak_wait_ratio_inst, w_ratio);
                if (std::isfinite(e_ratio)) peak_exec_ratio_inst = std::max(peak_exec_ratio_inst, e_ratio);
                ++delta_samples;
            }
            break;
        }
    }

    slots_ = next_slots;

    if (readable == 0U) {
        out.available = false;
        out.starvation_pressure = -1.0;
        return out;
    }

    supported_ = true;
    out.available = true;
    if (delta_samples == 0U) {
        // First tick only establishes baseline counters; stay neutral (-1.0) until a delta arrives.
        out.starvation_pressure = -1.0;
        return out;
    }

    // Normalize runqueue delay against live display frame budget T_frame = 1000 / Hz ms:
    // - 120 Hz (8.33 ms): reacts >= 1.0 ms avg wait/wakeup, saturates at 4.17 ms.
    // - 60 Hz (16.67 ms): reacts >= 2.0 ms avg wait/wakeup, saturates at 8.33 ms.
    const double frame_ms = 1000.0 / static_cast<double>(std::clamp(refresh_hz, 30U, 240U));
    const double wait_ms_score = std::clamp(
        (peak_wait_ms_inst - frame_ms * 0.12) / (frame_ms * 0.38), 0.0, 1.0);
    const double wait_ratio_score = std::clamp((peak_wait_ratio_inst - 0.05) / 0.20, 0.0, 1.0);
    const double inst_starvation = std::max(wait_ms_score, wait_ratio_score);

    ewma_wait_ms_ = (ewma_starvation_ < 0.0) ? peak_wait_ms_inst : (0.60 * ewma_wait_ms_ + 0.40 * peak_wait_ms_inst);
    ewma_wait_ratio_ = (ewma_starvation_ < 0.0) ? peak_wait_ratio_inst : (0.60 * ewma_wait_ratio_ + 0.40 * peak_wait_ratio_inst);
    ewma_starvation_ = (ewma_starvation_ < 0.0) ? inst_starvation : (0.60 * ewma_starvation_ + 0.40 * inst_starvation);

    out.peak_wait_ms = ewma_wait_ms_;
    out.wait_ratio = ewma_wait_ratio_;
    out.exec_ratio = peak_exec_ratio_inst;
    out.starvation_pressure = std::clamp(ewma_starvation_, 0.0, 1.0);
    return out;
}

} // namespace coreflow::v6
