#pragma once
#include "coreflow/frame_bridge.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <pthread.h>
#include <mutex>
namespace coreflow {
enum class TelemetryBackend : uint8_t { Unavailable, SysfsProcfs, Ebpf };
enum class DetectedWorkloadClass : uint8_t { Unknown, Gaming, IoHeavy, JankSensitive, CpuBound, GpuBound, MemoryPressure, Mixed };
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
inline constexpr std::size_t kWorkloadEventDrainBatchSize = 2048U;
struct WorkloadFeatureSnapshot final {
 uint64_t window_start_ns{0}, window_end_ns{0}, cpu_run_queue_wait_ns{0},
  wakeup_count{0}, io_bytes{0}, io_latency_ns_p95{0}, frame_deadline_misses{0},
  frame_deadline_count{0}, frame_actual_duration_ns_total{0}, frame_actual_duration_count{0};
 double frame_deadline_miss_pct{0.0}, wakeup_rate_per_sec{0.0}, cpu_pressure_some_pct{0.0},
  memory_pressure_some_pct{0.0}, io_pressure_some_pct{0.0}, gpu_busy_pct{0.0};
 uint32_t valid_features{FeatureNone}, sample_count{0}, event_count{0}, gpu_busy_sample_count{0};
 bool event_drain_saturated{false};
 TelemetryBackend source{TelemetryBackend::Unavailable};
};
struct WorkloadClassification final {
 DetectedWorkloadClass workload{DetectedWorkloadClass::Unknown};
 float confidence{0.0F};
 uint32_t evidence_mask{FeatureNone}, consecutive_windows{0};
 uint64_t classified_at_ns{0};
 bool stable{false};
 DetectedWorkloadClass pending_workload{DetectedWorkloadClass::Unknown};
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
public:
 explicit EbpfWorkloadSource(std::string sys_root="/sys",uint32_t ring_bytes=1U<<18U) noexcept;
 ~EbpfWorkloadSource() noexcept override;
 EbpfWorkloadSource(const EbpfWorkloadSource&)=delete;
 EbpfWorkloadSource& operator=(const EbpfWorkloadSource&)=delete;
 [[nodiscard]] TelemetryBackend backend() const noexcept final{return TelemetryBackend::Ebpf;}
 [[nodiscard]] bool start() noexcept override;
 void stop() noexcept override;
 [[nodiscard]] size_t drain(std::span<WorkloadEvent> out) noexcept override;
 [[nodiscard]] const BpfCapabilitySnapshot& capabilities() const noexcept{return capabilities_;}
 [[nodiscard]] bool active() const noexcept{return active_;}
private:
 void release_resources() noexcept;
 std::string sys_root_;
 uint32_t ring_bytes_{0};
 int map_fd_{-1},program_fd_{-1},wakeup_program_fd_{-1},switch_link_fd_{-1},wakeup_link_fd_{-1};
 void* consumer_mapping_{nullptr};
 void* producer_mapping_{nullptr};
 size_t page_size_{0};
 uint64_t ring_mask_{0};
 BpfCapabilitySnapshot capabilities_{};
 bool active_{false};
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

class GpuUtilizationSampler final {
public:
 explicit GpuUtilizationSampler(std::string sys_root="/sys") noexcept;
 [[nodiscard]] bool start() noexcept;
 void stop() noexcept;
 [[nodiscard]] bool sample(uint64_t now_ns,double& busy_pct) noexcept;
 [[nodiscard]] std::string_view source_name() const noexcept;
private:
 enum class Format : uint8_t { Unavailable, Percent, KgslIntervalSnapshot, CumulativeCounterDelta };
 std::string sys_root_,path_,source_name_;
 Format format_{Format::Unavailable};
 uint64_t previous_busy_{0},previous_total_{0},last_sample_ns_{0};
 bool previous_valid_{false},active_{false};
};
class FrameTimelineSampler final {
public:
 FrameTimelineSampler() noexcept=default;
 ~FrameTimelineSampler() noexcept;
 FrameTimelineSampler(const FrameTimelineSampler&)=delete;
 FrameTimelineSampler& operator=(const FrameTimelineSampler&)=delete;
 [[nodiscard]] bool start(uint32_t duration_ms=1000) noexcept;
 [[nodiscard]] bool finish(WorkloadFeatureSnapshot& features) noexcept;
 void stop() noexcept;
 [[nodiscard]] bool active() const noexcept{return child_pid_>0;}
private:
 int child_pid_{-1};
 std::string trace_path_;
};
[[nodiscard]] bool parse_frame_timeline_trace(std::span<const uint8_t> trace,
                                               WorkloadFeatureSnapshot& features) noexcept;

// Continuous low-priority consumer: bounded Perfetto captures run on a worker
// pthread, and only validated frame-count/miss windows cross into the reactor.
class PerfettoFrameTimelineEventSource final : public FrameEventSource {
public:
 [[nodiscard]] bool start(FrameEventListener listener) noexcept override;
 void stop() noexcept override;
 [[nodiscard]] bool latest_snapshot(WorkloadFeatureSnapshot& out) const noexcept;
 ~PerfettoFrameTimelineEventSource() override { stop(); }
 PerfettoFrameTimelineEventSource() = default;
 PerfettoFrameTimelineEventSource(const PerfettoFrameTimelineEventSource&) = delete;
 PerfettoFrameTimelineEventSource& operator=(const PerfettoFrameTimelineEventSource&) = delete;
private:
 static void* thread_entry(void* context) noexcept;
 void run() noexcept;
 mutable std::mutex lifecycle_mutex_;
 mutable std::mutex event_mutex_;
 mutable std::mutex snapshot_mutex_;
 FrameEventListener listener_{};
 WorkloadFeatureSnapshot latest_{};
 pthread_t worker_{};
 std::atomic<bool> running_{false};
 bool worker_started_{false};
};

class WorkloadTelemetryPipeline final {
public:
 explicit WorkloadTelemetryPipeline(std::string proc_root="/proc",std::string sys_root="/sys") noexcept;
 [[nodiscard]] bool start() noexcept;
 void stop() noexcept;
 [[nodiscard]] bool active() const noexcept{return active_;}
 [[nodiscard]] TelemetryBackend backend() const noexcept{return ebpf_.active()?TelemetryBackend::Ebpf:(fallback_active_?TelemetryBackend::SysfsProcfs:TelemetryBackend::Unavailable);}
 [[nodiscard]] WorkloadFeatureSnapshot sample(uint64_t window_start_ns,uint64_t window_end_ns) noexcept;
 [[nodiscard]] const BpfCapabilitySnapshot& bpf_capabilities() const noexcept{return ebpf_.capabilities();}
 [[nodiscard]] bool begin_frame_timeline_capture(uint32_t duration_ms=1000) noexcept{return frame_timeline_.start(duration_ms);}
 [[nodiscard]] bool finish_frame_timeline_capture(WorkloadFeatureSnapshot& features) noexcept{return frame_timeline_.finish(features);}
 [[nodiscard]] std::string_view gpu_source_name() const noexcept{return gpu_.source_name();}
private:
 EbpfWorkloadSource ebpf_;
 SysfsProcfsWorkloadSource fallback_;
 GpuUtilizationSampler gpu_;
 FrameTimelineSampler frame_timeline_;
 bool active_{false},fallback_active_{false},gpu_active_{false};
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
