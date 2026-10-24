#include "coreflow/discovery/classification.hpp"

#include <cassert>
#include <string>

using namespace coreflow;

int main() {
    // Block queue and the elevator directory that exists only for the active scheduler.
    assert(classifyKernelDomain("/sys/block/sda/queue/scheduler", "") == "sysfs.block.queue");
    assert(classifyKernelDomain("/sys/block/sda/queue/nr_requests", "") == "sysfs.block.queue");
    assert(classifyKernelDomain("/sys/block/sda/queue/iosched/read_expire", "") == "sysfs.block.iosched");
    assert(classifyKernelDomain("/sys/block/sda/queue/iosched/read_expire/x", "") == "sysfs.generic");

    // CPU policy: policy-level controls versus the active governor's tunables.
    const std::string policy = "/sys/devices/system/cpu/cpufreq/policy0";
    assert(classifyKernelDomain(policy + "/scaling_governor", "") == "sysfs.cpu.cpufreq");
    assert(classifyKernelDomain(policy + "/scaling_max_freq", "schedutil") == "sysfs.cpu.cpufreq");
    assert(classifyKernelDomain(policy + "/schedutil/rate_limit_us", "schedutil") == "sysfs.cpu.governor");
    // A governor directory that is not the active one is not tunable right now.
    assert(classifyKernelDomain(policy + "/ondemand/sampling_rate", "schedutil") == "sysfs.generic");
    // Policy subdirectories other than the active governor (stats/) are never governors.
    assert(classifyKernelDomain(policy + "/stats/time_in_state", "schedutil") == "sysfs.generic");
    assert(classifyKernelDomain(policy + "/stats/time_in_state", "") == "sysfs.generic");

    assert(classifyKernelDomain("/sys/class/devfreq/1d84000.ufshc/governor", "") == "sysfs.devfreq");
    assert(classifyKernelDomain("/proc/sys/vm/swappiness", "") == "proc_sys.vm");
    assert(classifyKernelDomain("/proc/sys/kernel/sched_latency_ns", "") == "proc_sys.kernel");
    assert(classifyKernelDomain("/sys/devices/virtual/foo/bar", "") == "sysfs.generic");
    assert(classifyKernelDomain("/home/user/file", "").empty());

    // Protected surfaces win over any semantic domain.
    assert(classifyKernelDomain("/sys/class/thermal/thermal_zone0/trip_point_0_temp", "") == "sysfs.protected");
    assert(classifyKernelDomain("/sys/kernel/debug/tracing/x", "") == "sysfs.protected");
    assert(isProtectedKernelControl("/sys/class/power_supply/battery/capacity"));
    assert(!isProtectedKernelControl("/sys/block/sda/queue/nr_requests"));

    // Mode bits: root bypasses DAC, so the owner write bit is the real signal.
    assert(hasWritePermissionBits(0644));
    assert(hasWritePermissionBits(0200));
    assert(!hasWritePermissionBits(0444));
    assert(!hasWritePermissionBits(0555));

    assert(isPrunedDiscoverySubtree("/sys/kernel/debug"));
    assert(isPrunedDiscoverySubtree("/sys/firmware"));
    assert(!isPrunedDiscoverySubtree("/sys/class"));
    assert(!isPrunedDiscoverySubtree("/sys/kernel/debug/x"));
    return 0;
}
