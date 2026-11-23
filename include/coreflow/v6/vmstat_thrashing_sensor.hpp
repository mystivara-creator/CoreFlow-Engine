/**
 * @file vmstat_thrashing_sensor.hpp
 * @brief CoreFlow v6.2+: Universal Zero-Heap, Zero-eBPF Page-Cache Refault & Direct-Reclaim Sensor.
 *
 * Streams /proc/vmstat with zero heap allocations to compute per-second rates of:
 *   - workingset_refault_file (fallback: workingset_refault on kernels < 5.9)
 *   - workingset_refault_anon
 *   - pgsteal_direct (fallback: sum of pgsteal_direct_* zone counters)
 *   - pswpin
 *
 * Detects page-cache thrashing and synchronous direct-reclaim stalls 2-5 seconds before
 * UI freezes, driving adaptive two-regime MGLRU / swappiness / prefetch gating in
 * StorageMemoryActuator. Fail-closed when /proc/vmstat is unavailable.
 * @license Apache-2.0
 */
#pragma once

#include <cstdint>
#include <string>

namespace coreflow::v6 {

struct VmstatThrashingSample {
    bool available{false};
    double refault_file_per_sec{0.0};
    double refault_anon_per_sec{0.0};
    double direct_reclaim_per_sec{0.0};
    double pswpin_per_sec{0.0};
    double thrashing_pressure{0.0}; // 0.0..1.0 EWMA-smoothed composite pressure
    bool direct_reclaim_storm{false};
};

class VmstatThrashingSensor {
public:
    static constexpr uint64_t kMinSampleIntervalMs = 1000ULL;
    static constexpr double kDirectReclaimStormThreshold = 64.0; // pages/sec

    explicit VmstatThrashingSensor(std::string procfs_root = "/proc") noexcept;

    // Rate-limited zero-heap /proc/vmstat sampler. Pass active=false on ScreenOff for 0 syscalls.
    [[nodiscard]] VmstatThrashingSample sample(uint64_t now_ms, bool active = true) noexcept;

    void reset() noexcept;

    [[nodiscard]] bool supported() const noexcept { return supported_; }

private:
    struct RawCounters {
        uint64_t refault_file{0};
        uint64_t refault_anon{0};
        uint64_t pgsteal_direct{0};
        uint64_t pswpin{0};
        bool has_any{false};
    };

    [[nodiscard]] RawCounters read_vmstat_zero_alloc() const noexcept;

    std::string vmstat_path_;
    bool supported_{false};
    bool has_baseline_{false};
    RawCounters prev_{};
    uint64_t prev_ms_{0};
    uint64_t last_sample_ms_{0};
    double ewma_pressure_{0.0};
    VmstatThrashingSample last_sample_{};
};

} // namespace coreflow::v6
