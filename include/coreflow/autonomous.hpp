#pragma once

#include <atomic>
#include <cstdint>

#include "coreflow/discovery.hpp"
#include "coreflow/observer.hpp"
#include "coreflow/policy.hpp"
#include "coreflow/config.hpp"
#include "coreflow/controller.hpp"

namespace coreflow {

class AutonomousEngine {
public:
    AutonomousEngine();
    int run();
    void requestStop() noexcept;

private:
    bool initialize();
    void tick();
    void logStartup() const;
    void logStateTransition(RuntimeState, RuntimeState) const;

    EngineConfig config_;
    MutationController controller_;
    EnvironmentDiscovery discovery_;
    RuntimeObserver observer_;
    AdaptivePolicy policy_;
    EngineSnapshot snapshot_;
    std::atomic<bool> stop_requested_{false};
    std::uint64_t sample_count_{0};
};

} // namespace coreflow
