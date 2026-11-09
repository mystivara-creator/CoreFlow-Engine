#include "coreflow/workload_intelligence.hpp"
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <utility>
namespace coreflow { namespace {
bool readable_dir(const std::string& p) noexcept { struct stat s{}; return ::stat(p.c_str(),&s)==0&&S_ISDIR(s.st_mode)&&::access(p.c_str(),R_OK|X_OK)==0; }
bool bpffs(const std::string& p) noexcept { struct statfs s{}; return ::statfs(p.c_str(),&s)==0&&static_cast<unsigned long>(s.f_type)==0xcafe4a11UL; }
CapabilityState file_state(const std::string& p) noexcept { struct stat s{}; if(::stat(p.c_str(),&s)!=0||!S_ISREG(s.st_mode))return CapabilityState::Unavailable; return ::access(p.c_str(),R_OK)==0?CapabilityState::Available:CapabilityState::Restricted; }
bool line(const std::string& p,char* b,size_t n) noexcept { if(!n)return false; FILE* f=std::fopen(p.c_str(),"r"); if(!f)return false; bool ok=std::fgets(b,static_cast<int>(n),f)!=nullptr; std::fclose(f); return ok; }
bool psi(const std::string& p,double& out) noexcept { char b[512]{}; if(!line(p,b,sizeof(b)))return false; char* s=std::strstr(b,"some "); if(!s)return false; char* v=std::strstr(s,"avg10="); if(!v)return false; v+=6; char* e=nullptr; double d=std::strtod(v,&e); if(e==v||d<0.0||d>100.0)return false; out=d; return true; }
bool disk_sectors(const std::string& p,uint64_t& out) noexcept {
 FILE* f=std::fopen(p.c_str(),"r"); if(!f)return false; char b[512]{}; uint64_t total=0; bool any=false;
 while(std::fgets(b,sizeof(b),f)){ unsigned ma=0,mi=0; char name[128]{}; unsigned long long a=0,c=0,rd=0,d=0,w=0,g=0,wr=0; int n=std::sscanf(b,"%u %u %127s %llu %llu %llu %llu %llu %llu %llu",&ma,&mi,name,&a,&c,&rd,&d,&w,&g,&wr); if(n<10||!std::strncmp(name,"loop",4)||!std::strncmp(name,"ram",3))continue; uint64_t x=static_cast<uint64_t>(rd),y=static_cast<uint64_t>(wr); if(x>UINT64_MAX-total||y>UINT64_MAX-total-x)continue; total+=x+y; any=true; }
 std::fclose(f); if(!any)return false; out=total; return true;
}
bool window_ok(const WorkloadFeatureSnapshot& f) noexcept{return f.window_start_ns!=0&&f.window_end_ns>f.window_start_ns;}
struct Candidate{WorkloadClass type{WorkloadClass::Unknown};float confidence{0};uint32_t mask{FeatureNone};};
Candidate decide(const WorkloadFeatureSnapshot& f) noexcept {
 auto has=[&](uint32_t bit){return (f.valid_features&bit)!=0;};
 if(has(FeatureFrameDeadline)&&f.frame_deadline_count>0&&f.frame_deadline_miss_pct>=0.08&&
   ((has(FeatureCpuPressure)&&f.cpu_pressure_some_pct>=5)||(has(FeatureIoPressure)&&f.io_pressure_some_pct>=3)||(has(FeatureGpuBusy)&&f.gpu_busy_pct>=65))){
  uint32_t m=FeatureFrameDeadline; if(has(FeatureCpuPressure)&&f.cpu_pressure_some_pct>=5)m|=FeatureCpuPressure; if(has(FeatureIoPressure)&&f.io_pressure_some_pct>=3)m|=FeatureIoPressure; if(has(FeatureGpuBusy)&&f.gpu_busy_pct>=65)m|=FeatureGpuBusy; return {WorkloadClass::JankSensitive,0.82F,m};}
 if(has(FeatureMemoryPressure)&&f.memory_pressure_some_pct>=10)return {WorkloadClass::MemoryPressure,0.86F,FeatureMemoryPressure};
 if(has(FeatureIoBytes)&&has(FeatureIoPressure)&&f.io_pressure_some_pct>=8&&f.io_bytes>0)return {WorkloadClass::IoHeavy,0.80F,FeatureIoBytes|FeatureIoPressure};
 if(has(FeatureGpuBusy)&&has(FeatureCpuPressure)&&f.gpu_busy_pct>=75&&f.cpu_pressure_some_pct<12)return {WorkloadClass::GpuBound,0.76F,FeatureGpuBusy|FeatureCpuPressure};
 if(has(FeatureCpuPressure)&&f.cpu_pressure_some_pct>=15&&(!has(FeatureCpuRunQueue)||f.cpu_run_queue_wait_ns>0))return {WorkloadClass::CpuBound,0.74F,FeatureCpuPressure|(has(FeatureCpuRunQueue)?FeatureCpuRunQueue:FeatureNone)};
 if(has(FeatureGpuBusy)&&has(FeatureFrameDeadline)&&f.gpu_busy_pct>=55&&f.frame_deadline_miss_pct<0.08)return {WorkloadClass::Gaming,0.70F,FeatureGpuBusy|FeatureFrameDeadline};
 return {};
}
} // namespace
BpfCapabilitySnapshot BpfCapabilityProbe::probe() const noexcept {
 BpfCapabilitySnapshot r{}; const std::string a=sys_root_+"/kernel/tracing",b=sys_root_+"/kernel/debug/tracing"; std::string root;
 if(readable_dir(a+"/events"))root=a;else if(readable_dir(b+"/events"))root=b;
 r.btf=file_state(sys_root_+"/kernel/btf/vmlinux"); r.tracefs=root.empty()?CapabilityState::Unavailable:CapabilityState::Available;
 if(!root.empty()){r.sched_switch=file_state(root+"/events/sched/sched_switch/id");r.sched_wakeup=file_state(root+"/events/sched/sched_wakeup/id");
 auto i=file_state(root+"/events/block/block_rq_issue/id"),c=file_state(root+"/events/block/block_rq_complete/id");
 r.block_request=(i==CapabilityState::Available&&c==CapabilityState::Available)?CapabilityState::Available:((i==CapabilityState::Restricted||c==CapabilityState::Restricted)?CapabilityState::Restricted:CapabilityState::Unavailable);
 }else{r.sched_switch=CapabilityState::Unavailable;r.sched_wakeup=CapabilityState::Unavailable;r.block_request=CapabilityState::Unavailable;}
 r.bpf_filesystem=bpffs(sys_root_+"/fs/bpf")?CapabilityState::Available:CapabilityState::Unavailable;
 // No BPF syscall/map/program operation is attempted here: filesystem visibility is not load permission.
 return r;
}
SysfsProcfsWorkloadSource::SysfsProcfsWorkloadSource(std::string p,std::string s) noexcept:proc_root_(std::move(p)),sys_root_(std::move(s)){}
bool SysfsProcfsWorkloadSource::start() noexcept {double v=0;uint64_t sectors=0;started_=psi(proc_root_+"/pressure/cpu",v)||psi(proc_root_+"/pressure/memory",v)||psi(proc_root_+"/pressure/io",v)||disk_sectors(proc_root_+"/diskstats",sectors);return started_;}
void SysfsProcfsWorkloadSource::stop() noexcept {started_=false;previous_io_sectors_=0;previous_sample_ns_=0;}
size_t SysfsProcfsWorkloadSource::drain(std::span<WorkloadEvent> out) noexcept {(void)out;return 0;}
WorkloadFeatureSnapshot SysfsProcfsWorkloadSource::sample(uint64_t start,uint64_t end) noexcept {
 WorkloadFeatureSnapshot r{};r.window_start_ns=start;r.window_end_ns=end;r.source=TelemetryBackend::SysfsProcfs;if(!started_||!window_ok(r))return r;
 double v=0;if(psi(proc_root_+"/pressure/cpu",v)){r.cpu_pressure_some_pct=v;r.valid_features|=FeatureCpuPressure;}
 if(psi(proc_root_+"/pressure/memory",v)){r.memory_pressure_some_pct=v;r.valid_features|=FeatureMemoryPressure;}
 if(psi(proc_root_+"/pressure/io",v)){r.io_pressure_some_pct=v;r.valid_features|=FeatureIoPressure;}
 uint64_t sectors=0;if(disk_sectors(proc_root_+"/diskstats",sectors)){if(previous_sample_ns_&&end>previous_sample_ns_&&sectors>=previous_io_sectors_){uint64_t delta=sectors-previous_io_sectors_;if(delta<=UINT64_MAX/512){r.io_bytes=delta*512;r.valid_features|=FeatureIoBytes;}}previous_io_sectors_=sectors;previous_sample_ns_=end;}
 r.sample_count=1;return r;
}
WorkloadClassification WorkloadClassifier::classify(const WorkloadFeatureSnapshot& f,const WorkloadClassification& prev,uint64_t now) const noexcept {
 WorkloadClassification r=prev;
 if(!window_ok(f)||f.window_end_ns>now||now-f.window_end_ns>3'000'000'000ULL||(prev.classified_at_ns&&f.window_end_ns<=prev.classified_at_ns)){r.stable=false;return r;}
 Candidate c=decide(f);r.confidence=c.confidence;r.evidence_mask=c.mask;r.classified_at_ns=f.window_end_ns;
 if(c.type==WorkloadClass::Unknown||c.confidence<0.65F){r.pending_workload=WorkloadClass::Unknown;r.pending_windows=0;r.consecutive_windows=0;r.stable=false;return r;}
 if(c.type==prev.workload){r.pending_workload=WorkloadClass::Unknown;r.pending_windows=0;r.consecutive_windows=prev.consecutive_windows+1;r.stable=true;return r;}
 r.pending_windows=c.type==prev.pending_workload?prev.pending_windows+1:1;r.pending_workload=c.type;r.consecutive_windows=r.pending_windows;
 bool initial=prev.workload==WorkloadClass::Unknown;bool dwell=initial||prev.last_transition_ns==0||(now>=prev.last_transition_ns&&now-prev.last_transition_ns>=3'000'000'000ULL);
 if(r.pending_windows>=2&&dwell){r.workload=c.type;r.last_transition_ns=now;r.pending_workload=WorkloadClass::Unknown;r.pending_windows=0;r.consecutive_windows=2;r.stable=true;}else r.stable=prev.workload!=WorkloadClass::Unknown;
 return r;
}
} // namespace coreflow
