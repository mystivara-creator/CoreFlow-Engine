#include "coreflow/control.hpp"

#include <cmath>

namespace coreflow {

PolicyPlan PolicyEngine::evaluate(
    const SystemContext& context,
    const ResourceStateModel& resources
) const noexcept {
    PolicyPlan plan;

    // Policy is advisory for ordinary operating conditions. It may report
    // missing headroom, workload uncertainty, or a recommendation to use a low
    // intervention level, but those warnings must not globally veto Adaptive.
    // Idle has no autonomous demand. ThermalGuard remains the explicit runtime
    // safety context; it may only expose stabilizing candidates when telemetry
    // is trustworthy enough to support them.
    if (context.state == RuntimeState::Idle) {
        plan.action = PolicyAction::Hold;
        plan.intervention = InterventionLevel::ObserveOnly;
        plan.mutation_eligible = false;
        plan.reason = "idle/unknown — no spontaneous mutation";
        return plan;
    }

    const bool stress_state =
        context.state == RuntimeState::ThermalGuard ||
        context.state == RuntimeState::Pressure;
    if (stress_state && !context.stabilizing_allowed_by_context) {
        plan.action = context.state == RuntimeState::ThermalGuard
                          ? PolicyAction::ReduceIntervention
                          : PolicyAction::Hold;
        plan.intervention = InterventionLevel::ObserveOnly;
        plan.mutation_eligible = false;
        plan.reason = context.state == RuntimeState::ThermalGuard
                          ? "thermal safety context lacks reliable stabilization evidence"
                          : "pressure context lacks reliable stabilization evidence";
        return plan;
    }

    plan.action = PolicyAction::Candidate;
    plan.intervention = InterventionLevel::Low;
    plan.mutation_eligible = true;
    plan.stabilizing_only = stress_state;
    if (context.state == RuntimeState::ThermalGuard) {
        plan.reason = "thermal guard: stabilizing candidates only";
    } else if (context.state == RuntimeState::Pressure) {
        plan.reason = "pressure warning: stabilizing candidates only";
    } else if (!context.mutation_allowed_by_context) {
        plan.reason = "advisory: context headroom/confidence warning; evaluate bounded candidates";
    } else if (context.state == RuntimeState::Warming ||
               context.state == RuntimeState::Elevated) {
        plan.reason = "warming/elevated context allows low autonomous tune";
    } else if (context.state == RuntimeState::Normal &&
               context.workload == WorkloadClass::CpuBound &&
               context.confidence >= 0.72 &&
               context.thermal_headroom &&
               context.memory_headroom &&
               context.power_headroom) {
        plan.intervention = InterventionLevel::Moderate;
        plan.reason = "cpu-bound with headroom";
    } else {
        plan.reason = "adaptive context: low autonomous tune";
    }

    // Build a structural candidate inventory, not an authorization decision.
    // The mutation controller independently checks grants, exact target safety,
    // baseline, effect evidence, bounded writes, journal, read-back and rollback.
    // A candidate missing mutation_permitted in this model must not cause every
    // otherwise viable resource to disappear before that per-target pipeline.
    for (const auto& state : resources.all()) {
        if (!state.readable || !state.writable || !state.runtime_verified) continue;
        if (!isTunableResourceDomain(state.domain)) continue;
        PolicyCandidate candidate;
        candidate.domain = state.domain;
        candidate.resource = state.name;
        candidate.confidence = context.confidence;
        candidate.risk = stress_state ? 0.30 : 0.40;
        candidate.action = PolicyAction::Candidate;
        plan.candidates.push_back(std::move(candidate));
    }

    if (plan.candidates.empty()) {
        plan.action = PolicyAction::Hold;
        plan.intervention = InterventionLevel::ObserveOnly;
        plan.mutation_eligible = false;
        plan.reason += " (no structurally usable tunables discovered)";
    }

    return plan;
}

const char* mutationPermitBlockReason(const PolicyPlan& plan,
                                      const EngineConfig& config,
                                      RuntimeState state,
                                      double confidence,
                                      MutationPermit::Scope scope) noexcept {
    if (!plan.mutation_eligible || plan.action != PolicyAction::Candidate) {
        return "policy_not_candidate";
    }
    // A stabilizing plan authorizes small load-reducing resource writes only.
    // It can never authorize the CPU governor.
    if (plan.stabilizing_only && scope != MutationPermit::Scope::Resource) {
        return "stabilizing_plan_disallows_cpu";
    }
    // Thermal/memory safety states may start only a stabilizing resource write.
    // Any other plan is hold/recovery-only and receives no permit.
    if ((state == RuntimeState::ThermalGuard || state == RuntimeState::Pressure) &&
        !plan.stabilizing_only) {
        return "stress_state_requires_stabilizing_plan";
    }
    if (config.mutationMode() != MutationMode::Adaptive || !config.mutationArmed()) {
        return "adaptive_mode_or_arming_required";
    }
    if (!std::isfinite(confidence)) {
        return "confidence_not_finite";
    }
    if (confidence < config.minConfidence()) {
        return "confidence_below_minimum";
    }
    // Idle is blocked for proactive candidates. Recovery does not require a
    // mutation permit and is performed by the controller's restore path.
    if (state == RuntimeState::Idle && plan.action == PolicyAction::Candidate) {
        return "idle_state_blocks_candidate";
    }
    return nullptr;
}

MutationPermit MutationAuthority::authorize(
    const PolicyPlan& plan,
    const EngineConfig& config,
    RuntimeState state,
    double confidence,
    MutationPermit::Scope scope
) const noexcept {
    if (mutationPermitBlockReason(plan, config, state, confidence, scope) != nullptr) {
        return MutationPermit(false, scope);
    }
    return MutationPermit(true, scope);
}

MutationPermit MutationAuthority::authorize(
    const PolicyPlan& plan,
    const EngineConfig& config,
    RuntimeState state,
    double confidence,
    const ActuatorCapability& capability,
    std::string_view target
) const noexcept {
    if (!capability.mutationSafe() || target.empty())
        return MutationPermit(false, MutationPermit::Scope::Resource);

    const MutationPermit::Scope scope =
        capability.id.domain == ActuatorDomain::CpuFreq
            ? MutationPermit::Scope::CpuFreq
            : MutationPermit::Scope::Resource;
    const MutationPermit coarse = authorize(plan, config, state, confidence, scope);
    if (!coarse.valid())
        return MutationPermit(false, MutationPermit::Scope::Resource);
    try {
        return MutationPermit(true, capability.id, std::string(target),
                              MutationPermit::Operation::Write);
    } catch (...) {
        return MutationPermit(false, MutationPermit::Scope::Resource);
    }
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
    if (find(actuator.id()) != nullptr) {
        return false;
    }

    try {
        actuators_.push_back(&actuator);
    } catch (...) {
        // Registration failure must not terminate the daemon.
        return false;
    }

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
