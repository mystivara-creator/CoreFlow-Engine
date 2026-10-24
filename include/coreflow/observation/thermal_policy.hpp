#pragma once

#include <charconv>
#include <cmath>
#include <string_view>
#include <system_error>

namespace coreflow::observation::thermal_policy {

inline constexpr double kRetentionThermalRiseC = 0.5;

// Veto retention only for a measured thermal regression across the same
// before/after window. A universal absolute ceiling is deliberately avoided:
// sensor location, package design and vendor policy differ across devices.
// Android Thermal HAL and vendor throttling remain authoritative.
[[nodiscard]] inline bool shouldVetoRetention(
    bool before_valid, double before_celsius,
    bool after_valid, double after_celsius) noexcept {
    if (!before_valid || !after_valid ||
        !std::isfinite(before_celsius) || !std::isfinite(after_celsius)) {
        return false;
    }
    return after_celsius > before_celsius + kRetentionThermalRiseC;
}

[[nodiscard]] inline bool isTrustedComputeThermalType(
    std::string_view type) noexcept {
    return type.starts_with("cpu-") ||
           type.starts_with("cpuss-") ||
           type.starts_with("gpuss-");
}

// Accepts only explicitly trusted sensor types and plausible millidegree
// Celsius readings. The output parameter is unchanged on failure.
[[nodiscard]] inline bool parseTrustedThermalCelsius(
    std::string_view type,
    std::string_view raw,
    double& celsius) noexcept {
    if (!isTrustedComputeThermalType(type) || raw.empty()) {
        return false;
    }

    double milli_celsius{};
    const char* first = raw.data();
    const char* last = first + raw.size();
    const auto parsed = std::from_chars(first, last, milli_celsius);

    if (parsed.ec != std::errc{} ||
        parsed.ptr != last ||
        !std::isfinite(milli_celsius) ||
        milli_celsius < 10000.0 ||
        milli_celsius > 125000.0) {
        return false;
    }

    const double converted = milli_celsius / 1000.0;
    if (!std::isfinite(converted) ||
        converted < 10.0 ||
        converted > 125.0) {
        return false;
    }

    celsius = converted;
    return true;
}

// Retention headroom guard. Near the vendor's own passive trip point the kernel
// starts throttling; a change that consumes this margin is unsafe even when a
// frame or pressure metric improved. The margin is relative to the vendor trip,
// so no device-wide absolute temperature target is assumed.
inline constexpr double kHeadroomNearC = 3.0;

[[nodiscard]] inline bool isPassiveTripType(std::string_view type) noexcept {
    return type == "passive";
}

// Parses a kernel trip_point_N_temp value (millidegrees Celsius) with the same
// plausibility bounds as the compute-zone reader. Output is unchanged on failure.
[[nodiscard]] inline bool parseTripCelsius(std::string_view raw, double& celsius) noexcept {
    if (raw.empty()) return false;
    double milli_celsius{};
    const char* first = raw.data();
    const char* last = first + raw.size();
    const auto parsed = std::from_chars(first, last, milli_celsius);
    if (parsed.ec != std::errc{} || parsed.ptr != last || !std::isfinite(milli_celsius) ||
        milli_celsius < 10000.0 || milli_celsius > 125000.0) {
        return false;
    }
    celsius = milli_celsius / 1000.0;
    return true;
}

[[nodiscard]] inline bool shouldVetoHeadroom(
    bool before_valid, double before_headroom_c,
    bool after_valid, double after_headroom_c) noexcept {
    if (!before_valid || !after_valid ||
        !std::isfinite(before_headroom_c) || !std::isfinite(after_headroom_c)) {
        return false;
    }
    return after_headroom_c < before_headroom_c - kRetentionThermalRiseC &&
           after_headroom_c < kHeadroomNearC;
}

} // namespace coreflow::observation::thermal_policy
