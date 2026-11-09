#include "coreflow/platform_context.hpp"
#include "coreflow/workload_intelligence.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unistd.h>
using namespace coreflow;
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
 WorkloadClassifier classifier;WorkloadClassification state{};state=classifier.classify(gaming(4'000'000'000ULL),state,4'100'000'000ULL);assert(state.workload==WorkloadClass::Unknown&&state.pending_windows==1);state=classifier.classify(gaming(5'000'000'000ULL),state,5'100'000'000ULL);assert(state.workload==WorkloadClass::Gaming&&state.stable&&state.confidence>=.65F);
 auto f1=gaming(6'000'000'000ULL);f1.source=TelemetryBackend::SysfsProcfs;auto a=classifier.classify(f1,state,6'100'000'000ULL);auto f2=gaming(6'000'000'000ULL);auto z=classifier.classify(f2,state,6'100'000'000ULL);assert(a.workload==z.workload&&a.confidence==z.confidence);
 auto stale=classifier.classify(gaming(1'000'000'000ULL),state,8'000'000'000ULL);assert(stale.workload==state.workload&&!stale.stable);auto future=classifier.classify(gaming(9'000'000'000ULL),state,8'000'000'000ULL);assert(future.workload==state.workload);auto dup=classifier.classify(f2,state,6'200'000'000ULL);assert(dup.workload==state.workload);
 WorkloadFeatureSnapshot j{};j.window_start_ns=7'000'000'000ULL;j.window_end_ns=7'500'000'000ULL;j.frame_deadline_count=1000;j.frame_deadline_misses=2;j.frame_deadline_miss_pct=.20;j.cpu_pressure_some_pct=9;j.valid_features=FeatureFrameDeadline|FeatureCpuPressure;auto jr=classifier.classify(j,WorkloadClassification{},7'600'000'000ULL);assert(jr.pending_workload==WorkloadClass::JankSensitive);
 WorkloadFeatureSnapshot m{};m.window_start_ns=8'000'000'000ULL;m.window_end_ns=8'500'000'000ULL;m.memory_pressure_some_pct=15;m.valid_features=FeatureMemoryPressure;auto mr=classifier.classify(m,WorkloadClassification{},8'600'000'000ULL);assert(mr.pending_workload==WorkloadClass::MemoryPressure);
 std::filesystem::remove_all(root);std::cout<<"P2-C sampler and deterministic classifier PASS\n";return 0;
}
