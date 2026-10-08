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
    bool mutation_eligible{false};
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
