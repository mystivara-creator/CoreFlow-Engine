#include "coreflow/discovery.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/system_properties.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <utility>

namespace coreflow {
namespace {

std::string property(const char* key) {
    char value[PROP_VALUE_MAX] = {};
    if (__system_property_get(key, value) <= 0) return {};
    return value;
}

bool policyAuthorizesResource(ResourceDomain domain, const std::string& name,
                              const std::string& path) {
    // These VM controls have explicit semantic priors in EffectModel. Keep the
    // allow-list narrow: discovered scheduler/sysfs writability is not evidence
    // that mutation is useful or safe on an unknown Android vendor kernel.
    if (domain != ResourceDomain::Memory ||
        path.rfind("/proc/sys/vm/", 0) != 0) return false;

    struct ApprovedControl { const char* name; const char* path; };
    static constexpr ApprovedControl supported[] = {
        {"vm.swappiness", "/proc/sys/vm/swappiness"},
        {"vm.dirty_ratio", "/proc/sys/vm/dirty_ratio"},
        {"vm.dirty_background_ratio", "/proc/sys/vm/dirty_background_ratio"},
        {"vm.vfs_cache_pressure", "/proc/sys/vm/vfs_cache_pressure"},
        {"vm.min_free_kbytes", "/proc/sys/vm/min_free_kbytes"},
        {"vm.dirty_expire_centisecs", "/proc/sys/vm/dirty_expire_centisecs"},
        {"vm.dirty_writeback_centisecs", "/proc/sys/vm/dirty_writeback_centisecs"},
    };
    for (const auto& candidate : supported) {
        if (name == candidate.name && path == candidate.path) return true;
    }
    return false;
}

void addCapability(EnvironmentCapabilityMatrix& matrix, ResourceDomain domain,
                   const std::string& name, const std::string& path,
                   bool exists, bool readable, bool writable,
                   bool permission_granted = false) {
    ResourceCapability cap;
    cap.domain = domain;
    cap.name = name;
    cap.path = path;
    cap.exists = exists;
    cap.readable = readable;
    cap.writable = writable;
    // Keep OS access evidence separate from policy approval. W_OK answers only
    // whether this process appears able to write the node; it is not consent.
    cap.permission_granted = permission_granted || (exists && writable);
    cap.policy_authorized = policyAuthorizesResource(domain, name, path);
    cap.runtime_verified = exists && readable;
    cap.mutation_ready = cap.exists && cap.readable && cap.writable &&
                         cap.permission_granted && cap.runtime_verified &&
                         cap.policy_authorized;
    matrix.resources.push_back(std::move(cap));
}

bool readText(const std::string& path, std::string& out) {
    std::ifstream file(path);
    if (!file) return false;
    std::getline(file, out);
    return !out.empty();
}

bool readUnsigned(const std::string& path, std::uint64_t& value) {
    std::ifstream file(path);
    if (!file) return false;
    std::uint64_t parsed = 0;
    if (!(file >> parsed)) return false;
    value = parsed;
    return true;
}

bool readSigned(const std::string& path, long long& value) {
    std::ifstream file(path);
    if (!file) return false;

    std::string text;
    std::getline(file, text);
    if (text.empty()) return false;

    char* end = nullptr;
    errno = 0;
    const long long parsed = std::strtoll(text.c_str(), &end, 10);
    if (errno != 0 || end == text.c_str()) return false;

    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') ++end;
    if (*end != '\0') return false;

    value = parsed;
    return true;
}

bool directoryExists(const char* path) {
    DIR* dir = opendir(path);
    if (!dir) return false;
    closedir(dir);
    return true;
}

std::vector<std::string> splitWords(const std::string& input) {
    std::istringstream stream(input);
    std::vector<std::string> result;
    std::string word;
    while (stream >> word) result.push_back(word);
    return result;
}

TunableCapability discoverNumeric(const std::string& path) {
    TunableCapability cap;
    cap.path = path;
    cap.readable = access(path.c_str(), R_OK) == 0;
    cap.writable = access(path.c_str(), W_OK) == 0;
    cap.numeric = readSigned(path, cap.value);
    return cap;
}

TextCapability discoverText(const std::string& path) {
    TextCapability cap;
    cap.path = path;
    cap.readable = access(path.c_str(), R_OK) == 0;
    cap.writable = access(path.c_str(), W_OK) == 0;
    if (cap.readable) readText(path, cap.value);
    return cap;
}

} // namespace

DeviceProfile EnvironmentDiscovery::discover() const {
    DeviceProfile profile;
    profile.proc_available = directoryExists("/proc");
    profile.sys_available = directoryExists("/sys");

    discoverEnvironment(profile);

    discoverCpuPolicies(profile);
    discoverThermalZones(profile);
    discoverCharging(profile);
    discoverIo(profile);
    discoverSystemControls(profile);
    discoverGpu(profile);
    finalizeCapabilities(profile);
    return profile;
}

void EnvironmentDiscovery::discoverEnvironment(DeviceProfile& profile) const {
    profile.environment.release = property("ro.build.version.release");
    profile.environment.sdk_level = property("ro.build.version.sdk");
    profile.environment.manufacturer = property("ro.product.manufacturer");
    profile.environment.model = property("ro.product.model");
    profile.environment.device = property("ro.product.device");
    profile.environment.product = property("ro.product.name");
    profile.environment.board = property("ro.product.board");
    profile.environment.hardware = property("ro.hardware");
    profile.environment.soc_manufacturer = property("ro.soc.manufacturer");
    profile.environment.soc_model = property("ro.soc.model");
    profile.environment.fingerprint = property("ro.build.fingerprint");
    profile.environment.security_patch = property("ro.build.version.security_patch");
    profile.environment.incremental = property("ro.build.version.incremental");
    profile.environment.treble_enabled = property("ro.treble.enabled");
    profile.environment.gsi_running = property("ro.gsid.image_running");
    profile.environment.hardware_sku = property("ro.boot.hardware.sku");
    if (profile.environment.hardware_sku.empty())
        profile.environment.hardware_sku = property("ro.boot.product.hardware.sku");
    profile.environment.bootloader = property("ro.bootloader");

    struct utsname uts {};
    if (uname(&uts) == 0) {
        profile.environment.kernel_release = uts.release;
        profile.environment.abi = uts.machine;
        profile.kernel_release = uts.release;
        profile.abi = uts.machine;
    }
    profile.android_release = profile.environment.release;

    const bool cgroup2 = access("/sys/fs/cgroup/cgroup.controllers", R_OK) == 0;
    profile.environment.cgroup_v2 = cgroup2;
    profile.environment.cpuset_available =
        access("/sys/fs/cgroup/cpuset.cpus.effective", R_OK) == 0 ||
        access("/sys/fs/cgroup/cpuset.cpus", R_OK) == 0;
    profile.environment.uclamp_available =
        access("/sys/fs/cgroup/cpu.uclamp.min", R_OK) == 0 ||
        access("/sys/fs/cgroup/cpu.uclamp.max", R_OK) == 0;
    profile.environment.scheduler_controls_available =
        access("/proc/sys/kernel/sched_latency_ns", R_OK) == 0 ||
        access("/sys/kernel/debug/sched_features", R_OK) == 0;
    profile.environment.devfreq_available = directoryExists("/sys/class/devfreq");
}

void EnvironmentDiscovery::discoverCpuPolicies(DeviceProfile& profile) const {
    constexpr const char* base = "/sys/devices/system/cpu/cpufreq";
    DIR* dir = opendir(base);
    if (!dir) return;

    while (dirent* entry = readdir(dir)) {
        const std::string name(entry->d_name);
        if (name.rfind("policy", 0) != 0 || name.size() <= 6) continue;

        CpuPolicy policy;
        policy.id = std::atoi(name.c_str() + 6);
        policy.path = std::string(base) + "/" + name;

        readText(policy.path + "/related_cpus", policy.related_cpus);
        policy.readable = readText(policy.path + "/scaling_governor", policy.governor);
        policy.governor_writable = access((policy.path + "/scaling_governor").c_str(), W_OK) == 0;
        policy.scaling_max_writable = access((policy.path + "/scaling_max_freq").c_str(), W_OK) == 0;

        readUnsigned(policy.path + "/cpuinfo_min_freq", policy.hardware_min_frequency);
        readUnsigned(policy.path + "/cpuinfo_max_freq", policy.hardware_max_frequency);
        readUnsigned(policy.path + "/scaling_min_freq", policy.scaling_min_frequency);
        readUnsigned(policy.path + "/scaling_max_freq", policy.scaling_max_frequency);

        std::string governors;
        if (readText(policy.path + "/scaling_available_governors", governors))
            policy.available_governors = splitWords(governors);

        policy.energy_performance_available =
            readText(policy.path + "/energy_performance_preference",
                     policy.energy_performance_preference);
        policy.boost_available = access((policy.path + "/boost").c_str(), F_OK) == 0;

        profile.cpu_policies.push_back(std::move(policy));
    }
    closedir(dir);

    std::sort(profile.cpu_policies.begin(), profile.cpu_policies.end(),
              [](const CpuPolicy& a, const CpuPolicy& b) { return a.id < b.id; });
}

void EnvironmentDiscovery::discoverThermalZones(DeviceProfile& profile) const {
    constexpr const char* base = "/sys/class/thermal";
    DIR* dir = opendir(base);
    if (!dir) return;

    while (dirent* entry = readdir(dir)) {
        const std::string name(entry->d_name);
        if (name.rfind("thermal_zone", 0) != 0) continue;

        ThermalZone zone;
        zone.path = std::string(base) + "/" + name;
        readText(zone.path + "/type", zone.type);

        // Inventory every zone. Sentinel values (-273000, 0, large negatives)
        // stay listed for completeness but are not marked readable; runtime
        // observation applies the same plausibility filter on every sample.
        std::ifstream temp(zone.path + "/temp");
        long milli = 0;
        if (temp && (temp >> milli)) {
            zone.temperature_millidegrees = milli;
            if (milli >= 10000 && milli <= 120000) {
                zone.readable = true;
            }
        }
        profile.thermal_zones.push_back(std::move(zone));
    }
    closedir(dir);

    std::sort(profile.thermal_zones.begin(), profile.thermal_zones.end(),
              [](const ThermalZone& a, const ThermalZone& b) { return a.path < b.path; });
}

void EnvironmentDiscovery::discoverCharging(DeviceProfile& profile) const {
    constexpr const char* base = "/sys/class/power_supply";
    DIR* dir = opendir(base);
    if (!dir) return;

    while (dirent* entry = readdir(dir)) {
        const std::string name(entry->d_name);
        if (name == "." || name == "..") continue;

        const std::string path = std::string(base) + "/" + name;
        std::string type;
        if (!readText(path + "/type", type) || type != "Battery") continue;

        profile.charging.battery_path = path;
        profile.charging.battery_available = true;
        profile.charging.status_readable = access((path + "/status").c_str(), R_OK) == 0;

        const bool temp = access((path + "/temp").c_str(), R_OK) == 0;
        const bool current = access((path + "/current_now").c_str(), R_OK) == 0;
        const bool voltage = access((path + "/voltage_now").c_str(), R_OK) == 0;
        profile.charging.telemetry_readable = temp || current || voltage;

        profile.charging.has_input_current_limit = access((path + "/input_current_limit").c_str(), F_OK) == 0;
        profile.charging.has_charge_current_limit = access((path + "/constant_charge_current_max").c_str(), F_OK) == 0 ||
                                                    access((path + "/charge_current").c_str(), F_OK) == 0;

        const std::string control = path + "/charge_control_limit";
        profile.charging.has_charge_control_limit = access(control.c_str(), F_OK) == 0;
        if (profile.charging.has_charge_control_limit) {
            profile.charging.charge_control_limit_path = control;
            profile.charging.charge_control_limit_readable = access(control.c_str(), R_OK) == 0;
            profile.charging.charge_control_limit_writable = access(control.c_str(), W_OK) == 0;
            profile.charging.charge_control_limit_numeric = readSigned(control, profile.charging.charge_control_limit_value);

            profile.charging.charge_control_limit_min_available =
                readSigned(path + "/charge_control_limit_min", profile.charging.charge_control_limit_min);
            profile.charging.charge_control_limit_max_available =
                readSigned(path + "/charge_control_limit_max", profile.charging.charge_control_limit_max);

            profile.charging.charge_control_limit_range_valid =
                profile.charging.charge_control_limit_numeric &&
                profile.charging.charge_control_limit_min_available &&
                profile.charging.charge_control_limit_max_available &&
                profile.charging.charge_control_limit_min <= profile.charging.charge_control_limit_max &&
                profile.charging.charge_control_limit_value >= profile.charging.charge_control_limit_min &&
                profile.charging.charge_control_limit_value <= profile.charging.charge_control_limit_max;
        }

        profile.charging.has_charging_enabled = access((path + "/charging_enabled").c_str(), F_OK) == 0;
        profile.charging.has_charge_disable = access((path + "/charge_disable").c_str(), F_OK) == 0;
        break;
    }
    closedir(dir);
}

void EnvironmentDiscovery::discoverIo(DeviceProfile& profile) const {
    constexpr const char* base = "/sys/block";
    DIR* dir = opendir(base);
    if (!dir) return;

    while (dirent* entry = readdir(dir)) {
        const std::string name(entry->d_name);
        if (name == "." || name == "..") continue;

        const std::string queue = std::string(base) + "/" + name + "/queue/read_ahead_kb";
        IoDevice device;
        device.path = std::string(base) + "/" + name;
        device.name = name;
        device.read_ahead_readable = access(queue.c_str(), R_OK) == 0;
        device.read_ahead_writable = access(queue.c_str(), W_OK) == 0;
        readUnsigned(queue, device.read_ahead_kb);

        const std::string requests = std::string(base) + "/" + name + "/queue/nr_requests";
        device.nr_requests_readable = access(requests.c_str(), R_OK) == 0;
        device.nr_requests_writable = access(requests.c_str(), W_OK) == 0;
        readUnsigned(requests, device.nr_requests);

        const std::string scheduler = std::string(base) + "/" + name + "/queue/scheduler";
        device.scheduler_readable = access(scheduler.c_str(), R_OK) == 0;
        device.scheduler_writable = access(scheduler.c_str(), W_OK) == 0;
        if (device.scheduler_readable) readText(scheduler, device.scheduler);

        if (device.read_ahead_readable || device.read_ahead_writable ||
            device.nr_requests_readable || device.nr_requests_writable ||
            device.scheduler_readable || device.scheduler_writable)
            profile.io_devices.push_back(std::move(device));
    }
    closedir(dir);

    std::sort(profile.io_devices.begin(), profile.io_devices.end(),
              [](const IoDevice& a, const IoDevice& b) { return a.name < b.name; });
}

void EnvironmentDiscovery::discoverSystemControls(DeviceProfile& profile) const {
    profile.vm_swappiness = discoverNumeric("/proc/sys/vm/swappiness");
    profile.uclamp_min = discoverNumeric("/sys/fs/cgroup/cpu.uclamp.min");
    profile.uclamp_max = discoverNumeric("/sys/fs/cgroup/cpu.uclamp.max");
    profile.cpuset_effective_cpus = discoverText("/sys/fs/cgroup/cpuset.cpus.effective");
}

void EnvironmentDiscovery::discoverGpu(DeviceProfile& profile) const {
    constexpr const char* base = "/sys/class/devfreq";
    DIR* dir = opendir(base);
    if (!dir) return;

    while (dirent* entry = readdir(dir)) {
        const std::string name(entry->d_name);
        if (name == "." || name == "..") continue;
        const std::string path = std::string(base) + "/" + name;
        const std::string governor = path + "/governor";
        const std::string cur = path + "/cur_freq";
        const bool exists = access(path.c_str(), F_OK) == 0;
        const bool readable = access(governor.c_str(), R_OK) == 0 || access(cur.c_str(), R_OK) == 0;
        const bool writable = access(governor.c_str(), W_OK) == 0;
        addCapability(profile.capabilities, ResourceDomain::Gpu, name, path, exists, readable, writable);
    }
    closedir(dir);
}

void EnvironmentDiscovery::finalizeCapabilities(DeviceProfile& profile) const {
    auto& matrix = profile.capabilities;
    matrix.environment = profile.environment;

    addCapability(matrix, ResourceDomain::CpuFreq, "policies",
                  "/sys/devices/system/cpu/cpufreq",
                  !profile.cpu_policies.empty(), !profile.cpu_policies.empty(), false);

    addCapability(matrix, ResourceDomain::UClamp, "cpu.uclamp.min",
                  profile.uclamp_min.path, profile.uclamp_min.readable || profile.uclamp_min.writable,
                  profile.uclamp_min.readable, profile.uclamp_min.writable);
    addCapability(matrix, ResourceDomain::UClamp, "cpu.uclamp.max",
                  profile.uclamp_max.path, profile.uclamp_max.readable || profile.uclamp_max.writable,
                  profile.uclamp_max.readable, profile.uclamp_max.writable);
    addCapability(matrix, ResourceDomain::CpuSet, "cpus.effective",
                  profile.cpuset_effective_cpus.path,
                  profile.cpuset_effective_cpus.readable || profile.cpuset_effective_cpus.writable,
                  profile.cpuset_effective_cpus.readable, profile.cpuset_effective_cpus.writable);

    struct NumericTunable { const char* name; const char* path; };
    constexpr NumericTunable tunables[] = {
        {"vm.swappiness", "/proc/sys/vm/swappiness"},
        {"vm.dirty_ratio", "/proc/sys/vm/dirty_ratio"},
        {"vm.dirty_background_ratio", "/proc/sys/vm/dirty_background_ratio"},
        {"vm.vfs_cache_pressure", "/proc/sys/vm/vfs_cache_pressure"},
        {"vm.min_free_kbytes", "/proc/sys/vm/min_free_kbytes"},
        {"vm.dirty_expire_centisecs", "/proc/sys/vm/dirty_expire_centisecs"},
        {"vm.dirty_writeback_centisecs", "/proc/sys/vm/dirty_writeback_centisecs"},
        {"kernel.sched_latency_ns", "/proc/sys/kernel/sched_latency_ns"},
        {"kernel.sched_min_granularity_ns", "/proc/sys/kernel/sched_min_granularity_ns"},
        {"kernel.sched_wakeup_granularity_ns", "/proc/sys/kernel/sched_wakeup_granularity_ns"}
    };
    for (const auto& item : tunables) {
        const bool readable = access(item.path, R_OK) == 0;
        const bool writable = access(item.path, W_OK) == 0;
        addCapability(matrix,
                       std::string(item.name).rfind("kernel.sched_", 0) == 0
                           ? ResourceDomain::Scheduler : ResourceDomain::Memory,
                       item.name, item.path, readable || writable, readable, writable);
    }

    addCapability(matrix, ResourceDomain::CGroup, "cgroup.controllers",
                  "/sys/fs/cgroup/cgroup.controllers", profile.environment.cgroup_v2,
                  profile.environment.cgroup_v2, false);
    addCapability(matrix, ResourceDomain::Thermal, "thermal-zones",
                  "/sys/class/thermal", !profile.thermal_zones.empty(), !profile.thermal_zones.empty(), false);
    addCapability(matrix, ResourceDomain::Charging, "battery",
                  profile.charging.battery_path, profile.charging.battery_available,
                  profile.charging.status_readable || profile.charging.telemetry_readable, false);
    for (const auto& device : profile.io_devices) {
        const std::string base = device.path;
        addCapability(matrix, ResourceDomain::Io, device.name + ":read_ahead_kb",
                      base + "/queue/read_ahead_kb",
                      device.read_ahead_readable || device.read_ahead_writable,
                      device.read_ahead_readable, device.read_ahead_writable);
        addCapability(matrix, ResourceDomain::Io, device.name + ":nr_requests",
                      base + "/queue/nr_requests",
                      device.nr_requests_readable || device.nr_requests_writable,
                      device.nr_requests_readable, device.nr_requests_writable);
        addCapability(matrix, ResourceDomain::Io, device.name + ":scheduler",
                      base + "/queue/scheduler",
                      device.scheduler_readable || device.scheduler_writable,
                      device.scheduler_readable, device.scheduler_writable);
    }
    addCapability(matrix, ResourceDomain::Io, "block-queues", "/sys/block",
                  !profile.io_devices.empty(), !profile.io_devices.empty(), false);
    addCapability(matrix, ResourceDomain::Power, "power-supply", "/sys/class/power_supply",
                  profile.charging.battery_available, profile.charging.status_readable, false);

    addCapability(matrix, ResourceDomain::AndroidRuntime, "android-properties", "system-properties",
                  !profile.environment.release.empty() || !profile.environment.sdk_level.empty(), true, false);

    // Wide Android ecosystem inventory (observe/export only).
    expandEcosystemSurface(profile);
}

void EnvironmentDiscovery::expandEcosystemSurface(DeviceProfile& profile) const {
    auto& matrix = profile.capabilities;

    // Probe helper: existence/read/write evidence only — never policy authorization.
    auto probe = [&](ResourceDomain domain, const char* name, const char* path) {
        const bool readable = access(path, R_OK) == 0;
        const bool writable = access(path, W_OK) == 0;
        const bool exists = readable || writable || (access(path, F_OK) == 0);
        if (!exists) return;
        addCapability(matrix, domain, name, path, true, readable, writable);
    };

    // --- Memory / VM surface (beyond the authorized allow-list) ---
    static constexpr const char* kVm[][2] = {
        {"vm.page-cluster", "/proc/sys/vm/page-cluster"},
        {"vm.overcommit_memory", "/proc/sys/vm/overcommit_memory"},
        {"vm.overcommit_ratio", "/proc/sys/vm/overcommit_ratio"},
        {"vm.swappiness", "/proc/sys/vm/swappiness"},
        {"vm.dirty_ratio", "/proc/sys/vm/dirty_ratio"},
        {"vm.dirty_background_ratio", "/proc/sys/vm/dirty_background_ratio"},
        {"vm.vfs_cache_pressure", "/proc/sys/vm/vfs_cache_pressure"},
        {"vm.min_free_kbytes", "/proc/sys/vm/min_free_kbytes"},
        {"vm.watermark_scale_factor", "/proc/sys/vm/watermark_scale_factor"},
        {"vm.stat_refresh", "/proc/sys/vm/stat_refresh"},
        {"vm.compact_memory", "/proc/sys/vm/compact_memory"},
        {"vm.drop_caches", "/proc/sys/vm/drop_caches"},
        {"vm.laptop_mode", "/proc/sys/vm/laptop_mode"},
        {"vm.zone_reclaim_mode", "/proc/sys/vm/zone_reclaim_mode"},
        {"vm.extfrag_threshold", "/proc/sys/vm/extfrag_threshold"},
    };
    for (const auto& item : kVm)
        probe(ResourceDomain::Memory, item[0], item[1]);

    // --- Scheduler / kernel ---
    static constexpr const char* kSched[][2] = {
        {"kernel.sched_latency_ns", "/proc/sys/kernel/sched_latency_ns"},
        {"kernel.sched_min_granularity_ns", "/proc/sys/kernel/sched_min_granularity_ns"},
        {"kernel.sched_wakeup_granularity_ns", "/proc/sys/kernel/sched_wakeup_granularity_ns"},
        {"kernel.sched_child_runs_first", "/proc/sys/kernel/sched_child_runs_first"},
        {"kernel.sched_tunable_scaling", "/proc/sys/kernel/sched_tunable_scaling"},
        {"kernel.sched_rt_period_us", "/proc/sys/kernel/sched_rt_period_us"},
        {"kernel.sched_rt_runtime_us", "/proc/sys/kernel/sched_rt_runtime_us"},
        {"kernel.sched_autogroup_enabled", "/proc/sys/kernel/sched_autogroup_enabled"},
        {"kernel.sched_schedstats", "/proc/sys/kernel/sched_schedstats"},
        {"kernel.timer_migration", "/proc/sys/kernel/timer_migration"},
        {"kernel.numa_balancing", "/proc/sys/kernel/numa_balancing"},
        {"kernel.pid_max", "/proc/sys/kernel/pid_max"},
        {"kernel.threads-max", "/proc/sys/kernel/threads-max"},
        {"kernel.randomize_va_space", "/proc/sys/kernel/randomize_va_space"},
        {"kernel.sched_features", "/sys/kernel/debug/sched_features"},
        {"kernel.cpu_boost", "/proc/sys/kernel/sched_boost"},
    };
    for (const auto& item : kSched)
        probe(ResourceDomain::Scheduler, item[0], item[1]);

    // --- CPU frequency / energy ---
    static constexpr const char* kCpu[][2] = {
        {"cpufreq.root", "/sys/devices/system/cpu/cpufreq"},
        {"cpu.present", "/sys/devices/system/cpu/present"},
        {"cpu.possible", "/sys/devices/system/cpu/possible"},
        {"cpu.online", "/sys/devices/system/cpu/online"},
        {"cpu.isolated", "/sys/devices/system/cpu/isolated"},
        {"cpu.offline", "/sys/devices/system/cpu/offline"},
        {"powercap", "/sys/class/powercap"},
        {"cpu.idle", "/sys/devices/system/cpu/cpuidle"},
    };
    for (const auto& item : kCpu)
        probe(ResourceDomain::CpuFreq, item[0], item[1]);

    // --- Cgroup v2 android surface ---
    static constexpr const char* kCgroup[][2] = {
        {"cgroup.controllers", "/sys/fs/cgroup/cgroup.controllers"},
        {"cgroup.subtree_control", "/sys/fs/cgroup/cgroup.subtree_control"},
        {"cpu.stat", "/sys/fs/cgroup/cpu.stat"},
        {"cpu.weight", "/sys/fs/cgroup/cpu.weight"},
        {"cpu.idle", "/sys/fs/cgroup/cpu.idle"},
        {"cpu.uclamp.min", "/sys/fs/cgroup/cpu.uclamp.min"},
        {"cpu.uclamp.max", "/sys/fs/cgroup/cpu.uclamp.max"},
        {"memory.current", "/sys/fs/cgroup/memory.current"},
        {"memory.stat", "/sys/fs/cgroup/memory.stat"},
        {"memory.pressure", "/sys/fs/cgroup/memory.pressure"},
        {"memory.high", "/sys/fs/cgroup/memory.high"},
        {"memory.max", "/sys/fs/cgroup/memory.max"},
        {"io.stat", "/sys/fs/cgroup/io.stat"},
        {"io.pressure", "/sys/fs/cgroup/io.pressure"},
        {"cpuset.cpus", "/sys/fs/cgroup/cpuset.cpus"},
        {"cpuset.cpus.effective", "/sys/fs/cgroup/cpuset.cpus.effective"},
        {"cpuset.mems", "/sys/fs/cgroup/cpuset.mems"},
        {"apps", "/sys/fs/cgroup/apps"},
        {"system", "/sys/fs/cgroup/system"},
        {"uid_0", "/sys/fs/cgroup/uid_0"},
    };
    for (const auto& item : kCgroup)
        probe(ResourceDomain::CGroup, item[0], item[1]);

    // --- GPU / graphics (vendor paths; observe only) ---
    static constexpr const char* kGpu[][2] = {
        {"kgsl-3d0", "/sys/class/kgsl/kgsl-3d0"},
        {"kgsl-3d0.gpuclk", "/sys/class/kgsl/kgsl-3d0/gpuclk"},
        {"kgsl-3d0.max_gpuclk", "/sys/class/kgsl/kgsl-3d0/max_gpuclk"},
        {"kgsl-3d0.gpu_busy", "/sys/class/kgsl/kgsl-3d0/gpu_busy_percentage"},
        {"mali0", "/sys/class/misc/mali0"},
        {"mali.devfreq", "/sys/class/devfreq/gpufreq"},
        {"graphics.devfreq", "/sys/class/devfreq"},
    };
    for (const auto& item : kGpu)
        probe(ResourceDomain::Gpu, item[0], item[1]);

    // Walk /sys/class/devfreq for SoC interconnect / GPU / DDR clocks.
    if (DIR* dir = opendir("/sys/class/devfreq")) {
        while (dirent* entry = readdir(dir)) {
            const std::string name(entry->d_name);
            if (name.empty() || name[0] == '.') continue;
            const std::string base = std::string("/sys/class/devfreq/") + name;
            probe(ResourceDomain::Gpu, ("devfreq." + name).c_str(), base.c_str());
            probe(ResourceDomain::Gpu, ("devfreq." + name + ".governor").c_str(),
                  (base + "/governor").c_str());
            probe(ResourceDomain::Gpu, ("devfreq." + name + ".cur_freq").c_str(),
                  (base + "/cur_freq").c_str());
        }
        closedir(dir);
        profile.environment.devfreq_available = true;
    }

    // --- Thermal cooling ---
    if (DIR* dir = opendir("/sys/class/thermal")) {
        while (dirent* entry = readdir(dir)) {
            const std::string name(entry->d_name);
            if (name.rfind("cooling_device", 0) != 0) continue;
            const std::string base = std::string("/sys/class/thermal/") + name;
            probe(ResourceDomain::Thermal, name.c_str(), base.c_str());
            probe(ResourceDomain::Thermal, (name + ".type").c_str(), (base + "/type").c_str());
            probe(ResourceDomain::Thermal, (name + ".cur_state").c_str(),
                  (base + "/cur_state").c_str());
        }
        closedir(dir);
    }

    // --- Power supplies (battery, USB, wireless, dc) ---
    if (DIR* dir = opendir("/sys/class/power_supply")) {
        while (dirent* entry = readdir(dir)) {
            const std::string name(entry->d_name);
            if (name.empty() || name[0] == '.') continue;
            const std::string base = std::string("/sys/class/power_supply/") + name;
            probe(ResourceDomain::Power, ("ps." + name).c_str(), base.c_str());
            probe(ResourceDomain::Power, ("ps." + name + ".type").c_str(),
                  (base + "/type").c_str());
            probe(ResourceDomain::Power, ("ps." + name + ".status").c_str(),
                  (base + "/status").c_str());
            probe(ResourceDomain::Power, ("ps." + name + ".capacity").c_str(),
                  (base + "/capacity").c_str());
            probe(ResourceDomain::Power, ("ps." + name + ".current_now").c_str(),
                  (base + "/current_now").c_str());
            probe(ResourceDomain::Power, ("ps." + name + ".voltage_now").c_str(),
                  (base + "/voltage_now").c_str());
        }
        closedir(dir);
    }

    // --- Block / zram / dm (inventory; queue mutation stays quarantined) ---
    if (DIR* dir = opendir("/sys/block")) {
        while (dirent* entry = readdir(dir)) {
            const std::string name(entry->d_name);
            if (name.empty() || name[0] == '.') continue;
            // Focus on interesting Android volumes; skip huge noise of loop* if many.
            if (name.rfind("loop", 0) == 0) continue;
            const std::string base = std::string("/sys/block/") + name;
            probe(ResourceDomain::Io, ("block." + name).c_str(), base.c_str());
            probe(ResourceDomain::Io, ("block." + name + ".scheduler").c_str(),
                  (base + "/queue/scheduler").c_str());
            probe(ResourceDomain::Io, ("block." + name + ".rotational").c_str(),
                  (base + "/queue/rotational").c_str());
            probe(ResourceDomain::Io, ("block." + name + ".nr_requests").c_str(),
                  (base + "/queue/nr_requests").c_str());
        }
        closedir(dir);
    }

    // --- Android runtime / framework surface markers ---
    static constexpr const char* kRuntime[][2] = {
        {"lmkd", "/sys/module/lowmemorykiller"},
        {"lmkd.params", "/sys/module/lowmemorykiller/parameters"},
        {"psi", "/proc/pressure"},
        {"psi.memory", "/proc/pressure/memory"},
        {"psi.cpu", "/proc/pressure/cpu"},
        {"psi.io", "/proc/pressure/io"},
        {"meminfo", "/proc/meminfo"},
        {"stat", "/proc/stat"},
        {"loadavg", "/proc/loadavg"},
        {"diskstats", "/proc/diskstats"},
        {"binder", "/dev/binder"},
        {"hwbinder", "/dev/hwbinder"},
        {"vndbinder", "/dev/vndbinder"},
        {"ion", "/dev/ion"},
        {"dma_heap", "/dev/dma_heap"},
    };
    for (const auto& item : kRuntime)
        probe(ResourceDomain::AndroidRuntime, item[0], item[1]);

    // Build fingerprint-style inventory markers (always present as metadata).
    addCapability(matrix, ResourceDomain::AndroidRuntime, "build.fingerprint",
                  profile.environment.fingerprint.empty() ? "prop:ro.build.fingerprint"
                                                          : profile.environment.fingerprint,
                  !profile.environment.fingerprint.empty(), true, false);
    addCapability(matrix, ResourceDomain::AndroidRuntime, "build.security_patch",
                  profile.environment.security_patch.empty() ? "prop:ro.build.version.security_patch"
                                                             : profile.environment.security_patch,
                  !profile.environment.security_patch.empty(), true, false);
    addCapability(matrix, ResourceDomain::AndroidRuntime, "treble.enabled",
                  profile.environment.treble_enabled.empty() ? "prop:ro.treble.enabled"
                                                             : profile.environment.treble_enabled,
                  !profile.environment.treble_enabled.empty(), true, false);
}

} // namespace coreflow
