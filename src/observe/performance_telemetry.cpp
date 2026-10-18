#include "coreflow/performance_telemetry.hpp"

#include <cmath>

namespace coreflow::performance {
namespace {

[[nodiscard]] constexpr bool expectsRatio(Signal signal) noexcept {
    switch (signal) {
        case Signal::GpuBusyRatio:
        case Signal::MemoryPressureSome:
        case Signal::IoWaitRatio:
        case Signal::ThermalHeadroomRatio:
            return true;
        case Signal::FrameInterval:
        case Signal::CpuRunQueueDelay:
            return false;
    }
    return false;
}

[[nodiscard]] constexpr bool knownSignal(Signal signal) noexcept {
    switch (signal) {
        case Signal::FrameInterval:
        case Signal::CpuRunQueueDelay:
        case Signal::GpuBusyRatio:
        case Signal::MemoryPressureSome:
        case Signal::IoWaitRatio:
        case Signal::ThermalHeadroomRatio:
            return true;
    }
    return false;
}

} // namespace

Validation validate(const Sample& sample) noexcept {
    if (sample.availability != Availability::Available) {
        return {false, sample.availability};
    }
    if (!knownSignal(sample.signal) || sample.timestamp_monotonic_ns == 0U ||
        !std::isfinite(sample.value) || sample.value < 0.0) {
        return {false, Availability::InvalidSample};
    }

    const bool ratio = expectsRatio(sample.signal);
    if ((ratio && sample.unit != Unit::Ratio) ||
        (!ratio && sample.unit != Unit::Nanoseconds)) {
        return {false, Availability::InvalidSample};
    }
    if (ratio && sample.value > 1.0) {
        return {false, Availability::InvalidSample};
    }
    // A zero frame interval is not a measured frame duration. Zero run-queue
    // delay is valid, so only the frame interval has this stricter bound.
    if (sample.signal == Signal::FrameInterval && sample.value <= 0.0) {
        return {false, Availability::InvalidSample};
    }
    return {true, Availability::Available};
}

std::optional<Sample> normalize(const Sample& sample) noexcept {
    const Validation result = validate(sample);
    if (!result.valid) return std::nullopt;
    return sample;
}

const char* signalName(Signal signal) noexcept {
    switch (signal) {
        case Signal::FrameInterval: return "frame_interval";
        case Signal::CpuRunQueueDelay: return "cpu_run_queue_delay";
        case Signal::GpuBusyRatio: return "gpu_busy_ratio";
        case Signal::MemoryPressureSome: return "memory_pressure_some";
        case Signal::IoWaitRatio: return "io_wait_ratio";
        case Signal::ThermalHeadroomRatio: return "thermal_headroom_ratio";
    }
    return "unknown";
}

const char* availabilityName(Availability availability) noexcept {
    switch (availability) {
        case Availability::Available: return "available";
        case Availability::Unsupported: return "unsupported";
        case Availability::PermissionDenied: return "permission_denied";
        case Availability::TemporarilyUnavailable: return "temporarily_unavailable";
        case Availability::InvalidSample: return "invalid_sample";
    }
    return "unknown";
}

const char* unitName(Unit unit) noexcept {
    switch (unit) {
        case Unit::Nanoseconds: return "ns";
        case Unit::Ratio: return "ratio_0_1";
    }
    return "unknown";
}

} // namespace coreflow::performance
