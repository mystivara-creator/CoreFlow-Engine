#pragma once

#include "coreflow/config.hpp"
#include "coreflow/safety_hold.hpp"
#include "coreflow/types.hpp"

namespace coreflow {

// Pure reason selection shared by the Android trace writer and host regression tests.
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

} // namespace coreflow
