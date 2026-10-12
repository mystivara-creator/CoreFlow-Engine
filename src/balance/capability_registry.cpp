#include "coreflow/capability_registry.hpp"

namespace coreflow {

bool CapabilityRegistry::registerActuator(IActuator& actuator) noexcept {
    RegisteredActuator ra;
    ra.actuator = &actuator;
    ra.capability = actuator.capability();
    if (ra.actuator == nullptr) return false;
    registered_.push_back(ra);
    return true;
}

void CapabilityRegistry::clear() noexcept {
    registered_.clear();
}

void CapabilityRegistry::discoverAll() noexcept {
    std::vector<RegisteredActuator> ok;
    ok.reserve(registered_.size());

    for (auto& r : registered_) {
        if (r.actuator == nullptr) continue;
        const auto st = r.actuator->discover();
        if (st == ActuatorStatus::Ok || st == ActuatorStatus::NoChange) {
            r.capability = r.actuator->capability();
            ok.push_back(r);
        }
    }

    registered_.swap(ok);
}

std::vector<RegisteredActuator> CapabilityRegistry::mutationSafe() const {
    std::vector<RegisteredActuator> out;
    for (const auto& r : registered_) {
        if (r.capability.mutationSafe()) out.push_back(r);
    }
    return out;
}

} // namespace coreflow