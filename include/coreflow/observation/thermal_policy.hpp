#pragma once

#include <charconv>
#include <cmath>
#include <string_view>
#include <system_error>

namespace coreflow::observation::thermal_policy {

// Veto retention only for a measured thermal regression across the same
// before/after window. The rise that counts is the device's own sample-to-sample
// variation (the caller supplies it from the learned noise floor); a universal
// absolute ceiling is deliberately avoided because sensor placement and vendor
// policy differ across devices.
[[nodiscard]] inline bool shouldVetoRetention(
    bool before_valid, double before_celsius,
    bool after_valid, double after_celsius,
    double deadband_celsius) noexcept {
    if (!before_valid || !after_valid ||
        !std::isfinite(before_celsius) || !std::isfinite(after_celsius) ||
        !std::isfinite(deadband_celsius) || deadband_celsius < 0.0) {
        return false;
    }
    return after_celsius > before_celsius + deadband_celsius;
}

// Headroom is the distance to the kernel's passive trip point. Retention is vetoed
// when the change consumed headroom beyond the device's noise AND the remaining
// headroom is inside the kernel's own hysteresis band for that trip. Without a
// published hysteresis the band is zero, so only an actual crossing is vetoed.
[[nodiscard]] inline bool shouldVetoHeadroom(
    bool before_valid, double before_headroom_c,
    bool after_valid, double after_headroom_c,
    double hysteresis_margin_c,
    double deadband_c) noexcept {
    if (!before_valid || !after_valid ||
        !std::isfinite(before_headroom_c) || !std::isfinite(after_headroom_c) ||
        !std::isfinite(hysteresis_margin_c) || !std::isfinite(deadband_c) ||
        hysteresis_margin_c < 0.0 || deadband_c < 0.0) {
        return false;
    }
    return after_headroom_c < before_headroom_c - deadband_c &&
           after_headroom_c < hysteresis_margin_c;
}

// Trip hysteresis is published as millidegrees. Zero is a legal value; the
// only sanity bound is that the value is finite and non-negative.
[[nodiscard]] inline bool parseHysteresisCelsius(std::string_view raw, double& celsius) noexcept {
    if (raw.empty()) return false;
    double milli_celsius{};
    const char* first = raw.data();
    const char* last = first + raw.size();
    const auto parsed = std::from_chars(first, last, milli_celsius);
    if (parsed.ec != std::errc{} || parsed.ptr != last || !std::isfinite(milli_celsius) ||
        milli_celsius < 0.0) {
        return false;
    }
    celsius = milli_celsius / 1000.0;
    return true;
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

} // namespace coreflow::observation::thermal_policy
