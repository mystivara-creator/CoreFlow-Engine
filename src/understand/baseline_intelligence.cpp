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
constexpr double kContextMismatchTolerance = 0.40;
// Minimum coefficient of variation treated as real variability. Sensors are
// quantized (thermal often 1 C steps), so a perfectly flat baseline followed by
// a single 1 C step must not be scored as a total stability collapse.
constexpr double kStabilityCvFloor = 0.02;

} // namespace

void BaselineIntelligence::reset() noexcept {
    window_count_ = 0;
    window_head_ = 0;
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

void BaselineIntelligence::accumulateMetric(
    MetricAggregate& metric,
    double value) noexcept {
    if (!finite(value)) return;

    metric.sum += value;
    metric.sum_sq += value * value;
    ++metric.samples;
}

double BaselineIntelligence::average(
    const MetricAggregate& metric) noexcept {
    if (metric.samples == 0U) return 0.0;
    return metric.sum / static_cast<double>(metric.samples);
}

double BaselineIntelligence::variance(
    const MetricAggregate& metric) noexcept {
    if (metric.samples < 2U) return 0.0;

    const double count = static_cast<double>(metric.samples);
    const double mean = metric.sum / count;
    const double raw = (metric.sum_sq / count) - (mean * mean);
    return std::max(0.0, raw);
}

double BaselineIntelligence::standardDeviation(
    const MetricAggregate& metric) noexcept {
    return std::sqrt(variance(metric));
}

BaselineIntelligence::BaselinePoint BaselineIntelligence::project(
    const RuntimeSample& sample) noexcept {
    BaselinePoint point;
    point.thermal = sample.thermal_available;
    point.thermal_c = static_cast<double>(sample.hottest_thermal_millidegrees) / 1000.0;
    point.load = sample.load1;
    point.cpu_available = sample.cpu_utilization_available;
    point.cpu = sample.cpu_utilization;
    point.memory = sample.mem_available_ratio;
    point.charging_telemetry = sample.charging_telemetry_available;
    point.charging = sample.charging;
    return point;
}

void BaselineIntelligence::accumulate(
    Aggregate& aggregate,
    const BaselinePoint& point) noexcept {
    ++aggregate.samples;

    if (point.thermal) {
        accumulateMetric(aggregate.thermal_c, point.thermal_c);
    }

    if (finite(point.load) && point.load >= 0.0) {
        accumulateMetric(aggregate.load, point.load);
    }

    if (point.cpu_available && finite(point.cpu) &&
        point.cpu >= 0.0 && point.cpu <= 1.0) {
        accumulateMetric(aggregate.cpu, point.cpu);
    }

    if (finite(point.memory) && point.memory >= 0.0 && point.memory <= 1.0) {
        accumulateMetric(aggregate.memory, point.memory);
    }

    if (point.charging_telemetry) {
        ++aggregate.charging_telemetry_samples;
        if (point.charging) {
            ++aggregate.charging_true_samples;
        }
    }
}

void BaselineIntelligence::accumulate(
    Aggregate& aggregate,
    const RuntimeSample& sample) noexcept {
    accumulate(aggregate, project(sample));
}

void BaselineIntelligence::rebuildBaseline() noexcept {
    baseline_.reset();
    for (std::size_t i = 0; i < window_count_; ++i) {
        accumulate(baseline_, window_[i]);
    }
}

double BaselineIntelligence::lowerIsBetter(
    double baseline,
    double candidate) noexcept {
    if (!finite(baseline) || !finite(candidate) || baseline <= 0.0) {
        return 0.5;
    }

    const double relativeChange = (baseline - candidate) / baseline;
    return clamp01(0.5 + (relativeChange * 2.0));
}

double BaselineIntelligence::higherIsBetter(
    double baseline,
    double candidate) noexcept {
    if (!finite(baseline) || !finite(candidate) || baseline <= 0.0) {
        return 0.5;
    }

    const double relativeChange = (candidate - baseline) / baseline;
    return clamp01(0.5 + (relativeChange * 2.0));
}

double BaselineIntelligence::stabilityScore(
    const MetricAggregate& baseline,
    const MetricAggregate& candidate) noexcept {
    if (baseline.samples < 2U || candidate.samples < 2U) {
        return 0.5;
    }

    const double baselineMean = std::abs(average(baseline));
    const double candidateMean = std::abs(average(candidate));
    const double baselineScale = std::max(baselineMean, 1.0e-6);
    const double candidateScale = std::max(candidateMean, 1.0e-6);

    const double baselineCv =
        standardDeviation(baseline) / baselineScale;
    const double candidateCv =
        standardDeviation(candidate) / candidateScale;

    if (!finite(baselineCv) || !finite(candidateCv)) {
        return 0.5;
    }

    // Variability below the noise floor is equivalent; only a change that
    // exceeds sensor quantization counts as more or less stable.
    const double flooredBaselineCv = std::max(baselineCv, kStabilityCvFloor);
    const double flooredCandidateCv = std::max(candidateCv, kStabilityCvFloor);

    const double relativeChange =
        (flooredBaselineCv - flooredCandidateCv) / flooredBaselineCv;
    return clamp01(0.5 + (relativeChange * 2.0));
}

double BaselineIntelligence::calculateStabilityScore(
    const Aggregate& baseline,
    const Aggregate& candidate) noexcept {
    double score = 0.0;
    double weight = 0.0;

    if (baseline.thermal_c.samples > 1U && candidate.thermal_c.samples > 1U) {
        score += stabilityScore(baseline.thermal_c, candidate.thermal_c) * 0.40;
        weight += 0.40;
    }

    if (baseline.load.samples > 1U && candidate.load.samples > 1U) {
        score += stabilityScore(baseline.load, candidate.load) * 0.20;
        weight += 0.20;
    }

    if (baseline.cpu.samples > 1U && candidate.cpu.samples > 1U) {
        score += stabilityScore(baseline.cpu, candidate.cpu) * 0.20;
        weight += 0.20;
    }

    if (baseline.memory.samples > 1U && candidate.memory.samples > 1U) {
        score += stabilityScore(baseline.memory, candidate.memory) * 0.20;
        weight += 0.20;
    }

    if (weight <= 0.0) return 0.5;
    return clamp01(score / weight);
}

bool BaselineIntelligence::contextCompatible(
    const Aggregate& baseline,
    const Aggregate& candidate) noexcept {
    if (baseline.charging_telemetry_samples == 0U ||
        candidate.charging_telemetry_samples == 0U) {
        return true;
    }

    const double baselineRatio =
        static_cast<double>(baseline.charging_true_samples) /
        static_cast<double>(baseline.charging_telemetry_samples);
    const double candidateRatio =
        static_cast<double>(candidate.charging_true_samples) /
        static_cast<double>(candidate.charging_telemetry_samples);

    return std::abs(baselineRatio - candidateRatio) <=
           kContextMismatchTolerance;
}

double BaselineIntelligence::calculateConfidence(
    const Aggregate& baseline,
    const Aggregate& candidate,
    bool contextCompatibleValue) noexcept {
    if (baseline.samples == 0U || candidate.samples == 0U) return 0.0;

    const double baselineCoverage =
        static_cast<double>(std::max({
            baseline.thermal_c.samples,
            baseline.load.samples,
            baseline.cpu.samples,
            baseline.memory.samples})) /
        static_cast<double>(baseline.samples);

    const double candidateCoverage =
        static_cast<double>(std::max({
            candidate.thermal_c.samples,
            candidate.load.samples,
            candidate.cpu.samples,
            candidate.memory.samples})) /
        static_cast<double>(candidate.samples);

    const double sampleConfidence = std::min(
        1.0,
        static_cast<double>(std::min(baseline.samples, candidate.samples)) /
            static_cast<double>(kObservationSamples));

    double confidence =
        (0.45 * sampleConfidence) +
        (0.30 * baselineCoverage) +
        (0.25 * candidateCoverage);

    if (!contextCompatibleValue) {
        confidence *= 0.50;
    }

    return clamp01(confidence);
}

bool BaselineIntelligence::captureBaseline(
    const RuntimeSample& sample) noexcept {
    // The baseline is frozen while a candidate is being observed.
    if (phase_ == Phase::Observing) return false;

    // Rolling window: the oldest sample is replaced, so the baseline always
    // describes the workload immediately before the next candidate.
    window_[window_head_] = project(sample);
    window_head_ = (window_head_ + 1U) % kBaselineSamples;
    if (window_count_ < kBaselineSamples) ++window_count_;
    rebuildBaseline();

    if (baseline_.samples >= kBaselineSamples) {
        if (phase_ != Phase::BaselineReady) {
            phase_ = Phase::BaselineReady;
            evaluation_ = Evaluation{};
        }
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

bool BaselineIntelligence::observe(
    const RuntimeSample& sample) noexcept {
    if (phase_ != Phase::Observing) return false;

    accumulate(observation_, sample);

    if (observation_.samples < kObservationSamples) return false;

    evaluation_ = evaluate();
    // The pre-mutation window no longer describes the device: the candidate was
    // either retained (new operating point) or rolled back. Require a fresh
    // window before the next candidate can be judged. evaluation_ stays readable.
    window_count_ = 0;
    window_head_ = 0;
    baseline_.reset();
    observation_.reset();
    phase_ = Phase::Idle;
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

    result.context_compatible =
        contextCompatible(baseline_, observation_);

    double weightedScore = 0.0;
    double totalWeight = 0.0;

    if (baseline_.thermal_c.samples > 0U &&
        observation_.thermal_c.samples > 0U &&
        result.context_compatible) {
        result.thermal_score = lowerIsBetter(
            average(baseline_.thermal_c),
            average(observation_.thermal_c));
        result.thermal_available = true;
        weightedScore += result.thermal_score * kThermalWeight;
        totalWeight += kThermalWeight;
    }

    if (baseline_.load.samples > 0U &&
        observation_.load.samples > 0U) {
        result.load_score = lowerIsBetter(
            average(baseline_.load),
            average(observation_.load));
        weightedScore += result.load_score * kLoadWeight;
        totalWeight += kLoadWeight;
    }

    if (baseline_.cpu.samples > 0U &&
        observation_.cpu.samples > 0U) {
        result.cpu_score = lowerIsBetter(
            average(baseline_.cpu),
            average(observation_.cpu));
        result.cpu_available = true;
        weightedScore += result.cpu_score * kCpuWeight;
        totalWeight += kCpuWeight;
    }

    if (baseline_.memory.samples > 0U &&
        observation_.memory.samples > 0U) {
        result.memory_score = higherIsBetter(
            average(baseline_.memory),
            average(observation_.memory));
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
    result.confidence = calculateConfidence(
        baseline_, observation_, result.context_compatible);
    result.valid = result.confidence >= 0.50;

    if (!result.valid) {
        result.outcome = Outcome::Inconclusive;
        return result;
    }

    const double delta = result.overall_score - 0.5;

    if (delta >= kBeneficialThreshold) {
        result.outcome = Outcome::Beneficial;
    } else if (delta <= kRegressionThreshold) {
        result.outcome = Outcome::Regression;
    } else {
        result.outcome = Outcome::Neutral;
    }

    return result;
}

} // namespace coreflow
