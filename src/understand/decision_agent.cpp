#include "coreflow/decision_agent.hpp"

#include <memory>
#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>

namespace coreflow {
namespace {

std::string formatProposalSummary(const EffectScore& best, std::size_t ranked_count) {
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
        << " conf=" << best.confidence;
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

    // Stage 3 — review every candidate against uncertainty and stress context.
    // Ranking may demote candidates but cannot make an ineligible candidate
    // eligible. A stressed context accepts only explicitly stabilizing actions.
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
    proposal.reasoning_summary = "observe → hypothesize → review → propose: " +
        formatProposalSummary(proposal.best, proposal.ranked.size());
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
