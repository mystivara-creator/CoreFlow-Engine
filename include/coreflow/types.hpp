#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace coreflow {

enum class RuntimeState {
    Idle,
    Normal,
    Warming,
    Elevated,
    Pressure,
    ThermalGuard
};

enum class Decision {
    NoAction,
    Observe,
    Notify,
    ReduceIntervention
};

enum class Trend {
    Unknown,
    Rising,
    Stable,
    Falling
};

enum class ThermalSource {
    Unknown,
    Primary,
    Fallback
};

enum class NotificationEvent {
    None,
    ThermalWarming,
    ThermalWarning,
    ThermalGuard,
    ChargingProtection
};

struct CpuPolicy {
    int id{-1};
    std::string path;
    std::string governor;
    std::uint64_t min_frequency{0};
    std::uint64_t max_frequency{0};
    bool readable{false};
};

struct ThermalZone {
    std::string path;
    std::string type;
    long temperature_millidegrees{0};
    bool readable{false};
};

struct DeviceProfile {
    std::string android_release;
    std::string kernel_release;
    std::string abi;
    std::vector<CpuPolicy> cpu_policies;
    std::vector<ThermalZone> thermal_zones;
    bool proc_available{false};
    bool sys_available{false};
};

struct RuntimeSample {
    std::uint64_t mem_available_kb{0};
    std::uint64_t mem_total_kb{0};
    double mem_available_ratio{0.0};

    std::uint64_t uptime_seconds{0};
    double load1{0.0};

    long thermal_millidegrees{0};
    long hottest_thermal_millidegrees{0};
    bool thermal_available{false};
    ThermalSource thermal_source{ThermalSource::Unknown};

    bool charging{false};
    bool charging_telemetry_available{false};
    long battery_temperature_millidegrees{0};
    long battery_current_microamps{0};
    long battery_voltage_microvolts{0};

    Trend thermal_trend{Trend::Unknown};
    Trend memory_trend{Trend::Unknown};
    Trend load_trend{Trend::Unknown};

    double confidence{0.0};
};

struct EngineSnapshot {
    DeviceProfile profile;
    RuntimeState state{RuntimeState::Idle};
};

const char* trendName(Trend);
const char* notificationEventName(NotificationEvent);

} // namespace coreflow
