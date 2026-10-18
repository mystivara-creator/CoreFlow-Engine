#include "coreflow/discovery.hpp"
#include "coreflow/consultation.hpp"
#include "coreflow/tunable_discovery.hpp"
#include "coreflow/vendor_knowledge.hpp"
#include "coreflow/kernel_roots.hpp"

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

void addCapability(EnvironmentCapabilityMatrix& matrix, ResourceDomain domain,
                   const std::string& name, const std::string& path,
                   bool exists, bool readable, bool writable,
                   bool permission_granted = false) {
    // One record per node: later probes of the same path add nothing new.
    for (const auto& existing : matrix.resources) {
        if (!path.empty() && existing.path == path) return;
    }
    // Real writability: root passes access(W_OK) on read-only sysfs nodes, so
    // filesystem paths must also be regular files with a kernel write bit.
    if (writable && !path.empty() && path.front() == '/') {
        writable = TunableDiscovery::kernelWritable(path);
    }
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
    // The eye asks the brain whether this surface class is admissible. It does
    // not decide admissibility itself; the consultation module owns that rule.
    cap.policy_authorized =
        classifySurface(domain, name, path) == SurfaceClass::Admissible;
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
    discoverGpu(profile);
    finalizeCapabilities(profile);
    // The tunable walk is the single authority for writable kernel surfaces.
    discoverTunables(profile);
    deriveEnvironmentAvailability(profile);
    profile.capabilities.environment = profile.environment;
    profile.expanded_domains = HighCapabilityExpansion::assess(profile.capabilities);
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

    // cgroup v2 is whatever the kernel mounted as cgroup2, not a guessed path.
    profile.environment.cgroup_v2 =
        hasFilesystemType(readKernelMounts(""), "cgroup2");
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
        // Same writability rule as ResourceActuator / TunableDiscovery: lstat
        // (no symlink follow) + kernel write-mode bit. Root always passes
        // access(W_OK) via CAP_DAC_OVERRIDE on read-only sysfs nodes.
        {
            const std::string gov = policy.path + "/scaling_governor";
            const std::string maxf = policy.path + "/scaling_max_freq";
            struct stat st {};
            policy.governor_writable =
                ::lstat(gov.c_str(), &st) == 0 && S_ISREG(st.st_mode) &&
                (st.st_mode & 0222) != 0 && ::access(gov.c_str(), W_OK) == 0;
            policy.scaling_max_writable =
                ::lstat(maxf.c_str(), &st) == 0 && S_ISREG(st.st_mode) &&
                (st.st_mode & 0222) != 0 && ::access(maxf.c_str(), W_OK) == 0;
        }

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

void EnvironmentDiscovery::discoverGpu(DeviceProfile& profile) const {
    constexpr const char* base = "/sys/class/devfreq";
    DIR* dir = opendir(base);
    if (!dir) return;
    profile.environment.devfreq_available = true;

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
    // Inventory entries only. Tunable nodes come from discoverTunables(), so
    // no parameter is listed here by name or by path.
    auto& matrix = profile.capabilities;

    addCapability(matrix, ResourceDomain::CpuFreq, "policies",
                  "/sys/devices/system/cpu/cpufreq",
                  !profile.cpu_policies.empty(), !profile.cpu_policies.empty(), false);

    addCapability(matrix, ResourceDomain::Thermal, "thermal-zones",
                  "/sys/class/thermal", !profile.thermal_zones.empty(),
                  !profile.thermal_zones.empty(), false);
    addCapability(matrix, ResourceDomain::Charging, "battery",
                  profile.charging.battery_path, profile.charging.battery_available,
                  profile.charging.status_readable || profile.charging.telemetry_readable, false);
    addCapability(matrix, ResourceDomain::Io, "block-queues", "/sys/block",
                  !profile.io_devices.empty(), !profile.io_devices.empty(), false);
    addCapability(matrix, ResourceDomain::Power, "power-supply", "/sys/class/power_supply",
                  profile.charging.battery_available, profile.charging.status_readable, false);

    // Pressure-stall inventory: every file the kernel publishes under /proc/pressure.
    if (DIR* dir = opendir("/proc/pressure")) {
        while (dirent* entry = readdir(dir)) {
            const std::string name(entry->d_name);
            if (name == "." || name == "..") continue;
            const std::string path = "/proc/pressure/" + name;
            const bool readable = access(path.c_str(), R_OK) == 0;
            addCapability(matrix, ResourceDomain::AndroidRuntime, "psi." + name, path,
                          readable, readable, false);
        }
        closedir(dir);
    }

    addCapability(matrix, ResourceDomain::AndroidRuntime, "android-properties",
                  "system-properties",
                  !profile.environment.release.empty() || !profile.environment.sdk_level.empty(),
                  true, false);

    // Build identity markers (observe-only evidence for vendor-specific policy).
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

void EnvironmentDiscovery::discoverTunables(DeviceProfile& profile) const {
    VendorKnowledge vendor;
    (void)vendor.harvest();
    const TunableDiscoveryStats stats = TunableDiscovery{}.discover(profile.capabilities, vendor);
    profile.vendor_files_parsed = vendor.filesParsed();
    profile.vendor_knobs = vendor.knobs().size();
    profile.tunables_walked = stats.walked;
    profile.tunables_added = stats.added;
    profile.tunables_fatal_skipped = stats.fatal_skipped;
    profile.tunables_unclaimed = stats.unclaimed;
    profile.tunables_truncated = stats.truncated;
}

void EnvironmentDiscovery::deriveEnvironmentAvailability(DeviceProfile& profile) const {
    // Availability is a fact about discovered, readable capabilities. It is not
    // inferred from a named path.
    auto anyReadable = [&profile](ResourceDomain domain) {
        for (const auto& cap : profile.capabilities.resources) {
            if (cap.domain == domain && cap.exists && cap.readable) return true;
        }
        return false;
    };
    profile.environment.uclamp_available = anyReadable(ResourceDomain::UClamp);
    profile.environment.cpuset_available = anyReadable(ResourceDomain::CpuSet);
    profile.environment.scheduler_controls_available = anyReadable(ResourceDomain::Scheduler);
}

} // namespace coreflow
