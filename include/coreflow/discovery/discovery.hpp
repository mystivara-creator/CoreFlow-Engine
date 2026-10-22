#pragma once
#include "coreflow/model/types.hpp"
#include <vector>
namespace coreflow {
class CapabilityDiscovery {
public:
    std::vector<Capability> discover() const;
};
}
