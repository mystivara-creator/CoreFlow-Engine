#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <string>

#include "coreflow/baseline_intelligence.hpp"
#include "coreflow/config.hpp"
#include "coreflow/context.hpp"
#include "coreflow/control.hpp"
#include "coreflow/resource_model.hpp"
#include "coreflow/resource_mutation.hpp"
#include "coreflow/discovery.hpp"
#include "coreflow/experience.hpp"
#include "coreflow/mutation.hpp"
#include "coreflow/mutation_journal.hpp"
#include "coreflow/retained_state.hpp"
#include "coreflow/safety_hold.hpp"
#include "coreflow/status_snapshot.hpp"
#include "coreflow/observer.hpp"
#include "coreflow/policy.hpp"
#include "coreflow/types.hpp"
#include "coreflow/thermal_predictor.hpp"
#include "coreflow/thermal_guard.hpp"
#include "coreflow/frame_telemetry.hpp"
#include "coreflow/foreground_tracker.hpp"
#include "coreflow/gpu_busy_source.hpp"
#include "coreflow/interactivity_boost.hpp"
#include "coreflow/interactivity_model.hpp"
#include "coreflow/interactivity_policy.hpp"
#include "coreflow/launch_detector.hpp"

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
    // Best-effort export of what the engine sees, decides and changed, for the WebUI.
    // It carries no authority and a failure here never affects control.
    void publishStatus(const RuntimeSample&, RuntimeState previous, RuntimeState next,
                       Decision, const SystemContext&, const PolicyPlan&, HoldReason,
                       MutationResult cpu, MutationResult resource) noexcept;
    void publishStoppedStatus() noexcept;

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
    FileMutationJournal resource_journal_;
    FileRetainedStateStore cpu_retained_store_;
    FileRetainedStateStore resource_retained_store_;
    MutationController controller_;
    ResourceMutationController resource_controller_;
    EnvironmentDiscovery discovery_;
    RuntimeObserver observer_;
    AdaptivePolicy policy_;
    ContextEngine context_engine_;
    PolicyEngine policy_engine_;
    MutationAuthority mutation_authority_;
    ResourceStateModel resource_model_;
    EngineSnapshot snapshot_;
    std::uint64_t sample_count_{0};
    std::deque<RuntimeSample> history_;
    std::uint64_t last_notification_sample_{0};
    NotificationEvent last_notification_{NotificationEvent::None};
    BaselineIntelligence baseline_intelligence_;
    ExperienceMemory experience_memory_;
    // Context in which the currently observed candidate was chosen.
    ExperienceMemory::Context observation_context_{};
    ThermalPredictor thermal_predictor_;
    std::uint64_t mutation_cooldown_until_sample_{0};
    ThermalGuardState thermal_guard_state_{};

    // v2.4.0 interactivity pipeline (observe by default; the boost path obeys
    // interactivity_mode plus the same global armed/adaptive authority gates).
    understand::InteractivityModel interactivity_model_;
    observe::GpuBusySource gpu_busy_source_;
    observe::ForegroundTracker foreground_tracker_;
    observe::LaunchBurstDetector launch_detector_;
    observe::FrameTelemetrySource frame_telemetry_;
    InteractivityPolicy interactivity_policy_;
    InteractivityBoostController boost_controller_;
    std::unique_ptr<FileMutationJournal> interactivity_journal_;
    bool interactivity_sources_ready_{false};
    understand::InteractionState last_interactivity_state_{understand::InteractionState::Idle};

    EngineStatus last_status_;
    std::uint64_t status_verified_{0};
    std::uint64_t status_rolled_back_{0};
    std::uint64_t status_failed_{0};
    std::uint64_t last_change_sample_{0};
    std::string last_change_kind_;
    std::string last_change_result_;
};

} // namespace coreflow
