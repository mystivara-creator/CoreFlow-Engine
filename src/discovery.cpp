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

    char release[PROP_VALUE_MAX] = {};
    if (__system_property_get("ro.build.version.release", release) > 0) {
        profile.android_release = release;
    }

    struct utsname uts {};
    if (uname(&uts) == 0) {
        profile.kernel_release = uts.release;
        profile.abi = uts.machine;
    }

    discoverCpuPolicies(profile);
    discoverThermalZones(profile);
    discoverCharging(profile);
    discoverIo(profile);
    discoverSystemControls(profile);
    return profile;
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

        std::ifstream temp(zone.path + "/temp");
        if (temp && (temp >> zone.temperature_millidegrees)) zone.readable = true;
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
        device.path = std::string(base) + "/" + name + "/queue";
        device.name = name;
        device.read_ahead_readable = access(queue.c_str(), R_OK) == 0;
        device.read_ahead_writable = access(queue.c_str(), W_OK) == 0;
        readUnsigned(queue, device.read_ahead_kb);
        if (device.read_ahead_readable || device.read_ahead_writable)
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

} // namespace coreflow
