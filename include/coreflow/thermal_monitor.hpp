#pragma once
#include <string>
#include <chrono>
#include <string_view>
#include <vector>
#include <array>
#include <cstdint>
#include <unordered_map>

namespace coreflow {
struct ThermalSensorReading {
    std::string id, type;
    int category{0}; // 0 diagnostic/unknown, 1 compute, 2 battery, 3 skin, 4 board/power
    float celsius{-1};
    bool valid{false}, policy_available{false};
    double pressure{0};
    double velocity_celsius_per_sec{0.0};
    bool velocity_valid{false};
    uint32_t velocity_samples_count{0};
    double velocity_sample_duration_sec{0.0};
};
struct TemperatureTrendSample {
    float celsius{-1.0f};
    uint64_t timestamp_ns{0};
};

struct TemperatureVelocityReading {
    double velocity_celsius_per_sec{0.0}; // d(temp)/dt in °C/s. Negative = cooling, positive = warming.
    bool valid{false};                   // True only if backed by valid actual physical sensor readings.
    uint32_t valid_samples_count{0};     // Number of valid actual temperature sensor samples in window.
    double sample_duration_sec{0.0};     // Time delta elapsed across the samples in seconds.
    float current_temperature_celsius{-1.0f}; // Most recent valid temperature in °C.
    uint64_t timestamp_ns{0};            // Monotonic timestamp of the reading in nanoseconds.
};

class TemperatureTrendTracker {
public:
    explicit TemperatureTrendTracker(size_t capacity = 8) noexcept;

    // Records an actual physical temperature reading with an explicit nanosecond monotonic timestamp.
    // Invalid readings (celsius < 0.0f, NaN, or non-finite) do not enter history.
    void record_sample(float celsius, uint64_t timestamp_ns) noexcept;

    // Evaluates current temperature velocity in °C/s via linear regression across the sample window.
    [[nodiscard]] TemperatureVelocityReading evaluate_velocity() const noexcept;

    // Clears all history (e.g. on sensor failure, stall, or state reset).
    void reset() noexcept;

    [[nodiscard]] size_t sample_count() const noexcept { return count_; }

private:
    static constexpr size_t kMaxSamples = 16;
    std::array<TemperatureTrendSample, kMaxSamples> samples_{};
    size_t count_{0};
    size_t capacity_{8};
};

struct ThermalAssessment {
    bool available{false};
    bool emergency{false};
    double pressure{0}; // Policy-relative index, never a temperature in Celsius.
    int severity{-1};
    std::string source{"unavailable"};
    std::string sensor;
    std::vector<ThermalSensorReading> readings;
    TemperatureVelocityReading temperature_velocity{};
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
    explicit ThermalMonitor(std::string_view sysfs_root = "/sys/class/thermal");
    [[nodiscard]] float hottest_celsius() const;
    [[nodiscard]] float composite_celsius() const;
    [[nodiscard]] float battery_celsius() const;
    [[nodiscard]] float skin_celsius() const;
    [[nodiscard]] MultiSensorProfile multi_sensor_profile() const;
    [[nodiscard]] ThermalAssessment assess(bool headroom = true) const;
    [[nodiscard]] TemperatureVelocityReading current_temperature_velocity() const;
    void record_temperature_sample(float celsius, uint64_t timestamp_ns) const;
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
    mutable TemperatureTrendTracker trend_tracker_{8};
    mutable std::unordered_map<std::string, TemperatureTrendTracker> sensor_trend_trackers_;
    void discover_sensors() const;
    float hottest_category(int category) const;
};
}
