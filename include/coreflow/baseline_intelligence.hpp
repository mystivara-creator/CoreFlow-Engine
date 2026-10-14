#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "coreflow/types.hpp"

namespace coreflow {

/**
 * Compares a verified post-mutation runtime window against the runtime
 * baseline captured immediately before that mutation.
 *
 * The baseline is a rolling window of the most recent kBaselineSamples
 * pre-mutation samples. It is frozen while a candidate is observed, so the
 * comparison is always "the same workload, just before vs. just after".
 * A one-shot baseline captured at daemon start compared candidates against a
 * workload that could be hours old and made outcomes mostly workload noise.
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
        double psi_score{0.0};          // lower PSI is better
        double io_latency_score{0.0};   // lower latency is better
        double confidence{0.0};
        std::size_t baseline_samples{0};
        std::size_t observation_samples{0};
        bool thermal_available{false};
        bool cpu_available{false};
        bool memory_available{false};
        bool psi_available{false};
        bool io_latency_available{false};
        bool context_compatible{true};
        bool valid{false};
    };

    static constexpr std::size_t kBaselineSamples = 4;
    static constexpr std::size_t kObservationSamples = 4;

    void reset() noexcept;

    bool captureBaseline(const RuntimeSample& sample) noexcept;

    bool baselineReady() const noexcept {
        return phase_ == Phase::BaselineReady ||
               phase_ == Phase::Observing;
    }

    // Samples currently held in the rolling pre-mutation baseline window.
    std::size_t baselineSampleCount() const noexcept { return baseline_.samples; }
    std::size_t observationSampleCount() const noexcept { return observation_.samples; }

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
        MetricAggregate psi_pressure;   // max of cpu/mem/io some avg10
        MetricAggregate io_latency_ms;

        std::size_t samples{0};
        std::size_t charging_telemetry_samples{0};
        std::size_t charging_true_samples{0};

        void reset() noexcept {
            thermal_c.reset();
            load.reset();
            cpu.reset();
            memory.reset();
            psi_pressure.reset();
            io_latency_ms.reset();
            samples = 0;
            charging_telemetry_samples = 0;
            charging_true_samples = 0;
        }
    };

    // Allocation-free projection of the RuntimeSample fields the aggregate uses,
    // so the rolling window never copies strings/vectors.
    struct BaselinePoint {
        bool thermal{false};
        double thermal_c{0.0};
        double load{0.0};
        bool cpu_available{false};
        double cpu{0.0};
        double memory{0.0};
        bool charging_telemetry{false};
        bool charging{false};
        bool psi_available{false};
        double psi_pressure{0.0};
        bool io_latency_available{false};
        double io_latency_ms{0.0};
    };

    static BaselinePoint project(const RuntimeSample&) noexcept;
    static void accumulate(Aggregate&, const BaselinePoint&) noexcept;
    static void accumulate(Aggregate&, const RuntimeSample&) noexcept;
    void rebuildBaseline() noexcept;

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

    std::array<BaselinePoint, kBaselineSamples> window_{};
    std::size_t window_count_{0};
    std::size_t window_head_{0};
    Aggregate baseline_{};
    Aggregate observation_{};
    Phase phase_{Phase::Idle};
    Evaluation evaluation_{};
};

} // namespace coreflow
