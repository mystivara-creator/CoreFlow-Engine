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

    // Balance: allow proactive path up to just below ThermalGuard enter (55°C).
    // 52°C still leaves margin; 50°C was so tight that Warming devices almost
    // never saw mutation_allowed_by_context on real SoCs.
    context.thermal_headroom =
        !sample.thermal_available ||
        sample.thermal_millidegrees < 52000;

    // Balance: Pressure enters at 0.10. Requiring 0.18 left a wide dead band
    // where the device was neither "stressed enough" for stabilizing nor
    // "safe enough" for proactive tuning. 0.14 keeps a real safety margin.
    context.memory_headroom =
        sample.mem_total_kb == 0 ||
        sample.mem_available_ratio >= 0.14;

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
    } else if (sample.cpu_utilization_available && sample.cpu_utilization >= 0.72) {
        // Balance: 0.80 was rarely sustained on mobile; 0.72 still means clear load.
        context.workload = WorkloadClass::CpuBound;
    } else if (sample.thermal_trend == Trend::Rising && sample.cpu_utilization >= 0.60) {
        context.workload = WorkloadClass::Sustained;
    } else {
        context.workload = WorkloadClass::Interactive;
    }

    // Safety is graded, not binary:
    //  * Proactive tuning needs a non-safety state with headroom and confident
    //    telemetry. Heavy I/O is not a reason to refuse tuning (it is when tuning
    //    helps), so io_headroom is informational only.
    //  * Under thermal/memory stress the engine may still make small, load-reducing
    //    writes (stabilizing). Whether a given write qualifies is decided by the
    //    EffectModel; this gate only says that stabilization is permitted.
    //  * Idle never tunes, and low confidence blocks both paths.
    const bool safety_state =
        state == RuntimeState::ThermalGuard || state == RuntimeState::Pressure;
    // Balance: 0.70 blocked too many real-device samples whose telemetry is
    // usable but not perfect. 0.60 still rejects noisy/unknown sensors.
    const bool confident = context.confidence >= 0.60;
    const bool stressed =
        safety_state || !context.thermal_headroom || !context.memory_headroom;

    context.mutation_allowed_by_context =
        !safety_state &&
        state != RuntimeState::Idle &&
        context.thermal_headroom &&
        context.memory_headroom &&
        context.power_headroom &&
        confident;

    context.stabilizing_allowed_by_context =
        state != RuntimeState::Idle && confident && stressed;

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
