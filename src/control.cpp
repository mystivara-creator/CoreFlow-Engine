#include "coreflow/control.hpp"

namespace coreflow {

PolicyPlan PolicyEngine::evaluate(
    const SystemContext& context,
    const ResourceStateModel& resources
) const noexcept {
    PolicyPlan plan;

    if (!context.mutation_allowed_by_context) {
        plan.action = (context.state == RuntimeState::ThermalGuard ||
                       context.state == RuntimeState::Pressure)
                          ? PolicyAction::ReduceIntervention
                          : PolicyAction::Hold;
        plan.intervention = InterventionLevel::ObserveOnly;
        plan.mutation_eligible = false;
        plan.reason = "context safety constraints block intervention";
        return plan;
    }

    // v2.1.0 autonomous policy — capability-driven candidates below.
    switch (context.state) {
        case RuntimeState::ThermalGuard:
        case RuntimeState::Pressure:
            plan.action = PolicyAction::ReduceIntervention;
            plan.intervention = InterventionLevel::ObserveOnly;
            plan.mutation_eligible = false;
            plan.reason = "safety state blocks new optimization; recover or observe";
            return plan;

        case RuntimeState::Warming:
        case RuntimeState::Elevated:
            plan.action = PolicyAction::Candidate;
            plan.intervention = InterventionLevel::Low;
            plan.mutation_eligible = true;
            plan.reason = "elevated/warming context allows low autonomous tune";
            break;

        case RuntimeState::Normal:
            if (context.workload == WorkloadClass::CpuBound &&
                context.confidence >= 0.85 &&
                context.thermal_headroom &&
                context.memory_headroom &&
                context.power_headroom) {
                plan.action = PolicyAction::Candidate;
                plan.intervention = InterventionLevel::Moderate;
                plan.mutation_eligible = true;
                plan.reason = "high-confidence cpu-bound with headroom";
            } else if (context.workload == WorkloadClass::Interactive ||
                       context.workload == WorkloadClass::Sustained ||
                       context.workload == WorkloadClass::IoBound ||
                       context.workload == WorkloadClass::GpuBound ||
                       context.workload == WorkloadClass::MemoryBound) {
                plan.action = PolicyAction::Candidate;
                plan.intervention = InterventionLevel::Low;
                plan.mutation_eligible = true;
                plan.reason = "active workload allows low autonomous tune";
            } else {
                plan.action = PolicyAction::Hold;
                plan.intervention = InterventionLevel::ObserveOnly;
                plan.mutation_eligible = false;
                plan.reason = "normal state without strong workload signal";
            }
            break;

        case RuntimeState::Idle:
        default:
            plan.action = PolicyAction::Hold;
            plan.intervention = InterventionLevel::ObserveOnly;
            plan.mutation_eligible = false;
            plan.reason = "idle/unknown — no spontaneous mutation";
            break;
    }

    if (!plan.mutation_eligible) {
        return plan;
    }

    // Capability-driven candidates from the resource model.
    for (const auto& state : resources.all()) {
        if (!state.mutation_permitted) continue;
        if (!state.readable || !state.writable || !state.runtime_verified) continue;
        if (state.domain != ResourceDomain::Memory &&
            state.domain != ResourceDomain::Io &&
            state.domain != ResourceDomain::Scheduler &&
            state.domain != ResourceDomain::CpuFreq) {
            continue;
        }
        PolicyCandidate c;
        c.domain = state.domain;
        c.resource = state.name;
        c.confidence = context.confidence;
        c.risk = 0.40;
        c.action = PolicyAction::Candidate;
        plan.candidates.push_back(std::move(c));
    }

    if (plan.candidates.empty() && plan.mutation_eligible) {
        plan.action = PolicyAction::Hold;
        plan.intervention = InterventionLevel::ObserveOnly;
        plan.mutation_eligible = false;
        plan.reason = plan.reason + " (no policy-approved candidates)";
    }

    return plan;
}

MutationPermit MutationAuthority::authorize(
    const PolicyPlan& plan,
    const EngineConfig& config,
    RuntimeState state,
    double confidence,
    MutationPermit::Scope scope
) const noexcept {
    if (!plan.mutation_eligible || plan.action != PolicyAction::Candidate) {
        return MutationPermit(false, scope);
    }
    // Safety states are recovery/hold-only. No new mutation permit is issued.
    if (state == RuntimeState::ThermalGuard || state == RuntimeState::Pressure) {
        return MutationPermit(false, scope);
    }
    if (config.mutationMode() != MutationMode::Adaptive || !config.mutationArmed()) {
        return MutationPermit(false, scope);
    }
    if (confidence < config.minConfidence()) {
        return MutationPermit(false, scope);
    }
    // Idle is blocked for proactive candidates. Recovery does not require a
    // mutation permit and is performed by the controller's restore path.
    if (state == RuntimeState::Idle && plan.action == PolicyAction::Candidate) {
        return MutationPermit(false, scope);
    }
    if (scope == MutationPermit::Scope::CpuFreq) {
        // CPU governor still requires explicit config allow.
        // (checked by caller via allowCpuGovernor when building plan)
    }
    return MutationPermit(true, scope);
}

bool ActuatorManager::registerActuator(IActuator& actuator) noexcept {
    if (find(actuator.id()) != nullptr) return false;
    actuators_.push_back(&actuator);
    return true;
}

bool ActuatorManager::discoverAll() noexcept {
    bool ok = true;
    for (auto* actuator : actuators_) {
        if (actuator == nullptr || actuator->discover() != ActuatorStatus::Ok) {
            ok = false;
        }
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

const char* interventionLevelName(InterventionLevel level) noexcept {
    switch (level) {
        case InterventionLevel::ObserveOnly: return "OBSERVE_ONLY";
        case InterventionLevel::Low: return "LOW";
        case InterventionLevel::Moderate: return "MODERATE";
        case InterventionLevel::High: return "HIGH";
    }
    return "UNKNOWN";
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
