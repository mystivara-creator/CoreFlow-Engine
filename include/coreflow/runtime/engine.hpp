#pragma once

#include "coreflow/decision/decision.hpp"
#include "coreflow/discovery/discovery.hpp"
#include "coreflow/mutation/mutation.hpp"
#include "coreflow/observation/observation.hpp"
#include "coreflow/optimization/optimization.hpp"
#include "coreflow/outcome/outcome.hpp"
#include "coreflow/recovery/learning.hpp"
#include "coreflow/runtime/state_store.hpp"
#include "coreflow/validation/validation.hpp"

#include <chrono>
#include <string>
#include <vector>

namespace coreflow {

// Composition root. Each cycle walks the same pipeline stages that the WebUI
// displays via status.json, so the console reflects the C++ code path in use.
class AutonomousEngine {
public:
    AutonomousEngine();
    void run();

private:
    using Clock = std::chrono::steady_clock;

    void applyCommand(Command command);
    void setStage(const char* stage) { status_.stage = stage; }
    void publish();
    void recordDecision(const ChangeProposal& candidate, const std::string& disposition,
                        const std::string& detail);
    void recordMutation(const ChangeProposal& candidate, const std::string& disposition,
                        const std::string& detail);

    CapabilityDiscovery discovery_;
    RuntimeObserver observer_;
    OptimizationEngine optimizer_;
    ChangeJustifier justifier_;
    CapabilitySafetyValidator validator_;
    ResourceMutation mutation_;
    OutcomeVerifier verifier_;
    OutcomePolicy policy_;
    LearningJournal journal_;
    StateStore state_;
    RuntimeStatus status_;
    Clock::time_point started_{Clock::now()};
    bool paused_{false};
    bool force_discovery_{false};
};

} // namespace coreflow
