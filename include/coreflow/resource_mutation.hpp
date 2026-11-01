#pragma once
#include "coreflow/trial_history.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "coreflow/config.hpp"
#include "coreflow/plan_option.hpp"
#include "coreflow/consultation.hpp"
#include "coreflow/control.hpp"
#include "coreflow/decision_agent.hpp"
#include "coreflow/experience.hpp"
#include "coreflow/mutation_journal.hpp"
#include "coreflow/retained_state.hpp"
#include "coreflow/resource_actuator.hpp"
#include "coreflow/resource_model.hpp"
#include "coreflow/types.hpp"

namespace coreflow {

struct DecisionTraceContext;

// Autonomous tuning for discovered VM/I/O/scheduler/GPU/cgroup resources.
// Candidate generation is capability-driven via EffectModel — not a fixed
// allow-list of resource names. Discovery decides what exists and is writable;
// EffectModel scores whether a bounded change improves stability; this
// controller applies one bounded, coordinated cohort per cycle with journal
// and rollback for every member.
enum class ResourceMutationFailureStage : std::uint8_t {
    None = 0,
    Recovery,
    JournalCorrupt,
    RecoveryActuator,
    JournalUnavailable,
    BaselineRead,
    JournalCommit,
    ActuatorRead,
    ActuatorSafety,
    ActuatorWriteOrVerify,
    ActuatorOther,
    JournalCleanup,
    Rollback
};

inline const char* resourceMutationFailureStageName(
    ResourceMutationFailureStage stage
) noexcept {
    switch (stage) {
        case ResourceMutationFailureStage::None: return "NONE";
        case ResourceMutationFailureStage::Recovery: return "RECOVERY_FAILED";
        case ResourceMutationFailureStage::JournalCorrupt: return "JOURNAL_CORRUPT";
        case ResourceMutationFailureStage::RecoveryActuator:
            return "RECOVERY_ACTUATOR_FAILED";
        case ResourceMutationFailureStage::JournalUnavailable: return "JOURNAL_UNAVAILABLE";
        case ResourceMutationFailureStage::BaselineRead: return "BASELINE_READ_FAILED";
        case ResourceMutationFailureStage::JournalCommit: return "JOURNAL_COMMIT_FAILED";
        case ResourceMutationFailureStage::ActuatorRead: return "ACTUATOR_READ_FAILED";
        case ResourceMutationFailureStage::ActuatorSafety: return "ACTUATOR_SAFETY_REJECTED";
        case ResourceMutationFailureStage::ActuatorWriteOrVerify:
            return "ACTUATOR_WRITE_OR_VERIFY_FAILED";
        case ResourceMutationFailureStage::ActuatorOther: return "ACTUATOR_FAILED";
        case ResourceMutationFailureStage::JournalCleanup: return "JOURNAL_CLEANUP_FAILED";
        case ResourceMutationFailureStage::Rollback: return "ROLLBACK_FAILED";
    }
    return "UNKNOWN";
}

class ResourceMutationController final {
public:
    // Quarantine is shared across actuator classes; it never changes global config.
    void setExternalControlPaths(const std::unordered_set<std::string>* paths) noexcept {
        external_control_paths_ = paths;
    }
    // Withdraw only externally replaced targets, without writing their kernel nodes.
    bool reconcileExternalChanges(std::unordered_set<std::string>& paths,
                                  std::string* failure = nullptr) noexcept;

    void setJournal(MutationJournal* journal) noexcept;
    void setRetainedStateStore(RetainedStateStore* store) noexcept;
    void setExperienceMemory(const ExperienceMemory* memory) noexcept;

    // Capture factory baselines only for explicitly policy-authorized,
    // mutation-ready resources. Block-queue numeric controls (read_ahead_kb /
    // nr_requests on physical devices) are admitted when grant_block_queue is
    // enabled; scheduler tokens remain out-of-class. Legacy journal restore is
    // still permitted for recovery.
    bool captureBaseline(const DeviceProfile& profile) noexcept;
    void setTrialHistory(const TrialHistory* history) noexcept { trial_history_=history; }
    bool appendPendingTrials(std::vector<TrialMember>& out) const noexcept;

    PlanOption preview(RuntimeState state, const RuntimeSample& sample, const DeviceProfile& profile,
                       const EngineConfig& config, const PolicyPlan& policy) noexcept;
    MutationResult applySelected(const PlanOption& selected, RuntimeState state,
        const RuntimeSample& sample, const DeviceProfile& profile, const EngineConfig& config,
        const PolicyPlan& policy, const MutationPermit& permit, const DecisionTraceContext* trace = nullptr) noexcept;
    bool recoveryReady() const noexcept { return !recovery_pending_ && !restore_failed_; }
    bool restoreAll() noexcept;
    // Re-evaluate one accepted change; caller must settle dependent controllers first.
    bool restoreOneRetained() noexcept;
    const std::vector<ResourceCapability>& discoveredCapabilities() const noexcept { return supplemental_; }
    bool admitNewBaselines(const DeviceProfile& profile) noexcept;
    bool retainLastMutation(const DeviceProfile& profile) noexcept;
    bool acceptCoordinatedMutation() noexcept;
    RetainedStateFailureStage lastRetainedStateFailureStage() const noexcept {
        return last_retained_state_failure_stage_;
    }
    bool rollbackLastMutation() noexcept;
    bool hasPendingOutcome() const noexcept { return pending_outcome_; }

    MutationResult apply(const RuntimeState state,
                         const RuntimeSample& sample,
                         const DeviceProfile& profile,
                         const EngineConfig& config,
                         const PolicyPlan& policy,
                         const MutationPermit& permit,
                         const DecisionTraceContext* trace_context = nullptr) noexcept;

    // Convenience overload used by tests — builds a permissive plan.
    MutationResult apply(const RuntimeState state,
                         const RuntimeSample& sample,
                         const DeviceProfile& profile,
                         const EngineConfig& config) noexcept;

    bool syncPolicyModel(ResourceStateModel& model) const noexcept;
    void rejectLastMutation() noexcept;

    const std::vector<std::pair<std::string, std::string>>&
    lastAppliedResources() const noexcept {
        return last_applied_resources_;
    }
    const std::vector<std::pair<std::string, std::string>>&
    activeResourceSettings() const noexcept { return active_resources_cache_; }

    bool mutated() const noexcept { return mutated_; }
    ResourceMutationFailureStage lastFailureStage() const noexcept {
        return last_failure_stage_;
    }
    ActuatorStatus lastActuatorStatus() const noexcept {
        return last_actuator_status_;
    }

    // Only one independently measurable resource change per outcome epoch.
    static constexpr std::size_t kMaxCohortWrites = 1U;
    // CPU-governor follow-ups may select only candidates with coupling evidence.
    void setCoupledOnly(bool value) noexcept { coupled_only_ = value; }
    void setConditionDriven(bool value) noexcept { condition_driven_ = value; }
    CandidateFunnel candidateFunnel() const noexcept { return candidate_funnel_; }

    // Last DecisionAgent reasoning summary for this cycle (empty if none).
    const std::string& lastAgentReason() const noexcept { return last_agent_reason_; }
    // True when every change currently held was made under a stabilizing-only
    // plan. Such a change may stay in place through a thermal/memory safety
    // state; any other change must be restored there.
    bool stabilizingEpoch() const noexcept { return mutated_ && stabilizing_epoch_; }
    // A candidate whose outcome regressed is not selected again for this many
    // controller cycles.
    static constexpr std::uint64_t kRejectionCooldownCycles = 120;
    std::size_t baselineSize() const noexcept { return baseline_.size(); }

    // Bounded record of consultation outcomes, for status and diagnostics.
    const ConsultationLedger& consultationLedger() const noexcept {
        return consultation_ledger_;
    }

private:
    std::vector<std::pair<std::string, std::string>> accepted_resources_;
    const std::unordered_set<std::string>* external_control_paths_{nullptr};

    struct Baseline {
        ResourceDomain domain{ResourceDomain::AndroidRuntime};
        std::string name;
        std::string path;
        std::string value;
        bool valid{false};
    };

    struct Candidate {
        ResourceDomain domain{ResourceDomain::AndroidRuntime};
        std::string name;
        std::string path;
        std::string requested;
        std::string baseline;
        std::string current;
        double confidence{0.0};
        double utility{0.0};
        double benefit{0.0};
        double risk{1.0};
        std::string reason;
    };

    bool recoverIfNeeded() noexcept;
    bool selectCandidate(
        RuntimeState state,
        const RuntimeSample& sample,
        const DeviceProfile& profile,
        const PolicyPlan& policy,
        const UserGrants& grants,
        Candidate& out, bool companion = false, bool condition_driven = false) const noexcept;
    MutationResult applyCandidate(const Candidate& candidate,
                                  RuntimeState state,
                                  double confidence,
                                  const EngineConfig& config,
                                  const PolicyPlan& policy,
                                  const DecisionTraceContext* trace_context) noexcept;
    MutationJournal::Entries factorySnapshot() const;
    bool verifySchedulerToken(const std::string& path,
                              const std::string& token,
                              std::string& resolved) const noexcept;

    struct PendingChange {
        std::string path, target, previous;
        bool was_dirty{false};
        bool from_retained{false};
    };
    std::vector<PendingChange> pending_changes_;
    std::vector<std::string> write_order_;
    std::vector<ResourceCapability> supplemental_;
    bool rollbackSingleMutation() noexcept;
    bool coupled_only_{false};
    bool condition_driven_{false};
    mutable CandidateFunnel candidate_funnel_;
    const PlanOption* selected_primary_{nullptr};
    MutationJournal* journal_{nullptr};
    RetainedStateStore* retained_store_{nullptr};
    RetainedStateStore::Entries retained_entries_;
    mutable ConsultationLedger consultation_ledger_;
    std::vector<std::pair<std::string, std::string>> retained_active_resources_;
    std::vector<std::pair<std::string, std::string>> active_resources_cache_;
    const ExperienceMemory* experience_memory_{nullptr};
    const TrialHistory* trial_history_{nullptr};
    ResourceActuator actuator_;
    ScoringDecisionAgent decision_agent_;
    std::unordered_map<std::string, Baseline> baseline_;
    std::unordered_set<std::string> dirty_resources_;
    std::vector<std::pair<std::string, std::string>> last_applied_resources_;
    bool recovery_pending_{true};
    bool baseline_captured_{false};
    bool journal_committed_{false};
    bool mutated_{false};
    bool restore_failed_{false};
    bool stabilizing_epoch_{false};
    bool pending_outcome_{false};
    RetainedStateFailureStage last_retained_state_failure_stage_{
        RetainedStateFailureStage::None
    };
    ResourceMutationFailureStage last_failure_stage_{ResourceMutationFailureStage::None};
    ActuatorStatus last_actuator_status_{ActuatorStatus::Invalid};
    std::string last_mutation_path_;
    std::string last_mutation_target_;
    std::string last_previous_value_;
    bool last_path_was_dirty_before_{false};
    bool last_candidate_from_retained_{false};
    // Written from const selectCandidate for status visibility only.
    mutable std::string last_agent_reason_;
    std::uint64_t cycle_{0};
    // "path\nrequested" -> cycle at which the candidate was rejected.
    std::unordered_map<std::string, std::uint64_t> rejected_;
    void refreshRetainedActiveCache() noexcept;
    void refreshActiveSettingsCache() noexcept;
    bool isRejected(const std::string& path,
                    const std::string& requested) const noexcept;
};

} // namespace coreflow
