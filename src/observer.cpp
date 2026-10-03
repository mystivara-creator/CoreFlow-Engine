#include "coreflow/observer.hpp"

#include <dirent.h>
#include <fstream>
#include <string>

namespace coreflow {

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
}

void RuntimeObserver::readLoad(RuntimeSample& sample) const {
    std::ifstream file("/proc/loadavg");
    if (file)
        file >> sample.load1;
}

void RuntimeObserver::readThermal(RuntimeSample& sample) const {
    constexpr const char* base = "/sys/class/thermal";

    DIR* dir = opendir(base);
    if (!dir) return;

    long hottest = 0;

    while (dirent* entry = readdir(dir)) {
        const std::string name(entry->d_name);

        if (name.rfind("thermal_zone", 0) != 0)
            continue;

        std::ifstream temp(
            std::string(base) + "/" + name + "/temp"
        );

        long value = 0;
        if (temp && (temp >> value)) {
            if (!sample.thermal_available || value > hottest)
                hottest = value;

            sample.thermal_available = true;
        }
    }

    closedir(dir);
    sample.hottest_thermal_millidegrees = hottest;
}

RuntimeSample RuntimeObserver::sample() const {
    RuntimeSample sample;

    std::ifstream uptime("/proc/uptime");
    double seconds = 0.0;

    if (uptime && (uptime >> seconds) && seconds >= 0.0)
        sample.uptime_seconds =
            static_cast<std::uint64_t>(seconds);

    readMemory(sample);
    readLoad(sample);
    readThermal(sample);

    return sample;
}

} // namespace coreflow
