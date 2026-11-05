#pragma once
#include <string>
#include <chrono>
#include <string_view>
#include <vector>

namespace coreflow {
struct ThermalSensorReading {
    std::string id, type;
    int category{0}; // 0 diagnostic/unknown, 1 compute, 2 battery, 3 skin, 4 board/power
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
    bool degraded{false};
    bool platform_status_valid{false};
    bool headroom_valid{false};
    float headroom_value{-1.0f};
    bool sysfs_trip_policy_valid{false};
    bool sensor_readings_complete{true};
    bool fallback_active{false};
};
struct MultiSensorProfile {
    float cpu_average{-1.0f};
    float cpu_peak{-1.0f};
    float gpu_average{-1.0f};
    float board_average{-1.0f};
    float battery{-1.0f};
    float skin{-1.0f};
    float composite_soc{-1.0f};
    size_t valid_cpu_count{0};
    size_t valid_gpu_count{0};
    size_t total_valid_sensors{0};
};

[[nodiscard]] ThermalAssessment assessment_from_android_severity(int severity);
[[nodiscard]] ThermalAssessment apply_android_headroom(ThermalAssessment base, float headroom);
[[nodiscard]] ThermalAssessment fuse_thermal_assessments(ThermalAssessment platform, ThermalAssessment sensors);

class ThermalMonitor {
public:
    static ThermalMonitor& instance();
    explicit ThermalMonitor(std::string_view sysfs_root = "/sys/class/thermal");
    [[nodiscard]] float hottest_celsius() const;
    [[nodiscard]] float composite_celsius() const;
    [[nodiscard]] float battery_celsius() const;
    [[nodiscard]] float skin_celsius() const;
    [[nodiscard]] MultiSensorProfile multi_sensor_profile() const;
    [[nodiscard]] ThermalAssessment assess(bool headroom = true) const;
private:
    struct Sensor {
        std::string path;
        std::string temp_path;
        std::string type;
        int category{0};
        bool is_hwmon{false};
    };
    std::string sysfs_root_;
    mutable std::vector<Sensor> sensors_;
    mutable std::chrono::steady_clock::time_point discovery_at_{};
    void discover_sensors() const;
    float hottest_category(int category) const;
};
}
