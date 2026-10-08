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

    plan.action = PolicyAction::Candidate;
    plan.reason = "context and resource preflight allow bounded autonomous mutation";

    // The plan is the single policy authority. It may authorize the CPU governor
    // path even when no generic resource candidate exists, while generic resources
    // must additionally match one of these explicitly enumerated candidates.
    plan.mutation_eligible = true;
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
    return plan;
}

MutationPermit MutationAuthority::authorize(const PolicyPlan& plan,
                                            const EngineConfig& config,
                                            RuntimeState state,
                                            double confidence,
                                            MutationPermit::Scope scope) const noexcept {
    if (!plan.mutation_eligible || plan.action != PolicyAction::Candidate) {
        return MutationPermit(false, scope);
    }
    if (config.mutationMode() != MutationMode::Adaptive || !config.mutationArmed()) {
        return MutationPermit(false, scope);
    }
    if (confidence < config.minConfidence()) return MutationPermit(false, scope);
    if (state == RuntimeState::Idle || state == RuntimeState::Pressure ||
        state == RuntimeState::ThermalGuard) {
        return MutationPermit(false, scope);
    }
    return MutationPermit(true, scope);
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
