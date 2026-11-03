#pragma once
#include <string>
#include <chrono>
#include <string_view>
#include <vector>

namespace coreflow {
struct ThermalSensorReading {
    std::string id, type;
    int category{0}; // 1 compute, 2 battery, 3 skin
    float celsius{-1};
    bool valid{false}, policy_available{false};
    double pressure{0};
};
struct ThermalAssessment {
    bool available{false};
    bool emergency{false};
    double pressure{0}; // Policy-relative index, never a temperature in Celsius.
    int severity{-1};
    std::string source{"unavailable"};
    std::string sensor;
    std::vector<ThermalSensorReading> readings;
    bool incomplete{false};
};
[[nodiscard]] ThermalAssessment assessment_from_android_severity(int severity);
[[nodiscard]] ThermalAssessment fuse_thermal_assessments(ThermalAssessment platform, ThermalAssessment sensors);
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
    mutable std::chrono::steady_clock::time_point discovery_at_{};
    void discover_sensors() const;
    float hottest_category(int category) const;
};
}
