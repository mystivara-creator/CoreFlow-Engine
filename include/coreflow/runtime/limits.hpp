#pragma once

#include <chrono>
#include <cstddef>

namespace coreflow::runtime_limits {

// Operational invariants of the control loop: they bound time, memory and the
// console's staleness. None of them is a device, vendor, workload or kernel
// tuning value. Tuning comes only from kernel-published metadata, the device's own
// learned noise floor, and measured outcomes.

// Scheduling of the loop when there is nothing to do.
inline constexpr auto kMinimumIdle = std::chrono::seconds(2);
inline constexpr auto kMaximumIdle = std::chrono::seconds(10);
inline constexpr auto kPausedPoll = std::chrono::seconds(2);

// Full inventory refresh. Structural changes are also picked up immediately after
// a retained selector change (see OptimizationEngine::consumeRediscoveryRequest).
inline constexpr auto kDiscoveryRefresh = std::chrono::minutes(2);

// Kernel PSI averages pressure over a 10 s window (avg10; Documentation/accounting/psi.rst).
// The outcome window must cover at least one full averaging window, or the measured
// pressure would still reflect history from before the change.
inline constexpr auto kOutcomeSettle = std::chrono::seconds(10);

// The console considers the daemon stale after this many worst-case publish
// intervals. The interval is derived from the loop itself: the longest gap between
// two status writes is one settle window plus one maximum idle sleep.
inline constexpr int kStaleIntervals = 3;
inline constexpr auto kStaleAfter = kStaleIntervals * (kMaximumIdle + kOutcomeSettle);

// Bounds on the size of the capability snapshot written for the console.
inline constexpr std::size_t kWebUiLimit = 2048;

} // namespace coreflow::runtime_limits
