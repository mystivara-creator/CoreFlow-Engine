#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace coreflow {

enum class RuntimeState {
    Idle,
    Normal,
    Elevated,
    Pressure,
    ThermalGuard
};

enum class Decision {
    NoAction,
    Observe,
    ReduceIntervention
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
    std::uint64_t uptime_seconds{0};
    double load1{0.0};
    long hottest_thermal_millidegrees{0};
    bool thermal_available{false};
};

struct EngineSnapshot {
    DeviceProfile profile;
    RuntimeState state{RuntimeState::Idle};
};

} // namespace coreflow
