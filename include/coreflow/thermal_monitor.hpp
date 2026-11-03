#pragma once
#include <string>
#include <vector>

namespace coreflow {

class ThermalMonitor {
public:
    static ThermalMonitor& instance();

    // Returns the hottest trusted CPU/SoC thermal zone in Celsius.
    // Negative means no trusted thermal sensor is available.
    [[nodiscard]] float hottest_celsius() const;

private:
    ThermalMonitor() = default;
    mutable std::vector<std::string> sensor_paths_;
    void discover_sensors() const;
};

} // namespace coreflow
