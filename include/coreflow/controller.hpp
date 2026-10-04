#pragma once

#include "coreflow/policy.hpp"
#include "coreflow/observer.hpp"

namespace coreflow {

class MutationController {
public:
    MutationController() = default;

    // Safety gate: charging mutation remains disabled.
    // The engine may observe and request protection, but it must not write
    // charging/current/voltage controls without a validated capability
    // adapter and platform-specific safety contract.
    bool isReady() const noexcept {
        return false;
    }

    void execute(
        Decision,
        const RuntimeSample&
    ) noexcept {
        // Intentionally no-op. Charging protection is observation-only
        // until a validated platform adapter exists.
    }
};

} // namespace coreflow
