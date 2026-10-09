#pragma once

#include <cstdint>

#include "coreflow/config.hpp"
#include "coreflow/control.hpp"
#include "coreflow/context.hpp"
#include "coreflow/policy.hpp"
#include "coreflow/safety_hold.hpp"
#include "coreflow/types.hpp"

namespace coreflow {

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
