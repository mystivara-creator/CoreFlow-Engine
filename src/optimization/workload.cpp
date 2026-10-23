#include "coreflow/optimization/workload.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

namespace coreflow {
namespace {
constexpr double kPressureGate = 1.0;
constexpr double kLowMemoryRatio = 0.20;
constexpr double kHighCpuBusyPercent = 85.0;
constexpr double kSlowFrameGate = 0.05;

double cpuScore(const SystemObservation& o) {
    double score = o.cpu_pressure_valid && std::isfinite(o.cpu_pressure_some_avg10)
        ? std::max(0.0, o.cpu_pressure_some_avg10) : 0.0;
    if (o.cpu_valid && std::isfinite(o.cpu_busy_percent) &&
        o.cpu_busy_percent >= kHighCpuBusyPercent) {
        score = std::max(score, o.cpu_busy_percent - (kHighCpuBusyPercent - 1.0));
    }
    return score >= kPressureGate ? score : 0.0;
}

double memoryScore(const SystemObservation& o) {
    double score = o.memory_pressure_valid && std::isfinite(o.memory_pressure_some_avg10)
        ? std::max(0.0, o.memory_pressure_some_avg10) : 0.0;
    if (o.memory_valid && std::isfinite(o.memory_available_ratio) &&
        o.memory_available_ratio < kLowMemoryRatio) {
        const double shortage = (kLowMemoryRatio -
            std::clamp(o.memory_available_ratio, 0.0, 1.0)) / kLowMemoryRatio;
        score = std::max(score, shortage * 100.0);
    }
    return score >= kPressureGate ? score : 0.0;
}

double ioScore(const SystemObservation& o) {
    if (!o.io_pressure_valid || !std::isfinite(o.io_pressure_some_avg10)) return 0.0;
    const double score = std::max(0.0, o.io_pressure_some_avg10);
    return score >= kPressureGate ? score : 0.0;
}

double frameScore(const SystemObservation& o) {
    if (!o.frame_timing_valid || !std::isfinite(o.slow_frame_ratio) ||
        !std::isfinite(o.frame_p95_interval_ms) ||
        !std::isfinite(o.frame_p99_interval_ms) ||
        o.frame_p95_interval_ms <= 0.0 || o.frame_p99_interval_ms <= 0.0) return 0.0;
    const double score = std::clamp(o.slow_frame_ratio, 0.0, 1.0) * 100.0;
    // The slow-frame ratio already measures the tail against the observed mean;
    // p95 and p99 may legitimately land in the same coarse histogram bucket.
    return score >= kSlowFrameGate * 100.0 ? score : 0.0;
}
} // namespace

double workloadScoreForDomain(const std::string& domain,
                              const SystemObservation& observation) {
    if (domain == "proc_sys.vm") return memoryScore(observation);
    if (domain == "proc_sys.kernel" || domain == "sysfs.cpu.cpufreq")
        return cpuScore(observation);
    if (domain == "sysfs.block.queue") return ioScore(observation);
    if (domain == "sysfs.devfreq") return frameScore(observation);
    // Unknown semantics stay observe-only. A numeric range alone is not proof
    // that changing a control is relevant or safe for the current workload.
    return 0.0;
}

std::string workloadSignature(const SystemObservation& observation) {
    std::vector<std::string> active;
    if (cpuScore(observation) > 0.0) active.emplace_back("cpu");
    if (memoryScore(observation) > 0.0) active.emplace_back("memory");
    if (ioScore(observation) > 0.0) active.emplace_back("io");
    if (frameScore(observation) > 0.0) {
        active.emplace_back(observation.frame_package.empty()
            ? "frame-pacing" : "frame-pacing:" + observation.frame_package);
    }
    if (active.empty()) return "idle";

    std::ostringstream out;
    for (std::size_t i = 0; i < active.size(); ++i) {
        if (i) out << '+';
        out << active[i];
    }
    return out.str();
}

} // namespace coreflow
