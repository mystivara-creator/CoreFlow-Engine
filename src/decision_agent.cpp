#include "coreflow/decision_agent.hpp"

#include <memory>
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
    proposal.mode = AgentReasoningMode::ScoringOnly;

    if (obs.matrix == nullptr || obs.baselines == nullptr) {
        proposal.reasoning_summary = "missing capability matrix or baselines";
        return proposal;
    }

    auto ranked = model_.rank(
        *obs.matrix, *obs.baselines, obs.effect_ctx, obs.max_candidates);
    if (ranked.empty()) {
        proposal.reasoning_summary = "no eligible bounded candidates";
        return proposal;
    }

    // Under stress only stabilizing proposals are allowed through.
    if (obs.stabilizing_allowed_by_context && !obs.mutation_allowed_by_context) {
        std::vector<EffectScore> stabilizing;
        stabilizing.reserve(ranked.size());
        for (auto& s : ranked) {
            if (s.stabilizing && s.eligible) {
                stabilizing.push_back(std::move(s));
            }
        }
        ranked = std::move(stabilizing);
        if (ranked.empty()) {
            proposal.reasoning_summary = "stressed context; no stabilizing candidates";
            return proposal;
        }
    }

    proposal.ranked = std::move(ranked);
    proposal.best = proposal.ranked.front();
    proposal.valid = proposal.best.eligible;
    proposal.overall_confidence = proposal.best.confidence;
    proposal.reasoning_summary =
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
