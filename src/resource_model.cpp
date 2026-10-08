#include "coreflow/resource_model.hpp"

namespace coreflow {

void ResourceStateModel::reset(const EnvironmentCapabilityMatrix& capabilities) noexcept {
    states_.clear();
    states_.reserve(capabilities.resources.size());
    for (const auto& capability : capabilities.resources) {
        ResourceState state;
        state.domain = capability.domain;
        state.name = capability.name;
        state.path = capability.path;
        state.readable = capability.readable;
        state.writable = capability.writable;
        state.runtime_verified = capability.runtime_verified;
        state.mutation_permitted = false;
        state.lifecycle = capability.exists || capability.readable || capability.writable
                              ? ResourceLifecycle::Observed : ResourceLifecycle::Unknown;
        states_.push_back(std::move(state));
    }
}

ResourceState* ResourceStateModel::findInternal(ResourceDomain domain,
                                                 const std::string& name) noexcept {
    for (auto& state : states_) {
        if (state.domain == domain && state.name == name) return &state;
    }
    return nullptr;
}

ResourceState* ResourceStateModel::find(ResourceDomain domain,
                                         const std::string& name) noexcept {
    return findInternal(domain, name);
}

const ResourceState* ResourceStateModel::find(ResourceDomain domain,
                                               const std::string& name) const noexcept {
    for (const auto& state : states_) {
        if (state.domain == domain && state.name == name) return &state;
    }
    return nullptr;
}

bool ResourceStateModel::observe(ResourceDomain domain, const std::string& name,
                                 const std::string& value) noexcept {
    auto* state = findInternal(domain, name);
    if (!state) return false;
    state->observed = value;
    state->lifecycle = ResourceLifecycle::Observed;
    return true;
}

bool ResourceStateModel::setBaseline(ResourceDomain domain, const std::string& name,
                                     const std::string& value) noexcept {
    auto* state = findInternal(domain, name);
    if (!state) return false;
    state->baseline = value;
    state->lifecycle = ResourceLifecycle::BaselineCaptured;
    return true;
}

bool ResourceStateModel::setDesired(ResourceDomain domain, const std::string& name,
                                    const std::string& value) noexcept {
    auto* state = findInternal(domain, name);
    if (!state) return false;
    state->desired = value;
    state->lifecycle = ResourceLifecycle::Desired;
    return true;
}

bool ResourceStateModel::setVerified(ResourceDomain domain, const std::string& name,
                                     const std::string& value) noexcept {
    auto* state = findInternal(domain, name);
    if (!state) return false;
    state->verified = value;
    state->runtime_verified = true;
    state->lifecycle = ResourceLifecycle::Verified;
    return true;
}

bool ResourceStateModel::addConstraint(ResourceDomain domain, const std::string& name,
                                       ResourceConstraint constraint) noexcept {
    auto* state = findInternal(domain, name);
    if (!state) return false;
    if (constraint.blocking) state->lifecycle = ResourceLifecycle::Constrained;
    state->constraints.push_back(std::move(constraint));
    return true;
}

bool ResourceStateModel::setMutationPermitted(ResourceDomain domain,
                                               const std::string& name,
                                               bool permitted) noexcept {
    auto* state = findInternal(domain, name);
    if (!state) return false;
    // This layer can explicitly revoke permission, but it cannot manufacture a
    // mutation permit. The final permit remains owned by MutationController.
    if (!permitted) {
        state->mutation_permitted = false;
        return true;
    }
    if (!state->readable || !state->writable || !state->runtime_verified) return false;
    for (const auto& constraint : state->constraints) {
        if (constraint.blocking) return false;
    }
    state->mutation_permitted = true;
    return true;
}

const char* resourceLifecycleName(ResourceLifecycle lifecycle) noexcept {
    switch (lifecycle) {
        case ResourceLifecycle::Unknown: return "UNKNOWN";
        case ResourceLifecycle::Observed: return "OBSERVED";
        case ResourceLifecycle::BaselineCaptured: return "BASELINE_CAPTURED";
        case ResourceLifecycle::Desired: return "DESIRED";
        case ResourceLifecycle::Verified: return "VERIFIED";
        case ResourceLifecycle::Constrained: return "CONSTRAINED";
        case ResourceLifecycle::RecoveryRequired: return "RECOVERY_REQUIRED";
    }
    return "UNKNOWN";
}

} // namespace coreflow
