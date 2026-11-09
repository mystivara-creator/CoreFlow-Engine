/**
 * @file workload_intelligence.hpp
 * @brief Event-driven workload telemetry contracts with a bounded procfs/sysfs fallback.
 * @license MIT
 *
 * Telemetry is evidence only. This layer never writes kernel controls or owns
 * mutation authority; classifications are inputs to policy and MutationCoordinator.
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <string_view>

namespace coreflow {

enum class TelemetryBackend : uint8_t { Unavailable, SysfsProcfs, Ebpf };
enum class WorkloadClass : uint8_t {
    Unknown, Gaming, IoHeavy, JankSensitive, CpuBound, GpuBound, MemoryPressure, Mixed
};
enum class WorkloadEventType : uint16_t {
    Unknown, SchedSwitch, SchedWakeup, BlockRequestIssue, BlockRequestComplete,
    CpuFrequency, PsiSample, FrameDeadlineMiss
};
enum class CapabilityState : uint8_t { Unknown, Unavailable, Restricted, Available };

enum WorkloadFeatureValidity : uint32_t {
    FeatureNone = 0,
    FeatureCpuRunQueue = 1U << 0U,
    FeatureWakeupRate = 1U << 1U,
    FeatureIoLatency = 1U << 2U,
    FeatureIoBytes = 1U << 3U,
    FeatureCpuPressure = 1U << 4U,
    FeatureMemoryPressure = 1U << 5U,
    FeatureIoPressure = 1U << 6U,
    FeatureFrameDeadline = 1U << 7U,
    FeatureGpuBusy = 1U << 8U
};

struct BpfCapabilitySnapshot final {
    CapabilityState syscall{CapabilityState::Unknown};
    CapabilityState btf{CapabilityState::Unknown};
    CapabilityState tracefs{CapabilityState::Unknown};
    CapabilityState ring_buffer{CapabilityState::Unknown};
    CapabilityState sched_switch{CapabilityState::Unknown};
    CapabilityState sched_wakeup{CapabilityState::Unknown};
    CapabilityState block_request{CapabilityState::Unknown};
    CapabilityState bpf_filesystem{CapabilityState::Unknown};
    bool selinux_allows_load{false};
    bool loader_verified{false};

    [[nodiscard]] bool event_pipeline_ready() const noexcept {
        return syscall == CapabilityState::Available &&
               btf == CapabilityState::Available &&
               tracefs == CapabilityState::Available &&
               ring_buffer == CapabilityState::Available &&
               sched_switch == CapabilityState::Available &&
               sched_wakeup == CapabilityState::Available &&
               selinux_allows_load && loader_verified;
    }
};

class BpfCapabilityProbe final {
public:
    explicit BpfCapabilityProbe(std::string_view sys_root = "/sys") : sys_root_(sys_root) {}
    [[nodiscard]] BpfCapabilitySnapshot probe() const noexcept;

private:
    std::string sys_root_;
};

struct WorkloadEvent final {
    uint64_t monotonic_timestamp_ns{0};
    uint64_t value0{0};
    uint64_t value1{0};
    uint32_t pid{0};
    uint32_t tid{0};
    uint32_t cpu{0};
    WorkloadEventType type{WorkloadEventType::Unknown};
    uint16_t flags{0};
};

struct WorkloadFeatureSnapshot final {
    uint64_t window_start_ns{0};
    uint64_t window_end_ns{0};
    uint64_t cpu_run_queue_wait_ns{0};
    uint64_t wakeup_count{0};
    uint64_t io_bytes{0};
    uint64_t io_latency_ns_p95{0};
    uint64_t frame_deadline_misses{0};
    double cpu_pressure_some_pct{0.0};
    double memory_pressure_some_pct{0.0};
    double io_pressure_some_pct{0.0};
    double gpu_busy_pct{0.0};
    uint32_t valid_features{FeatureNone};
    uint32_t sample_count{0};
    TelemetryBackend source{TelemetryBackend::Unavailable};
};

struct WorkloadClassification final {
    WorkloadClass workload{WorkloadClass::Unknown};
    float confidence{0.0F};
    uint32_t evidence_mask{FeatureNone};
    uint32_t consecutive_windows{0};
    uint64_t classified_at_ns{0};
    bool stable{false};
};

class WorkloadTelemetrySource {
public:
    virtual ~WorkloadTelemetrySource() = default;
    [[nodiscard]] virtual TelemetryBackend backend() const noexcept = 0;
    [[nodiscard]] virtual bool start() noexcept = 0;
    virtual void stop() noexcept = 0;
    // Returns the number of records written; implementations must never exceed out.size().
    [[nodiscard]] virtual size_t drain(std::span<WorkloadEvent> out) noexcept = 0;
};

class EbpfWorkloadSource : public WorkloadTelemetrySource {
public:
    [[nodiscard]] TelemetryBackend backend() const noexcept final { return TelemetryBackend::Ebpf; }
    // Concrete loader/ring-buffer implementation is intentionally gated on
    // BpfCapabilitySnapshot::event_pipeline_ready() and is introduced in P2-C.
};

class SysfsProcfsWorkloadSource : public WorkloadTelemetrySource {
public:
    [[nodiscard]] TelemetryBackend backend() const noexcept final { return TelemetryBackend::SysfsProcfs; }
    // Fallback samples PSI, CPU topology/frequency and available I/O counters.
    // Missing signals remain invalid; they are never synthesized as zero.
};

class WorkloadClassifier final {
public:
    // Pure evidence-to-classification boundary. No actuator or mutation API is exposed.
    [[nodiscard]] WorkloadClassification classify(
        const WorkloadFeatureSnapshot& features,
        const WorkloadClassification& previous) const noexcept;
};

[[nodiscard]] inline TelemetryBackend select_telemetry_backend(
    const BpfCapabilitySnapshot& bpf, bool fallback_available) noexcept {
    if (bpf.event_pipeline_ready()) return TelemetryBackend::Ebpf;
    return fallback_available ? TelemetryBackend::SysfsProcfs : TelemetryBackend::Unavailable;
}

} // namespace coreflow
