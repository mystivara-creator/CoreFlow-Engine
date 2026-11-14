#include "coreflow/workload_intelligence.hpp"
#include <linux/bpf.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/vfs.h>
#include <dirent.h>
#include <fcntl.h>
#include <spawn.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cctype>
#include <thread>
#include <vector>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <utility>
extern char **environ;

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

bool read_text(const std::string& path, char* buffer, size_t capacity) noexcept {
    if (capacity == 0) {
        return false;
    }
    FILE* file = std::fopen(path.c_str(), "r");
    if (file == nullptr) {
        return false;
    }
    const bool ok = std::fgets(buffer, static_cast<int>(capacity), file) != nullptr;
    std::fclose(file);
    return ok;
}

bool parse_scalar_percent(const std::string& path, double& value) noexcept {
    char text[128]{};
    if (!read_text(path, text, sizeof(text))) {
        return false;
    }
    char* end = nullptr;
    const double parsed = std::strtod(text, &end);
    if (end == text || !std::isfinite(parsed) || parsed < 0.0 || parsed > 100.0) {
        return false;
    }
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') {
        ++end;
    }
    if (*end == '%') {
        ++end;
        while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') {
            ++end;
        }
    }
    if (*end != '\0') {
        return false;
    }
    value = parsed;
    return true;
}

bool parse_u64_file(const std::string& path, uint64_t& value) noexcept {
    char text[96]{};
    if (!read_text(path, text, sizeof(text))) {
        return false;
    }
    char* end = nullptr;
    errno = 0;
    const unsigned long long parsed = std::strtoull(text, &end, 10);
    if (errno != 0 || end == text) {
        return false;
    }
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') {
        ++end;
    }
    if (*end != '\0') {
        return false;
    }
    value = static_cast<uint64_t>(parsed);
    return true;
}

bool parse_pair(const std::string& path, uint64_t& first, uint64_t& second) noexcept {
    char text[160]{};
    if (!read_text(path, text, sizeof(text))) {
        return false;
    }
    char* cursor = text;
    while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n') {
        ++cursor;
    }
    // strtoull accepts optional signs; sysfs sentinels such as "-1" must
    // never be reinterpreted as huge unsigned utilization counters.
    if (*cursor < '0' || *cursor > '9') {
        return false;
    }
    char* end = nullptr;
    errno = 0;
    const unsigned long long a = std::strtoull(cursor, &end, 10);
    if (errno != 0 || end == cursor) {
        return false;
    }
    while (*end == ' ' || *end == '\t') {
        ++end;
    }
    if (*end < '0' || *end > '9') {
        return false;
    }
    errno = 0;
    char* second_end = nullptr;
    const unsigned long long b = std::strtoull(end, &second_end, 10);
    if (errno != 0 || second_end == end) {
        return false;
    }
    while (*second_end == ' ' || *second_end == '\t' ||
           *second_end == '\r' || *second_end == '\n') {
        ++second_end;
    }
    if (*second_end != '\0') {
        return false;
    }
    first = static_cast<uint64_t>(a);
    second = static_cast<uint64_t>(b);
    return true;
}

bool contains_gpu_token(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value.find("gpu") != std::string::npos ||
           value.find("mali") != std::string::npos ||
           value.find("kgsl") != std::string::npos ||
           value.find("3d") != std::string::npos;
}

bool read_name_matches_gpu(const std::string& path) noexcept {
    char text[128]{};
    return read_text(path, text, sizeof(text)) && contains_gpu_token(text);
}

bool read_varint(std::span<const uint8_t> bytes, size_t& offset, uint64_t& value) noexcept {
    value = 0;
    unsigned shift = 0;
    while (offset < bytes.size() && shift < 70) {
        const uint8_t byte = bytes[offset++];
        if (shift == 63 && (byte & 0xfeU) != 0) {
            return false;
        }
        value |= static_cast<uint64_t>(byte & 0x7fU) << shift;
        if ((byte & 0x80U) == 0) {
            return true;
        }
        shift += 7;
    }
    return false;
}

struct ProtoField final {
    uint32_t number{0};
    uint8_t wire{0};
    uint64_t integer{0};
    std::span<const uint8_t> data{};
};

bool next_proto_field(std::span<const uint8_t> bytes, size_t& offset, ProtoField& field) noexcept {
    if (offset >= bytes.size()) {
        return false;
    }
    uint64_t tag = 0;
    if (!read_varint(bytes, offset, tag) || tag == 0) {
        return false;
    }
    field = {};
    field.number = static_cast<uint32_t>(tag >> 3U);
    field.wire = static_cast<uint8_t>(tag & 7U);
    if (field.number == 0) {
        return false;
    }
    switch (field.wire) {
        case 0:
            return read_varint(bytes, offset, field.integer);
        case 1:
            if (bytes.size() - offset < 8) {
                return false;
            }
            offset += 8;
            return true;
        case 2: {
            uint64_t length = 0;
            if (!read_varint(bytes, offset, length) || length > bytes.size() - offset) {
                return false;
            }
            field.data = bytes.subspan(offset, static_cast<size_t>(length));
            offset += static_cast<size_t>(length);
            return true;
        }
        case 5:
            if (bytes.size() - offset < 4) {
                return false;
            }
            offset += 4;
            return true;
        default:
            return false;
    }
}

struct FrameTimelineParseState final {
    struct PendingDisplayFrame final {
        uint64_t cookie{0};
        uint64_t start_timestamp_ns{0};
        bool active{false};
    };
    uint64_t frames{0};
    uint64_t misses{0};
    uint64_t actual_duration_ns_total{0};
    uint64_t actual_duration_count{0};
    std::array<PendingDisplayFrame, 512> pending{};
};

bool parse_actual_display_frame(
    std::span<const uint8_t> bytes,
    uint64_t packet_timestamp_ns,
    FrameTimelineParseState& state) noexcept {
    size_t offset = 0;
    uint64_t cookie = 0;
    uint64_t present = 0;
    uint64_t jank = 0;
    bool has_cookie = false;
    bool has_present = false;
    bool has_on_time = false;
    bool on_time = true;
    ProtoField field{};
    while (offset < bytes.size()) {
        if (!next_proto_field(bytes, offset, field)) return false;
        if (field.wire != 0) continue;
        if (field.number == 1) {
            cookie = field.integer;
            has_cookie = true;
        } else if (field.number == 4) {
            present = field.integer;
            has_present = true;
        } else if (field.number == 5) {
            on_time = field.integer != 0;
            has_on_time = true;
        } else if (field.number == 7) {
            jank = field.integer;
        }
    }
    if (!has_present || present < 1U || present > 5U) return true;
    if (state.frames == UINT64_MAX) return false;
    ++state.frames;

    constexpr uint64_t kSfCpuDeadline = 16ULL;
    constexpr uint64_t kSfGpuDeadline = 32ULL;
    constexpr uint64_t kDropped = 1024ULL;
    if (present == 2U || present == 4U || (has_on_time && !on_time) ||
        (jank & (kSfCpuDeadline | kSfGpuDeadline | kDropped)) != 0U) {
        if (state.misses == UINT64_MAX) return false;
        ++state.misses;
    }

    if (has_cookie && cookie != 0U && packet_timestamp_ns != 0U) {
        FrameTimelineParseState::PendingDisplayFrame* slot = nullptr;
        for (auto& pending : state.pending) {
            if (pending.active && pending.cookie == cookie) {
                slot = &pending;
                break;
            }
            if (!pending.active && slot == nullptr) slot = &pending;
        }
        if (slot != nullptr) {
            slot->cookie = cookie;
            slot->start_timestamp_ns = packet_timestamp_ns;
            slot->active = true;
        }
    }
    return true;
}

bool parse_frame_end(
    std::span<const uint8_t> bytes,
    uint64_t packet_timestamp_ns,
    FrameTimelineParseState& state) noexcept {
    size_t offset = 0;
    uint64_t cookie = 0;
    bool has_cookie = false;
    ProtoField field{};
    while (offset < bytes.size()) {
        if (!next_proto_field(bytes, offset, field)) return false;
        if (field.number == 1U && field.wire == 0U) {
            cookie = field.integer;
            has_cookie = true;
        }
    }
    if (!has_cookie || packet_timestamp_ns == 0U) return true;
    for (auto& pending : state.pending) {
        if (!pending.active || pending.cookie != cookie) continue;
        const uint64_t duration = packet_timestamp_ns > pending.start_timestamp_ns
            ? packet_timestamp_ns - pending.start_timestamp_ns : 0U;
        pending.active = false;
        pending.cookie = 0U;
        pending.start_timestamp_ns = 0U;
        // Reject impossible/clock-corrupted samples instead of poisoning the mean.
        if (duration == 0U || duration > 5'000'000'000ULL) return true;
        if (state.actual_duration_ns_total > UINT64_MAX - duration ||
            state.actual_duration_count == UINT64_MAX) return false;
        state.actual_duration_ns_total += duration;
        ++state.actual_duration_count;
        break;
    }
    return true;
}

bool parse_frame_timeline_event(
    std::span<const uint8_t> bytes,
    uint64_t packet_timestamp_ns,
    FrameTimelineParseState& state) noexcept {
    size_t offset = 0;
    ProtoField field{};
    while (offset < bytes.size()) {
        if (!next_proto_field(bytes, offset, field)) return false;
        if (field.number == 2U && field.wire == 2U) {
            if (!parse_actual_display_frame(field.data, packet_timestamp_ns, state)) return false;
        } else if (field.number == 5U && field.wire == 2U) {
            if (!parse_frame_end(field.data, packet_timestamp_ns, state)) return false;
        }
    }
    return true;
}

bool parse_trace_packet(
    std::span<const uint8_t> bytes,
    FrameTimelineParseState& state) noexcept {
    size_t offset = 0;
    uint64_t packet_timestamp_ns = 0U;
    std::span<const uint8_t> frame_timeline_event{};
    ProtoField field{};
    while (offset < bytes.size()) {
        if (!next_proto_field(bytes, offset, field)) return false;
        if (field.number == 8U && field.wire == 0U) {
            packet_timestamp_ns = field.integer;
        } else if (field.number == 76U && field.wire == 2U) {
            frame_timeline_event = field.data;
        }
    }
    return frame_timeline_event.empty() ||
        parse_frame_timeline_event(frame_timeline_event, packet_timestamp_ns, state);
}
struct Candidate final {
    DetectedWorkloadClass classification{DetectedWorkloadClass::Unknown};
    float confidence{0.0F};
    uint32_t mask{FeatureNone};
};

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
GpuUtilizationSampler::GpuUtilizationSampler(std::string root) noexcept:sys_root_(std::move(root)){}
bool GpuUtilizationSampler::start() noexcept {
 stop();
 // KGSL nodes are resolved below the class discovery root. The device
 // instance name is never assumed; ABI filenames identify the counter type.
 struct CandidatePath { const char* suffix; Format format; const char* label; };
 static constexpr CandidatePath candidates[]{
  {"gpubusy",Format::KgslIntervalSnapshot,"kgsl/gpubusy"},
  {"gpu_busy_percent",Format::Percent,"kgsl/gpu_busy_percent"},
  {"devfreq/gpu_load",Format::Percent,"kgsl/devfreq/gpu_load"},
  {"gpu_busy_percentage",Format::Percent,"kgsl/gpu_busy_percentage"}
 };
 const std::string kgsl_root=sys_root_+"/class/kgsl";
 DIR* kgsl=::opendir(kgsl_root.c_str());
 if(kgsl){
  while(const dirent* entry=::readdir(kgsl)){
   if(entry->d_name[0]=='.')continue;
   const std::string device=kgsl_root+"/"+entry->d_name;
   for(const auto& candidate:candidates){
    const std::string path=device+"/"+candidate.suffix;struct stat st{};
    if(::stat(path.c_str(),&st)!=0||!S_ISREG(st.st_mode)||::access(path.c_str(),R_OK)!=0)continue;
    double percent=0;uint64_t busy=0,total=0;
    const bool valid=candidate.format==Format::Percent
      ? parse_scalar_percent(path,percent) : parse_pair(path,busy,total);
    if(!valid)continue;
    if(candidate.format==Format::KgslIntervalSnapshot &&
       ((total==0&&busy!=0)||(total!=0&&busy>total)))continue;
    path_=path;source_name_=candidate.label;format_=candidate.format;active_=true;
    break;
   }
   if(active_)break;
  }
  ::closedir(kgsl);
 }
 // Generic devfreq GPU domains are also enumerated dynamically and selected
 // by their driver/name ABI rather than a device-specific directory.
 if(!active_){
  const std::string directory=sys_root_+"/class/devfreq";DIR* dir=::opendir(directory.c_str());
  if(dir){
   while(const dirent* entry=::readdir(dir)){
    if(entry->d_name[0]=='.')continue;
    const std::string node=directory+"/"+entry->d_name;
    if(!read_name_matches_gpu(node+"/name"))continue;
    double percent=0;const std::string load=node+"/load";
    if(parse_scalar_percent(load,percent)){
     path_=load;source_name_="devfreq/named-device/load";format_=Format::Percent;active_=true;break;
    }
    const std::string busy_path=node+"/busy_time",total_path=node+"/total_time";
    uint64_t busy=0,total=0;
    if(parse_u64_file(busy_path,busy)&&parse_u64_file(total_path,total)&&total>0&&busy<=total){
     path_=busy_path;source_name_="devfreq/named-device/busy_time-total_time";
     format_=Format::CumulativeCounterDelta;active_=true;previous_busy_=busy;previous_total_=total;previous_valid_=true;break;
    }
   }
   ::closedir(dir);
  }
 }
 last_sample_ns_=0;return active_;
}
void GpuUtilizationSampler::stop() noexcept {
 path_.clear();source_name_.clear();format_=Format::Unavailable;
 previous_busy_=previous_total_=last_sample_ns_=0;previous_valid_=false;active_=false;
}
bool GpuUtilizationSampler::sample(uint64_t now,double& output) noexcept {
 if(!active_||now==0||(last_sample_ns_&&(now<=last_sample_ns_||now-last_sample_ns_<100'000'000ULL)))return false;
 last_sample_ns_=now;
 if(format_==Format::Percent)return parse_scalar_percent(path_,output);
 uint64_t busy=0,total=0;
 if(format_==Format::KgslIntervalSnapshot){
  if(!parse_pair(path_,busy,total))return false;
  if(previous_valid_&&busy==previous_busy_&&total==previous_total_)return false;
  if((total==0&&busy!=0)||(total>0&&busy>total))return false;
  previous_busy_=busy;previous_total_=total;previous_valid_=true;
  if(total==0){output=0.0;return true;}
  output=100.0*static_cast<double>(busy)/static_cast<double>(total);
  return std::isfinite(output)&&output>=0.0&&output<=100.0;
 }
 if(format_==Format::CumulativeCounterDelta){
  const std::string total_path=path_.substr(0,path_.rfind('/')+1)+"total_time";
  if(!parse_u64_file(path_,busy)||!parse_u64_file(total_path,total))return false;
  if(!previous_valid_){previous_busy_=busy;previous_total_=total;previous_valid_=true;return false;}
  if(busy<previous_busy_||total<previous_total_){previous_busy_=busy;previous_total_=total;return false;}
  const uint64_t busy_delta=busy-previous_busy_,total_delta=total-previous_total_;
  previous_busy_=busy;previous_total_=total;
  if(total_delta==0||busy_delta>total_delta)return false;
  output=100.0*static_cast<double>(busy_delta)/static_cast<double>(total_delta);return true;
 }
 return false;
}
std::string_view GpuUtilizationSampler::source_name() const noexcept {
 return active_?std::string_view(source_name_):std::string_view("unavailable");
}
FrameTimelineSampler::~FrameTimelineSampler() noexcept {stop();}
bool FrameTimelineSampler::start(uint32_t duration_ms) noexcept {
 stop();if(duration_ms<250||duration_ms>3000)return false;
 constexpr const char* binary="/system/bin/perfetto";
 constexpr const char* output_dir="/data/misc/perfetto-traces";
 if(::access(binary,X_OK)!=0||::access(output_dir,W_OK|X_OK)!=0)return false;
 const auto stamp=static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
  std::chrono::steady_clock::now().time_since_epoch()).count());
 trace_path_=std::string(output_dir)+"/coreflow-frame-"+std::to_string(::getpid())+"-"+
  std::to_string(stamp)+".pftrace";
 int input[2]{-1,-1};if(::pipe(input)!=0){trace_path_.clear();return false;}
 posix_spawn_file_actions_t actions;
 if(::posix_spawn_file_actions_init(&actions)!=0){::close(input[0]);::close(input[1]);trace_path_.clear();return false;}
 int action_status=::posix_spawn_file_actions_adddup2(&actions,input[0],STDIN_FILENO);
 if(action_status==0)action_status=::posix_spawn_file_actions_addclose(&actions,input[0]);
 if(action_status==0)action_status=::posix_spawn_file_actions_addclose(&actions,input[1]);
 if(action_status==0)action_status=::posix_spawn_file_actions_addopen(&actions,STDOUT_FILENO,"/dev/null",O_WRONLY,0);
 if(action_status==0)action_status=::posix_spawn_file_actions_addopen(&actions,STDERR_FILENO,"/dev/null",O_WRONLY,0);
 if(action_status!=0){::posix_spawn_file_actions_destroy(&actions);::close(input[0]);::close(input[1]);trace_path_.clear();return false;}
 const std::string output=trace_path_;
 char* const argv[]{const_cast<char*>(binary),const_cast<char*>("--txt"),const_cast<char*>("-c"),
  const_cast<char*>("-"),const_cast<char*>("-o"),const_cast<char*>(output.c_str()),nullptr};
 pid_t child=-1;const int spawn_status=::posix_spawn(&child,binary,&actions,nullptr,argv,environ);
 ::posix_spawn_file_actions_destroy(&actions);::close(input[0]);
 if(spawn_status!=0){::close(input[1]);trace_path_.clear();return false;}
 child_pid_=static_cast<int>(child);
 const std::string config="duration_ms: "+std::to_string(duration_ms)+
  "\nbuffers { size_kb: 2048 fill_policy: DISCARD }\n"
  "data_sources { config { name: \"android.surfaceflinger.frametimeline\" } }\n";
 struct sigaction ignore{},previous{};ignore.sa_handler=SIG_IGN;::sigemptyset(&ignore.sa_mask);
 (void)::sigaction(SIGPIPE,&ignore,&previous);
 size_t written=0;bool ok=true;
 while(written<config.size()){
  const ssize_t count=::write(input[1],config.data()+written,config.size()-written);
  if(count<0&&errno==EINTR)continue;
  if(count<=0){ok=false;break;}written+=static_cast<size_t>(count);
 }
 ::close(input[1]);(void)::sigaction(SIGPIPE,&previous,nullptr);
 if(!ok){stop();return false;}return true;
}
bool FrameTimelineSampler::finish(WorkloadFeatureSnapshot& features) noexcept {
 if(child_pid_<=0||trace_path_.empty())return false;
 int status=0;pid_t waited=0;const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
 do{
  waited=::waitpid(static_cast<pid_t>(child_pid_),&status,WNOHANG);
  if(waited<0&&errno==EINTR)continue;
  if(waited!=0)break;
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
 }while(std::chrono::steady_clock::now()<deadline);
 if(waited==0){
  (void)::kill(static_cast<pid_t>(child_pid_),SIGTERM);
  do{waited=::waitpid(static_cast<pid_t>(child_pid_),&status,0);}while(waited<0&&errno==EINTR);
  child_pid_=-1;(void)::unlink(trace_path_.c_str());trace_path_.clear();return false;
 }
 child_pid_=-1;bool ok=waited>0&&WIFEXITED(status)&&WEXITSTATUS(status)==0;
 if(ok){
  const int fd=::open(trace_path_.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
  if(fd<0)ok=false;
  else{
   struct stat st{};if(::fstat(fd,&st)!=0||!S_ISREG(st.st_mode)||st.st_size<=0||st.st_size>(16LL<<20))ok=false;
   if(ok){
    std::vector<uint8_t> bytes(static_cast<size_t>(st.st_size));size_t read_bytes=0;
    while(read_bytes<bytes.size()){
     const ssize_t count=::read(fd,bytes.data()+read_bytes,bytes.size()-read_bytes);
     if(count<0&&errno==EINTR)continue;
     if(count<=0){ok=false;break;}read_bytes+=static_cast<size_t>(count);
    }
    if(ok)ok=parse_frame_timeline_trace(bytes,features);
   }
   ::close(fd);
  }
 }
 (void)::unlink(trace_path_.c_str());trace_path_.clear();return ok;
}
void FrameTimelineSampler::stop() noexcept {
 if(child_pid_>0){
  (void)::kill(static_cast<pid_t>(child_pid_),SIGTERM);int status=0;
  while(::waitpid(static_cast<pid_t>(child_pid_),&status,0)<0&&errno==EINTR){}
  child_pid_=-1;
 }
 if(!trace_path_.empty())(void)::unlink(trace_path_.c_str());
 trace_path_.clear();
}
bool parse_frame_timeline_trace(
    std::span<const uint8_t> trace,
    WorkloadFeatureSnapshot& features) noexcept {
    FrameTimelineParseState state{};
    size_t offset = 0U;
    ProtoField field{};
    while (offset < trace.size()) {
        if (!next_proto_field(trace, offset, field)) return false;
        if (field.number == 1U && field.wire == 2U &&
            !parse_trace_packet(field.data, state)) return false;
    }
    if (state.frames == 0U || state.misses > state.frames) return false;
    features.frame_deadline_count = state.frames;
    features.frame_deadline_misses = state.misses;
    features.frame_deadline_miss_pct =
        100.0 * static_cast<double>(state.misses) / static_cast<double>(state.frames);
    features.frame_actual_duration_ns_total = state.actual_duration_ns_total;
    features.frame_actual_duration_count = state.actual_duration_count;
    features.valid_features |= FeatureFrameDeadline;
    return true;
}
namespace {
uint64_t frame_monotonic_now_ns() noexcept {
 timespec value{};
 if (::clock_gettime(CLOCK_MONOTONIC, &value) != 0 || value.tv_sec < 0 || value.tv_nsec < 0) return 0;
 return static_cast<uint64_t>(value.tv_sec) * 1'000'000'000ULL + static_cast<uint64_t>(value.tv_nsec);
}
}

bool PerfettoFrameTimelineEventSource::start(FrameEventListener listener) noexcept {
 if (!listener) return false;
 std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
 if (worker_started_) return true;
 {
  std::lock_guard<std::mutex> event_lock(event_mutex_);
  listener_ = std::move(listener);
 }
 running_.store(true, std::memory_order_release);
 if (::pthread_create(&worker_, nullptr, &PerfettoFrameTimelineEventSource::thread_entry, this) != 0) {
  running_.store(false, std::memory_order_release);
  std::lock_guard<std::mutex> event_lock(event_mutex_);
  listener_ = {};
  return false;
 }
 worker_started_ = true;
 return true;
}

void PerfettoFrameTimelineEventSource::stop() noexcept {
 std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
 if (worker_started_) {
  running_.store(false, std::memory_order_release);
  (void)::pthread_join(worker_, nullptr);
  worker_started_ = false;
 }
 std::lock_guard<std::mutex> event_lock(event_mutex_);
 listener_ = {};
}

bool PerfettoFrameTimelineEventSource::latest_snapshot(WorkloadFeatureSnapshot& out) const noexcept {
 std::lock_guard<std::mutex> lock(snapshot_mutex_);
 if ((latest_.valid_features & FeatureFrameDeadline) == 0U || latest_.frame_deadline_count == 0U) return false;
 out = latest_;
 return true;
}

void* PerfettoFrameTimelineEventSource::thread_entry(void* context) noexcept {
 if (context != nullptr) static_cast<PerfettoFrameTimelineEventSource*>(context)->run();
 return nullptr;
}

void PerfettoFrameTimelineEventSource::run() noexcept {
 FrameTimelineSampler sampler;
 while (running_.load(std::memory_order_acquire)) {
  WorkloadFeatureSnapshot features{};
  const uint64_t capture_start_ns = frame_monotonic_now_ns();
  bool captured = sampler.start(750U) && sampler.finish(features) &&
      (features.valid_features & FeatureFrameDeadline) != 0U &&
      features.frame_deadline_count > 0U &&
      features.frame_deadline_misses <= features.frame_deadline_count;
  if (captured) {
   features.window_start_ns = capture_start_ns;
   features.window_end_ns = frame_monotonic_now_ns();
   if (features.window_end_ns <= features.window_start_ns) captured = false;
  }
  if (captured) {
   {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    latest_ = features;
   }
   FrameEvent event{};
   event.kind = FrameEventKind::FrameTimelineWindow;
   event.vsync_timestamp_ns = features.window_end_ns;
   event.frame_count = features.frame_deadline_count;
   event.missed_frames = features.frame_deadline_misses;
   if (features.frame_actual_duration_count > 0U) {
    event.frame_duration_ns =
        features.frame_actual_duration_ns_total / features.frame_actual_duration_count;
   }
   FrameEventListener listener;
   {
    std::lock_guard<std::mutex> lock(event_mutex_);
    listener = listener_;
   }
   if (listener && running_.load(std::memory_order_acquire)) listener(event);
  } else {
   sampler.stop();
  }
  if (!running_.load(std::memory_order_acquire)) break;
  if (!captured) {
   const timespec retry{0, 200'000'000L};
   (void)::nanosleep(&retry, nullptr);
  }
 }
 sampler.stop();
}


WorkloadTelemetryPipeline::WorkloadTelemetryPipeline(std::string proc,std::string sys) noexcept
 :ebpf_(sys),fallback_(std::move(proc),sys),gpu_(std::move(sys)){}
bool WorkloadTelemetryPipeline::start() noexcept {
 stop();
 fallback_active_=fallback_.start();
 gpu_active_=gpu_.start();
 const bool bpf_active=ebpf_.start();
 active_=fallback_active_||bpf_active;
 return active_;
}
void WorkloadTelemetryPipeline::stop() noexcept {
 frame_timeline_.stop();
 ebpf_.stop();
 fallback_.stop();
 gpu_.stop();
 active_=false;
 fallback_active_=false;
 gpu_active_=false;
}
WorkloadFeatureSnapshot WorkloadTelemetryPipeline::sample(uint64_t start,uint64_t end) noexcept {
 WorkloadFeatureSnapshot result{};
 result.window_start_ns=start;result.window_end_ns=end;
 if(!active_||end<=start)return result;
 if(fallback_active_)result=fallback_.sample(start,end);
 result.window_start_ns=start;result.window_end_ns=end;
 if(gpu_active_){double gpu_busy=0.0;if(gpu_.sample(end,gpu_busy)){
  result.gpu_busy_pct=gpu_busy;result.valid_features|=FeatureGpuBusy;result.gpu_busy_sample_count=1;
 }}
 if(ebpf_.active()){
  result.source=TelemetryBackend::Ebpf;
  std::array<WorkloadEvent,kWorkloadEventDrainBatchSize> events{};
  const size_t count=ebpf_.drain(events);
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
