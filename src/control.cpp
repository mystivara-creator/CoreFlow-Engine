#include "coreflow/control.hpp"

namespace coreflow {

PolicyPlan PolicyEngine::evaluate(
    const SystemContext& context,
    const ResourceStateModel& resources
) const noexcept {
    PolicyPlan plan;

    if (!context.mutation_allowed_by_context) {
        if (context.stabilizing_allowed_by_context) {
            PolicyPlan stabilizing;
            stabilizing.action = PolicyAction::Candidate;
            stabilizing.intervention = InterventionLevel::Low;
            stabilizing.mutation_eligible = true;
            stabilizing.stabilizing_only = true;
            stabilizing.reason =
                "stressed context: small load-reducing writes only";
            for (const auto& state : resources.all()) {
                if (!state.mutation_permitted) continue;
                if (!state.readable || !state.writable || !state.runtime_verified) continue;
                if (state.domain != ResourceDomain::Memory &&
                    state.domain != ResourceDomain::Io) {
                    continue;
                }
                PolicyCandidate c;
                c.domain = state.domain;
                c.resource = state.name;
                c.confidence = context.confidence;
                c.risk = 0.30;
                c.action = PolicyAction::Candidate;
                stabilizing.candidates.push_back(std::move(c));
            }
            if (!stabilizing.candidates.empty()) return stabilizing;
        }

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
            plan.reason = "safety state without stabilizing candidates; recover or observe";
            return plan;

        case RuntimeState::Warming:
        case RuntimeState::Elevated:
            plan.action = PolicyAction::Candidate;
            plan.intervention = InterventionLevel::Low;
            plan.mutation_eligible = true;
            plan.reason = "elevated/warming context allows low autonomous tune";
            break;

        case RuntimeState::Normal:
            // Balance: CpuBound no longer demands 0.85 confidence — that gate
            // made Moderate almost unreachable on real telemetry. 0.72 still
            // requires clear signal; Low remains available for active workloads.
            if (context.workload == WorkloadClass::CpuBound &&
                context.confidence >= 0.72 &&
                context.thermal_headroom &&
                context.memory_headroom &&
                context.power_headroom) {
                plan.action = PolicyAction::Candidate;
                plan.intervention = InterventionLevel::Moderate;
                plan.mutation_eligible = true;
                plan.reason = "cpu-bound with headroom";
            } else if (context.workload == WorkloadClass::Interactive ||
                       context.workload == WorkloadClass::Sustained ||
                       context.workload == WorkloadClass::IoBound ||
                       context.workload == WorkloadClass::GpuBound ||
                       context.workload == WorkloadClass::MemoryBound) {
                plan.action = PolicyAction::Candidate;
                plan.intervention = InterventionLevel::Low;
                plan.mutation_eligible = true;
                plan.reason = "active workload allows low autonomous tune";
            } else if (context.confidence >= 0.65 &&
                       context.thermal_headroom &&
                       context.memory_headroom &&
                       context.power_headroom) {
                // Mild Normal activity: still allow cautious Low so Balance/Adapt
                // is not dead when workload classification is weak.
                plan.action = PolicyAction::Candidate;
                plan.intervention = InterventionLevel::Low;
                plan.mutation_eligible = true;
                plan.reason = "normal activity with headroom — low autonomous tune";
            } else {
                plan.action = PolicyAction::Hold;
                plan.intervention = InterventionLevel::ObserveOnly;
                plan.mutation_eligible = false;
                plan.reason = "normal state without actionable workload signal";
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
    // A stabilizing plan authorizes small load-reducing resource writes only.
    // It can never authorize the CPU governor.
    if (plan.stabilizing_only && scope != MutationPermit::Scope::Resource) {
        return MutationPermit(false, scope);
    }
    // Thermal/memory safety states may start only a stabilizing resource write.
    // Any other plan is hold/recovery-only and receives no permit.
    if ((state == RuntimeState::ThermalGuard || state == RuntimeState::Pressure) &&
        !plan.stabilizing_only) {
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
    // The CPU governor is a second, separate opt-in. Enforce it here so the
    // permit itself cannot be issued without it.
    if (scope == MutationPermit::Scope::CpuFreq && !config.allowCpuGovernor()) {
        return MutationPermit(false, scope);
    }
    return MutationPermit(true, scope);
}

bool shouldRestoreEpochNow(const EpochRestoreInputs& in) noexcept {
    if (in.state == RuntimeState::Idle) return true;
    if (!in.mode_adaptive || !in.armed) return true;
    const bool stress =
        in.state == RuntimeState::Pressure || in.state == RuntimeState::ThermalGuard;
    if (!stress) return false;
    const bool keep = !in.cpu_mutated && in.resource_epoch_stabilizing &&
                      in.plan_stabilizing_only && in.plan_mutation_eligible;
    return !keep;
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
