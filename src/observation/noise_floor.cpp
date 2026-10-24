#include "coreflow/observation/noise_floor.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <optional>
#include <vector>

namespace coreflow {
namespace {

constexpr std::size_t kMinimumSamples = 3;

double lowerMedian(std::vector<double> values) {
    const auto middle = values.begin() + static_cast<std::ptrdiff_t>((values.size() - 1) / 2);
    std::nth_element(values.begin(), middle, values.end());
    return *middle;
}

std::optional<double> signalValue(const SystemObservation& s, Signal signal) {
    switch (signal) {
        case Signal::CpuBusy:
            if (s.cpu_valid) return s.cpu_busy_percent;
            break;
        case Signal::CpuPressure:
            if (s.cpu_pressure_valid) return s.cpu_pressure_some_avg10;
            break;
        case Signal::MemoryPressure:
            if (s.memory_pressure_valid) return s.memory_pressure_some_avg10;
            break;
        case Signal::IoPressure:
            if (s.io_pressure_valid) return s.io_pressure_some_avg10;
            break;
        case Signal::MemoryAvailable:
            if (s.memory_valid) return s.memory_available_ratio;
            break;
        case Signal::Temperature:
            if (s.temperature_valid) return s.maximum_temperature_c;
            break;
        case Signal::ThermalHeadroom:
            if (s.thermal_headroom_valid) return s.thermal_headroom_c;
            break;
        case Signal::FrameFps:
            if (s.frame_timing_valid) return s.frame_average_fps;
            break;
        case Signal::FrameP95:
            if (s.frame_timing_valid) return s.frame_p95_interval_ms;
            break;
        case Signal::FrameP99:
            if (s.frame_timing_valid) return s.frame_p99_interval_ms;
            break;
        case Signal::SlowFrameRatio:
            if (s.frame_timing_valid) return s.slow_frame_ratio;
            break;
        case Signal::Count:
            break;
    }
    return std::nullopt;
}

double sourceResolution(const SystemObservation& s, Signal signal) {
    switch (signal) {
        case Signal::CpuBusy: return s.cpu_busy_resolution;
        case Signal::CpuPressure: return s.cpu_pressure_resolution;
        case Signal::MemoryPressure: return s.memory_pressure_resolution;
        case Signal::IoPressure: return s.io_pressure_resolution;
        case Signal::MemoryAvailable: return s.memory_available_resolution;
        case Signal::Temperature: return s.temperature_resolution;
        case Signal::ThermalHeadroom: return s.thermal_headroom_resolution;
        case Signal::FrameP95:
        case Signal::FrameP99: return s.frame_interval_resolution_ms;
        case Signal::SlowFrameRatio: return s.slow_frame_ratio_resolution;
        case Signal::FrameFps: return s.frame_average_fps_resolution;
        case Signal::Count: return 0.0;
    }
    return 0.0;
}

bool highIsAdverse(Signal signal) {
    switch (signal) {
        case Signal::CpuBusy:
        case Signal::CpuPressure:
        case Signal::MemoryPressure:
        case Signal::IoPressure:
        case Signal::Temperature:
        case Signal::FrameP95:
        case Signal::FrameP99:
        case Signal::SlowFrameRatio:
            return true;
        case Signal::MemoryAvailable:
        case Signal::ThermalHeadroom:
        case Signal::FrameFps:
        case Signal::Count:
            return false;
    }
    return false;
}

bool adverseExcursion(double value, const SignalBaseline& baseline, Signal signal) {
    if (!baseline.ready || !std::isfinite(value) || !(baseline.deadband > 0.0)) return false;
    return highIsAdverse(signal)
        ? value > baseline.median + baseline.deadband
        : value < baseline.median - baseline.deadband;
}

} // namespace

SignalBaseline baselineFromHistory(const std::deque<double>& values,
                                   double measurement_resolution) {
    SignalBaseline baseline;
    std::vector<double> finite_values;
    finite_values.reserve(values.size());
    for (const double value : values) {
        if (std::isfinite(value)) finite_values.push_back(value);
    }
    if (finite_values.size() < kMinimumSamples) return baseline;

    // Lower median resists a half-window transition: averaging the two middle
    // values would put the baseline halfway into a new load before it is normal.
    baseline.median = lowerMedian(finite_values);

    std::vector<double> differences;
    differences.reserve(finite_values.size() - 1);
    for (std::size_t i = 1; i < finite_values.size(); ++i) {
        const double step = std::fabs(finite_values[i] - finite_values[i - 1]);
        if (std::isfinite(step)) differences.push_back(step);
    }
    if (differences.empty()) return baseline;

    const double observed_variation = lowerMedian(std::move(differences));
    const double resolution = std::isfinite(measurement_resolution) &&
                              measurement_resolution > 0.0
        ? measurement_resolution : 0.0;
    baseline.deadband = std::max(resolution, observed_variation);
    baseline.ready = std::isfinite(baseline.median) &&
                     std::isfinite(baseline.deadband) && baseline.deadband > 0.0;
    return baseline;
}

void NoiseFloorTracker::observe(const SystemObservation& sample) {
    for (std::size_t i = 0; i < history_.size(); ++i) {
        const auto signal = static_cast<Signal>(i);
        const auto value = signalValue(sample, signal);
        if (!value || !std::isfinite(*value)) continue;

        const double reported_resolution = sourceResolution(sample, signal);
        if (std::isfinite(reported_resolution) && reported_resolution > 0.0) {
            auto& known_resolution = resolution_[i];
            // Finer valid source representations improve the quantization estimate;
            // an observed jump is never allowed to become the resolution itself.
            if (!(known_resolution > 0.0) || reported_resolution < known_resolution) {
                known_resolution = reported_resolution;
            }
        }

        auto& history = history_[i];
        const auto baseline = baselineFromHistory(history, resolution_[i]);
        if (adverseExcursion(*value, baseline, signal)) {
            // Do not train the baseline on the workload excursion that decisions
            // are intended to detect. Recovery samples naturally re-enter history.
            continue;
        }

        history.push_back(*value);
        while (history.size() > kHistoryDepth) history.pop_front();
    }
}

NoiseFloor NoiseFloorTracker::floor() const {
    NoiseFloor result;
    for (std::size_t i = 0; i < history_.size(); ++i) {
        result.signals[i] = baselineFromHistory(history_[i], resolution_[i]);
    }
    return result;
}

} // namespace coreflow
