#pragma once
#include "coreflow/mutation.hpp"
#include "coreflow/resource_mutation.hpp"

namespace coreflow {
struct CoordinatedResult {
    MutationResult cpu{MutationResult::Skipped};
    MutationResult resource{MutationResult::Skipped};
    std::string identity;
    std::size_t members{0};
    std::vector<TrialMember> trials;
    std::string decision_reason, winner, cpu_reason, resource_reason;
    double cpu_utility{0.0}, resource_utility{0.0};
    CandidateFunnel funnel;
    bool verified() const noexcept {
        return cpu == MutationResult::Verified || resource == MutationResult::Verified;
    }
};
// Read-only proposals compete on a common net-utility scale against keeping
// the current configuration. Dependency order affects execution, not selection.
bool rollbackCoordinatedPlan(MutationController& cpu, ResourceMutationController& resources) noexcept;
bool restoreCoordinatedPlan(MutationController& cpu, ResourceMutationController& resources) noexcept;
CoordinatedResult executeCoordinatedPlan(MutationController& cpu,
    ResourceMutationController& resources, RuntimeState state, const RuntimeSample& sample,
    DeviceProfile& profile, const EngineConfig& config, const PolicyPlan& plan,
    const DecisionTraceContext* trace = nullptr) noexcept;
} // namespace coreflow
