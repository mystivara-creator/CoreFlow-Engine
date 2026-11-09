#include "coreflow/workload_intelligence.hpp"
#include <linux/bpf.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <utility>
namespace coreflow {
static_assert(sizeof(WorkloadEvent)==40);
namespace {
constexpr uint32_t kBusy=1U<<31U,kDiscard=1U<<30U,kLengthMask=~(kBusy|kDiscard);
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
struct Candidate{DetectedWorkloadClass type{DetectedWorkloadClass::Unknown};float confidence{0};uint32_t mask{FeatureNone};};
Candidate decide(const WorkloadFeatureSnapshot& f) noexcept {
 auto has=[&](uint32_t bit){return (f.valid_features&bit)!=0;};
 if(has(FeatureFrameDeadline)&&f.frame_deadline_count>0&&f.frame_deadline_miss_pct>=0.08&&
   ((has(FeatureCpuPressure)&&f.cpu_pressure_some_pct>=5)||(has(FeatureIoPressure)&&f.io_pressure_some_pct>=3)||(has(FeatureGpuBusy)&&f.gpu_busy_pct>=65))){
  uint32_t m=FeatureFrameDeadline; if(has(FeatureCpuPressure)&&f.cpu_pressure_some_pct>=5)m|=FeatureCpuPressure; if(has(FeatureIoPressure)&&f.io_pressure_some_pct>=3)m|=FeatureIoPressure; if(has(FeatureGpuBusy)&&f.gpu_busy_pct>=65)m|=FeatureGpuBusy; return {DetectedWorkloadClass::JankSensitive,0.82F,m};}
 if(has(FeatureMemoryPressure)&&f.memory_pressure_some_pct>=10)return {DetectedWorkloadClass::MemoryPressure,0.86F,FeatureMemoryPressure};
 if(has(FeatureIoBytes)&&has(FeatureIoPressure)&&f.io_pressure_some_pct>=8&&f.io_bytes>0)return {DetectedWorkloadClass::IoHeavy,0.80F,FeatureIoBytes|FeatureIoPressure};
 if(has(FeatureGpuBusy)&&has(FeatureCpuPressure)&&f.gpu_busy_pct>=75&&f.cpu_pressure_some_pct<12)return {DetectedWorkloadClass::GpuBound,0.76F,FeatureGpuBusy|FeatureCpuPressure};
 if(has(FeatureCpuPressure)&&f.cpu_pressure_some_pct>=15&&(!has(FeatureCpuRunQueue)||f.cpu_run_queue_wait_ns>0))return {DetectedWorkloadClass::CpuBound,0.74F,FeatureCpuPressure|(has(FeatureCpuRunQueue)?FeatureCpuRunQueue:FeatureNone)};
 if(has(FeatureGpuBusy)&&has(FeatureFrameDeadline)&&f.gpu_busy_pct>=55&&f.frame_deadline_miss_pct<0.08)return {DetectedWorkloadClass::Gaming,0.70F,FeatureGpuBusy|FeatureFrameDeadline};
 return {};
}
int bpf_call(enum bpf_cmd cmd,union bpf_attr& attr) noexcept {
#ifdef __NR_bpf
 return static_cast<int>(::syscall(__NR_bpf,cmd,&attr,sizeof(attr)));
#else
 (void)cmd;(void)attr;errno=ENOSYS;return -1;
#endif
}
int create_ring_map(uint32_t bytes) noexcept {
 union bpf_attr a{};a.map_type=BPF_MAP_TYPE_RINGBUF;a.max_entries=bytes;
 return bpf_call(BPF_MAP_CREATE,a);
}
constexpr bpf_insn make_insn(uint8_t code,uint8_t dst,uint8_t src,int16_t off,int32_t imm) noexcept {
 return bpf_insn{code,static_cast<__u8>(dst),static_cast<__u8>(src),off,imm};
}
int load_raw_tracepoint_program(int map_fd,WorkloadEventType type) noexcept {
 const std::array<bpf_insn,21> insns{{
  make_insn(BPF_ALU64|BPF_MOV|BPF_X,BPF_REG_1,BPF_REG_10,0,0),
  make_insn(BPF_ALU64|BPF_ADD|BPF_K,BPF_REG_1,0,0,-40),
  make_insn(BPF_JMP|BPF_CALL,0,0,0,BPF_FUNC_ktime_get_ns),
  make_insn(BPF_STX|BPF_MEM|BPF_DW,BPF_REG_10,BPF_REG_0,-40,0),
  make_insn(BPF_JMP|BPF_CALL,0,0,0,BPF_FUNC_get_current_pid_tgid),
  make_insn(BPF_STX|BPF_MEM|BPF_DW,BPF_REG_10,BPF_REG_0,-32,0),
  make_insn(BPF_ST|BPF_MEM|BPF_DW,BPF_REG_10,0,-24,0),
  make_insn(BPF_ST|BPF_MEM|BPF_DW,BPF_REG_10,0,-16,0),
  make_insn(BPF_JMP|BPF_CALL,0,0,0,BPF_FUNC_get_smp_processor_id),
  make_insn(BPF_STX|BPF_MEM|BPF_W,BPF_REG_10,BPF_REG_0,-8,0),
  make_insn(BPF_ST|BPF_MEM|BPF_H,BPF_REG_10,0,-4,static_cast<int32_t>(type)),
  make_insn(BPF_ST|BPF_MEM|BPF_H,BPF_REG_10,0,-2,0),
  make_insn(BPF_LD|BPF_DW|BPF_IMM,BPF_REG_1,BPF_PSEUDO_MAP_FD,0,map_fd),
  make_insn(0,0,0,0,0),
  make_insn(BPF_ALU64|BPF_MOV|BPF_X,BPF_REG_2,BPF_REG_10,0,0),
  make_insn(BPF_ALU64|BPF_ADD|BPF_K,BPF_REG_2,0,0,-40),
  make_insn(BPF_ALU64|BPF_MOV|BPF_K,BPF_REG_3,0,0,static_cast<int32_t>(sizeof(WorkloadEvent))),
  make_insn(BPF_ALU64|BPF_MOV|BPF_K,BPF_REG_4,0,0,0),
  make_insn(BPF_JMP|BPF_CALL,0,0,0,BPF_FUNC_ringbuf_output),
  make_insn(BPF_ALU64|BPF_MOV|BPF_K,BPF_REG_0,0,0,0),
  make_insn(BPF_JMP|BPF_EXIT,0,0,0,0)
 }};
 static const char license[]="GPL";
 char verifier_log[65536]{};
 union bpf_attr a{};a.prog_type=BPF_PROG_TYPE_RAW_TRACEPOINT;
 a.insn_cnt=static_cast<__u32>(insns.size());
 a.insns=reinterpret_cast<__u64>(insns.data());a.license=reinterpret_cast<__u64>(license);
 a.log_buf=reinterpret_cast<__u64>(verifier_log);a.log_size=sizeof(verifier_log);a.log_level=1;
 return bpf_call(BPF_PROG_LOAD,a);
}
int attach_raw_tracepoint(int program_fd,const char* name) noexcept {
 union bpf_attr a{};a.raw_tracepoint.name=reinterpret_cast<__u64>(name);a.raw_tracepoint.prog_fd=static_cast<__u32>(program_fd);
 return bpf_call(BPF_RAW_TRACEPOINT_OPEN,a);
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
 return r;
}
EbpfWorkloadSource::EbpfWorkloadSource(std::string root,uint32_t bytes) noexcept:sys_root_(std::move(root)),ring_bytes_(bytes){}
EbpfWorkloadSource::~EbpfWorkloadSource() noexcept {stop();}
void EbpfWorkloadSource::release_resources() noexcept {
 active_=false;capabilities_.loader_verified=false;
 if(wakeup_link_fd_>=0){::close(wakeup_link_fd_);wakeup_link_fd_=-1;}
 if(switch_link_fd_>=0){::close(switch_link_fd_);switch_link_fd_=-1;}
 if(wakeup_program_fd_>=0){::close(wakeup_program_fd_);wakeup_program_fd_=-1;}
 if(program_fd_>=0){::close(program_fd_);program_fd_=-1;}
 if(consumer_mapping_){::munmap(consumer_mapping_,page_size_);consumer_mapping_=nullptr;}
 if(producer_mapping_){::munmap(producer_mapping_,page_size_+static_cast<size_t>(ring_bytes_)*2U);producer_mapping_=nullptr;}
 if(map_fd_>=0){::close(map_fd_);map_fd_=-1;}
 ring_mask_=0;
}
bool EbpfWorkloadSource::start() noexcept {
 stop();capabilities_=BpfCapabilityProbe(sys_root_).probe();
 if(ring_bytes_<4096U||ring_bytes_>(4U<<20U)||(ring_bytes_&(ring_bytes_-1U))!=0U||
    capabilities_.btf!=CapabilityState::Available||capabilities_.tracefs!=CapabilityState::Available||
    capabilities_.sched_switch!=CapabilityState::Available||capabilities_.sched_wakeup!=CapabilityState::Available)return false;
 const long page_size=::sysconf(_SC_PAGESIZE);
 if(page_size<=0)return false;
 page_size_=static_cast<size_t>(page_size);
 if(ring_bytes_<page_size_)return false;
 map_fd_=create_ring_map(ring_bytes_);
 if(map_fd_<0){
  if(errno==EPERM||errno==EACCES)capabilities_.syscall=CapabilityState::Restricted;
  else if(errno==ENOSYS)capabilities_.syscall=CapabilityState::Unavailable;
  else if(errno==EINVAL){capabilities_.syscall=CapabilityState::Available;capabilities_.ring_buffer=CapabilityState::Unavailable;}
  else capabilities_.syscall=CapabilityState::Unknown;
  return false;
 }
 capabilities_.syscall=CapabilityState::Available;capabilities_.ring_buffer=CapabilityState::Available;
 program_fd_=load_raw_tracepoint_program(map_fd_,WorkloadEventType::SchedSwitch);
 if(program_fd_<0){capabilities_.selinux_allows_load=false;release_resources();return false;}
 capabilities_.selinux_allows_load=true;
 wakeup_program_fd_=load_raw_tracepoint_program(map_fd_,WorkloadEventType::SchedWakeup);
 if(wakeup_program_fd_<0){release_resources();return false;}
 switch_link_fd_=attach_raw_tracepoint(program_fd_,"sched_switch");
 if(switch_link_fd_<0){release_resources();return false;}
 wakeup_link_fd_=attach_raw_tracepoint(wakeup_program_fd_,"sched_wakeup");
 if(wakeup_link_fd_<0){release_resources();return false;}
 consumer_mapping_=::mmap(nullptr,page_size_,PROT_READ|PROT_WRITE,MAP_SHARED,map_fd_,0);
 if(consumer_mapping_==MAP_FAILED){consumer_mapping_=nullptr;release_resources();return false;}
 producer_mapping_=::mmap(nullptr,page_size_+static_cast<size_t>(ring_bytes_)*2U,PROT_READ,MAP_SHARED,map_fd_,static_cast<off_t>(page_size_));
 if(producer_mapping_==MAP_FAILED){producer_mapping_=nullptr;release_resources();return false;}
 ring_mask_=static_cast<uint64_t>(ring_bytes_-1U);capabilities_.loader_verified=true;
 if(!capabilities_.event_pipeline_ready()){release_resources();return false;}
 active_=true;return true;
}
void EbpfWorkloadSource::stop() noexcept {release_resources();}
size_t EbpfWorkloadSource::drain(std::span<WorkloadEvent> out) noexcept {
 if(!active_||!consumer_mapping_||!producer_mapping_||out.empty())return 0;
 auto* consumer=reinterpret_cast<uint64_t*>(consumer_mapping_);
 auto* producer=reinterpret_cast<uint64_t*>(producer_mapping_);
 auto* data=static_cast<unsigned char*>(producer_mapping_)+page_size_;
 uint64_t read_pos=__atomic_load_n(consumer,__ATOMIC_RELAXED);
 const uint64_t write_pos=__atomic_load_n(producer,__ATOMIC_ACQUIRE);
 size_t count=0;
 while(read_pos<write_pos&&count<out.size()){
  auto* header=reinterpret_cast<uint32_t*>(data+(read_pos&ring_mask_));
  const uint32_t raw_len=__atomic_load_n(header,__ATOMIC_ACQUIRE);
  if(raw_len&kBusy)break;
  const uint32_t len=raw_len&kLengthMask;
  const uint64_t record_size=(static_cast<uint64_t>(len)+8U+7U)&~7ULL;
  if(record_size>ring_bytes_||record_size<8U)break;
  if(!(raw_len&kDiscard)&&len==sizeof(WorkloadEvent)){
   WorkloadEvent event{};std::memcpy(&event,reinterpret_cast<unsigned char*>(header)+8U,sizeof(event));
   const uint64_t pid_tgid=event.value0;event.pid=static_cast<uint32_t>(pid_tgid>>32U);event.tid=static_cast<uint32_t>(pid_tgid);
   out[count++]=event;
  }
  read_pos+=record_size;__atomic_store_n(consumer,read_pos,__ATOMIC_RELEASE);
 }
 return count;
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
WorkloadTelemetryPipeline::WorkloadTelemetryPipeline(std::string proc,std::string sys) noexcept
 :ebpf_(sys),fallback_(std::move(proc),std::move(sys)){}
bool WorkloadTelemetryPipeline::start() noexcept {
 stop();fallback_active_=fallback_.start();
 const bool bpf_active=ebpf_.start();
 active_=fallback_active_||bpf_active;
 return active_;
}
void WorkloadTelemetryPipeline::stop() noexcept {
 ebpf_.stop();fallback_.stop();active_=false;fallback_active_=false;
}
WorkloadFeatureSnapshot WorkloadTelemetryPipeline::sample(uint64_t start,uint64_t end) noexcept {
 WorkloadFeatureSnapshot result{};
 result.window_start_ns=start;result.window_end_ns=end;
 if(!active_||end<=start)return result;
 if(fallback_active_)result=fallback_.sample(start,end);
 result.window_start_ns=start;result.window_end_ns=end;
 if(ebpf_.active()){
  result.source=TelemetryBackend::Ebpf;
  WorkloadEvent events[512]{};const size_t count=ebpf_.drain(events);
  uint64_t wakeups=0;
  for(size_t i=0;i<count;++i){
   if(events[i].monotonic_timestamp_ns<start||events[i].monotonic_timestamp_ns>end)continue;
   if(events[i].type==WorkloadEventType::SchedWakeup)++wakeups;
  }
  result.wakeup_count=wakeups;
  result.wakeup_rate_per_sec=static_cast<double>(wakeups)*1'000'000'000.0/static_cast<double>(end-start);
  result.valid_features|=FeatureWakeupRate;
  result.event_count=static_cast<uint32_t>(count);
  result.event_drain_saturated=count==events.size();
  result.sample_count+=static_cast<uint32_t>(count);
 }else if(fallback_active_){
  result.source=TelemetryBackend::SysfsProcfs;
 }
 return result;
}
WorkloadClassification WorkloadClassifier::classify(const WorkloadFeatureSnapshot& f,const WorkloadClassification& prev,uint64_t now) const noexcept {
 WorkloadClassification r=prev;
 if(!window_ok(f)||f.window_end_ns>now||now-f.window_end_ns>3'000'000'000ULL||(prev.classified_at_ns&&f.window_end_ns<=prev.classified_at_ns)){r.stable=false;return r;}
 Candidate c=decide(f);r.confidence=c.confidence;r.evidence_mask=c.mask;r.classified_at_ns=f.window_end_ns;
 if(c.type==DetectedWorkloadClass::Unknown||c.confidence<0.65F){r.pending_workload=DetectedWorkloadClass::Unknown;r.pending_windows=0;r.consecutive_windows=0;r.stable=false;return r;}
 if(c.type==prev.workload){r.pending_workload=DetectedWorkloadClass::Unknown;r.pending_windows=0;r.consecutive_windows=prev.consecutive_windows+1;r.stable=true;return r;}
 r.pending_windows=c.type==prev.pending_workload?prev.pending_windows+1:1;r.pending_workload=c.type;r.consecutive_windows=r.pending_windows;
 bool initial=prev.workload==DetectedWorkloadClass::Unknown;bool dwell=initial||prev.last_transition_ns==0||(now>=prev.last_transition_ns&&now-prev.last_transition_ns>=3'000'000'000ULL);
 if(r.pending_windows>=2&&dwell){r.workload=c.type;r.last_transition_ns=now;r.pending_workload=DetectedWorkloadClass::Unknown;r.pending_windows=0;r.consecutive_windows=2;r.stable=true;}else r.stable=prev.workload!=DetectedWorkloadClass::Unknown;
 return r;
}
} // namespace coreflow
