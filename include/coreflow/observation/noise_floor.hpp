#pragma once

#include "coreflow/model/types.hpp"

#include <array>
#include <cstddef>
#include <deque>

namespace coreflow {

// Learns, per signal, what this device normally reports and how much that report
// moves between consecutive samples. The deadband is the smallest change that can
// be attributed to a control rather than to sampling noise:
//
//   deadband = max(source representation quantum, median(|successive difference|))
//
// The representation quantum is derived from the source format or counter
// granularity; it is not guessed from a workload transition. If a source exposes
// neither precision nor usable variation, its signal remains unready and decisions
// that depend on it fail closed.
class NoiseFloorTracker {
public:
    void observe(const SystemObservation& sample);
    [[nodiscard]] NoiseFloor floor() const;

private:
    // Bounded memory for the history of each signal. Structural, not tuning.
    static constexpr std::size_t kHistoryDepth = 32;

    std::array<std::deque<double>, static_cast<std::size_t>(Signal::Count)> history_{};
    std::array<double, static_cast<std::size_t>(Signal::Count)> resolution_{};
};

// Exposed for direct verification of the estimator.
[[nodiscard]] SignalBaseline baselineFromHistory(const std::deque<double>& values,
                                                 double measurement_resolution = 0.0);

} // namespace coreflow
