#pragma once

namespace coreflow::pressure_limits {

// Shared entry points for I/O pressure used by policy and effect ranking.
// PSI avg10 is represented as a fraction in RuntimeSample/EffectContext.
inline constexpr double kIoPsiSomeAvg10Enter = 0.08;
inline constexpr double kIoLatencyEnterMs = 25.0;

} // namespace coreflow::pressure_limits
