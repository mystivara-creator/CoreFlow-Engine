#pragma once

#include <cstdint>

#include "coreflow/types.hpp"

namespace coreflow {

enum class WorkloadClass : std::uint8_t {
    Unknown = 0,
    Idle,
    Interactive,
    CpuBound,
    GpuBound,
    IoBound,
    MemoryBound,
    Sustained,
    ThermalLimited,
    PowerConstrained
};

struct SystemContext {
    WorkloadClass workload{WorkloadClass::Unknown};
    RuntimeState state{RuntimeState::Idle};
    bool thermal_headroom{false};
    bool memory_headroom{false};
    bool power_headroom{false};
    bool io_headroom{true};
    bool battery_headroom{true};
    // Full proactive tuning: non-idle, confident, with thermal/memory/power headroom.
    bool mutation_allowed_by_context{false};
    // Stabilizing-only tuning: the system is stressed (thermal/memory) but telemetry
    // is trustworthy. Only small, load-reducing writes are justified, never a
    // more aggressive setting. This is the adaptive alternative to a hard block.
    bool stabilizing_allowed_by_context{false};
    double confidence{0.0};
};

class ContextEngine final {
public:
    SystemContext evaluate(const RuntimeSample& sample,
                           RuntimeState state) const noexcept;
};

const char* workloadClassName(WorkloadClass workload) noexcept;

} // namespace coreflow
