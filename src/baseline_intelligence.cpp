#include "coreflow/baseline_intelligence.hpp"

#include <algorithm>
#include <cmath>

namespace coreflow {

namespace {

constexpr double kThermalWeight = 0.30;
constexpr double kLoadWeight = 0.20;
constexpr double kCpuWeight = 0.15;
constexpr double kMemoryWeight = 0.15;
constexpr double kStabilityWeight = 0.20;

constexpr double kBeneficialThreshold = 0.05;
constexpr double kRegressionThreshold = -0.05;

} // namespace

void BaselineIntelligence::reset() noexcept {
    baseline_.reset();
    observation_.reset();
    phase_ = Phase::Idle;
    evaluation_ = Evaluation{};
}

bool BaselineIntelligence::finite(double value) noexcept {
    return std::isfinite(value);
}

double BaselineIntelligence::clamp01(double value) noexcept {
    if (!finite(value)) return 0.0;
    return std::clamp(value, 0.0, 1.0);
}

double BaselineIntelligence::safeAverage(double sum,
                                         std::size_t count) noexcept {
    if (count == 0U) return 0.0;
    return sum / static_cast<double>(count);
}

void BaselineIntelligence::accumulate(Aggregate& aggregate,
                                      const RuntimeSample& sample) noexcept {
    ++aggregate.samples;

    if (sample.thermal_available) {
        const double thermal =
            static_cast<double>(sample.hottest_thermal_millidegrees) / 1000.0;
        if (finite(thermal)) {
            aggregate.thermal_sum_c += thermal;
            ++aggregate.thermal_samples;
        }
    }

    if (finite(sample.load1) && sample.load1 >= 0.0) {
        aggregate.load_sum += sample.load1;
        ++aggregate.load_samples;
    }

    if (sample.cpu_utilization_available &&
        finite(sample.cpu_utilization) &&
        sample.cpu_utilization >= 0.0 &&
        sample.cpu_utilization <= 1.0) {
        aggregate.cpu_sum += sample.cpu_utilization;
        ++aggregate.cpu_samples;
    }

    if (finite(sample.mem_available_ratio) &&
        sample.mem_available_ratio >= 0.0 &&
        sample.mem_available_ratio <= 1.0) {
        aggregate.memory_sum += sample.mem_available_ratio;
        ++aggregate.memory_samples;
    }
}

double BaselineIntelligence::lowerIsBetter(double baseline,
                                           double candidate) noexcept {
    if (!finite(baseline) || !finite(candidate) || baseline <= 0.0) {
        return 0.5;
    }

    const double relative_change = (baseline - candidate) / baseline;
    return clamp01(0.5 + (relative_change * 2.0));
}

double BaselineIntelligence::higherIsBetter(double baseline,
                                            double candidate) noexcept {
    if (!finite(baseline) || !finite(candidate) || baseline <= 0.0) {
        return 0.5;
    }

    const double relative_change = (candidate - baseline) / baseline;
    return clamp01(0.5 + (relative_change * 2.0));
}

double BaselineIntelligence::calculateStabilityScore(
    const Aggregate& baseline,
    const Aggregate& candidate) noexcept {
    /*
     * v1.1 deliberately uses sample availability as the stability signal.
     * It does not invent a variance model that RuntimeSample does not expose.
     */
    const double baselineCoverage =
        static_cast<double>(
            std::max({baseline.thermal_samples,
                      baseline.load_samples,
                      baseline.memory_samples}))
        / static_cast<double>(std::max<std::size_t>(1U, baseline.samples));

    const double candidateCoverage =
        static_cast<double>(
            std::max({candidate.thermal_samples,
                      candidate.load_samples,
                      candidate.memory_samples}))
        / static_cast<double>(std::max<std::size_t>(1U, candidate.samples));

    // Coverage is not itself an efficiency gain. Keep equal coverage centered
    // at the neutral score (0.50); only a meaningful coverage change may move
    // this component away from neutral.
    return clamp01(0.5 + (candidateCoverage - baselineCoverage));
}

double BaselineIntelligence::calculateConfidence(
    const Aggregate& baseline,
    const Aggregate& candidate) noexcept {
    if (baseline.samples == 0U || candidate.samples == 0U) return 0.0;

    const double baselineCoverage =
        static_cast<double>(
            std::max({baseline.thermal_samples,
                      baseline.load_samples,
                      baseline.memory_samples}))
        / static_cast<double>(baseline.samples);

    const double candidateCoverage =
        static_cast<double>(
            std::max({candidate.thermal_samples,
                      candidate.load_samples,
                      candidate.memory_samples}))
        / static_cast<double>(candidate.samples);

    const double sampleConfidence =
        std::min(1.0,
                 static_cast<double>(
                     std::min(baseline.samples, candidate.samples)) /
                 static_cast<double>(kObservationSamples));

    return clamp01(
        (0.40 * sampleConfidence) +
        (0.30 * baselineCoverage) +
        (0.30 * candidateCoverage));
}

bool BaselineIntelligence::captureBaseline(
    const RuntimeSample& sample) noexcept {
    if (phase_ == Phase::Observing) return false;

    accumulate(baseline_, sample);

    if (baseline_.samples >= kBaselineSamples) {
        phase_ = Phase::BaselineReady;
        evaluation_ = Evaluation{};
        return true;
    }

    return false;
}

bool BaselineIntelligence::beginObservation() noexcept {
    if (!baselineReady() || phase_ == Phase::Observing) return false;

    observation_.reset();
    evaluation_ = Evaluation{};
    phase_ = Phase::Observing;
    return true;
}

bool BaselineIntelligence::observe(const RuntimeSample& sample) noexcept {
    if (phase_ != Phase::Observing) return false;

    accumulate(observation_, sample);

    if (observation_.samples < kObservationSamples) return false;

    evaluation_ = evaluate();
    phase_ = Phase::BaselineReady;
    return true;
}

BaselineIntelligence::Evaluation
BaselineIntelligence::evaluate() const noexcept {
    Evaluation result{};
    result.baseline_samples = baseline_.samples;
    result.observation_samples = observation_.samples;

    if (baseline_.samples < kBaselineSamples ||
        observation_.samples < kObservationSamples) {
        result.outcome = Outcome::Inconclusive;
        return result;
    }

    double weightedScore = 0.0;
    double totalWeight = 0.0;

    if (baseline_.thermal_samples > 0U &&
        observation_.thermal_samples > 0U) {
        const double baseline =
            safeAverage(baseline_.thermal_sum_c,
                        baseline_.thermal_samples);
        const double candidate =
            safeAverage(observation_.thermal_sum_c,
                        observation_.thermal_samples);

        result.thermal_score = lowerIsBetter(baseline, candidate);
        result.thermal_available = true;
        weightedScore += result.thermal_score * kThermalWeight;
        totalWeight += kThermalWeight;
    }

    if (baseline_.load_samples > 0U &&
        observation_.load_samples > 0U) {
        const double baseline =
            safeAverage(baseline_.load_sum, baseline_.load_samples);
        const double candidate =
            safeAverage(observation_.load_sum, observation_.load_samples);

        result.load_score = lowerIsBetter(baseline, candidate);
        weightedScore += result.load_score * kLoadWeight;
        totalWeight += kLoadWeight;
    }

    if (baseline_.cpu_samples > 0U &&
        observation_.cpu_samples > 0U) {
        const double baseline =
            safeAverage(baseline_.cpu_sum, baseline_.cpu_samples);
        const double candidate =
            safeAverage(observation_.cpu_sum, observation_.cpu_samples);

        result.cpu_score = lowerIsBetter(baseline, candidate);
        result.cpu_available = true;
        weightedScore += result.cpu_score * kCpuWeight;
        totalWeight += kCpuWeight;
    }

    if (baseline_.memory_samples > 0U &&
        observation_.memory_samples > 0U) {
        const double baseline =
            safeAverage(baseline_.memory_sum, baseline_.memory_samples);
        const double candidate =
            safeAverage(observation_.memory_sum, observation_.memory_samples);

        result.memory_score = higherIsBetter(baseline, candidate);
        result.memory_available = true;
        weightedScore += result.memory_score * kMemoryWeight;
        totalWeight += kMemoryWeight;
    }

    result.stability_score =
        calculateStabilityScore(baseline_, observation_);

    weightedScore += result.stability_score * kStabilityWeight;
    totalWeight += kStabilityWeight;

    if (totalWeight <= 0.0) {
        result.outcome = Outcome::Inconclusive;
        return result;
    }

    result.overall_score = clamp01(weightedScore / totalWeight);
    result.confidence = calculateConfidence(baseline_, observation_);
    result.valid = result.confidence > 0.0;

    const double delta = result.overall_score - 0.5;

    if (!result.valid) {
        result.outcome = Outcome::Inconclusive;
    } else if (delta >= kBeneficialThreshold) {
        result.outcome = Outcome::Beneficial;
    } else if (delta <= kRegressionThreshold) {
        result.outcome = Outcome::Regression;
    } else {
        result.outcome = Outcome::Neutral;
    }

    return result;
}

} // namespace coreflow
