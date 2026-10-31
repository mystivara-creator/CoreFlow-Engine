#include "coreflow/decision_agent.hpp"
#include "coreflow/runtime_intelligence.hpp"

#include <memory>
#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>

namespace coreflow {
namespace {

struct ParetoWeights {
    double perf{0.25};
    double latency{0.25};
    double thermal{0.25};
    double power{0.25};
};

ParetoWeights deriveParetoWeights(const EffectContext& ctx, RuntimeState state) noexcept {
    double w_perf = 0.22;
    double w_lat = 0.22;
    double w_therm = 0.30;
    double w_pwr = 0.26;

    if (state == RuntimeState::ThermalGuard ||
        (ctx.thermal_available && ctx.thermal_c >= 52.0) ||
        ctx.throttling_detected) {
        w_therm += 0.45;
        w_pwr += 0.15;
        w_perf = 0.08;
        w_lat = 0.12;
    } else if (ctx.gaming_active ||
               (ctx.gpu_busy_available && ctx.gpu_busy_ratio >= 0.45) ||
               (ctx.frame_jank_available && ctx.frame_jank_rate >= 0.04)) {
        w_perf += 0.24;
        w_lat += 0.30;
        if (ctx.thermal_available && ctx.thermal_c >= 48.0) {
            w_therm += 0.14;
        }
        w_pwr = ctx.charging ? 0.10 : 0.16;
    } else if (ctx.cpu_utilization_available && ctx.cpu_utilization >= 0.65) {
        w_perf += 0.20;
        w_lat += 0.18;
    } else if (state == RuntimeState::Idle) {
        w_pwr += 0.32;
        w_therm += 0.18;
        w_perf = 0.10;
        w_lat = 0.12;
    }

    const double sum = std::max(1e-6, w_perf + w_lat + w_therm + w_pwr);
    return ParetoWeights{w_perf / sum, w_lat / sum, w_therm / sum, w_pwr / sum};
}

double evaluateParetoAlignment(
    const EffectScore& candidate,
    const ParetoWeights& w,
    const EnvironmentCapabilityMatrix& matrix,
    const std::unordered_map<std::string, std::string>& baselines) noexcept {
    double perf_axis = candidate.benefit;
    double latency_axis = candidate.benefit;
    double thermal_axis = candidate.stabilizing ? 0.85 : (1.0 - candidate.risk);
    double power_axis = candidate.stabilizing ? 0.80 : (1.0 - candidate.risk * 0.85);

    switch (candidate.domain) {
        case ResourceDomain::UClamp:
        case ResourceDomain::Scheduler:
        case ResourceDomain::Gpu:
            if (!candidate.stabilizing) {
                latency_axis = std::clamp(latency_axis + 0.12, 0.0, 1.0);
                perf_axis = std::clamp(perf_axis + 0.08, 0.0, 1.0);
            }
            break;
        case ResourceDomain::CpuFreq:
            if (!candidate.stabilizing) {
                perf_axis = std::clamp(perf_axis + 0.10, 0.0, 1.0);
                latency_axis = std::clamp(latency_axis + 0.08, 0.0, 1.0);
            }
            break;
        case ResourceDomain::CpuSet:
            thermal_axis = std::clamp(thermal_axis + 0.08, 0.0, 1.0);
            power_axis = std::clamp(power_axis + 0.08, 0.0, 1.0);
            break;
        case ResourceDomain::Io:
            latency_axis = std::clamp(latency_axis + 0.08, 0.0, 1.0);
            break;
        default:
            break;
    }

    // Silicon Potential Gap detection: check if the hardware capability has a
    // vendor Performance operating point or upper OPP scale currently capped by
    // the OEM factory baseline.
    double silicon_gap_bonus = 0.0;
    if (!candidate.stabilizing &&
        (candidate.source == CandidateSource::VendorPerformance ||
         candidate.source == CandidateSource::OrderedScale ||
         candidate.source == CandidateSource::BoundedStep)) {
        const auto base_it = baselines.find(candidate.path);
        if (base_it != baselines.end() && base_it->second != candidate.requested) {
            for (const auto& cap : matrix.resources) {
                if (cap.path != candidate.path) continue;
                for (const auto& vv : cap.vendor_values) {
                    if (vv.profile == VendorProfile::Performance &&
                        vv.value != base_it->second) {
                        silicon_gap_bonus = 0.025;
                        break;
                    }
                }
                if (silicon_gap_bonus == 0.0 && cap.ordered_scale && cap.options.size() >= 4U) {
                    silicon_gap_bonus = 0.015;
                }
                break;
            }
        }
    }

    const double pareto =
        w.perf * perf_axis +
        w.latency * latency_axis +
        w.thermal * thermal_axis +
        w.power * power_axis;
    // Small centered multi-objective refinement around EffectModel's base utility.
    return (pareto - 0.50) * 0.06 + silicon_gap_bonus;
}

std::string formatProposalSummary(
    const EffectScore& best,
    std::size_t ranked_count,
    const ParetoWeights& w) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out.precision(3);
    if (!best.resource_name.empty()) {
        out << best.resource_name;
    } else if (!best.path.empty()) {
        out << best.path;
    } else {
        out << "candidate";
    }
    if (!best.requested.empty()) {
        out << " -> " << best.requested;
    }
    out << " | util=" << best.utility
        << " benefit=" << best.benefit
        << " risk=" << best.risk
        << " conf=" << best.confidence
        << " pareto(P=" << w.perf << ",L=" << w.latency << ",T=" << w.thermal << ")";
    if (best.stabilizing) out << " | stabilizing";
    if (ranked_count > 1) out << " | ranked=" << ranked_count;
    if (!best.reason.empty()) out << " | " << best.reason;
    return out.str();
}

} // namespace

AgentProposal ScoringDecisionAgent::reason(const AgentObservation& obs) const noexcept {
    AgentProposal proposal;
    proposal.mode = AgentReasoningMode::ReActLite;

    // Stage 1 — validate the evidence needed to reason about candidate effects.
    if (obs.matrix == nullptr || obs.baselines == nullptr) {
        proposal.reasoning_summary = "observe: missing capability matrix or factory baselines";
        return proposal;
    }
    if (!std::isfinite(obs.sample_confidence) || obs.sample_confidence < 0.0 ||
        obs.sample_confidence > 1.0) {
        proposal.reasoning_summary = "observe: invalid sample confidence; fail closed";
        return proposal;
    }
    if (!obs.mutation_allowed_by_context && !obs.stabilizing_allowed_by_context) {
        proposal.reasoning_summary = "context review: mutation not permitted; observe only";
        return proposal;
    }

    // Stage 2 — generate candidates using semantic priors and the existing
    // capability/authority metadata. This stage never performs I/O or writes.
    auto ranked = model_.rank(
        *obs.matrix, *obs.baselines, obs.effect_ctx, obs.max_candidates);
    if (ranked.empty()) {
        proposal.reasoning_summary = "hypothesis: no bounded eligible candidate";
        return proposal;
    }

    // Stage 3 — multi-objective Pareto + Contextual UCB1-Tuned Bandit + safety review.
    // Ranking may demote candidates or refine ordering among already-eligible
    // candidates, but NEVER makes an ineligible candidate eligible.
    const ParetoWeights pareto_weights = deriveParetoWeights(obs.effect_ctx, obs.state);
    const std::uint64_t total_exp_obs =
        obs.experience != nullptr ? obs.experience->totalObservations() : 0U;

    RuntimeSample signals;
    signals.confidence = obs.sample_confidence;
    signals.thermal_available = obs.effect_ctx.thermal_available;
    signals.throttling_detected = obs.effect_ctx.throttling_detected;
    signals.throttle_ratio = obs.effect_ctx.throttle_ratio;
    signals.psi_available = obs.effect_ctx.psi_available;
    signals.psi_cpu_some_avg10 = obs.effect_ctx.psi_cpu;
    signals.psi_memory_some_avg10 = obs.effect_ctx.psi_memory;
    signals.psi_io_some_avg10 = obs.effect_ctx.psi_io;
    signals.memory_stall_detected = obs.effect_ctx.memory_stall_detected;
    signals.memory_stall_score = obs.effect_ctx.memory_stall_score;
    signals.io_latency_available = obs.effect_ctx.io_latency_available;
    signals.io_avg_latency_ms = obs.effect_ctx.io_avg_latency_ms;
    signals.gpu_busy_available = obs.effect_ctx.gpu_busy_available;
    signals.gpu_busy_ratio = obs.effect_ctx.gpu_busy_ratio;
    signals.frame_jank_available = obs.effect_ctx.frame_jank_available;
    signals.frame_jank_rate = obs.effect_ctx.frame_jank_rate;
    const auto diagnosis = diagnoseRuntime(signals);
    std::vector<EffectScore> reviewed;
    reviewed.reserve(ranked.size());
    for (auto& candidate : ranked) {
        if (!candidate.eligible || !std::isfinite(candidate.utility) ||
            !std::isfinite(candidate.risk) || !std::isfinite(candidate.confidence)) {
            continue;
        }
        if (candidate.risk < 0.0 || candidate.risk > 1.0 ||
            candidate.confidence < 0.0 || candidate.confidence > 1.0) {
            continue;
        }
        if (obs.stabilizing_allowed_by_context && !obs.mutation_allowed_by_context &&
            !candidate.stabilizing) {
            continue;
        }

        // Evidence quality is deliberately a penalty, not a bonus capable of
        // overriding the EffectModel's risk assessment.
        const double evidence_confidence =
            std::min(candidate.confidence, obs.sample_confidence);
        candidate.utility -= (1.0 - evidence_confidence) * 0.04;
        candidate.utility += evaluateParetoAlignment(
            candidate, pareto_weights, *obs.matrix, *obs.baselines);

        if (obs.experience != nullptr && !obs.stabilizing_allowed_by_context) {
            ExperienceMemory::CandidateIdentity id;
            id.key = "resource:" + candidate.path + ":" + candidate.requested;
            const auto* rec = obs.experience->find(id, obs.experience_ctx);
            const double norm_prior = std::clamp((candidate.utility + 0.29) / 1.04, 0.0, 1.0);
            const double ucb_norm = contextualBanditScore(norm_prior, rec, total_exp_obs, 0.065);
            candidate.utility = ucb_norm * 1.04 - 0.29;
        }

        if (!candidate.stabilizing) {
            candidate.utility -= unrelatedResourcePenalty(diagnosis, candidate.domain);
            if (obs.effect_ctx.storage_health_caution && candidate.domain == ResourceDomain::Io)
                candidate.utility -= 0.12;
        }
        if (!std::isfinite(candidate.utility) || candidate.utility <= 0.0) {
            continue;
        }
        reviewed.push_back(std::move(candidate));
    }
    if (reviewed.empty()) {
        proposal.reasoning_summary =
            "safety review: evidence, risk, or context rejected all candidates";
        return proposal;
    }

    // Stage 4 — stable ordering and ambiguity check. When the top two options
    // are nearly tied, retain the ranked alternatives for diagnostics but do
    // not invent confidence or force a decision.
    std::stable_sort(reviewed.begin(), reviewed.end(),
        [](const EffectScore& lhs, const EffectScore& rhs) {
            if (lhs.utility != rhs.utility) return lhs.utility > rhs.utility;
            if (lhs.risk != rhs.risk) return lhs.risk < rhs.risk;
            return lhs.path < rhs.path;
        });

    if (reviewed.size() > obs.max_candidates) reviewed.resize(obs.max_candidates);
    const double best_confidence = obs.sample_confidence;
    if (best_confidence < 0.30) {
        proposal.reasoning_summary = "decision: insufficient confidence; observe only";
        return proposal;
    }
    if (reviewed.size() > 1U &&
        (reviewed[0].utility - reviewed[1].utility) < 0.015 &&
        best_confidence < 0.60) {
        proposal.reasoning_summary =
            "decision: candidates ambiguous under low confidence; observe only";
        return proposal;
    }

    // Stage 5 — emit a proposal only. The caller must still obtain a fresh
    // MutationPermit and use the normal actuator verification/rollback path.
    proposal.ranked = std::move(reviewed);
    proposal.best = proposal.ranked.front();
    proposal.valid = proposal.best.eligible;
    proposal.overall_confidence = best_confidence;
    proposal.reasoning_summary = "observe → pareto+ucb → review → propose: " +
        formatProposalSummary(proposal.best, proposal.ranked.size(), pareto_weights);
    return proposal;
}

std::unique_ptr<IDecisionAgent> createDefaultDecisionAgent() noexcept {
    return std::make_unique<ScoringDecisionAgent>();
}

const char* agentReasoningModeName(AgentReasoningMode mode) noexcept {
    switch (mode) {
        case AgentReasoningMode::ScoringOnly: return "ScoringOnly";
        case AgentReasoningMode::ReActLite:   return "ReActLite";
        case AgentReasoningMode::PlanExecute: return "PlanExecute";
        default: return "Unknown";
    }
}

} // namespace coreflow
