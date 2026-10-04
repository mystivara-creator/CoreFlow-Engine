#pragma once

#include <atomic>
#include <cstdint>
#include <deque>

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

    void updateTrends(RuntimeSample&) const;
    double calculateConfidence(const RuntimeSample&) const;
    NotificationEvent selectNotification(
        const RuntimeSample&,
        RuntimeState,
        RuntimeState
    ) const;
    void emitNotification(NotificationEvent, const RuntimeSample&) const;

    EngineConfig config_;
    MutationController controller_;
    EnvironmentDiscovery discovery_;
    RuntimeObserver observer_;
    AdaptivePolicy policy_;
    EngineSnapshot snapshot_;
    std::atomic<bool> stop_requested_{false};
    std::atomic<bool>
    std::uint64_t sample_count_{0};

    std::deque<RuntimeSample> history_;
    std::uint64_t last_notification_sample_{0};
    NotificationEvent last_notification_{NotificationEvent::None};
};

} // namespace coreflow
