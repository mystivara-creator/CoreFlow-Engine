#include "coreflow/thermal_monitor.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>

namespace coreflow {

ThermalMonitor& ThermalMonitor::instance() {
    static ThermalMonitor monitor;
    return monitor;
}

void ThermalMonitor::discover_sensors() const {
    if (!sensor_paths_.empty()) return;

    for (int i = 0; i < 128; ++i) {
        std::string base{"/sys/class/thermal/thermal_zone"};
        base.append(std::to_string(i));

        std::ifstream type_file(base + "/type");
        std::string type;
        if (!std::getline(type_file, type)) continue;

        std::transform(type.begin(), type.end(), type.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        if (type.find("cpu") == std::string::npos &&
            type.find("soc") == std::string::npos &&
            type.find("tsens") == std::string::npos &&
            type.find("apss") == std::string::npos) {
            continue;
        }

        std::ifstream temp_file(base + "/temp");
        float raw = 0.0f;
        if (!(temp_file >> raw)) continue;
        const float celsius = raw > 1000.0f ? raw / 1000.0f : raw;
        if (celsius >= -10.0f && celsius <= 130.0f)
            sensor_paths_.push_back(base + "/temp");
    }
}

float ThermalMonitor::hottest_celsius() const {
    discover_sensors();
    float hottest = -1.0f;
    for (const auto& path : sensor_paths_) {
        std::ifstream file(path);
        float raw = 0.0f;
        if (!(file >> raw)) continue;
        const float celsius = raw > 1000.0f ? raw / 1000.0f : raw;
        if (celsius >= -10.0f && celsius <= 130.0f)
            hottest = std::max(hottest, celsius);
    }
    return hottest;
}

} // namespace coreflow
