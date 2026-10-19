#include "coreflow/baseline_intelligence.hpp"

#include <algorithm>
#include <cmath>

namespace coreflow {
namespace {

constexpr double kThermalWeight = 0.22;
constexpr double kLoadWeight = 0.12;
constexpr double kCpuWeight = 0.10;
constexpr double kMemoryWeight = 0.12;
constexpr double kStabilityWeight = 0.16;
constexpr double kPsiWeight = 0.18;
constexpr double kIoLatencyWeight = 0.10;

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
    point.load_available = sample.load_available;
    point.load = sample.load1;
    point.cpu_available = sample.cpu_utilization_available;
    point.cpu = sample.cpu_utilization;
    point.memory_available = sample.mem_total_kb > 0U;
    point.memory = sample.mem_available_ratio;
    point.charging_telemetry = sample.charging_telemetry_available;
    point.charging = sample.charging;
    point.psi_available = sample.psi_available;
    if (sample.psi_available) {
        point.psi_pressure = std::max({
            sample.psi_cpu_some_avg10,
            sample.psi_memory_some_avg10,
            sample.psi_io_some_avg10,
            sample.throttle_ratio,
            sample.memory_stall_score});
    }
    point.io_latency_available = sample.io_latency_available;
    point.io_latency_ms = sample.io_avg_latency_ms;
    return point;
}

void BaselineIntelligence::accumulate(
    Aggregate& aggregate,
    const BaselinePoint& point) noexcept {
    ++aggregate.samples;

    if (point.thermal) {
        accumulateMetric(aggregate.thermal_c, point.thermal_c);
    }

    if (point.load_available && finite(point.load) && point.load >= 0.0) {
        accumulateMetric(aggregate.load, point.load);
    }

    if (point.cpu_available && finite(point.cpu) &&
        point.cpu >= 0.0 && point.cpu <= 1.0) {
        accumulateMetric(aggregate.cpu, point.cpu);
    }

    if (point.memory_available && finite(point.memory) &&
        point.memory >= 0.0 && point.memory <= 1.0) {
        accumulateMetric(aggregate.memory, point.memory);
    }

    if (point.psi_available && finite(point.psi_pressure) &&
        point.psi_pressure >= 0.0 && point.psi_pressure <= 1.0) {
        accumulateMetric(aggregate.psi_pressure, point.psi_pressure);
    }

    if (point.io_latency_available && finite(point.io_latency_ms) &&
        point.io_latency_ms >= 0.0) {
        accumulateMetric(aggregate.io_latency_ms, point.io_latency_ms);
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
    if (!finite(baseline) || !finite(candidate)) {
        return 0.5;
    }
    // Relative change needs a positive baseline. A non-positive baseline
    // (e.g. zero load) cannot form a ratio; treat as neutral rather than
    // inventing an improvement or regression.
    if (baseline <= 0.0) {
        return 0.5;
    }

    const double relativeChange = (baseline - candidate) / baseline;
    return clamp01(0.5 + (relativeChange * 2.0));
}

double BaselineIntelligence::higherIsBetter(
    double baseline,
    double candidate) noexcept {
    if (!finite(baseline) || !finite(candidate)) {
        return 0.5;
    }
    // Memory available ratio can legitimately be 0.0 (fully used). A relative
    // ratio is undefined then; score a clear absolute recovery modestly and
    // stay neutral otherwise so a zero baseline cannot force Beneficial.
    if (baseline <= 0.0) {
        return (candidate > 0.0) ? 0.60 : 0.5;
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
    const Aggregate& candidate,
    bool contextCompatibleValue) noexcept {
    double score = 0.0;
    double weight = 0.0;

    if (contextCompatibleValue && baseline.thermal_c.samples > 1U &&
        candidate.thermal_c.samples > 1U) {
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

    const auto coverageCount = [](std::size_t metricSamples,
                                  std::size_t sampleCount) noexcept {
        if (sampleCount == 0U) return 0.0;
        return clamp01(static_cast<double>(metricSamples) /
                       static_cast<double>(sampleCount));
    };
    const auto coverage = [&coverageCount](const MetricAggregate& metric,
                                           std::size_t sampleCount) noexcept {
        return coverageCount(metric.samples, sampleCount);
    };

    // Average coverage across dimensions rather than taking the maximum. A
    // single fully available sensor must not report near-perfect confidence
    // while all other telemetry is missing. Charging availability is included
    // as context evidence, not as an outcome score.
    const double baselineCoverage = (
        (contextCompatibleValue ? coverage(baseline.thermal_c, baseline.samples) : 0.0) +
        coverage(baseline.load, baseline.samples) +
        coverage(baseline.cpu, baseline.samples) +
        coverage(baseline.memory, baseline.samples) +
        coverage(baseline.psi_pressure, baseline.samples) +
        coverage(baseline.io_latency_ms, baseline.samples) +
        coverageCount(baseline.charging_telemetry_samples, baseline.samples)) / 7.0;

    const double candidateCoverage = (
        (contextCompatibleValue ? coverage(candidate.thermal_c, candidate.samples) : 0.0) +
        coverage(candidate.load, candidate.samples) +
        coverage(candidate.cpu, candidate.samples) +
        coverage(candidate.memory, candidate.samples) +
        coverage(candidate.psi_pressure, candidate.samples) +
        coverage(candidate.io_latency_ms, candidate.samples) +
        coverageCount(candidate.charging_telemetry_samples, candidate.samples)) / 7.0;

    const double sampleConfidence = std::min(
        1.0,
        static_cast<double>(std::min(baseline.samples, candidate.samples)) /
            static_cast<double>(kObservationSamples));

    double confidence =
        (0.45 * sampleConfidence) +
        (0.30 * baselineCoverage) +
        (0.25 * candidateCoverage);

    if (!contextCompatibleValue) confidence *= 0.50;
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

    const auto paired = [](const MetricAggregate& before,
                           const MetricAggregate& after) noexcept {
        return before.samples > 0U && after.samples > 0U;
    };
    result.load_available = paired(baseline_.load, observation_.load);
    result.comparable_metrics =
        static_cast<std::size_t>(result.context_compatible &&
            paired(baseline_.thermal_c, observation_.thermal_c)) +
        static_cast<std::size_t>(result.load_available) +
        static_cast<std::size_t>(paired(baseline_.cpu, observation_.cpu)) +
        static_cast<std::size_t>(paired(baseline_.memory, observation_.memory)) +
        static_cast<std::size_t>(paired(baseline_.psi_pressure, observation_.psi_pressure)) +
        static_cast<std::size_t>(paired(baseline_.io_latency_ms, observation_.io_latency_ms));

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

    result.stability_score = calculateStabilityScore(
        baseline_, observation_, result.context_compatible);
    const bool stabilityEvidence =
        (result.context_compatible && baseline_.thermal_c.samples > 1U &&
         observation_.thermal_c.samples > 1U) ||
        (baseline_.load.samples > 1U && observation_.load.samples > 1U) ||
        (baseline_.cpu.samples > 1U && observation_.cpu.samples > 1U) ||
        (baseline_.memory.samples > 1U && observation_.memory.samples > 1U);
    if (stabilityEvidence) {
        weightedScore += result.stability_score * kStabilityWeight;
        totalWeight += kStabilityWeight;
    }

    if (baseline_.psi_pressure.samples > 0U &&
        observation_.psi_pressure.samples > 0U) {
        result.psi_score = lowerIsBetter(
            average(baseline_.psi_pressure),
            average(observation_.psi_pressure));
        result.psi_available = true;
        weightedScore += result.psi_score * kPsiWeight;
        totalWeight += kPsiWeight;
    }

    if (baseline_.io_latency_ms.samples > 0U &&
        observation_.io_latency_ms.samples > 0U) {
        result.io_latency_score = lowerIsBetter(
            average(baseline_.io_latency_ms),
            average(observation_.io_latency_ms));
        result.io_latency_available = true;
        weightedScore += result.io_latency_score * kIoLatencyWeight;
        totalWeight += kIoLatencyWeight;
    }

    if (totalWeight <= 0.0) {
        result.outcome = Outcome::Inconclusive;
        return result;
    }

    result.overall_score = clamp01(weightedScore / totalWeight);
    result.confidence = calculateConfidence(
        baseline_, observation_, result.context_compatible);
    result.valid = result.confidence >= 0.50 && result.comparable_metrics >= 2U;

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
