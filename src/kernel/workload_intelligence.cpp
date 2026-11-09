/**
 * @file workload_intelligence.cpp
 * @brief Read-only kernel BPF prerequisites and tracepoint inventory.
 * @license MIT
 */
#include "coreflow/workload_intelligence.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <string>

namespace coreflow {
namespace {

bool readable_regular_file(const std::string& path) noexcept {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode) &&
           ::access(path.c_str(), R_OK) == 0;
}

bool readable_directory(const std::string& path) noexcept {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode) &&
           ::access(path.c_str(), R_OK | X_OK) == 0;
}

CapabilityState file_state(const std::string& path) noexcept {
    struct stat st{};
    if (::stat(path.c_str(), &st) != 0) return CapabilityState::Unavailable;
    if (!S_ISREG(st.st_mode)) return CapabilityState::Unavailable;
    return ::access(path.c_str(), R_OK) == 0
        ? CapabilityState::Available : CapabilityState::Restricted;
}

} // namespace

BpfCapabilitySnapshot BpfCapabilityProbe::probe() const noexcept {
    BpfCapabilitySnapshot result{};
    const std::string tracefs = sys_root_ + "/kernel/tracing";
    const std::string debug_tracefs = sys_root_ + "/kernel/debug/tracing";
    std::string trace_root;
    if (readable_directory(tracefs + "/events")) trace_root = tracefs;
    else if (readable_directory(debug_tracefs + "/events")) trace_root = debug_tracefs;

    result.btf = file_state(sys_root_ + "/kernel/btf/vmlinux");
    result.tracefs = trace_root.empty()
        ? CapabilityState::Unavailable : CapabilityState::Available;
    if (!trace_root.empty()) {
        result.sched_switch = file_state(trace_root + "/events/sched/sched_switch/id");
        result.sched_wakeup = file_state(trace_root + "/events/sched/sched_wakeup/id");
        const auto issue = file_state(trace_root + "/events/block/block_rq_issue/id");
        const auto complete = file_state(trace_root + "/events/block/block_rq_complete/id");
        result.block_request =
            issue == CapabilityState::Available && complete == CapabilityState::Available
                ? CapabilityState::Available
                : (issue == CapabilityState::Restricted || complete == CapabilityState::Restricted)
                    ? CapabilityState::Restricted : CapabilityState::Unavailable;
    } else {
        result.sched_switch = CapabilityState::Unavailable;
        result.sched_wakeup = CapabilityState::Unavailable;
        result.block_request = CapabilityState::Unavailable;
    }

    result.bpf_filesystem = readable_directory(sys_root_ + "/fs/bpf")
        ? CapabilityState::Available : CapabilityState::Unavailable;

    // The probe is intentionally passive. BPF syscall permission, ring-buffer
    // map support, SELinux load permission and loader/verifier acceptance remain
    // Unknown/false until the dedicated P2-C loader probe establishes them.
    return result;
}

} // namespace coreflow
