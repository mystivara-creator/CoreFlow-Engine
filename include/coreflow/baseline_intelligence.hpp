#pragma once

#include <cstddef>
#include <cstdint>

#include "coreflow/types.hpp"

namespace coreflow {

/**
 * Compares a verified post-mutation runtime window against the factory/OEM
 * runtime baseline captured before mutation.
 *
 * This class is deliberately independent from MutationController's restore
 * baseline. MutationController answers "what must be restored?" while this
 * class answers "did the candidate runtime behave more efficiently?".
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
        bool context_compatible{true};
        bool valid{false};
    };

    static constexpr std::size_t kBaselineSamples = 5;
    static constexpr std::size_t kObservationSamples = 5;

    void reset() noexcept;

    bool captureBaseline(const RuntimeSample& sample) noexcept;

    bool baselineReady() const noexcept {
        return phase_ == Phase::BaselineReady ||
               phase_ == Phase::Observing;
    }

    bool beginObservation() noexcept;
    bool observing() const noexcept { return phase_ == Phase::Observing; }
    bool observe(const RuntimeSample& sample) noexcept;

    Evaluation evaluate() const noexcept;

    Outcome outcome() const noexcept { return evaluation_.outcome; }
    const Evaluation& result() const noexcept { return evaluation_; }

private:
    struct MetricAggregate {
        double sum{0.0};
        double sum_sq{0.0};
        std::size_t samples{0};

        void reset() noexcept {
            sum = 0.0;
            sum_sq = 0.0;
            samples = 0;
        }
    };

    struct Aggregate {
        MetricAggregate thermal_c;
        MetricAggregate load;
        MetricAggregate cpu;
        MetricAggregate memory;

        std::size_t samples{0};
        std::size_t charging_telemetry_samples{0};
        std::size_t charging_true_samples{0};

        void reset() noexcept {
            thermal_c.reset();
            load.reset();
            cpu.reset();
            memory.reset();
            samples = 0;
            charging_telemetry_samples = 0;
            charging_true_samples = 0;
        }
    };

    static void accumulate(Aggregate&, const RuntimeSample&) noexcept;

    static void accumulateMetric(MetricAggregate&, double) noexcept;
    static double average(const MetricAggregate&) noexcept;
    static double variance(const MetricAggregate&) noexcept;
    static double standardDeviation(const MetricAggregate&) noexcept;

    static double lowerIsBetter(double baseline,
                                double candidate) noexcept;
    static double higherIsBetter(double baseline,
                                 double candidate) noexcept;
    static double stabilityScore(const MetricAggregate& baseline,
                                 const MetricAggregate& candidate) noexcept;
    static double calculateStabilityScore(const Aggregate& baseline,
                                          const Aggregate& candidate) noexcept;
    static double calculateConfidence(const Aggregate& baseline,
                                      const Aggregate& candidate,
                                      bool contextCompatible) noexcept;

    static bool finite(double value) noexcept;
    static double clamp01(double value) noexcept;
    static bool contextCompatible(const Aggregate& baseline,
                                  const Aggregate& candidate) noexcept;

    Aggregate baseline_{};
    Aggregate observation_{};
    Phase phase_{Phase::Idle};
    Evaluation evaluation_{};
};

} // namespace coreflow
