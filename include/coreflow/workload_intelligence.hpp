#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
namespace coreflow {
enum class TelemetryBackend : uint8_t { Unavailable, SysfsProcfs, Ebpf };
enum class WorkloadClass : uint8_t { Unknown, Gaming, IoHeavy, JankSensitive, CpuBound, GpuBound, MemoryPressure, Mixed };
enum class WorkloadEventType : uint16_t { Unknown, SchedSwitch, SchedWakeup, BlockRequestIssue, BlockRequestComplete, CpuFrequency, PsiSample, FrameDeadlineMiss };
enum class CapabilityState : uint8_t { Unknown, Unavailable, Restricted, Available };
enum WorkloadFeatureValidity : uint32_t {
 FeatureNone=0, FeatureCpuRunQueue=1U<<0U, FeatureWakeupRate=1U<<1U,
 FeatureIoLatency=1U<<2U, FeatureIoBytes=1U<<3U, FeatureCpuPressure=1U<<4U,
 FeatureMemoryPressure=1U<<5U, FeatureIoPressure=1U<<6U,
 FeatureFrameDeadline=1U<<7U, FeatureGpuBusy=1U<<8U
};
struct BpfCapabilitySnapshot final {
 CapabilityState syscall{CapabilityState::Unknown}, btf{CapabilityState::Unknown},
 tracefs{CapabilityState::Unknown}, ring_buffer{CapabilityState::Unknown},
 sched_switch{CapabilityState::Unknown}, sched_wakeup{CapabilityState::Unknown},
 block_request{CapabilityState::Unknown}, bpf_filesystem{CapabilityState::Unknown};
 bool selinux_allows_load{false}, loader_verified{false};
 [[nodiscard]] bool event_pipeline_ready() const noexcept {
  return syscall==CapabilityState::Available && btf==CapabilityState::Available &&
   tracefs==CapabilityState::Available && ring_buffer==CapabilityState::Available &&
   sched_switch==CapabilityState::Available && sched_wakeup==CapabilityState::Available &&
   selinux_allows_load && loader_verified;
 }
};
class BpfCapabilityProbe final {
public:
 explicit BpfCapabilityProbe(std::string_view sys_root="/sys"):sys_root_(sys_root){}
 [[nodiscard]] BpfCapabilitySnapshot probe() const noexcept;
private: std::string sys_root_;
};
struct WorkloadEvent final {
 uint64_t monotonic_timestamp_ns{0}, value0{0}, value1{0};
 uint32_t pid{0}, tid{0}, cpu{0};
 WorkloadEventType type{WorkloadEventType::Unknown};
 uint16_t flags{0};
};
struct WorkloadFeatureSnapshot final {
 uint64_t window_start_ns{0}, window_end_ns{0}, cpu_run_queue_wait_ns{0},
  wakeup_count{0}, io_bytes{0}, io_latency_ns_p95{0}, frame_deadline_misses{0},
  frame_deadline_count{0};
 double frame_deadline_miss_pct{0.0}, cpu_pressure_some_pct{0.0},
  memory_pressure_some_pct{0.0}, io_pressure_some_pct{0.0}, gpu_busy_pct{0.0};
 uint32_t valid_features{FeatureNone}, sample_count{0};
 TelemetryBackend source{TelemetryBackend::Unavailable};
};
struct WorkloadClassification final {
 WorkloadClass workload{WorkloadClass::Unknown};
 float confidence{0.0F};
 uint32_t evidence_mask{FeatureNone}, consecutive_windows{0};
 uint64_t classified_at_ns{0};
 bool stable{false};
 WorkloadClass pending_workload{WorkloadClass::Unknown};
 uint32_t pending_windows{0};
 uint64_t last_transition_ns{0};
};
class WorkloadTelemetrySource {
public:
 virtual ~WorkloadTelemetrySource()=default;
 [[nodiscard]] virtual TelemetryBackend backend() const noexcept=0;
 [[nodiscard]] virtual bool start() noexcept=0;
 virtual void stop() noexcept=0;
 [[nodiscard]] virtual size_t drain(std::span<WorkloadEvent> out) noexcept=0;
};
class EbpfWorkloadSource:public WorkloadTelemetrySource {
public: [[nodiscard]] TelemetryBackend backend() const noexcept final{return TelemetryBackend::Ebpf;}
};
class SysfsProcfsWorkloadSource final:public WorkloadTelemetrySource {
public:
 explicit SysfsProcfsWorkloadSource(std::string proc_root="/proc",std::string sys_root="/sys") noexcept;
 [[nodiscard]] TelemetryBackend backend() const noexcept final{return TelemetryBackend::SysfsProcfs;}
 [[nodiscard]] bool start() noexcept override;
 void stop() noexcept override;
 [[nodiscard]] size_t drain(std::span<WorkloadEvent> out) noexcept override;
 [[nodiscard]] WorkloadFeatureSnapshot sample(uint64_t window_start_ns,uint64_t window_end_ns) noexcept;
private:
 std::string proc_root_,sys_root_;
 uint64_t previous_io_sectors_{0},previous_sample_ns_{0};
 bool started_{false};
};
class WorkloadClassifier final {
public:
 [[nodiscard]] WorkloadClassification classify(const WorkloadFeatureSnapshot& features,
  const WorkloadClassification& previous,uint64_t now_ns) const noexcept;
};
[[nodiscard]] inline TelemetryBackend select_telemetry_backend(const BpfCapabilitySnapshot& bpf,bool fallback_available) noexcept {
 if(bpf.event_pipeline_ready()) return TelemetryBackend::Ebpf;
 return fallback_available?TelemetryBackend::SysfsProcfs:TelemetryBackend::Unavailable;
}
} // namespace coreflow
