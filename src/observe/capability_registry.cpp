#include "coreflow/capability_registry.hpp"

namespace coreflow {
namespace {

bool sameId(const ActuatorId& lhs, const ActuatorId& rhs) noexcept {
    return lhs.domain == rhs.domain && lhs.instance == rhs.instance;
}

bool discoverySucceeded(ActuatorStatus status) noexcept {
    return status == ActuatorStatus::Ok || status == ActuatorStatus::NoChange;
}

} // namespace

bool CapabilityRegistry::registerActuator(IActuator& actuator) noexcept {
    // A registry entry must describe the same actuator through both interfaces.
    // Reject inconsistent or duplicate IDs rather than risking authorization
    // being granted to a different actuator than the one that was registered.
    const ActuatorId id = actuator.id();
    const ActuatorCapability capability = actuator.capability();
    if (!sameId(id, capability.id)) return false;

    for (const auto& existing : registered_) {
        if (existing.actuator == &actuator || sameId(existing.capability.id, id)) {
            return false;
        }
    }

    try {
        registered_.push_back(RegisteredActuator{&actuator, capability});
    } catch (...) {
        // This API is noexcept because registration may happen during startup.
        // Allocation failure must fail closed, not terminate the daemon.
        return false;
    }
    return true;
}

void CapabilityRegistry::clear() noexcept {
    registered_.clear();
}

void CapabilityRegistry::discoverAll() noexcept {
    std::vector<RegisteredActuator> discovered;
    try {
        discovered.reserve(registered_.size());
        for (const auto& entry : registered_) {
            if (entry.actuator == nullptr) continue;

            const ActuatorStatus status = entry.actuator->discover();
            if (!discoverySucceeded(status)) continue;

            const ActuatorId actualId = entry.actuator->id();
            const ActuatorCapability capability = entry.actuator->capability();
            if (!sameId(actualId, capability.id) ||
                !sameId(entry.capability.id, capability.id)) {
                continue;
            }
            discovered.push_back(RegisteredActuator{entry.actuator, capability});
        }
    } catch (...) {
        // Preserve the previous registry snapshot if refreshing it cannot be
        // completed (for example, due to allocation failure).
        return;
    }

    // Publish only a complete, validated snapshot.
    registered_.swap(discovered);
}

std::vector<RegisteredActuator> CapabilityRegistry::mutationSafe() const {
    std::vector<RegisteredActuator> out;
    out.reserve(registered_.size());
    for (const auto& entry : registered_) {
        if (entry.actuator != nullptr && entry.capability.mutationSafe()) {
            out.push_back(entry);
        }
    }
    return out;
}

} // namespace coreflow
