#include "coreflow/mutation/intent_actuator.hpp"

namespace coreflow::mutation {

RetentionResult RetainedLifecycle::commit(
    BaselineStore& baselines,
    CentralMutationCoordinator& coordinator,
    MutationDomain domain,
    std::string_view controller_id,
    std::string_view transaction_id,
    std::string_view endpoint,
    std::string_view verified_value) noexcept {
    if (transaction_id.empty() || controller_id.empty() || endpoint.empty() ||
        verified_value.empty()) return RetentionResult::PersistenceFailed;

    if (!baselines.retain_performance_baseline(
            std::string(transaction_id), std::string(endpoint), std::string(verified_value))) {
        return RetentionResult::PersistenceFailed;
    }

    coordinator.release_lease(domain, controller_id, transaction_id);
    if (coordinator.lease_is_live(domain, controller_id, transaction_id))
        return RetentionResult::OwnershipReleaseFailed;
    return RetentionResult::Retained;
}

} // namespace coreflow::mutation
