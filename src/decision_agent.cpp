#include "coreflow/decision_agent.hpp"

#include <memory>

namespace coreflow {

AgentProposal ScoringDecisionAgent::reason(const AgentObservation& obs) const noexcept {
    AgentProposal proposal;
    proposal.mode = AgentReasoningMode::ScoringOnly;

    if (obs.matrix == nullptr || obs.baselines == nullptr) {
        proposal.reasoning_summary = "missing capability matrix or baselines";
        return proposal;
    }

    // Classic EffectModel ranking — behaviour preserved from prior releases.
    auto ranked = model_.rank(*obs.matrix, *obs.baselines, obs.effect_ctx, obs.max_candidates);
    if (ranked.empty()) {
        proposal.reasoning_summary = "no eligible bounded candidates";
        return proposal;
    }

    // Under stress we only accept stabilizing proposals.
    if (obs.stabilizing_allowed_by_context && !obs.mutation_allowed_by_context) {
        std::vector<EffectScore> stabilizing;
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
    proposal.reasoning_summary = proposal.best.reason.empty()
        ? "effect-model ranked candidate"
        : proposal.best.reason;

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
