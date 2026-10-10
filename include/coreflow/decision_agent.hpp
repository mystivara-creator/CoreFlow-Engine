#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "coreflow/effect_model.hpp"
#include "coreflow/environment.hpp"
#include "coreflow/resource_model.hpp"
#include "coreflow/types.hpp"

namespace coreflow {

// ---------------------------------------------------------------------------
// DecisionAgent — lightweight reasoning layer for CoreFlow (v2.0.1+)
//
// Design goals (inspired by edge-agent patterns, rewritten for CoreFlow):
//   - Observe  → understand device capability + runtime context
//   - Reason   → produce ranked, bounded candidates
//   - Propose  → never write; only hand structured proposals to MutationAuthority
//   - Fail-closed → any uncertainty or missing prior yields ObserveOnly
//
// This interface deliberately does NOT depend on external agent SDKs.
// Implementations may later use local models (ONNX / small LLM) while
// remaining fully on-device and under CoreFlow safety gates.
// ---------------------------------------------------------------------------

enum class AgentReasoningMode : std::uint8_t {
    ScoringOnly = 0,   // classic EffectModel ranking (current default)
    ReActLite,         // reason → act proposal → evaluate (future)
    PlanExecute        // multi-step plan then single best action (future)
};

struct AgentProposal {
    bool valid{false};
    EffectScore best;
    std::vector<EffectScore> ranked;
    std::string reasoning_summary;   // short human-readable why
    AgentReasoningMode mode{AgentReasoningMode::ScoringOnly};
    double overall_confidence{0.0};
};

struct AgentObservation {
    EffectContext effect_ctx;
    const EnvironmentCapabilityMatrix* matrix{nullptr};
    const std::unordered_map<std::string, std::string>* baselines{nullptr};
    const ResourceStateModel* resources{nullptr};
    RuntimeState state{RuntimeState::Idle};
    double sample_confidence{0.0};
    bool mutation_allowed_by_context{false};
    bool stabilizing_allowed_by_context{false};
};

// Pure interface. Implementations must be noexcept where possible and
// must never perform I/O or kernel writes.
class IDecisionAgent {
public:
    virtual ~IDecisionAgent() = default;

    virtual AgentReasoningMode mode() const noexcept = 0;

    // Produce a proposal from the current observation.
    // Must return a valid=false proposal when no safe action exists.
    virtual AgentProposal reason(const AgentObservation& obs) const noexcept = 0;
};

// Default implementation: thin wrapper around the existing EffectModel.
// Keeps behaviour identical to v2.0.x / early v2.1 while opening the door
// for richer agents later without touching the safety path.
class ScoringDecisionAgent final : public IDecisionAgent {
public:
    AgentReasoningMode mode() const noexcept override {
        return AgentReasoningMode::ScoringOnly;
    }

    AgentProposal reason(const AgentObservation& obs) const noexcept override;

private:
    EffectModel model_;
};

// Factory helper (future: select by config string).
std::unique_ptr<IDecisionAgent> createDefaultDecisionAgent() noexcept;

const char* agentReasoningModeName(AgentReasoningMode mode) noexcept;

} // namespace coreflow
