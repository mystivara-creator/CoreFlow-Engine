#pragma once

#include <vector>
#include <cstdint>
#include "coreflow/actuator.hpp"

namespace coreflow {

struct RegisteredActuator {
    IActuator* actuator{nullptr};
    ActuatorCapability capability{};
};

class CapabilityRegistry final {
public:
    bool registerActuator(IActuator& actuator) noexcept;
    void clear() noexcept;

    // call discover() for each actuator and keep only successfully discovered ones
    void discoverAll() noexcept;

    const std::vector<RegisteredActuator>& all() const noexcept { return registered_; }

    std::vector<RegisteredActuator> mutationSafe() const;

private:
    std::vector<RegisteredActuator> registered_{};
};

} // namespace coreflow