#pragma once
#include "coreflow/types.hpp"
#include "coreflow/environment.hpp"
#include "coreflow/pressure_limits.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

namespace coreflow {
struct CandidateFunnel {
    bool assessed{false};
    std::size_t discovered{0}, structural{0}, baseline{0}, granted{0}, relevant{0}, ranked{0}, selectable{0}, history_held{0};
};
struct PlanOption {
    bool valid{false};
    ResourceDomain domain{ResourceDomain::Vendor};
    std::string name, path, current, requested, reason;
    double benefit{0.0}, risk{1.0}, confidence{0.0}, utility{0.0};
};
inline bool domainNeedsTuning(const RuntimeSample& s, RuntimeState state, ResourceDomain domain) noexcept {
    const auto fraction = [](double v) { return std::isfinite(v) && v >= 0. && v <= 1.; };
    if (state == RuntimeState::ThermalGuard) return true; // per-target stabilization still mandatory
    const bool memory = (s.mem_total_kb > 0U && fraction(s.mem_available_ratio) && s.mem_available_ratio < .20) ||
        (s.memory_stall_detected && fraction(s.memory_stall_score) && s.memory_stall_score >= .10) ||
        (s.psi_available && fraction(s.psi_memory_some_avg10) && s.psi_memory_some_avg10 >= .08);
    const bool io = (s.psi_available && fraction(s.psi_io_some_avg10) &&
        s.psi_io_some_avg10 >= pressure_limits::kIoPsiSomeAvg10Enter) ||
        (s.io_latency_available && std::isfinite(s.io_avg_latency_ms) && s.io_avg_latency_ms >= pressure_limits::kIoLatencyEnterMs);
    const bool rendering = s.frame_jank_available && fraction(s.frame_jank_rate) && s.frame_jank_rate >= .05;
    const bool cpu = (s.cpu_utilization_available && fraction(s.cpu_utilization) && s.cpu_utilization >= .65) ||
        (!s.cpu_utilization_available && s.load_available && std::isfinite(s.load1) && s.load1 >= 1.5) ||
        (s.psi_available && fraction(s.psi_cpu_some_avg10) && s.psi_cpu_some_avg10 >= .10);
    switch (domain) {
        case ResourceDomain::Memory: return memory;
        case ResourceDomain::Io: return io;
        case ResourceDomain::CpuFreq: case ResourceDomain::Scheduler:
        case ResourceDomain::UClamp: case ResourceDomain::CpuSet: return cpu || rendering;
        case ResourceDomain::Gpu: return rendering && s.gpu_busy_available && fraction(s.gpu_busy_ratio) && s.gpu_busy_ratio >= .45;
        default: return memory || io || cpu || rendering;
    }
}
inline bool measuredTuningNeed(const RuntimeSample& s, RuntimeState state) noexcept {
    return state == RuntimeState::ThermalGuard || domainNeedsTuning(s,state,ResourceDomain::Memory) ||
        domainNeedsTuning(s,state,ResourceDomain::Io) || domainNeedsTuning(s,state,ResourceDomain::CpuFreq) ||
        domainNeedsTuning(s,state,ResourceDomain::Gpu);
}
inline double candidateUtility(double benefit, double risk, double confidence) noexcept {
    return benefit * .65 - risk * .25 + confidence * .10;
}
} // namespace coreflow
