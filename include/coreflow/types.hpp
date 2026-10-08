#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "coreflow/environment.hpp"

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

enum class InterventionLevel : std::uint8_t {
    ObserveOnly = 0,
    Low,
    Moderate,
    High
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

enum class MutationResult {
    Skipped,
    Applied,
    Verified,
    Failed,
    RolledBack
};

struct CpuPolicy {
    int id{-1};
    std::string path;
    std::string related_cpus;
    std::string governor;
    std::vector<std::string> available_governors;
    std::uint64_t hardware_min_frequency{0};
    std::uint64_t hardware_max_frequency{0};
    std::uint64_t scaling_min_frequency{0};
    std::uint64_t scaling_max_frequency{0};
    bool readable{false};
    bool governor_writable{false};
    bool scaling_max_writable{false};
    bool energy_performance_available{false};
    std::string energy_performance_preference;
    bool boost_available{false};
};

struct ThermalZone {
    std::string path;
    std::string type;
    long temperature_millidegrees{0};
    bool readable{false};
};

struct IoDevice {
    std::string path;
    std::string name;
    std::uint64_t read_ahead_kb{0};
    bool read_ahead_readable{false};
    bool read_ahead_writable{false};
    std::uint64_t nr_requests{0};
    bool nr_requests_readable{false};
    bool nr_requests_writable{false};
    std::string scheduler;
    bool scheduler_readable{false};
    bool scheduler_writable{false};
};

struct TunableCapability {
    std::string path;
    bool readable{false};
    bool writable{false};
    bool numeric{false};
    long long value{0};
};

struct TextCapability {
    std::string path;
    bool readable{false};
    bool writable{false};
    std::string value;
};

struct ChargingCapability {
    std::string battery_path;
    bool battery_available{false};
    bool status_readable{false};
    bool telemetry_readable{false};

    bool has_input_current_limit{false};
    bool has_charge_current_limit{false};
    bool has_charge_control_limit{false};

    std::string charge_control_limit_path;
    bool charge_control_limit_readable{false};
    bool charge_control_limit_writable{false};
    bool charge_control_limit_numeric{false};
    long long charge_control_limit_value{0};
    bool charge_control_limit_min_available{false};
    bool charge_control_limit_max_available{false};
    long long charge_control_limit_min{0};
    long long charge_control_limit_max{0};
    bool charge_control_limit_range_valid{false};
    bool charge_control_limit_semantics_validated{false};
    bool charge_control_limit_mutation_ready{false};

    bool has_charging_enabled{false};
    bool has_charge_disable{false};
};

struct DeviceProfile {
    std::string android_release;
    std::string kernel_release;
    std::string abi;
    std::vector<CpuPolicy> cpu_policies;
    std::vector<ThermalZone> thermal_zones;
    std::vector<IoDevice> io_devices;

    TunableCapability vm_swappiness;
    TunableCapability uclamp_min;
    TunableCapability uclamp_max;
    TextCapability cpuset_effective_cpus;

    bool proc_available{false};
    bool sys_available{false};
    ChargingCapability charging;
    AndroidEnvironment environment;
    EnvironmentCapabilityMatrix capabilities;
};

struct RuntimeSample {
    std::uint64_t mem_available_kb{0};
    std::uint64_t mem_total_kb{0};
    double mem_available_ratio{0.0};

    std::uint64_t uptime_seconds{0};
    double load1{0.0};
    double cpu_utilization{0.0};
    bool cpu_utilization_available{false};

    long thermal_millidegrees{0};
    long hottest_thermal_millidegrees{0};
    bool thermal_available{false};
    ThermalSource thermal_source{ThermalSource::Unknown};

    bool charging{false};
    bool charging_telemetry_available{false};
    long battery_temperature_millidegrees{0};
    long battery_current_microamps{0};
    long battery_voltage_microvolts{0};
    int battery_level_percent{-1};
    std::string battery_status;

    double io_read_kb_per_sec{0.0};
    double io_write_kb_per_sec{0.0};
    bool io_activity_available{false};

    std::uint32_t process_count{0};
    std::string top_process_name;
    double top_process_cpu_ratio{0.0};
    std::uint64_t top_process_memory_kb{0};
    bool process_profile_available{false};

    Trend thermal_trend{Trend::Unknown};
    Trend memory_trend{Trend::Unknown};
    Trend load_trend{Trend::Unknown};

    double confidence{0.0};
};

struct EngineSnapshot {
    DeviceProfile profile;
    RuntimeState state{RuntimeState::Idle};
};

const char* stateName(RuntimeState);
const char* decisionName(Decision);
const char* trendName(Trend);
const char* notificationEventName(NotificationEvent);
const char* mutationResultName(MutationResult);
const char* interventionLevelName(InterventionLevel) noexcept;

} // namespace coreflow
