/**
 * @file schedstat_sensor.hpp
 * @brief CoreFlow v6.2+: Universal Zero-Heap, Zero-eBPF CPU Runqueue Starvation Sensor.
 *
 * Reads Linux /proc/<pid>/task/<tid>/schedstat (run_delay_ns, sum_exec_runtime_ns, pcount)
 * across foreground critical threads (including the universal main PID) and SurfaceFlinger
 * threads to measure exact CPU runqueue wait latency in milliseconds on any SoC/engine.
 *
 * Strictly zero heap allocations in the hot path (fixed std::array + stack buffers) and
 * fail-closed (available = false, starvation_pressure = -1.0) when CONFIG_SCHEDSTATS=n.
 * @license Apache-2.0
 */
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace coreflow::v6 {

struct SchedstatSample {
    bool available{false};
    double peak_wait_ms{0.0};         // Smoothed peak runqueue wait per wakeup (ms)
    double wait_ratio{0.0};           // Smoothed peak fraction of wall time spent runnable-waiting (0..1)
    double exec_ratio{0.0};           // Peak CPU execution saturation from schedstat exec_ns (0..1)
    double starvation_pressure{-1.0}; // -1.0 when unavailable/baseline; 0.0..1.0 when active
};

class SchedstatSensor {
public:
    static constexpr size_t kMaxTrackedTids = 12U;

    explicit SchedstatSensor(std::string procfs_root = "/proc") noexcept;

    // Samples schedstat deltas across foreground TIDs and SurfaceFlinger TIDs.
    // Returns fail-closed {available=false, starvation_pressure=-1.0} if schedstat is unsupported.
    [[nodiscard]] SchedstatSample sample(
        int32_t fg_pid,
        const std::vector<int32_t>& fg_tids,
        const std::vector<int32_t>& sf_tids,
        uint32_t refresh_hz,
        double elapsed_sec) noexcept;

    // Resets baselines & EWMAs cleanly (e.g. on ScreenOff; performs zero syscalls).
    void reset() noexcept;

    [[nodiscard]] bool supported() const noexcept { return supported_; }

private:
    struct TidSlot {
        int32_t tid{0};
        uint64_t exec_ns{0};
        uint64_t delay_ns{0};
        uint64_t pcount{0};
        bool valid{false};
    };

    bool read_tid_schedstat(int32_t pid_hint, int32_t tid,
                            uint64_t& exec_ns, uint64_t& delay_ns, uint64_t& pcount) const noexcept;

    std::string procfs_root_;
    std::array<TidSlot, kMaxTrackedTids> slots_{};
    bool supported_{false};
    double ewma_wait_ms_{0.0};
    double ewma_wait_ratio_{0.0};
    double ewma_starvation_{-1.0};
};

} // namespace coreflow::v6
