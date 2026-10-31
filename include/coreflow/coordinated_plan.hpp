#pragma once
#include "coreflow/mutation.hpp"
#include "coreflow/resource_mutation.hpp"

namespace coreflow {
struct CoordinatedResult {
    MutationResult cpu{MutationResult::Skipped};
    MutationResult resource{MutationResult::Skipped};
    std::string identity;
    std::size_t members{0};
    bool verified() const noexcept {
        return cpu == MutationResult::Verified || resource == MutationResult::Verified;
    }
};
// Resource evidence gets first choice. A governor fallback may add only
// justified companions, then all members share one outcome window.
bool rollbackCoordinatedPlan(MutationController& cpu, ResourceMutationController& resources) noexcept;
bool restoreCoordinatedPlan(MutationController& cpu, ResourceMutationController& resources) noexcept;
CoordinatedResult executeCoordinatedPlan(MutationController& cpu,
    ResourceMutationController& resources, RuntimeState state, const RuntimeSample& sample,
    DeviceProfile& profile, const EngineConfig& config, const PolicyPlan& plan,
    const DecisionTraceContext* trace = nullptr) noexcept;
} // namespace coreflow
