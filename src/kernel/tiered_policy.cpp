#include "coreflow/tiered_policy.hpp"

#include <cmath>
#include <cstdint>

namespace coreflow {
namespace {

constexpr uint32_t kCpuPressureEvidence = FeatureCpuPressure;

bool has_cpu_pressure_evidence(const PolicyEvidence& evidence) noexcept {
    const auto& telemetry = evidence.telemetry;
    const auto& classification = evidence.classification;

    if (!evidence.telemetry_fresh || evidence.event_stream_saturated ||
        telemetry.event_drain_saturated || telemetry.source == TelemetryBackend::Unavailable ||
        (telemetry.valid_features & kCpuPressureEvidence) == 0U ||
        (classification.evidence_mask & kCpuPressureEvidence) == 0U) {
        return false;
    }

    if (telemetry.window_start_ns == 0U ||
        telemetry.window_end_ns <= telemetry.window_start_ns ||
        evidence.evaluated_at_ns < telemetry.window_end_ns ||
        evidence.max_telemetry_age_ns == 0U ||
        evidence.evaluated_at_ns - telemetry.window_end_ns > evidence.max_telemetry_age_ns ||
        classification.classified_at_ns != telemetry.window_end_ns) {
        return false;
    }

    if (!std::isfinite(telemetry.cpu_pressure_some_pct) ||
        telemetry.cpu_pressure_some_pct < 0.0 ||
        telemetry.cpu_pressure_some_pct > 100.0 ||
        !std::isfinite(classification.confidence) ||
        classification.confidence < 0.0F ||
        classification.confidence > 1.0F) {
        return false;
    }

    return classification.workload == DetectedWorkloadClass::CpuBound &&
           classification.stable;
}

PolicyDecision base_decision(const PolicyEvidence& evidence) noexcept {
    PolicyDecision decision{};
    decision.evidence_mask = evidence.telemetry.valid_features &
                             evidence.classification.evidence_mask;
    decision.confidence = evidence.classification.confidence;
    return decision;
}

PolicyDecision evaluate_policy(
    const PolicyEvidence& evidence,
    double tier1_cpu_psi,
    float tier1_confidence,
    uint32_t tier1_windows,
    double tier2_cpu_psi,
    float tier2_confidence,
    uint32_t tier2_windows,
    double tier1_headroom_c,
    double tier2_headroom_c,
    double tier1_scale,
    double tier2_scale,
    uint32_t tier1_settle_ms,
    uint32_t tier2_settle_ms) noexcept {

    PolicyDecision decision = base_decision(evidence);

    // Thermal recovery is a higher-priority control objective than workload
    // performance. It is intentionally independent of CPU/GPU telemetry.
    if (evidence.thermal_recovery_required) {
        decision.tier = OptimizationTier::Tier3ThermalRecovery;
        decision.action = PolicyAction::ThermalRecovery;
        decision.requested_capacity_scale = 1.0;
        decision.settle_window_ms = 250U;
        decision.rationale = "thermal recovery preempts performance policy";
        decision.requires_mutation_admission = evidence.mutation_runtime_ready;
        return decision;
    }

    if (!has_cpu_pressure_evidence(evidence) ||
        evidence.classification.confidence < tier1_confidence ||
        evidence.classification.consecutive_windows < tier1_windows) {
        decision.rationale = "CPU-bound evidence is stale, incomplete, or unstable";
        return decision;
    }

    const double cpu_psi = evidence.telemetry.cpu_pressure_some_pct;
    if (cpu_psi < tier1_cpu_psi) {
        decision.rationale = "CPU pressure is below this policy's response threshold";
        return decision;
    }

    decision.tier = OptimizationTier::Tier1InteractiveResponse;
    decision.action = PolicyAction::CpuInteractiveResponse;
    decision.settle_window_ms = tier1_settle_ms;
    decision.rationale = "stable CPU-bound workload; interactive response candidate";

    const bool thermal_admitted =
        evidence.thermal_headroom_valid &&
        std::isfinite(evidence.thermal_headroom_c) &&
        evidence.thermal_headroom_c >= tier1_headroom_c;

    const bool frequency_admitted =
        evidence.cpu_frequency_capability_valid &&
        evidence.mutation_runtime_ready &&
        thermal_admitted;

    if (cpu_psi >= tier2_cpu_psi &&
        evidence.classification.confidence >= tier2_confidence &&
        evidence.classification.consecutive_windows >= tier2_windows) {
        const bool sustained_admitted =
            frequency_admitted &&
            evidence.thermal_headroom_c >= tier2_headroom_c;

        if (sustained_admitted) {
            decision.tier = OptimizationTier::Tier2SustainedOptimization;
            decision.action = PolicyAction::CpuFrequencyStep;
            decision.requested_capacity_scale = tier2_scale;
            decision.settle_window_ms = tier2_settle_ms;
            decision.rationale = "sustained CPU contention with validated thermal and frequency admission";
            decision.requires_mutation_admission = true;
            return decision;
        }

        decision.rationale = "sustained CPU contention detected; frequency admission withheld";
        return decision;
    }

    // Tier 1 can request a small, reversible step only after all independent
    // actuator gates are present. Otherwise it remains a non-mutating intent.
    if (frequency_admitted) {
        decision.action = PolicyAction::CpuFrequencyStep;
        decision.requested_capacity_scale = tier1_scale;
        decision.requires_mutation_admission = true;
        decision.rationale = "interactive CPU contention with validated frequency admission";
    }

    return decision;
}

} // namespace

PolicyDecision CoreFlowBase::evaluate(const PolicyEvidence& evidence) const noexcept {
    return evaluate_policy(
        evidence,
        20.0, 0.78F, 3U,
        38.0, 0.82F, 5U,
        8.0, 12.0,
        1.02, 1.05,
        750U, 1000U);
}

PolicyDecision HyperOS4Inspired::evaluate(const PolicyEvidence& evidence) const noexcept {
    return evaluate_policy(
        evidence,
        15.0, 0.70F, 2U,
        30.0, 0.74F, 4U,
        6.0, 9.0,
        1.03, 1.08,
        500U, 750U);
}

TieredPolicySelection TieredPolicyRouter::select(
    const PlatformContextSnapshot& platform) const noexcept {
    if (platform.xiaomi_policy_enabled()) {
        return {&hyperos_, true};
    }
    return {&base_, false};
}

PolicyDecision TieredPolicyRouter::evaluate(
    const PlatformContextSnapshot& platform,
    const PolicyEvidence& evidence) const noexcept {
    const TieredPolicySelection selection = select(platform);
    const OptimizationPolicy* const policy =
        selection.policy != nullptr ? selection.policy : &base_;
    return policy->evaluate(evidence);
}

} // namespace coreflow
