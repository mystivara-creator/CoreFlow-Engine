#include "coreflow/discovery.hpp"

#include <cstdlib>
#include <cerrno>
#include <dirent.h>
#include <fstream>
#include <string>
#include <sys/utsname.h>
#include <unistd.h>

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

    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')
        ++end;

    if (*end != '\0') return false;

    value = parsed;
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
    discoverCharging(profile);

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


void EnvironmentDiscovery::discoverCharging(DeviceProfile& profile) const {
    constexpr const char* base = "/sys/class/power_supply";

    DIR* dir = opendir(base);
    if (!dir) return;

    while (dirent* entry = readdir(dir)) {
        const std::string name(entry->d_name);
        if (name == "." || name == "..")
            continue;

        const std::string path = std::string(base) + "/" + name;

        std::string type;
        if (!readText(path + "/type", type) || type != "Battery")
            continue;

        profile.charging.battery_path = path;
        profile.charging.battery_available = true;
        profile.charging.status_readable =
            access((path + "/status").c_str(), R_OK) == 0;
        profile.charging.telemetry_readable =
            access((path + "/current_now").c_str(), R_OK) == 0 ||
            access((path + "/voltage_now").c_str(), R_OK) == 0 ||
            access((path + "/temp").c_str(), R_OK) == 0;

        // Read-only capability discovery. The presence of a node is recorded;
        // no value is read as a control setting and nothing is written.
        profile.charging.has_input_current_limit =
            access((path + "/input_current_limit").c_str(), F_OK) == 0;
        profile.charging.has_charge_current_limit =
            access((path + "/constant_charge_current").c_str(), F_OK) == 0;
        const std::string controlLimitPath =
            path + "/charge_control_limit";

        profile.charging.has_charge_control_limit =
            access(controlLimitPath.c_str(), F_OK) == 0;

        if (profile.charging.has_charge_control_limit) {
            profile.charging.charge_control_limit_path = controlLimitPath;
            profile.charging.charge_control_limit_readable =
                access(controlLimitPath.c_str(), R_OK) == 0;
            profile.charging.charge_control_limit_writable =
                access(controlLimitPath.c_str(), W_OK) == 0;
            profile.charging.charge_control_limit_numeric =
                readSigned(controlLimitPath,
                           profile.charging.charge_control_limit_value);

            const std::string minPath =
                path + "/charge_control_limit_min";
            const std::string maxPath =
                path + "/charge_control_limit_max";

            profile.charging.charge_control_limit_min_available =
                readSigned(minPath,
                           profile.charging.charge_control_limit_min);
            profile.charging.charge_control_limit_max_available =
                readSigned(maxPath,
                           profile.charging.charge_control_limit_max);

            // Presence/readability/numeric checks do not prove the semantic
            // meaning of the control. Keep this false until a platform
            // contract explicitly validates its units and behavior.
            profile.charging.charge_control_limit_semantics_validated = false;
        }

        profile.charging.has_charging_enabled =
            access((path + "/charging_enabled").c_str(), F_OK) == 0;
        profile.charging.has_charge_disable =
            access((path + "/charge_disable").c_str(), F_OK) == 0;

        break;
    }

    closedir(dir);
}

} // namespace coreflow
