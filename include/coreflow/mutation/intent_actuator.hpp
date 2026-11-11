#pragma once

#include "coreflow/baseline_store.hpp"
#include "coreflow/autonomous_tuner.hpp"

#include <cstdint>
#include <string_view>

namespace coreflow::mutation {

// Narrow lifecycle primitive shared by transactional actuators. It does not
// authorize mutations; callers must already hold coordinator ownership.
enum class RetentionResult : std::uint8_t { Retained, AlreadyRetained, PersistenceFailed, OwnershipReleaseFailed };

class RetainedLifecycle final {
public:
    [[nodiscard]] static RetentionResult commit(
        BaselineStore& baselines,
        CentralMutationCoordinator& coordinator,
        MutationDomain domain,
        std::string_view controller_id,
        std::string_view transaction_id,
        std::string_view endpoint,
        std::string_view verified_value) noexcept;
};

} // namespace coreflow::mutation
