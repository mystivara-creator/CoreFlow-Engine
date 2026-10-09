#include "coreflow/context.hpp"

#include <algorithm>
#include <cmath>

namespace coreflow {

SystemContext ContextEngine::evaluate(
    const RuntimeSample& sample,
    RuntimeState state
) const noexcept {
    SystemContext context;
    context.state = state;

    context.thermal_headroom =
        !sample.thermal_available ||
        sample.thermal_millidegrees < 50000;

    context.memory_headroom =
        sample.mem_total_kb == 0 ||
        sample.mem_available_ratio >= 0.18;

    context.power_headroom =
        !sample.charging_telemetry_available ||
        sample.battery_level_percent < 0 ||
        sample.battery_level_percent >= 20;

    context.battery_headroom = context.power_headroom;

    context.io_headroom =
        !sample.io_activity_available ||
        (sample.io_read_kb_per_sec + sample.io_write_kb_per_sec) < 102400.0;

    context.confidence = std::clamp(sample.confidence, 0.0, 1.0);

    if (state == RuntimeState::ThermalGuard) {
        context.workload = WorkloadClass::ThermalLimited;
    } else if (!context.power_headroom || !context.battery_headroom) {
        context.workload = WorkloadClass::PowerConstrained;
    } else if (state == RuntimeState::Pressure) {
        context.workload = WorkloadClass::MemoryBound;
    } else if (sample.io_activity_available &&
               (sample.io_read_kb_per_sec + sample.io_write_kb_per_sec) >= 4096.0 &&
               (!sample.cpu_utilization_available || sample.cpu_utilization < 0.65)) {
        context.workload = WorkloadClass::IoBound;
    } else if (sample.mem_available_ratio > 0.0 && sample.mem_available_ratio < 0.20) {
        context.workload = WorkloadClass::MemoryBound;
    } else if (sample.load1 < 0.20 &&
               (!sample.cpu_utilization_available || sample.cpu_utilization < 0.15)) {
        context.workload = WorkloadClass::Idle;
    } else if (sample.cpu_utilization_available && sample.cpu_utilization >= 0.80) {
        context.workload = WorkloadClass::CpuBound;
    } else if (sample.thermal_trend == Trend::Rising && sample.cpu_utilization >= 0.65) {
        context.workload = WorkloadClass::Sustained;
    } else {
        context.workload = WorkloadClass::Interactive;
    }

    // Only non-safety states with fresh, sufficiently confident telemetry and
    // headroom may start a new optimization. ThermalGuard and Pressure are
    // hold/recovery states; restoration is handled outside this gate.
    const bool proactive_path =
        state != RuntimeState::ThermalGuard &&
        state != RuntimeState::Pressure &&
        state != RuntimeState::Idle &&
        context.thermal_headroom &&
        context.memory_headroom &&
        context.power_headroom &&
        context.io_headroom &&
        context.confidence >= 0.70;

    context.mutation_allowed_by_context = proactive_path;

    return context;
}

const char* workloadClassName(WorkloadClass workload) noexcept {
    switch (workload) {
        case WorkloadClass::Unknown: return "UNKNOWN";
        case WorkloadClass::Idle: return "IDLE";
        case WorkloadClass::Interactive: return "INTERACTIVE";
        case WorkloadClass::CpuBound: return "CPU_BOUND";
        case WorkloadClass::GpuBound: return "GPU_BOUND";
        case WorkloadClass::IoBound: return "IO_BOUND";
        case WorkloadClass::MemoryBound: return "MEMORY_BOUND";
        case WorkloadClass::Sustained: return "SUSTAINED";
        case WorkloadClass::ThermalLimited: return "THERMAL_LIMITED";
        case WorkloadClass::PowerConstrained: return "POWER_CONSTRAINED";
    }
    return "UNKNOWN";
}

} // namespace coreflow
