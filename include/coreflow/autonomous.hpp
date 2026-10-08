#pragma once

#include <cstdint>
#include <deque>

#include "coreflow/baseline_intelligence.hpp"
#include "coreflow/config.hpp"
#include "coreflow/context.hpp"
#include "coreflow/control.hpp"
#include "coreflow/resource_model.hpp"
#include "coreflow/controller.hpp"
#include "coreflow/discovery.hpp"
#include "coreflow/experience.hpp"
#include "coreflow/mutation.hpp"
#include "coreflow/mutation_journal.hpp"
#include "coreflow/safety_hold.hpp"
#include "coreflow/observer.hpp"
#include "coreflow/policy.hpp"
#include "coreflow/types.hpp"
#include "coreflow/thermal_predictor.hpp"

namespace coreflow {

class AutonomousEngine {
public:
    AutonomousEngine();
    ~AutonomousEngine();

    AutonomousEngine(const AutonomousEngine&) = delete;
    AutonomousEngine& operator=(const AutonomousEngine&) = delete;

    int run();

private:
    bool initialize();
    bool refreshEnvironment();
    void tick();
    void logStartup() const;
    void logStateTransition(RuntimeState, RuntimeState) const;
    void logMutation(MutationResult, RuntimeState) const;

    bool has_restored_{false};
    bool refresh_pending_{false};
    bool validateSample(const RuntimeSample&) const;
    void updateTrends(RuntimeSample&) const;
    double calculateConfidence(const RuntimeSample&) const;
    NotificationEvent selectNotification(
        const RuntimeSample&, RuntimeState, RuntimeState
    ) const;
    void emitNotification(NotificationEvent, const RuntimeSample&) const;

    EngineConfig config_;
    MutationMode configured_mode_{MutationMode::Disabled};
    bool configured_armed_{false};
    HoldReason hold_reason_{HoldReason::None};
    bool last_context_eligible_{false};
    FileMutationJournal journal_;
    MutationController controller_;
    EnvironmentDiscovery discovery_;
    RuntimeObserver observer_;
    AdaptivePolicy policy_;
    ContextEngine context_engine_;
    PolicyEngine policy_engine_;
    ResourceStateModel resource_model_;
    EngineSnapshot snapshot_;
    std::uint64_t sample_count_{0};
    std::deque<RuntimeSample> history_;
    std::uint64_t last_notification_sample_{0};
    NotificationEvent last_notification_{NotificationEvent::None};
    BaselineIntelligence baseline_intelligence_;
    ExperienceMemory experience_memory_;
    ThermalPredictor thermal_predictor_;
    std::uint64_t mutation_cooldown_until_sample_{0};
};

} // namespace coreflow
