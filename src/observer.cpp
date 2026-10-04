#include "coreflow/observer.hpp"

#include <algorithm>
#include <cctype>
#include <dirent.h>
#include <fstream>
#include <string>

namespace coreflow {
namespace {

std::string lowerCopy(const std::string& input) {
    std::string out = input;
    std::transform(
        out.begin(), out.end(), out.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        }
    );
    return out;
}

bool isPrimaryThermalType(const std::string& type) {
    const std::string t = lowerCopy(type);

    return t.find("cpu") != std::string::npos ||
           t.find("soc") != std::string::npos ||
           t.find("gpu") != std::string::npos ||
           t.find("tsens") != std::string::npos;
}

long readLong(const std::string& path, bool& ok) {
    std::ifstream file(path);
    long value = 0;
    ok = static_cast<bool>(file >> value);
    return value;
}

long normalizeBatteryTemperature(long raw) {
    // Android power_supply temperature is commonly reported in tenths
    // of a degree Celsius (e.g. 380 == 38.0C), while some vendor nodes
    // expose millidegrees directly. Keep this read-only and conservative.
    if (raw > -1000 && raw < 1000)
        return raw * 100;

    return raw;
}

} // namespace

void RuntimeObserver::readMemory(RuntimeSample& sample) const {
    std::ifstream file("/proc/meminfo");
    if (!file) return;

    std::string key;
    std::uint64_t value = 0;
    std::string unit;

    while (file >> key >> value >> unit) {
        if (key == "MemTotal:")
            sample.mem_total_kb = value;
        else if (key == "MemAvailable:")
            sample.mem_available_kb = value;
    }

    if (sample.mem_total_kb > 0) {
        sample.mem_available_ratio =
            static_cast<double>(sample.mem_available_kb) /
            static_cast<double>(sample.mem_total_kb);
    }
}

void RuntimeObserver::readLoad(RuntimeSample& sample) const {
    std::ifstream file("/proc/loadavg");
    if (file)
        file >> sample.load1;
}

void RuntimeObserver::readThermal(
    RuntimeSample& sample,
    const DeviceProfile& profile
) const {
    long hottest = 0;
    long primary_hottest = 0;
    bool any = false;
    bool primary = false;

    for (const ThermalZone& zone : profile.thermal_zones) {
        bool ok = false;
        const long value = readLong(zone.path + "/temp", ok);

        if (!ok) continue;

        any = true;
        if (value > hottest)
            hottest = value;

        if (isPrimaryThermalType(zone.type)) {
            primary = true;
            if (value > primary_hottest)
                primary_hottest = value;
        }
    }

    if (!any) return;

    sample.thermal_available = true;
    sample.hottest_thermal_millidegrees = hottest;

    if (primary) {
        sample.thermal_millidegrees = primary_hottest;
        sample.thermal_source = ThermalSource::Primary;
    } else {
        sample.thermal_millidegrees = hottest;
        sample.thermal_source = ThermalSource::Fallback;
    }
}

void RuntimeObserver::readCharging(RuntimeSample& sample) const {
    constexpr const char* base = "/sys/class/power_supply";

    DIR* dir = opendir(base);
    if (!dir) return;

    bool status_found = false;
    bool battery_found = false;

    while (dirent* entry = readdir(dir)) {
        const std::string name(entry->d_name);

        if (name == "." || name == "..")
            continue;

        const std::string path =
            std::string(base) + "/" + name;

        std::ifstream type_file(path + "/type");
        std::string type;
        std::getline(type_file, type);

        if (type != "Battery")
            continue;

        battery_found = true;

        std::ifstream status_file(path + "/status");
        std::string status;

        if (status_file && std::getline(status_file, status)) {
            status_found = true;
            // Full means the battery is full, not that active charging
            // current is necessarily flowing. Keep charging telemetry
            // conservative and report only the active Charging state.
            sample.charging = (status == "Charging");
        }

        bool ok = false;

        const long temp =
            readLong(path + "/temp", ok);

        if (ok)
            sample.battery_temperature_millidegrees =
                normalizeBatteryTemperature(temp);

        const long current =
            readLong(path + "/current_now", ok);

        if (ok)
            sample.battery_current_microamps = current;

        const long voltage =
            readLong(path + "/voltage_now", ok);

        if (ok)
            sample.battery_voltage_microvolts = voltage;

        break;
    }

    closedir(dir);

    sample.charging_telemetry_available =
        battery_found && (status_found ||
                          sample.battery_temperature_millidegrees != 0 ||
                          sample.battery_current_microamps != 0 ||
                          sample.battery_voltage_microvolts != 0);
}

RuntimeSample RuntimeObserver::sample(
    const DeviceProfile& profile
) const {
    RuntimeSample sample;

    std::ifstream uptime("/proc/uptime");
    double seconds = 0.0;

    if (uptime && (uptime >> seconds) && seconds >= 0.0) {
        sample.uptime_seconds =
            static_cast<std::uint64_t>(seconds);
    }

    readMemory(sample);
    readLoad(sample);
    readThermal(sample, profile);
    readCharging(sample);

    return sample;
}

} // namespace coreflow
