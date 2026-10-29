#include "coreflow/runtime_intelligence.hpp"
#include "coreflow/pressure_limits.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace coreflow {
namespace {
bool fraction(double v) noexcept { return std::isfinite(v) && v >= 0.0 && v <= 1.0; }
}
RuntimeDiagnosis diagnoseRuntime(const RuntimeSample& s) noexcept {
    RuntimeDiagnosis d;
    d.rendering_available = s.frame_jank_available && fraction(s.frame_jank_rate);
    d.rendering_disturbed = d.rendering_available && s.frame_jank_rate >= 0.05;
    // Thresholds classify evidence. They are not tunable values written to hardware.
    std::array<double, 5> evidence{};
    if (s.thermal_available && s.throttling_detected && fraction(s.throttle_ratio))
        evidence[0] = s.throttle_ratio;
    if (s.memory_stall_detected && fraction(s.memory_stall_score))
        evidence[1] = s.memory_stall_score;
    if (s.psi_available) {
        if (fraction(s.psi_memory_some_avg10))
            evidence[1] = std::max(evidence[1], s.psi_memory_some_avg10);
        if (fraction(s.psi_io_some_avg10)) evidence[2] = s.psi_io_some_avg10;
        if (fraction(s.psi_cpu_some_avg10)) evidence[3] = s.psi_cpu_some_avg10;
    }
    if (s.io_latency_available && std::isfinite(s.io_avg_latency_ms) &&
        s.io_avg_latency_ms >= pressure_limits::kIoLatencyEnterMs)
        evidence[2] = std::max(evidence[2], std::min(1.0,
            s.io_avg_latency_ms / (4.0 * pressure_limits::kIoLatencyEnterMs)));
    // High utilization alone is not proof of a GPU bottleneck. Require jank too.
    if (d.rendering_disturbed && s.gpu_busy_available && fraction(s.gpu_busy_ratio) &&
        s.gpu_busy_ratio >= 0.80) evidence[4] = s.gpu_busy_ratio * s.frame_jank_rate;
    const bool measured = s.psi_available || s.memory_stall_detected ||
        s.io_latency_available || s.thermal_available || s.cpu_utilization_available ||
        s.gpu_busy_available || d.rendering_available;
    const auto best = std::max_element(evidence.begin(), evidence.end());
    if (*best < 0.05) {
        d.dominant = measured ? RuntimePressure::None : RuntimePressure::Unknown;
        return d;
    }
    constexpr std::array<RuntimePressure, 5> kinds{RuntimePressure::Thermal,
        RuntimePressure::Memory, RuntimePressure::Io, RuntimePressure::Cpu,
        RuntimePressure::Gpu};
    d.dominant = kinds[static_cast<std::size_t>(best - evidence.begin())];
    d.severity = *best;
    std::size_t similar = 0;
    for (const double value : evidence) if (value >= *best * 0.75) ++similar;
    d.conflicting_signals = similar > 1U;
    d.confidence = fraction(s.confidence) ? s.confidence : 0.0;
    if (d.conflicting_signals) d.confidence *= 0.5;
    return d;
}
const char* runtimePressureName(RuntimePressure p) noexcept {
    switch (p) {
        case RuntimePressure::Unknown: return "UNKNOWN";
        case RuntimePressure::None: return "NO_MEASURED_PRESSURE";
        case RuntimePressure::Thermal: return "THERMAL";
        case RuntimePressure::Memory: return "MEMORY_STALL";
        case RuntimePressure::Io: return "IO_STALL";
        case RuntimePressure::Cpu: return "CPU_STALL";
        case RuntimePressure::Gpu: return "GPU_RENDER_PRESSURE";
    }
    return "UNKNOWN";
}
double unrelatedResourcePenalty(const RuntimeDiagnosis& d, ResourceDomain domain) noexcept {
    if (d.conflicting_signals || !fraction(d.confidence) ||
        !fraction(d.severity) || d.confidence < 0.60) return 0.0;
    bool related = true;
    switch (d.dominant) {
        case RuntimePressure::Memory: related = domain == ResourceDomain::Memory; break;
        case RuntimePressure::Io: related = domain == ResourceDomain::Io; break;
        case RuntimePressure::Gpu: related = domain == ResourceDomain::Gpu; break;
        case RuntimePressure::Cpu:
            related = domain == ResourceDomain::CpuFreq || domain == ResourceDomain::UClamp ||
                      domain == ResourceDomain::Scheduler || domain == ResourceDomain::CpuSet;
            break;
        default: break; // thermal stabilization can legitimately span every domain
    }
    return related ? 0.0 : 0.08 * std::clamp(d.severity, 0.0, 1.0);
}
} // namespace coreflow
