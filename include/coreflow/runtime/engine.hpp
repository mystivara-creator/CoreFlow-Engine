#pragma once
#include "coreflow/decision/decision.hpp"
#include "coreflow/discovery/discovery.hpp"
#include "coreflow/mutation/mutation.hpp"
#include "coreflow/observation/observation.hpp"
#include "coreflow/optimization/optimization.hpp"
#include "coreflow/outcome/outcome.hpp"
#include "coreflow/recovery/learning.hpp"
#include "coreflow/validation/validation.hpp"
namespace coreflow {
class AutonomousEngine {
public:
    void run();
private:
    CapabilityDiscovery discovery_;
    RuntimeObserver observer_;
    OptimizationEngine optimizer_;
    ChangeJustifier justifier_;
    CapabilitySafetyValidator validator_;
    ResourceMutation mutation_;
    OutcomeVerifier verifier_;
    OutcomePolicy policy_;
    LearningJournal journal_;
};
}
