#pragma once

#include <cstddef>
#include <cstdint>

#include "coreflow/types.hpp"

namespace coreflow {

/**
 * Baseline Intelligence
 *
 * Measures runtime efficiency relative to a locally observed factory/OEM
 * baseline. It does not replace MutationController's safety/restore baseline.
 *
 * v1.1.0-A intentionally uses only telemetry already exposed by RuntimeSample.
 */
class BaselineIntelligence {
public:
    enum class Phase {
        Idle,
        BaselineReady,
        Observing
    };

    enum class Outcome {
        Unknown,
        Beneficial,
        Neutral,
        Regression,
        Inconclusive
    };

    struct Evaluation {
        Outcome outcome{Outcome::Unknown};
        double overall_score{0.0};
        double thermal_score{0.0};
        double load_score{0.0};
        double cpu_score{0.0};
        double memory_score{0.0};
        double stability_score{0.0};
        double confidence{0.0};
        std::size_t baseline_samples{0};
        std::size_t observation_samples{0};
        bool thermal_available{false};
        bool cpu_available{false};
        bool memory_available{false};
        bool valid{false};
    };

    static constexpr std::size_t kBaselineSamples = 5;
    static constexpr std::size_t kObservationSamples = 5;

    void reset() noexcept;

    /**
     * Feed samples captured while the device is at the factory/baseline state.
     * Returns true when enough baseline samples have been collected.
     */
    bool captureBaseline(const RuntimeSample& sample) noexcept;

    bool baselineReady() const noexcept {
        return phase_ == Phase::BaselineReady ||
               phase_ == Phase::Observing;
    }

    /**
     * Begin an observation window after a verified mutation.
     * The evaluator will not accept another observation window while one is
     * already active.
     */
    bool beginObservation() noexcept;

    bool observing() const noexcept { return phase_ == Phase::Observing; }

    /**
     * Feed post-mutation samples. Returns true when the window completes.
     */
    bool observe(const RuntimeSample& sample) noexcept;

    Evaluation evaluate() const noexcept;

    Outcome outcome() const noexcept { return evaluation_.outcome; }
    const Evaluation& result() const noexcept { return evaluation_; }

private:
    struct Aggregate {
        double thermal_sum_c{0.0};
        double load_sum{0.0};
        double cpu_sum{0.0};
        double memory_sum{0.0};

        std::size_t thermal_samples{0};
        std::size_t load_samples{0};
        std::size_t cpu_samples{0};
        std::size_t memory_samples{0};

        std::size_t samples{0};

        void reset() noexcept {
            thermal_sum_c = 0.0;
            load_sum = 0.0;
            cpu_sum = 0.0;
            memory_sum = 0.0;
            thermal_samples = 0;
            load_samples = 0;
            cpu_samples = 0;
            memory_samples = 0;
            samples = 0;
        }
    };

    static void accumulate(Aggregate&, const RuntimeSample&) noexcept;

    static double lowerIsBetter(double baseline,
                                double candidate) noexcept;
    static double higherIsBetter(double baseline,
                                 double candidate) noexcept;

    static bool finite(double value) noexcept;

    static double safeAverage(double sum, std::size_t count) noexcept;

    static double clamp01(double value) noexcept;

    static double calculateStabilityScore(const Aggregate& baseline,
                                          const Aggregate& candidate) noexcept;

    static double calculateConfidence(const Aggregate& baseline,
                                      const Aggregate& candidate) noexcept;

    Aggregate baseline_{};
    Aggregate observation_{};
    Phase phase_{Phase::Idle};
    Evaluation evaluation_{};
};

} // namespace coreflow
