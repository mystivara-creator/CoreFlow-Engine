#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "coreflow/actuator.hpp"
#include "coreflow/context.hpp"
#include "coreflow/resource_model.hpp"

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
