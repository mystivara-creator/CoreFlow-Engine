#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace coreflow {

class ThermalMonitor {
public:
    static ThermalMonitor& instance();
    explicit ThermalMonitor(std::string_view sysfs_root = "/sys/class/thermal");

    // Returns the hottest trusted CPU/SoC thermal zone in Celsius.
    // Negative means no trusted thermal sensor is available.
    [[nodiscard]] float hottest_celsius() const;

private:
    std::string sysfs_root_;
    mutable std::vector<std::string> sensor_paths_;
    void discover_sensors() const;
};

} // namespace coreflow
