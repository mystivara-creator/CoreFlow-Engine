#pragma once

#include <cstdint>

#include "coreflow/config.hpp"
#include "coreflow/control.hpp"
#include "coreflow/safety_hold.hpp"
#include "coreflow/context.hpp"
#include "coreflow/policy.hpp"
#include "coreflow/types.hpp"

namespace coreflow {

// Pure reason selection shared by the trace writer and host regression tests.
// Safety and mode gates remain higher priority than mutation outcomes.
inline const char* decisionTraceReason(
    HoldReason hold,
    MutationMode mode,
    bool armed,
    bool mutationEligible,
    MutationResult cpuMutation,
    MutationResult resourceMutation
) noexcept {
    if (hold != HoldReason::None) return "SAFETY_HOLD";
    if (mode != MutationMode::Adaptive || !armed) return "OBSERVE_OR_NOT_ARMED";
    if (cpuMutation == MutationResult::Failed ||
        resourceMutation == MutationResult::Failed) return "MUTATION_FAILED";
    if (!mutationEligible) return "POLICY_BLOCKED";
    if (cpuMutation == MutationResult::Verified ||
        resourceMutation == MutationResult::Verified) return "MUTATION_VERIFIED";
    if (cpuMutation == MutationResult::RolledBack ||
        resourceMutation == MutationResult::RolledBack) return "MUTATION_ROLLED_BACK";
    return "NO_VERIFIED_MUTATION";
}

// Immutable cycle metadata shared by real events emitted at their execution
// boundaries. Context/plan pointers are borrowed only for the synchronous call.
struct DecisionTraceContext {
    std::uint64_t cycle{0};
    RuntimeState previous_state{RuntimeState::Idle};
    RuntimeState state{RuntimeState::Idle};
    Decision decision{Decision::NoAction};
    double confidence{0.0};
    const SystemContext* context{nullptr};
    const PolicyPlan* plan{nullptr};
    HoldReason hold{HoldReason::None};
    MutationMode mode{MutationMode::Disabled};
    bool armed{false};
    bool cpu_governor_allowed{false};
    MutationResult cpu_mutation{MutationResult::Skipped};
    MutationResult resource_mutation{MutationResult::Skipped};
    // A result is unknown until that controller has returned in this cycle.
    bool cpu_mutation_known{false};
    bool resource_mutation_known{false};
};

inline const char* mutationTraceOutcomeReason(MutationResult result) noexcept {
    switch (result) {
        case MutationResult::Skipped: return "controller_skipped_no_verified_write";
        case MutationResult::Applied: return "controller_applied_verification_not_confirmed";
        case MutationResult::Verified: return "controller_reported_write_and_readback_verified";
        case MutationResult::Failed: return "controller_reported_failure_check_diagnostics";
        case MutationResult::RolledBack: return "controller_reported_rollback";
    }
    return "unknown_controller_result";
}

struct DecisionTraceEvent {
    const char* event{"event"};
    const char* scope{"engine"};
    const char* result{"INFO"};
    const char* reason{nullptr};
    bool permit_known{false};
    bool permit_valid{false};
    const char* failure_stage{nullptr};
    const char* actuator_status{nullptr};
    const char* target{nullptr};
    const char* requested{nullptr};
    const char* current_value{nullptr};
    const char* baseline_value{nullptr};
    bool write_counts_known{false};
    unsigned writes_attempted{0U};
    unsigned writes_verified{0U};
};

void appendDecisionTraceEvent(const DecisionTraceContext& context,
                              const DecisionTraceEvent& event) noexcept;

void appendDecisionTrace(
    std::uint64_t cycle,
    RuntimeState previous,
    RuntimeState current,
    Decision decision,
    double confidence,
    const SystemContext& context,
    const PolicyPlan& plan,
    HoldReason hold,
    MutationMode mode,
    bool armed,
    bool cpuGovernorAllowed,
    MutationResult cpuMutation,
    MutationResult resourceMutation
) noexcept;

} // namespace coreflow
