#include "coreflow/ecosystem_knowledge.hpp"

#include <cstring>

namespace coreflow {
namespace {

// Curated from:
// - https://source.android.com/docs/core/perf/cgroups
// - https://source.android.com/docs/core/perf/lmkd
// - https://source.android.com/docs/core/power/values
// - https://source.android.com/docs/core/architecture/kernel
// - https://docs.kernel.org/admin-guide/sysctl/vm.html
// - https://docs.kernel.org/block/queue-sysfs.html
// - https://docs.kernel.org/admin-guide/cgroup-v2.html
//
// This is an embedded semantic index for the daemon — not a full mirror of
// every kernel page. Unknown vendor paths remain ObserveOnly / VendorVolatile.

constexpr EcosystemKnowledgeEntry kCatalog[] = {
    // ---- Linux VM (docs.kernel.org admin-guide/sysctl/vm) ----
    {"vm.swappiness", "/proc/sys/vm/swappiness", ResourceDomain::Memory,
     KnowledgeLayer::LinuxKernel, KnowledgeRisk::BoundedTune, true,
     "Tendency to reclaim anonymous pages vs file cache; higher prefers swap/ZRAM.",
     "docs.kernel.org/admin-guide/sysctl/vm.html"},
    {"vm.dirty_ratio", "/proc/sys/vm/dirty_ratio", ResourceDomain::Memory,
     KnowledgeLayer::LinuxKernel, KnowledgeRisk::BoundedTune, true,
     "Percent of RAM at which a process generating dirty pages is throttled.",
     "docs.kernel.org/admin-guide/sysctl/vm.html"},
    {"vm.dirty_background_ratio", "/proc/sys/vm/dirty_background_ratio", ResourceDomain::Memory,
     KnowledgeLayer::LinuxKernel, KnowledgeRisk::BoundedTune, true,
     "Percent of RAM at which background writeback begins.",
     "docs.kernel.org/admin-guide/sysctl/vm.html"},
    {"vm.vfs_cache_pressure", "/proc/sys/vm/vfs_cache_pressure", ResourceDomain::Memory,
     KnowledgeLayer::LinuxKernel, KnowledgeRisk::BoundedTune, true,
     "Tendency to reclaim dentry/inode caches versus page cache.",
     "docs.kernel.org/admin-guide/sysctl/vm.html"},
    {"vm.min_free_kbytes", "/proc/sys/vm/min_free_kbytes", ResourceDomain::Memory,
     KnowledgeLayer::LinuxKernel, KnowledgeRisk::BoundedTune, true,
     "Minimum free memory the VM tries to keep; too low risks allocation stalls.",
     "docs.kernel.org/admin-guide/sysctl/vm.html"},
    {"vm.dirty_expire_centisecs", "/proc/sys/vm/dirty_expire_centisecs", ResourceDomain::Memory,
     KnowledgeLayer::LinuxKernel, KnowledgeRisk::BoundedTune, true,
     "Age after which dirty data is considered old enough for writeback.",
     "docs.kernel.org/admin-guide/sysctl/vm.html"},
    {"vm.dirty_writeback_centisecs", "/proc/sys/vm/dirty_writeback_centisecs", ResourceDomain::Memory,
     KnowledgeLayer::LinuxKernel, KnowledgeRisk::BoundedTune, true,
     "Interval for the periodic writeback wakeup.",
     "docs.kernel.org/admin-guide/sysctl/vm.html"},
    {"vm.overcommit_memory", "/proc/sys/vm/overcommit_memory", ResourceDomain::Memory,
     KnowledgeLayer::LinuxKernel, KnowledgeRisk::ObserveOnly, true,
     "0 heuristic, 1 always overcommit, 2 never overcommit (swap+ratio).",
     "docs.kernel.org/vm/overcommit-accounting.html"},
    {"vm.page-cluster", "/proc/sys/vm/page-cluster", ResourceDomain::Memory,
     KnowledgeLayer::LinuxKernel, KnowledgeRisk::ObserveOnly, true,
     "Log2 pages read ahead from swap; Android/ZRAM sensitive.",
     "docs.kernel.org/admin-guide/sysctl/vm.html"},
    {"vm.drop_caches", "/proc/sys/vm/drop_caches", ResourceDomain::Memory,
     KnowledgeLayer::LinuxKernel, KnowledgeRisk::Quarantined, true,
     "Diagnostic drop of pagecache/dentries; not a sustained tuning knob.",
     "docs.kernel.org/admin-guide/sysctl/vm.html"},

    // ---- Scheduler ----
    {"kernel.sched_latency_ns", "/proc/sys/kernel/sched_latency_ns", ResourceDomain::Scheduler,
     KnowledgeLayer::LinuxKernel, KnowledgeRisk::VendorVolatile, true,
     "Target scheduling latency for CFS; vendor kernels may ignore or clamp.",
     "docs.kernel.org/scheduler/sched-design-CFS.html"},
    {"kernel.sched_min_granularity_ns", "/proc/sys/kernel/sched_min_granularity_ns", ResourceDomain::Scheduler,
     KnowledgeLayer::LinuxKernel, KnowledgeRisk::VendorVolatile, true,
     "Minimum preemption granularity for CFS tasks.",
     "docs.kernel.org/scheduler/sched-design-CFS.html"},
    {"kernel.sched_wakeup_granularity_ns", "/proc/sys/kernel/sched_wakeup_granularity_ns", ResourceDomain::Scheduler,
     KnowledgeLayer::LinuxKernel, KnowledgeRisk::VendorVolatile, true,
     "Wakeup preemption granularity; aggressive values add context-switch cost.",
     "docs.kernel.org/scheduler/sched-design-CFS.html"},
    {"kernel.sched_rt_runtime_us", "/proc/sys/kernel/sched_rt_runtime_us", ResourceDomain::Scheduler,
     KnowledgeLayer::LinuxKernel, KnowledgeRisk::ObserveOnly, true,
     "RT bandwidth limit; mis-tune can starve SCHED_OTHER.",
     "docs.kernel.org/scheduler/sched-rt-group.html"},
    {"kernel.pid_max", "/proc/sys/kernel/pid_max", ResourceDomain::Scheduler,
     KnowledgeLayer::LinuxKernel, KnowledgeRisk::ObserveOnly, true,
     "Maximum PID value; inventory only for process-table pressure context.",
     "docs.kernel.org/admin-guide/sysctl/kernel.html"},

    // ---- CPUFreq / power (AOSP power values + sysfs) ----
    {"cpufreq.policies", "/sys/devices/system/cpu/cpufreq", ResourceDomain::CpuFreq,
     KnowledgeLayer::AndroidCommonKernel, KnowledgeRisk::BoundedTune, false,
     "Per-cluster cpufreq policies; governors and min/max are primary CPU knobs.",
     "source.android.com/docs/core/power/values"},
    {"cpufreq.stats", "/sys/devices/system/cpu/cpu0/cpufreq/stats", ResourceDomain::CpuFreq,
     KnowledgeLayer::AndroidCommonKernel, KnowledgeRisk::ObserveOnly, false,
     "time_in_state used by Android power profiles and battery attribution.",
     "source.android.com/docs/core/power/component"},
    {"cpu.idle", "/sys/devices/system/cpu/cpuidle", ResourceDomain::CpuFreq,
     KnowledgeLayer::LinuxKernel, KnowledgeRisk::ObserveOnly, false,
     "Idle-state residency; informs power vs latency tradeoffs.",
     "docs.kernel.org/admin-guide/pm/cpuidle.html"},
    {"sys.power.state", "/sys/power/state", ResourceDomain::Power,
     KnowledgeLayer::AndroidFramework, KnowledgeRisk::FrameworkOwned, true,
     "Suspend entry (mem/disk); owned by Android power / automotive stack.",
     "source.android.com/docs/automotive/power/power"},
    {"sys.power.wake_lock", "/sys/power/wake_lock", ResourceDomain::Power,
     KnowledgeLayer::AndroidFramework, KnowledgeRisk::FrameworkOwned, true,
     "Kernel wake locks; framework and drivers own suspend blocking.",
     "source.android.com/docs/core/power/component"},

    // ---- Cgroup v2 (AOSP cgroups + kernel cgroup-v2) ----
    {"cgroup.controllers", "/sys/fs/cgroup/cgroup.controllers", ResourceDomain::CGroup,
     KnowledgeLayer::AndroidFramework, KnowledgeRisk::ObserveOnly, false,
     "Unified hierarchy controllers available on this build.",
     "source.android.com/docs/core/perf/cgroups"},
    {"cgroup.cpu.uclamp.min", "/sys/fs/cgroup/cpu.uclamp.min", ResourceDomain::UClamp,
     KnowledgeLayer::AndroidCommonKernel, KnowledgeRisk::VendorVolatile, true,
     "Utilization clamp min; Android task profiles may own per-app values.",
     "source.android.com/docs/core/perf/cgroups"},
    {"cgroup.cpu.uclamp.max", "/sys/fs/cgroup/cpu.uclamp.max", ResourceDomain::UClamp,
     KnowledgeLayer::AndroidCommonKernel, KnowledgeRisk::VendorVolatile, true,
     "Utilization clamp max; fighting AMS/task_profiles is unsafe.",
     "source.android.com/docs/core/perf/cgroups"},
    {"cgroup.memory.current", "/sys/fs/cgroup/memory.current", ResourceDomain::CGroup,
     KnowledgeLayer::AndroidCommonKernel, KnowledgeRisk::ObserveOnly, false,
     "Current cgroup memory usage; basis for Memory Limiter / PMGD.",
     "source.android.com/docs/core/perf/memory-limiter"},
    {"cgroup.memory.high", "/sys/fs/cgroup/memory.high", ResourceDomain::CGroup,
     KnowledgeLayer::AndroidFramework, KnowledgeRisk::FrameworkOwned, true,
     "Soft memory limit; Android 17 Memory Limiter / PMGD may set per-process.",
     "source.android.com/docs/core/perf/pmgd"},
    {"cgroup.memory.pressure", "/sys/fs/cgroup/memory.pressure", ResourceDomain::CGroup,
     KnowledgeLayer::AndroidCommonKernel, KnowledgeRisk::ObserveOnly, false,
     "PSI-style pressure file inside cgroup v2 memory controller.",
     "docs.kernel.org/admin-guide/cgroup-v2.html"},
    {"cgroup.cpuset.cpus.effective", "/sys/fs/cgroup/cpuset.cpus.effective", ResourceDomain::CpuSet,
     KnowledgeLayer::AndroidFramework, KnowledgeRisk::ObserveOnly, false,
     "Effective CPU set after hierarchy constraints (top-app / background).",
     "source.android.com/docs/core/perf/cgroups"},

    // ---- PSI / LMKD (AOSP lmkd) ----
    {"psi.memory", "/proc/pressure/memory", ResourceDomain::AndroidRuntime,
     KnowledgeLayer::AndroidCommonKernel, KnowledgeRisk::ObserveOnly, false,
     "Pressure Stall Information for memory; preferred LMKD input on modern Android.",
     "source.android.com/docs/core/perf/lmkd"},
    {"psi.cpu", "/proc/pressure/cpu", ResourceDomain::AndroidRuntime,
     KnowledgeLayer::AndroidCommonKernel, KnowledgeRisk::ObserveOnly, false,
     "CPU PSI; tasks delayed waiting for CPU.",
     "source.android.com/docs/core/perf/lmkd"},
    {"psi.io", "/proc/pressure/io", ResourceDomain::AndroidRuntime,
     KnowledgeLayer::AndroidCommonKernel, KnowledgeRisk::ObserveOnly, false,
     "I/O PSI; storage backlog signal.",
     "source.android.com/docs/core/perf/lmkd"},
    {"lmkd.module", "/sys/module/lowmemorykiller", ResourceDomain::AndroidRuntime,
     KnowledgeLayer::AndroidFramework, KnowledgeRisk::FrameworkOwned, false,
     "Legacy in-kernel LMK; modern Android uses userspace lmkd + PSI/memcg.",
     "source.android.com/docs/core/perf/lmkd"},

    // ---- Block I/O (kernel queue-sysfs) — quarantined for mutation ----
    {"block.queue.scheduler", "/sys/block", ResourceDomain::Io,
     KnowledgeLayer::LinuxKernel, KnowledgeRisk::Quarantined, true,
     "I/O scheduler selection; Android storage stacks need topology proof.",
     "docs.kernel.org/block/queue-sysfs.html"},
    {"block.queue.read_ahead_kb", "/sys/block", ResourceDomain::Io,
     KnowledgeLayer::LinuxKernel, KnowledgeRisk::Quarantined, true,
     "Read-ahead window; interacts with filesystem and dm/zram.",
     "docs.kernel.org/block/queue-sysfs.html"},
    {"block.queue.nr_requests", "/sys/block", ResourceDomain::Io,
     KnowledgeLayer::LinuxKernel, KnowledgeRisk::Quarantined, true,
     "Request queue depth; wrong bounds hurt latency under vendor IO stacks.",
     "docs.kernel.org/block/queue-sysfs.html"},

    // ---- GPU / devfreq (vendor) ----
    {"gpu.kgsl", "/sys/class/kgsl", ResourceDomain::Gpu,
     KnowledgeLayer::VendorHAL, KnowledgeRisk::VendorVolatile, false,
     "Qualcomm KGSL GPU sysfs; clocks and busy% are vendor-specific.",
     "vendor:qualcomm-kgsl"},
    {"gpu.devfreq", "/sys/class/devfreq", ResourceDomain::Gpu,
     KnowledgeLayer::AndroidCommonKernel, KnowledgeRisk::VendorVolatile, false,
     "Generic devfreq devices (GPU/DDR/interconnect); governors vary by SoC.",
     "docs.kernel.org/driver-api/devfreq.html"},

    // ---- Thermal ----
    {"thermal.zones", "/sys/class/thermal", ResourceDomain::Thermal,
     KnowledgeLayer::AndroidCommonKernel, KnowledgeRisk::ObserveOnly, false,
     "Thermal zones and cooling devices; policy uses validated zones only.",
     "docs.kernel.org/driver-api/thermal/sysfs-api.html"},

    // ---- Android identity ----
    {"android.build.fingerprint", "prop:ro.build.fingerprint", ResourceDomain::AndroidRuntime,
     KnowledgeLayer::AndroidFramework, KnowledgeRisk::ObserveOnly, false,
     "Unique build identity; required context for any vendor-specific policy.",
     "source.android.com/docs/core/architecture"},
    {"android.treble", "prop:ro.treble.enabled", ResourceDomain::AndroidRuntime,
     KnowledgeLayer::AndroidFramework, KnowledgeRisk::ObserveOnly, false,
     "Project Treble enabled; HALs separated from framework.",
     "source.android.com/docs/core/architecture"},
    {"android.security_patch", "prop:ro.build.version.security_patch", ResourceDomain::AndroidRuntime,
     KnowledgeLayer::AndroidFramework, KnowledgeRisk::ObserveOnly, false,
     "Security patch level; correlates with ACK backports available.",
     "source.android.com/docs/core/architecture/kernel/android-common"},
};

} // namespace

std::size_t ecosystemKnowledgeCount() noexcept {
    return sizeof(kCatalog) / sizeof(kCatalog[0]);
}

const EcosystemKnowledgeEntry* lookupEcosystemKnowledgeByPath(
    const std::string& path) noexcept {
    if (path.empty()) return nullptr;
    for (const auto& e : kCatalog) {
        if (path == e.path) return &e;
        // Prefix match for directory roots such as /sys/block
        const std::size_t n = std::strlen(e.path);
        if (n > 1 && path.size() >= n && path.compare(0, n, e.path) == 0 &&
            (path.size() == n || path[n] == '/')) {
            return &e;
        }
    }
    return nullptr;
}

const EcosystemKnowledgeEntry* lookupEcosystemKnowledgeByKey(
    const std::string& key) noexcept {
    if (key.empty()) return nullptr;
    for (const auto& e : kCatalog) {
        if (key == e.key) return &e;
    }
    return nullptr;
}

const char* knowledgeRiskName(KnowledgeRisk risk) noexcept {
    switch (risk) {
        case KnowledgeRisk::ObserveOnly: return "OBSERVE_ONLY";
        case KnowledgeRisk::BoundedTune: return "BOUNDED_TUNE";
        case KnowledgeRisk::VendorVolatile: return "VENDOR_VOLATILE";
        case KnowledgeRisk::Quarantined: return "QUARANTINED";
        case KnowledgeRisk::FrameworkOwned: return "FRAMEWORK_OWNED";
    }
    return "UNKNOWN";
}

const char* knowledgeLayerName(KnowledgeLayer layer) noexcept {
    switch (layer) {
        case KnowledgeLayer::LinuxKernel: return "LINUX_KERNEL";
        case KnowledgeLayer::AndroidCommonKernel: return "ANDROID_COMMON_KERNEL";
        case KnowledgeLayer::AndroidFramework: return "ANDROID_FRAMEWORK";
        case KnowledgeLayer::VendorHAL: return "VENDOR_HAL";
    }
    return "UNKNOWN";
}

std::string describeEcosystemNode(const ResourceCapability& cap) noexcept {
    const EcosystemKnowledgeEntry* e = lookupEcosystemKnowledgeByPath(cap.path);
    if (!e) e = lookupEcosystemKnowledgeByKey(cap.name);
    if (!e) {
        return std::string("unindexed surface; treat as observe-only until reviewed");
    }
    std::string out = e->summary;
    out.append(" [");
    out.append(knowledgeLayerName(e->layer));
    out.append(" / ");
    out.append(knowledgeRiskName(e->risk));
    out.append("]");
    return out;
}

} // namespace coreflow
