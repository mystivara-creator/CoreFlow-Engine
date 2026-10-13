#include "coreflow/fatal_surface.hpp"

#include <cerrno>
#include <cstdlib>
#include <string>

namespace coreflow {
namespace {

bool startsWith(std::string_view value, std::string_view prefix) noexcept {
    return value.size() >= prefix.size() &&
           value.compare(0, prefix.size(), prefix) == 0;
}

bool contains(std::string_view value, std::string_view needle) noexcept {
    return value.find(needle) != std::string_view::npos;
}

std::string_view leafOf(std::string_view path) noexcept {
    const auto slash = path.rfind('/');
    return slash == std::string_view::npos ? path : path.substr(slash + 1);
}

bool leafIs(std::string_view leaf, std::initializer_list<std::string_view> names) noexcept {
    for (const auto name : names) {
        if (leaf == name) return true;
    }
    return false;
}

bool containsAny(std::string_view value,
                 std::initializer_list<std::string_view> needles) noexcept {
    for (const auto needle : needles) {
        if (contains(value, needle)) return true;
    }
    return false;
}

bool parseSigned(std::string_view text, long long& out) noexcept {
    std::string copy(text);
    while (!copy.empty() && (copy.back() == ' ' || copy.back() == '\t' ||
                             copy.back() == '\n' || copy.back() == '\r')) {
        copy.pop_back();
    }
    std::size_t first = 0;
    while (first < copy.size() && (copy[first] == ' ' || copy[first] == '\t')) ++first;
    copy.erase(0, first);
    if (copy.empty()) return false;
    char* end = nullptr;
    errno = 0;
    const long long value = std::strtoll(copy.c_str(), &end, 10);
    if (errno != 0 || end == copy.c_str() || *end != '\0') return false;
    out = value;
    return true;
}

} // namespace

FatalClass classifyFatalSurface(std::string_view path) noexcept {
    if (path.empty() || path.front() != '/') return FatalClass::NotKernelTunable;
    if (contains(path, "/../") || contains(path, "/./")) return FatalClass::NotKernelTunable;

#ifdef COREFLOW_HOST_TEST_FIXTURES
    // Host fixtures stand in for sysfs/procfs with temporary files; the class
    // checks below still apply to the fixture-relative path.
    const bool host_fixture = startsWith(path, "/tmp/");
#else
    const bool host_fixture = false;
#endif

    // Only kernel tunable filesystems (and Android cgroup v1 mounts) are
    // candidates. Every other location is outside the actuator's domain.
    const bool tunable_root =
        host_fixture ||
        startsWith(path, "/proc/sys/") || startsWith(path, "/sys/") ||
        startsWith(path, "/dev/cpuctl/") || startsWith(path, "/dev/cpuset/") ||
        startsWith(path, "/dev/stune/") || startsWith(path, "/dev/blkio/");
    if (!tunable_root) return FatalClass::RawInterface;

    const std::string_view leaf = leafOf(path);

    // ---- Raw / privileged interfaces ----
    if (startsWith(path, "/sys/kernel/debug") || startsWith(path, "/sys/kernel/tracing") ||
        startsWith(path, "/sys/kernel/security") || startsWith(path, "/sys/fs/selinux") ||
        startsWith(path, "/sys/fs/pstore") || startsWith(path, "/sys/fs/bpf") ||
        startsWith(path, "/sys/firmware") || startsWith(path, "/sys/bus/") ||
        startsWith(path, "/sys/kernel/config") || startsWith(path, "/proc/sys/debug") ||
        startsWith(path, "/proc/sys/fs/") || startsWith(path, "/proc/sys/user/") ||
        startsWith(path, "/proc/sys/abi/")) {
        return FatalClass::RawInterface;
    }

    // ---- One-shot actions and triggers (write == perform an operation) ----
    if (leafIs(leaf, {"uevent", "uevent_helper", "bind", "unbind", "new_id", "remove_id",
                      "driver_override", "remove", "rescan", "reset", "delete", "eject",
                      "trigger", "sysrq-trigger", "drop_caches", "compact_memory",
                      "stat_refresh", "force_empty", "memory.force_empty", "cgroup.kill",
                      "cgroup.procs", "cgroup.threads", "tasks", "cgroup.subtree_control",
                      "cgroup.type", "cgroup.freeze", "cgroup.max.depth",
                      "cgroup.max.descendants", "notify_on_release", "release_agent",
                      "memory.reclaim", "flush", "shutdown", "blank", "kill"}) ||
        contains(path, "_trigger") || contains(path, "/sysrq") ||
        startsWith(leaf, "cgroup.")) {
        // cgroup.* files define hierarchy structure and membership.
        return FatalClass::ActionTrigger;
    }

    // ---- Crash, panic, watchdog and lockup detection ----
    if (containsAny(path, {"panic", "watchdog", "lockup", "hung_task", "oops",
                           "core_pattern", "core_pipe", "reboot", "poweroff",
                           "ctrl-alt-del", "cad_pid", "kexec", "crash"})) {
        return FatalClass::CrashControl;
    }

    // ---- Kernel security and integrity ----
    if (containsAny(path, {"selinux", "enforce", "modprobe", "modules_disabled", "kptr",
                           "dmesg_restrict", "perf_event", "randomize_va_space", "bpf",
                           "userns", "yama", "/keys/", "/random/", "seccomp", "printk",
                           "sysctl_writes_strict", "usermodehelper", "tainted", "ftrace",
                           "tracer", "cap_last_cap", "overflowuid", "overflowgid",
                           "hotplug", "mmap_min_addr", "mmap_rnd", "unprivileged"})) {
        return FatalClass::Security;
    }

    // ---- Thermal protection ----
    if (startsWith(path, "/sys/class/thermal") ||
        startsWith(path, "/sys/devices/virtual/thermal") ||
        containsAny(path, {"thermal", "trip_point", "cooling_device", "tsens", "throttl"})) {
        return FatalClass::ThermalProtection;
    }

    // ---- Charging, battery, voltage and power state ----
    if (startsWith(path, "/sys/power/") ||
        startsWith(path, "/sys/class/power_supply") || startsWith(path, "/sys/class/regulator") ||
        startsWith(path, "/sys/class/usb") || startsWith(path, "/sys/class/typec") ||
        startsWith(path, "/sys/class/gpio") || startsWith(path, "/sys/class/rtc") ||
        containsAny(path, {"power_supply", "charge", "battery", "regulator", "voltage",
                           "/vdd", "/clk/", "/gpio", "pmic", "/wakeup", "wake_lock",
                           "wake_unlock", "autosleep", "suspend"})) {
        return FatalClass::PowerDelivery;
    }

    // ---- CPU / memory hotplug ----
    if (leafIs(leaf, {"online", "offline", "probe", "soft_offline_page",
                      "hard_offline_page"}) ||
        startsWith(path, "/sys/devices/system/memory")) {
        return FatalClass::Hotplug;
    }

    // ---- Hard memory / process limits (OOM, hang, fork failure) ----
    if (leafIs(leaf, {"overcommit_memory", "overcommit_ratio", "overcommit_kbytes",
                      "oom_kill_allocating_task", "max_map_count", "admin_reserve_kbytes",
                      "user_reserve_kbytes", "nr_hugepages", "nr_hugepages_mempolicy",
                      "nr_overcommit_hugepages", "memory.max", "memory.high", "memory.min",
                      "memory.low", "memory.swap.max", "memory.oom.group",
                      "memory.limit_in_bytes", "memory.memsw.limit_in_bytes",
                      "memory.kmem.limit_in_bytes", "pids.max", "pid_max", "threads-max",
                      "ns_last_pid", "sched_rt_runtime_us", "sched_rt_period_us",
                      "legacy_va_layout"}) ||
        contains(path, "hugetlb") || contains(path, "memory_failure") ||
        contains(path, "rt_runtime") || contains(path, "rt_period")) {
        return FatalClass::HardLimit;
    }

    // ---- cpuset hierarchy invariants (memory nodes, exclusivity, balancing) ----
    if (leafIs(leaf, {"mems", "cpuset.mems", "cpu_exclusive", "mem_exclusive",
                      "cpuset.cpus.exclusive", "cpuset.cpus.partition", "sched_load_balance",
                      "sched_relax_domain_level", "memory_migrate", "mem_hardwall",
                      "memory_pressure_enabled", "memory_spread_page", "memory_spread_slab"})) {
        return FatalClass::HardLimit;
    }

    // ---- Storage integrity ----
    if (leafIs(leaf, {"write_cache", "fua", "discard_max_bytes", "discard_zeroes_data",
                      "write_same_max_bytes", "write_zeroes_max_bytes", "zone_append_max_bytes",
                      "dax", "ro", "force_ro"}) ||
        contains(path, "/rpmb") || contains(path, "/ufshc")) {
        return FatalClass::StorageIntegrity;
    }

    return FatalClass::None;
}

bool isFatalValue(std::string_view factory, std::string_view requested) noexcept {
    if (requested.empty() || requested.size() > 256U) return true;
    for (const char c : requested) {
        const auto u = static_cast<unsigned char>(c);
        if (u < 0x20U || u == 0x7FU) return true;  // newline / control chars
    }
    long long base = 0;
    long long next = 0;
    if (parseSigned(factory, base) && parseSigned(requested, next)) {
        if (base >= 0 && next < 0) return true;
    }
    return false;
}

const char* fatalClassName(FatalClass value) noexcept {
    switch (value) {
        case FatalClass::None:              return "NONE";
        case FatalClass::ActionTrigger:     return "ACTION_TRIGGER";
        case FatalClass::CrashControl:      return "CRASH_CONTROL";
        case FatalClass::Security:          return "SECURITY";
        case FatalClass::ThermalProtection: return "THERMAL_PROTECTION";
        case FatalClass::PowerDelivery:     return "POWER_DELIVERY";
        case FatalClass::Hotplug:           return "HOTPLUG";
        case FatalClass::HardLimit:         return "HARD_LIMIT";
        case FatalClass::StorageIntegrity:  return "STORAGE_INTEGRITY";
        case FatalClass::RawInterface:      return "RAW_INTERFACE";
        case FatalClass::NotKernelTunable:  return "NOT_KERNEL_TUNABLE";
    }
    return "UNKNOWN";
}

} // namespace coreflow
