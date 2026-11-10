#include "coreflow/platform_context.hpp"
#include "coreflow/workload_intelligence.hpp"
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <unistd.h>
using namespace coreflow;
static_assert(kWorkloadEventDrainBatchSize==2048U);
class FakeProperties final:public AndroidPropertySource{
public:std::unordered_map<std::string,std::string> values;
 bool read(std::string_view k,std::string& v)const noexcept override{auto i=values.find(std::string(k));if(i==values.end())return false;v=i->second;return true;}
};
static WorkloadFeatureSnapshot gaming(uint64_t end){WorkloadFeatureSnapshot f{};f.window_start_ns=end-1'000'000'000ULL;f.window_end_ns=end;f.gpu_busy_pct=82;f.frame_deadline_count=600;f.valid_features=FeatureGpuBusy|FeatureFrameDeadline;f.source=TelemetryBackend::Ebpf;return f;}
int main(){
 FakeProperties x;x.values={{"ro.product.manufacturer","Xiaomi"},{"ro.product.brand","Redmi"},{"ro.product.vendor.manufacturer","Xiaomi"},{"ro.mi.os.version.name","2.0.204.0"},{"ro.build.version.release","15"},{"ro.build.fingerprint","Xiaomi/kunzite_global/kunzite:15/UP1A:user/release-keys"}};
 auto t=PlatformContextDetector(x).detect(7);assert(t.tier1_eligible&&t.generation==7);
 FakeProperties x3=x;x3.values["ro.mi.os.version.name"]="3.0.1";assert(PlatformContextDetector(x3).detect().tier1_eligible);
 FakeProperties x4=x;x4.values["ro.mi.os.version.name"]="4.0.0";auto h4=PlatformContextDetector(x4).detect();assert(!h4.tier1_eligible&&h4.execution_tier==ExecutionTier::UniversalFallback);
 FakeProperties miui;miui.values={{"ro.product.manufacturer","Xiaomi"},{"ro.product.brand","Redmi"},{"ro.miui.ui.version.name","V14"}};assert(!PlatformContextDetector(miui).detect().tier1_eligible);
 FakeProperties samsung;samsung.values={{"ro.product.manufacturer","Samsung"},{"ro.product.brand","Samsung"}};assert(PlatformContextDetector(samsung).detect().policy_family==PolicyFamily::CoreFlowBase);
 FakeProperties conflict;conflict.values={{"ro.product.manufacturer","Samsung"},{"ro.product.brand","Redmi"},{"ro.mi.os.version.name","3.0"}};assert(!PlatformContextDetector(conflict).detect().tier1_eligible);
 FakeProperties custom=x;custom.values["ro.build.fingerprint"]="lineage/kunzite/kunzite:15/userdebug";assert(!PlatformContextDetector(custom).detect().tier1_eligible);
 char tmp[]="/tmp/coreflow-p2c-XXXXXX";assert(::mkdtemp(tmp));std::filesystem::path root(tmp);
 auto write=[&](const std::filesystem::path& p,const char* v){std::filesystem::create_directories(p.parent_path());std::ofstream o(p);o<<v;assert(o.good());};
 write(root/"kernel/btf/vmlinux","btf");write(root/"kernel/tracing/events/sched/sched_switch/id","1");write(root/"kernel/tracing/events/sched/sched_wakeup/id","2");write(root/"kernel/tracing/events/block/block_rq_issue/id","3");write(root/"kernel/tracing/events/block/block_rq_complete/id","4");std::filesystem::create_directories(root/"fs/bpf");
 auto b=BpfCapabilityProbe(root.string()).probe();assert(b.btf==CapabilityState::Available&&b.tracefs==CapabilityState::Available);assert(b.bpf_filesystem==CapabilityState::Unavailable);assert(b.syscall==CapabilityState::Unknown&&b.ring_buffer==CapabilityState::Unknown&&!b.event_pipeline_ready());assert(select_telemetry_backend(b,true)==TelemetryBackend::SysfsProcfs);
 auto proc=root/"proc";write(proc/"pressure/cpu","some avg10=12.50 avg60=2 total=10\n");write(proc/"pressure/memory","some avg10=3.25 avg60=2 total=10\n");write(proc/"pressure/io","some avg10=8.75 avg60=2 total=10\n");write(proc/"diskstats","8 0 sda 10 0 100 0 20 0 200 0 0 0 0\n");
 SysfsProcfsWorkloadSource sampler(proc.string(),root.string());assert(sampler.start());auto s1=sampler.sample(1'000'000'000ULL,2'000'000'000ULL);assert(s1.source==TelemetryBackend::SysfsProcfs);assert((s1.valid_features&FeatureCpuPressure)&&(s1.valid_features&FeatureMemoryPressure)&&(s1.valid_features&FeatureIoPressure));assert(!(s1.valid_features&FeatureIoBytes));
 write(proc/"diskstats","8 0 sda 20 0 150 0 40 0 250 0 0 0 0\n");auto s2=sampler.sample(2'000'000'000ULL,3'000'000'000ULL);assert((s2.valid_features&FeatureIoBytes)&&s2.io_bytes==51'200);assert(!(s2.valid_features&FeatureGpuBusy));

 // KGSL gpubusy is a cumulative busy/total counter pair. Prefer its deltas
 // over the OEM compatibility percentage node, which may remain frozen at 0.
 char gpu_root_template[]="/tmp/coreflow-gpu-counter-XXXXXX";assert(::mkdtemp(gpu_root_template));
 std::filesystem::path gpu_root(gpu_root_template);
 write(gpu_root/"class/kgsl/kgsl-3d0/gpubusy","100 200\n");
 write(gpu_root/"class/kgsl/kgsl-3d0/gpu_busy_percentage","0\n");
 GpuUtilizationSampler gpu_sampler(gpu_root.string());assert(gpu_sampler.start());
 assert(gpu_sampler.source_name()=="kgsl/gpubusy");
 write(gpu_root/"class/kgsl/kgsl-3d0/gpubusy","160 300\n");
 double gpu_pct=-1.0;assert(gpu_sampler.sample(1'000'000'000ULL,gpu_pct));
 assert(std::abs(gpu_pct-60.0)<0.001);
 write(gpu_root/"class/kgsl/kgsl-3d0/gpubusy","260 500\n");
 assert(gpu_sampler.sample(1'200'000'000ULL,gpu_pct));assert(std::abs(gpu_pct-50.0)<0.001);
 // A counter reset re-establishes the baseline instead of emitting a spike.
 write(gpu_root/"class/kgsl/kgsl-3d0/gpubusy","20 40\n");
 assert(!gpu_sampler.sample(1'400'000'000ULL,gpu_pct));
 write(gpu_root/"class/kgsl/kgsl-3d0/gpubusy","45 90\n");
 assert(gpu_sampler.sample(1'600'000'000ULL,gpu_pct));assert(std::abs(gpu_pct-50.0)<0.001);
 gpu_sampler.stop();std::filesystem::remove_all(gpu_root);

 // If gpubusy is malformed, gpu_busy_percentage remains the final fallback.
 char gpu_fallback_template[]="/tmp/coreflow-gpu-fallback-XXXXXX";assert(::mkdtemp(gpu_fallback_template));
 std::filesystem::path gpu_fallback(gpu_fallback_template);
 write(gpu_fallback/"class/kgsl/kgsl-3d0/gpubusy","frozen or malformed\n");
 write(gpu_fallback/"class/kgsl/kgsl-3d0/gpu_busy_percentage","37\n");
 GpuUtilizationSampler fallback_gpu(gpu_fallback.string());assert(fallback_gpu.start());
 assert(fallback_gpu.source_name()=="kgsl/gpu_busy_percentage");
 assert(fallback_gpu.sample(2'000'000'000ULL,gpu_pct));assert(std::abs(gpu_pct-37.0)<0.001);
 fallback_gpu.stop();std::filesystem::remove_all(gpu_fallback);


 // Perfetto FrameTimeline parser: one on-time display frame and one dropped frame.
 auto varint=[](std::vector<uint8_t>& out,uint64_t value){
  while(value>=0x80U){out.push_back(static_cast<uint8_t>((value&0x7fU)|0x80U));value>>=7U;}
  out.push_back(static_cast<uint8_t>(value));
 };
 auto field_bytes=[&](std::vector<uint8_t>& out,uint32_t id,const std::vector<uint8_t>& payload){
  varint(out,(static_cast<uint64_t>(id)<<3U)|2U);varint(out,payload.size());out.insert(out.end(),payload.begin(),payload.end());
 };
 auto field_varint=[&](std::vector<uint8_t>& out,uint32_t id,uint64_t value){
  varint(out,static_cast<uint64_t>(id)<<3U);varint(out,value);
 };
 std::vector<uint8_t> on_time;field_varint(on_time,4,1);field_varint(on_time,5,1);
 std::vector<uint8_t> dropped;field_varint(dropped,4,4);field_varint(dropped,5,0);field_varint(dropped,7,1024);
 std::vector<uint8_t> event_a;field_bytes(event_a,2,on_time);
 std::vector<uint8_t> event_b;field_bytes(event_b,2,dropped);
 std::vector<uint8_t> packet_a;field_bytes(packet_a,76,event_a);
 std::vector<uint8_t> packet_b;field_bytes(packet_b,76,event_b);
 std::vector<uint8_t> trace;field_bytes(trace,1,packet_a);field_bytes(trace,1,packet_b);
 WorkloadFeatureSnapshot frame_stats{};
 assert(parse_frame_timeline_trace(trace,frame_stats));
 assert((frame_stats.valid_features&FeatureFrameDeadline)&&frame_stats.frame_deadline_count==2);
 assert(frame_stats.frame_deadline_misses==1&&frame_stats.frame_deadline_miss_pct==50.0);
 std::vector<uint8_t> malformed{0x0a,0x05,0x82};
 WorkloadFeatureSnapshot invalid_frame{};
 assert(!parse_frame_timeline_trace(malformed,invalid_frame));

 WorkloadClassifier classifier;WorkloadClassification state{};state=classifier.classify(gaming(4'000'000'000ULL),state,4'100'000'000ULL);assert(state.workload==DetectedWorkloadClass::Unknown&&state.pending_windows==1);state=classifier.classify(gaming(5'000'000'000ULL),state,5'100'000'000ULL);assert(state.workload==DetectedWorkloadClass::Gaming&&state.stable&&state.confidence>=.65F);
 auto f1=gaming(6'000'000'000ULL);f1.source=TelemetryBackend::SysfsProcfs;auto a=classifier.classify(f1,state,6'100'000'000ULL);auto f2=gaming(6'000'000'000ULL);auto z=classifier.classify(f2,state,6'100'000'000ULL);assert(a.workload==z.workload&&a.confidence==z.confidence);
 auto stale=classifier.classify(gaming(1'000'000'000ULL),state,8'000'000'000ULL);assert(stale.workload==state.workload&&!stale.stable);auto future=classifier.classify(gaming(9'000'000'000ULL),state,8'000'000'000ULL);assert(future.workload==state.workload);auto once=classifier.classify(f2,state,6'100'000'000ULL);
 auto dup=classifier.classify(f2,once,6'200'000'000ULL);assert(dup.workload==once.workload&&!dup.stable);
 WorkloadFeatureSnapshot j{};j.window_start_ns=7'000'000'000ULL;j.window_end_ns=7'500'000'000ULL;j.frame_deadline_count=1000;j.frame_deadline_misses=2;j.frame_deadline_miss_pct=.20;j.cpu_pressure_some_pct=9;j.valid_features=FeatureFrameDeadline|FeatureCpuPressure;auto jr=classifier.classify(j,WorkloadClassification{},7'600'000'000ULL);assert(jr.pending_workload==DetectedWorkloadClass::JankSensitive);
 WorkloadFeatureSnapshot m{};m.window_start_ns=8'000'000'000ULL;m.window_end_ns=8'500'000'000ULL;m.memory_pressure_some_pct=15;m.valid_features=FeatureMemoryPressure;auto mr=classifier.classify(m,WorkloadClassification{},8'600'000'000ULL);assert(mr.pending_workload==DetectedWorkloadClass::MemoryPressure);

 // BPF loader must fail closed on missing prerequisites and remain safely stoppable.
 char no_btf[]="/tmp/coreflow-bpf-gate-XXXXXX";assert(::mkdtemp(no_btf));
 write(std::filesystem::path(no_btf)/"class/kgsl/kgsl-3d0/gpu_busy_percentage","64\n");
 EbpfWorkloadSource gated(no_btf);assert(!gated.start());assert(!gated.active());
 assert(select_telemetry_backend(gated.capabilities(),true)==TelemetryBackend::SysfsProcfs);
 gated.stop();gated.stop();
 WorkloadTelemetryPipeline pipeline(proc.string(),no_btf);assert(pipeline.start());
 assert(pipeline.active()&&pipeline.backend()==TelemetryBackend::SysfsProcfs);
 auto unified=pipeline.sample(3'000'000'000ULL,4'000'000'000ULL);
 assert(unified.source==TelemetryBackend::SysfsProcfs&&(unified.valid_features&FeatureCpuPressure));
 assert((unified.valid_features&FeatureGpuBusy)&&unified.gpu_busy_pct==64.0&&unified.gpu_busy_sample_count==1);
 write(std::filesystem::path(no_btf)/"class/kgsl/kgsl-3d0/gpu_busy_percentage","101\n");
 auto invalid_gpu=pipeline.sample(4'200'000'000ULL,4'300'000'000ULL);
 assert((invalid_gpu.valid_features&FeatureGpuBusy)==0);
 pipeline.stop();pipeline.stop();std::filesystem::remove_all(no_btf);
 // Decision thresholds are inclusive only where explicitly specified.
 WorkloadFeatureSnapshot below_jank{};below_jank.window_start_ns=9'000'000'000ULL;below_jank.window_end_ns=9'500'000'000ULL;
 below_jank.frame_deadline_count=1000;below_jank.frame_deadline_miss_pct=0.0799;below_jank.cpu_pressure_some_pct=4.99;
 below_jank.valid_features=FeatureFrameDeadline|FeatureCpuPressure;
 auto below=classifier.classify(below_jank,WorkloadClassification{},9'600'000'000ULL);assert(below.pending_workload==DetectedWorkloadClass::Unknown);
 WorkloadFeatureSnapshot at_jank=below_jank;at_jank.window_start_ns=10'000'000'000ULL;at_jank.window_end_ns=10'500'000'000ULL;
 at_jank.frame_deadline_miss_pct=0.08;at_jank.cpu_pressure_some_pct=5.0;
 auto at=classifier.classify(at_jank,WorkloadClassification{},10'600'000'000ULL);assert(at.pending_workload==DetectedWorkloadClass::JankSensitive);
 // Hysteresis: a new CPU-bound candidate cannot replace Gaming before dwell expires.
 WorkloadFeatureSnapshot cpu{};cpu.window_start_ns=6'000'000'000ULL;cpu.window_end_ns=6'500'000'000ULL;
 cpu.cpu_pressure_some_pct=20;cpu.valid_features=FeatureCpuPressure;cpu.source=TelemetryBackend::SysfsProcfs;
 auto sw1=classifier.classify(cpu,state,6'600'000'000ULL);assert(sw1.workload==DetectedWorkloadClass::Gaming&&sw1.pending_windows==1);
 cpu.window_start_ns=7'000'000'000ULL;cpu.window_end_ns=7'500'000'000ULL;
 auto sw2=classifier.classify(cpu,sw1,7'600'000'000ULL);assert(sw2.workload==DetectedWorkloadClass::Gaming&&sw2.pending_windows==2);
 cpu.window_start_ns=7'500'000'000ULL;cpu.window_end_ns=8'000'000'000ULL;
 auto sw3=classifier.classify(cpu,sw2,8'100'000'000ULL);assert(sw3.workload==DetectedWorkloadClass::CpuBound&&sw3.stable);
 // A transient competing candidate is discarded and cannot accumulate across an unknown window.
 WorkloadClassification reset{};cpu.window_start_ns=11'000'000'000ULL;cpu.window_end_ns=11'500'000'000ULL;
 reset=classifier.classify(cpu,reset,11'600'000'000ULL);assert(reset.pending_windows==1);
 WorkloadFeatureSnapshot neutral{};neutral.window_start_ns=12'000'000'000ULL;neutral.window_end_ns=12'500'000'000ULL;
 reset=classifier.classify(neutral,reset,12'600'000'000ULL);assert(reset.pending_workload==DetectedWorkloadClass::Unknown&&reset.pending_windows==0);
 cpu.window_start_ns=13'000'000'000ULL;cpu.window_end_ns=13'500'000'000ULL;
 reset=classifier.classify(cpu,reset,13'600'000'000ULL);assert(reset.pending_windows==1);
 // Exact 3-second freshness boundary is accepted; +1 ns is rejected.
 auto exact=gaming(15'000'000'000ULL);WorkloadClassification empty{};
 auto exact_ok=classifier.classify(exact,empty,18'000'000'000ULL);assert(exact_ok.pending_windows==1);
 auto too_old=classifier.classify(gaming(14'999'999'999ULL),empty,18'000'000'001ULL);
 assert(too_old.workload==DetectedWorkloadClass::Unknown&&too_old.pending_windows==0);

 // I/O-heavy and GPU-bound branches require their independent validity evidence.
 WorkloadFeatureSnapshot io{};io.window_start_ns=19'000'000'000ULL;io.window_end_ns=19'500'000'000ULL;
 io.io_bytes=4096;io.io_pressure_some_pct=9;io.valid_features=FeatureIoBytes|FeatureIoPressure;
 auto io_result=classifier.classify(io,WorkloadClassification{},19'600'000'000ULL);
 assert(io_result.pending_workload==DetectedWorkloadClass::IoHeavy);
 WorkloadFeatureSnapshot gpu{};gpu.window_start_ns=20'000'000'000ULL;gpu.window_end_ns=20'500'000'000ULL;
 gpu.gpu_busy_pct=80;gpu.cpu_pressure_some_pct=4;gpu.valid_features=FeatureGpuBusy|FeatureCpuPressure;
 auto gpu_result=classifier.classify(gpu,WorkloadClassification{},20'600'000'000ULL);
 assert(gpu_result.pending_workload==DetectedWorkloadClass::GpuBound);
 std::filesystem::remove_all(root);std::cout<<"P2-C sampler and deterministic classifier PASS\n";return 0;
}
