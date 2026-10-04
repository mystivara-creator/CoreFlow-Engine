#pragma once

#include "coreflow/policy.hpp"
#include "coreflow/observer.hpp"

namespace coreflow {

class MutationController {
public:
    MutationController() = default;

    // Observation Intelligence v1 is read-only.
    // Mutation remains disabled until a concrete capability
    // has been discovered and validated for the device.
    bool isReady() const noexcept {
        return false;
    }

    void execute(
        Decision,
        const RuntimeSample&
    ) noexcept {
        // Intentionally disabled in Observation Intelligence v1.
    }
};

} // namespace coreflow
