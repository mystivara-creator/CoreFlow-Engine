#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "coreflow/actuator.hpp"
#include "coreflow/config.hpp"
#include "coreflow/context.hpp"
#include "coreflow/resource_model.hpp"
#include "coreflow/types.hpp"

namespace coreflow {

enum class PolicyAction : std::uint8_t {
    Observe = 0,
    Hold,
    ReduceIntervention,
    Candidate
};

struct PolicyCandidate {
    ResourceDomain domain{ResourceDomain::AndroidRuntime};
    std::string resource;
    std::string requested;
    double confidence{0.0};
    double risk{1.0};
    PolicyAction action{PolicyAction::Observe};
};

struct PolicyPlan {
    PolicyAction action{PolicyAction::Observe};
    InterventionLevel intervention{
        InterventionLevel::ObserveOnly
    };
    bool mutation_eligible{false};
    // True when the plan permits only stabilizing (load-reducing) resource
    // writes. Such a plan never authorizes the CPU governor.
    bool stabilizing_only{false};
    std::string reason;
    std::vector<PolicyCandidate> candidates;

    bool allows(ResourceDomain domain, const std::string& resource) const noexcept {
        if (!mutation_eligible || action != PolicyAction::Candidate) return false;
        for (const auto& candidate : candidates) {
            if (candidate.domain == domain && candidate.resource == resource &&
                candidate.action == PolicyAction::Candidate) {
                return true;
            }
        }
        return false;
    }
};

// Inputs for the per-cycle decision to end the active mutation epoch immediately.
struct EpochRestoreInputs {
    RuntimeState state{RuntimeState::Idle};
    bool mode_adaptive{false};
    bool armed{false};
    bool cpu_mutated{false};
    // Every change the resource controller holds was made under a stabilizing plan.
    bool resource_epoch_stabilizing{false};
    bool plan_stabilizing_only{false};
    bool plan_mutation_eligible{false};
};

// True when the active epoch must be restored this cycle rather than held for
// its observation/outcome window. Idle, a disarmed config (this includes the
// kill switch and SAFE_MODE, which disable the config) and any CPU governor change
// under stress always restore. Under thermal/memory stress a held change is kept
// only if it is a stabilizing resource write and the current plan still permits one.
bool shouldRestoreEpochNow(const EpochRestoreInputs& in) noexcept;

class MutationAuthority final {
public:
    MutationPermit authorize(const PolicyPlan& plan,
                             const EngineConfig& config,
                             RuntimeState state,
                             double confidence,
                             MutationPermit::Scope scope) const noexcept;
};

class PolicyEngine final {
public:
    PolicyPlan evaluate(const SystemContext& context,
                        const ResourceStateModel& resources) const noexcept;
};

class ActuatorManager final {
public:
    bool registerActuator(IActuator& actuator) noexcept;
    bool discoverAll() noexcept;
    std::size_t size() const noexcept { return actuators_.size(); }
    std::size_t mutationSafeCount() const noexcept;
    const IActuator* find(ActuatorId id) const noexcept;

private:
    std::vector<IActuator*> actuators_;
};

const char* policyActionName(PolicyAction action) noexcept;

} // namespace coreflow
