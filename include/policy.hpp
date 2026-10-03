#pragma once
#include "coreflow/types.hpp"

namespace coreflow {

class AdaptivePolicy {
public:
    RuntimeState evaluate(const RuntimeSample&, RuntimeState previous) const;
    Decision decide(const RuntimeSample&, RuntimeState state) const;
};

const char* stateName(RuntimeState);
const char* decisionName(Decision);

} // namespace coreflow
