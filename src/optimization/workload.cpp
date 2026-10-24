#include "coreflow/optimization/workload.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

namespace coreflow {
namespace {
// A signal is elevated when its current value exceeds its own typical level by more
// than its own sample-to-sample variation. The score is that excess expressed in
// units of the variation, so every signal is dimensionless and comparable.
// Non-elevated or unready signals score zero.
double elevation(double value, const SignalBaseline& baseline) {
    if (!baseline.ready || !std::isfinite(value) || !(baseline.deadband > 0.0)) return 0.0;
    const double signal_to_noise = (value - baseline.median) / baseline.deadband;
    // "Above noise" means the excess is larger than the variation itself.
    return signal_to_noise > 1.0 ? signal_to_noise : 0.0;
}

double cpuScore(const SystemObservation& o, const NoiseFloor& n) {
    double score = 0.0;
    if (o.cpu_pressure_valid && std::isfinite(o.cpu_pressure_some_avg10)) {
        score = elevation(o.cpu_pressure_some_avg10, n[Signal::CpuPressure]);
    }
    // PSI is absent on some Android kernels. CPU busy percentage is a separate
    // signal with its own native resolution and learned baseline; use the stronger
    // dimensionless score rather than a fixed absolute busy-percent gate.
    if (o.cpu_valid && std::isfinite(o.cpu_busy_percent)) {
        score = std::max(score, elevation(o.cpu_busy_percent, n[Signal::CpuBusy]));
    }
    return score;
}

double downwardElevation(double value, const SignalBaseline& baseline) {
    if (!baseline.ready || !std::isfinite(value) || !(baseline.deadband > 0.0)) return 0.0;
    const double signal_to_noise = (baseline.median - value) / baseline.deadband;
    return signal_to_noise > 1.0 ? signal_to_noise : 0.0;
}

double memoryScore(const SystemObservation& o, const NoiseFloor& n) {
    double score = 0.0;
    const auto& pressure = n[Signal::MemoryPressure];
    if (o.memory_pressure_valid && std::isfinite(o.memory_pressure_some_avg10) &&
        pressure.ready && pressure.deadband > 0.0) {
        score = elevation(o.memory_pressure_some_avg10, pressure);
    }

    // PSI and MemAvailable describe different failure modes: reclaim stalls and
    // remaining headroom. Score each against its own baseline, then take the
    // stronger dimensionless signal instead of summing or comparing raw units.
    // This also preserves operation on kernels without memory PSI.
    if (o.memory_valid && std::isfinite(o.memory_available_ratio)) {
        const double ratio = std::clamp(o.memory_available_ratio, 0.0, 1.0);
        score = std::max(score, downwardElevation(ratio, n[Signal::MemoryAvailable]));
    }
    return score;
}

double ioScore(const SystemObservation& o, const NoiseFloor& n) {
    return o.io_pressure_valid
        ? elevation(o.io_pressure_some_avg10, n[Signal::IoPressure]) : 0.0;
}

double frameScore(const SystemObservation& o, const NoiseFloor& n) {
    if (!o.frame_timing_valid) return 0.0;
    return elevation(o.slow_frame_ratio, n[Signal::SlowFrameRatio]);
}
} // namespace

double workloadScoreForDomain(const std::string& domain,
                              const SystemObservation& observation,
                              const NoiseFloor& noise) {
    if (domain == "proc_sys.vm") return memoryScore(observation, noise);
    if (domain == "proc_sys.kernel" || domain == "sysfs.cpu.cpufreq" ||
        domain == "sysfs.cpu.governor")
        return cpuScore(observation, noise);
    if (domain == "sysfs.block.queue" || domain == "sysfs.block.iosched")
        return ioScore(observation, noise);
    if (domain == "sysfs.devfreq") return frameScore(observation, noise);
    // Unknown semantics stay observe-only. A numeric range alone is not proof
    // that changing a control is relevant or safe for the current workload.
    return 0.0;
}

std::string workloadSignature(const SystemObservation& observation, const NoiseFloor& noise) {
    std::vector<std::string> active;
    if (cpuScore(observation, noise) > 0.0) active.emplace_back("cpu");
    if (memoryScore(observation, noise) > 0.0) active.emplace_back("memory");
    if (ioScore(observation, noise) > 0.0) active.emplace_back("io");
    if (frameScore(observation, noise) > 0.0) {
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
