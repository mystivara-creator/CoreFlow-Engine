#pragma once

#include <cstdint>
#include <deque>

#include "coreflow/config.hpp"
#include "coreflow/controller.hpp"
#include "coreflow/discovery.hpp"
#include "coreflow/mutation.hpp"
#include "coreflow/observer.hpp"
#include "coreflow/policy.hpp"
#include "coreflow/types.hpp"

namespace coreflow {

class AutonomousEngine {
public:
    AutonomousEngine();
    ~AutonomousEngine();

    int run();

private:
    bool initialize();
    bool refreshEnvironment();
    void tick();
    void logStartup() const;
    void logStateTransition(RuntimeState, RuntimeState) const;
    void logMutation(MutationResult, RuntimeState) const;

    void updateTrends(RuntimeSample&) const;
    double calculateConfidence(const RuntimeSample&) const;
    NotificationEvent selectNotification(
        const RuntimeSample&, RuntimeState, RuntimeState
    ) const;
    void emitNotification(NotificationEvent, const RuntimeSample&) const;

    EngineConfig config_;
    MutationController controller_;
    EnvironmentDiscovery discovery_;
    RuntimeObserver observer_;
    AdaptivePolicy policy_;
    EngineSnapshot snapshot_;
    std::uint64_t sample_count_{0};
    std::deque<RuntimeSample> history_;
    std::uint64_t last_notification_sample_{0};
    NotificationEvent last_notification_{NotificationEvent::None};
};

} // namespace coreflow
