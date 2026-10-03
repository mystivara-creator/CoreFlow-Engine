#include "coreflow/discovery.hpp"

#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <string>
#include <sys/utsname.h>

namespace coreflow {
namespace {

bool readText(const std::string& path, std::string& out) {
    std::ifstream file(path);
    if (!file) return false;
    std::getline(file, out);
    return !out.empty();
}

std::uint64_t readUnsigned(const std::string& path) {
    std::ifstream file(path);
    std::uint64_t value = 0;
    if (file) file >> value;
    return value;
}

bool directoryExists(const char* path) {
    DIR* dir = opendir(path);
    if (!dir) return false;
    closedir(dir);
    return true;
}

} // namespace

DeviceProfile EnvironmentDiscovery::discover() const {
    DeviceProfile profile;

    profile.proc_available = directoryExists("/proc");
    profile.sys_available = directoryExists("/sys");

    // build.prop is treated as optional telemetry, not as a required
    // source of truth. Modern Android properties may not be readable here.
    readText("/system/build.prop", profile.android_release);

    struct utsname uts {};
    if (uname(&uts) == 0) {
        profile.kernel_release = uts.release;
        profile.abi = uts.machine;
    }

    discoverCpuPolicies(profile);
    discoverThermalZones(profile);

    return profile;
}

void EnvironmentDiscovery::discoverCpuPolicies(DeviceProfile& profile) const {
    constexpr const char* base = "/sys/devices/system/cpu/cpufreq";

    DIR* dir = opendir(base);
    if (!dir) return;

    while (dirent* entry = readdir(dir)) {
        const std::string name(entry->d_name);

        if (name.rfind("policy", 0) != 0 || name.size() <= 6)
            continue;

        CpuPolicy policy;
        policy.id = std::atoi(name.c_str() + 6);
        policy.path = std::string(base) + "/" + name;

        std::string governor;
        policy.readable = readText(
            policy.path + "/scaling_governor", governor
        );

        if (policy.readable)
            policy.governor = governor;

        policy.min_frequency =
            readUnsigned(policy.path + "/cpuinfo_min_freq");

        policy.max_frequency =
            readUnsigned(policy.path + "/cpuinfo_max_freq");

        profile.cpu_policies.push_back(policy);
    }

    closedir(dir);
}

void EnvironmentDiscovery::discoverThermalZones(DeviceProfile& profile) const {
    constexpr const char* base = "/sys/class/thermal";

    DIR* dir = opendir(base);
    if (!dir) return;

    while (dirent* entry = readdir(dir)) {
        const std::string name(entry->d_name);

        if (name.rfind("thermal_zone", 0) != 0)
            continue;

        ThermalZone zone;
        zone.path = std::string(base) + "/" + name;

        readText(zone.path + "/type", zone.type);

        std::ifstream temp(zone.path + "/temp");
        if (temp) {
            temp >> zone.temperature_millidegrees;
            zone.readable = true;
        }

        profile.thermal_zones.push_back(zone);
    }

    closedir(dir);
}

} // namespace coreflow
