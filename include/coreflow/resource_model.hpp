#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "coreflow/environment.hpp"
#include "coreflow/types.hpp"

namespace coreflow {

enum class ResourceLifecycle : std::uint8_t {
    Unknown = 0,
    Observed,
    BaselineCaptured,
    Desired,
    Verified,
    Constrained,
    RecoveryRequired
};

struct ResourceConstraint {
    std::string name;
    std::string reason;
    bool blocking{false};
};

struct ResourceState {
    ResourceDomain domain{ResourceDomain::AndroidRuntime};
    std::string name;
    std::string path;
    std::string observed;
    std::string baseline;
    std::string desired;
    std::string verified;
    ResourceLifecycle lifecycle{ResourceLifecycle::Unknown};
    bool readable{false};
    bool writable{false};
    bool runtime_verified{false};
    bool mutation_permitted{false};
    std::vector<ResourceConstraint> constraints;

    bool safeForMutation() const noexcept {
        if (!readable || !writable || !runtime_verified || !mutation_permitted) return false;
        for (const auto& constraint : constraints) {
            if (constraint.blocking) return false;
        }
        return true;
    }
};

class ResourceStateModel final {
public:
    void reset(const EnvironmentCapabilityMatrix& capabilities) noexcept;
    bool observe(ResourceDomain domain, const std::string& name,
                 const std::string& value) noexcept;
    bool setBaseline(ResourceDomain domain, const std::string& name,
                     const std::string& value) noexcept;
    bool setDesired(ResourceDomain domain, const std::string& name,
                    const std::string& value) noexcept;
    bool setVerified(ResourceDomain domain, const std::string& name,
                     const std::string& value) noexcept;
    bool addConstraint(ResourceDomain domain, const std::string& name,
                       ResourceConstraint constraint) noexcept;
    bool setMutationPermitted(ResourceDomain domain, const std::string& name,
                              bool permitted) noexcept;

    const ResourceState* find(ResourceDomain domain, const std::string& name) const noexcept;
    ResourceState* find(ResourceDomain domain, const std::string& name) noexcept;
    const std::vector<ResourceState>& all() const noexcept { return states_; }
    std::size_t size() const noexcept { return states_.size(); }

private:
    ResourceState* findInternal(ResourceDomain domain, const std::string& name) noexcept;
    std::vector<ResourceState> states_;
};

const char* resourceLifecycleName(ResourceLifecycle lifecycle) noexcept;

} // namespace coreflow
