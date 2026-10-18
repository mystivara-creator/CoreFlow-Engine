#pragma once

#include <cstdint>
#include <optional>

namespace coreflow::performance {

// Stable, vendor-neutral telemetry contract. Timestamps use a monotonic clock
// domain (nanoseconds); adapters must not pass wall-clock timestamps here.
enum class Signal : std::uint8_t {
    FrameInterval,
    CpuRunQueueDelay,
    GpuBusyRatio,
    MemoryPressureSome,
    IoWaitRatio,
    ThermalHeadroomRatio,
};

enum class Availability : std::uint8_t {
    Available,
    Unsupported,
    PermissionDenied,
    TemporarilyUnavailable,
    InvalidSample,
};

enum class Unit : std::uint8_t {
    Nanoseconds,
    Ratio,
};

struct Sample final {
    Signal signal{Signal::FrameInterval};
    Availability availability{Availability::TemporarilyUnavailable};
    Unit unit{Unit::Nanoseconds};
    std::uint64_t timestamp_monotonic_ns{0};
    double value{0.0};
};

struct Validation final {
    bool valid{false};
    Availability availability{Availability::InvalidSample};
};

// Validates the signal/unit pairing, timestamp and value. Ratios are in [0,1].
// Invalid input is rejected, never silently clamped into plausible telemetry.
[[nodiscard]] Validation validate(const Sample& sample) noexcept;

// Returns a normalized copy only for valid, available samples. The original
// sample remains untouched on every failure path.
[[nodiscard]] std::optional<Sample> normalize(const Sample& sample) noexcept;

[[nodiscard]] const char* signalName(Signal signal) noexcept;
[[nodiscard]] const char* availabilityName(Availability availability) noexcept;
[[nodiscard]] const char* unitName(Unit unit) noexcept;

} // namespace coreflow::performance
