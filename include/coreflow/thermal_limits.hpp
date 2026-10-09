#pragma once

// Single source of truth for thermal plausibility bounds (millidegrees C).
// Used by the observer (per-zone filtering) and the engine (sample validation)
// so the two layers can never disagree about what counts as a valid reading.

namespace coreflow {

// Realistic operating band for a mobile SoC / skin zone. Values outside this
// band are sensor sentinels (-273000, -40000), disconnected zones (0) or
// physically implausible readings and must not drive policy.
inline constexpr long kThermalPlausibleMinMillidegrees = 10000;   // 10 C
inline constexpr long kThermalPlausibleMaxMillidegrees = 120000;  // 120 C

constexpr bool isPlausibleThermalMilli(long milli) noexcept {
    return milli >= kThermalPlausibleMinMillidegrees &&
           milli <= kThermalPlausibleMaxMillidegrees;
}

}  // namespace coreflow
