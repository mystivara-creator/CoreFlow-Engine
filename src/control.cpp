#include "coreflow/control.hpp"

namespace coreflow {

PolicyPlan PolicyEngine::evaluate(const SystemContext& context,
                                  const ResourceStateModel& resources) const noexcept {
    PolicyPlan plan;
    if (!context.mutation_allowed_by_context) {
        plan.action = (context.state == RuntimeState::ThermalGuard ||
                       context.state == RuntimeState::Pressure)
                          ? PolicyAction::ReduceIntervention
                          : PolicyAction::Hold;
        plan.reason = "context safety constraints block intervention";
        return plan;
    }

    plan.action = PolicyAction::Observe;
    plan.reason = "v1.9 foundation is plan-only; no resource mutation is selected automatically";

    // Build candidates only from resources that are already verified and explicitly
    // permitted by the resource model. The model never grants the final actuator permit.
    for (const auto& resource : resources.all()) {
        if (!resource.safeForMutation()) continue;
        PolicyCandidate candidate;
        candidate.domain = resource.domain;
        candidate.resource = resource.name;
        candidate.requested = resource.desired;
        candidate.confidence = context.confidence;
        candidate.risk = 1.0;
        candidate.action = PolicyAction::Candidate;
        plan.candidates.push_back(std::move(candidate));
    }

    if (!plan.candidates.empty()) {
        plan.mutation_eligible = true;
    }
    return plan;
}

bool ActuatorManager::registerActuator(IActuator& actuator) noexcept {
    const auto existing = find(actuator.id());
    if (existing != nullptr) return false;
    actuators_.push_back(&actuator);
    return true;
}

bool ActuatorManager::discoverAll() noexcept {
    bool ok = true;
    for (auto* actuator : actuators_) {
        if (actuator == nullptr || actuator->discover() != ActuatorStatus::Ok) ok = false;
    }
    return ok;
}

std::size_t ActuatorManager::mutationSafeCount() const noexcept {
    std::size_t count = 0;
    for (const auto* actuator : actuators_) {
        if (actuator != nullptr && actuator->capability().mutationSafe()) ++count;
    }
    return count;
}

const IActuator* ActuatorManager::find(ActuatorId id) const noexcept {
    for (const auto* actuator : actuators_) {
        if (actuator != nullptr && actuator->id() == id) return actuator;
    }
    return nullptr;
}

const char* policyActionName(PolicyAction action) noexcept {
    switch (action) {
        case PolicyAction::Observe: return "OBSERVE";
        case PolicyAction::Hold: return "HOLD";
        case PolicyAction::ReduceIntervention: return "REDUCE_INTERVENTION";
        case PolicyAction::Candidate: return "CANDIDATE";
    }
    return "UNKNOWN";
}

} // namespace coreflow
