#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace coreflow {

struct WarmupFileRange final {
    std::string path;
    uint64_t offset{0};
    uint64_t length{0};
};

struct MemoryWarmupAdmission final {
    uint64_t available_headroom_bytes{0};
    uint64_t configured_budget_bytes{0};
    uint64_t max_single_file_bytes{0};
    uint64_t telemetry_timestamp_ns{0};
    uint64_t now_ns{0};
    uint64_t max_telemetry_age_ns{0};
    double memory_pressure_pct{0.0};
    double io_pressure_pct{0.0};
    bool memory_pressure_valid{false};
    bool io_pressure_valid{false};
    bool thermal_admission_valid{false};
    bool thermal_admission_granted{false};
    bool working_set_prediction_valid{false};
    bool foreground_identity_valid{false};
};

struct MemoryWarmupResult final {
    uint64_t requested_bytes{0};
    uint64_t admitted_bytes{0};
    uint32_t files_advised{0};
    uint32_t files_rejected{0};
    bool cancelled{false};
};

// Advisory page-cache warmup only. It does not lock pages, prevent reclaim,
// launch background applications, or claim that pages are resident.
class MemoryWarmupController final {
public:
    explicit MemoryWarmupController(uint64_t hard_budget_bytes = 32ULL * 1024ULL * 1024ULL) noexcept;
    ~MemoryWarmupController() noexcept;
    MemoryWarmupController(const MemoryWarmupController&) = delete;
    MemoryWarmupController& operator=(const MemoryWarmupController&) = delete;

    [[nodiscard]] MemoryWarmupResult warmup(
        const std::vector<WarmupFileRange>& ranges,
        const MemoryWarmupAdmission& admission) noexcept;

    void cancel() noexcept;
    [[nodiscard]] uint64_t hard_budget_bytes() const noexcept { return hard_budget_bytes_; }

private:
    uint64_t hard_budget_bytes_{0};
    std::atomic<bool> cancelled_{false};
};

} // namespace coreflow
