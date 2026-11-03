#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace coreflow {
struct ThermalAssessment {
    bool available{false};
    bool emergency{false};
    double pressure{0}; // Policy-relative index, never a temperature in Celsius.
    int severity{-1};
    std::string source{"unavailable"};
    std::string sensor;
};
[[nodiscard]] ThermalAssessment assessment_from_android_severity(int severity);
class ThermalMonitor {
public:
    static ThermalMonitor& instance();
    explicit ThermalMonitor(std::string_view sysfs_root = "/sys/class/thermal");
    [[nodiscard]] float hottest_celsius() const;
    [[nodiscard]] float battery_celsius() const;
    [[nodiscard]] float skin_celsius() const;
    [[nodiscard]] ThermalAssessment assess(bool headroom = true) const;
private:
    struct Sensor { std::string path, type; int category; };
    std::string sysfs_root_;
    mutable std::vector<Sensor> sensors_;
    void discover_sensors() const;
    float hottest_category(int category) const;
};
}
