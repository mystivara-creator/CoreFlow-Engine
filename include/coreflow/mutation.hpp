#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "coreflow/config.hpp"
#include "coreflow/control.hpp"
#include "coreflow/cpufreq_actuator.hpp"
#include "coreflow/experience.hpp"
#include "coreflow/mutation_journal.hpp"
#include "coreflow/retained_state.hpp"
#include "coreflow/types.hpp"

namespace coreflow {

struct DecisionTraceContext;

/**
 * Bounded, verified, restorable CPUFreq governor controller.
 *
 * Safety invariants:
 *   1. The factory baseline is captured only while live governors are known
 *      to be unmutated. A baseline is never taken from a mutated state.
 *   2. Before the first sysfs write, the factory values are committed to a
 *      durable journal. If the daemon dies mid-mutation, the next start
 *      restores them before doing anything else.
 *   3. Every write is read back and verified. A failed restore blocks new
 *      mutations until the restore succeeds.
 *   4. Regressed candidates are suppressed for a bounded cooldown.
 */
class MutationController {
public:
    // Quarantine is shared across actuator classes; it never changes global config.
    void setExternalControlPaths(const std::unordered_set<std::string>* paths) noexcept {
        external_control_paths_ = paths;
    }
    // Withdraw only externally replaced targets, without writing their kernel nodes.
    bool reconcileExternalChanges(std::unordered_set<std::string>& paths) noexcept;

    MutationController() = default;

    void setJournal(MutationJournal* journal) noexcept;
    void setRetainedStateStore(RetainedStateStore* store) noexcept;
    void setExperienceMemory(const ExperienceMemory* memory) noexcept;

    // Returns false when no trustworthy baseline exists (pending journal not
    // recovered, or governors still mutated). Callers must not mutate then.
    bool captureBaseline(const DeviceProfile& profile) noexcept;

    MutationResult apply(
        RuntimeState state,
        const RuntimeSample& sample,
        const DeviceProfile& profile,
        const EngineConfig& config,
        const PolicyPlan& policy,
        const MutationPermit& permit,
        const DecisionTraceContext* trace_context = nullptr
    ) noexcept;

    // Compatibility entry point for deterministic/unit callers. It still obtains
    // its permit from MutationAuthority; no actuator bypass is introduced.
    MutationResult apply(
        RuntimeState state,
        const RuntimeSample& sample,
        const DeviceProfile& profile,
        const EngineConfig& config
    ) noexcept;

    // True only when every still-owned governor is restored; externally handed-off
    // paths are no longer restore obligations.
    bool restoreAll() noexcept;

    // Persist the most recent candidate only after a beneficial outcome was measured.
    bool retainLastMutation(const DeviceProfile& profile) noexcept;
    RetainedStateFailureStage lastRetainedStateFailureStage() const noexcept {
        return last_retained_state_failure_stage_;
    }
    // Roll back only the candidate under evaluation, preserving earlier retained settings.
    bool rollbackLastMutation() noexcept;
    void rejectLastMutation() noexcept;

    bool mutated() const noexcept { return mutated_; }
    bool hasPendingOutcome() const noexcept { return pending_outcome_; }
    std::size_t baselineSize() const noexcept { return baseline_.size(); }
    const std::vector<std::pair<std::string, std::string>>& lastAppliedGovernors() const noexcept {
        return last_applied_governors_;
    }
    const std::vector<std::pair<std::string, std::string>>&
    activeGovernorSettings() const noexcept { return active_governors_cache_; }

    static constexpr std::uint64_t kRejectionCooldownCycles = 120;

private:
    const std::unordered_set<std::string>* external_control_paths_{nullptr};

    struct Baseline {
        std::string value;
        bool valid{false};
    };

    struct MutationPlanEntry {
        std::string path;
        std::string target;
        std::string baseline;
    };

    bool ensureRecovered(const DeviceProfile& profile) noexcept;
    bool buildPlan(
        RuntimeState state,
        const RuntimeSample& sample,
        const DeviceProfile& profile,
        std::vector<MutationPlanEntry>& plan
    ) const noexcept;
    bool isRejected(const std::string& policy_path, const std::string& governor) const noexcept;
    void refreshRetainedActiveCache() noexcept;
    void refreshActiveSettingsCache() noexcept;
    double governorScore(
        const std::string& governor,
        RuntimeState state,
        const RuntimeSample& sample
    ) const noexcept;
    MutationResult applyPlan(const std::vector<MutationPlanEntry>& plan,
                              const MutationPermit& permit,
                              const DecisionTraceContext* trace_context) noexcept;
    MutationResult restoreGovernors() noexcept;
    // Restore only if this controller actually changed a governor. A controller
    // that never mutated must not write to the kernel at all.
    MutationResult relaxToBaseline() noexcept;
    MutationJournal::Entries factorySnapshot() const;
    MutationJournal::Entries journalEntriesWith(const std::string& path) const;

    std::unordered_map<std::string, Baseline> baseline_;
    std::vector<std::pair<std::string, std::string>> last_applied_governors_;
    // policy_path -> governor -> decision cycle at which it was rejected
    std::unordered_map<std::string, std::unordered_map<std::string, std::uint64_t>> rejected_;
    const ExperienceMemory* experience_memory_{nullptr};
    MutationJournal* journal_{nullptr};
    RetainedStateStore* retained_store_{nullptr};
    RetainedStateStore::Entries retained_entries_;
    std::vector<std::pair<std::string, std::string>> retained_active_governors_;
    std::vector<std::pair<std::string, std::string>> active_governors_cache_;
    const DeviceProfile* profile_{nullptr};
    CpuFreqActuator cpufreq_actuator_{};
    std::uint64_t decision_cycle_{0};

    bool actuator_ready_{false};
    bool baseline_captured_{false};
    bool mutated_{false};            // a write may have changed live governors
    // Per-policy record of governors this controller may have changed. A single
    // rolled-back write must not erase the record of another policy's mutation.
    std::unordered_set<std::string> dirty_governors_;
    bool journal_committed_{false};  // journal holds factory values for this epoch
    bool restore_failed_{false};     // last restore incomplete: block new mutations
    bool recovery_pending_{false};    // a previous run's journal must be recovered
    bool pending_outcome_{false};
    RetainedStateFailureStage last_retained_state_failure_stage_{
        RetainedStateFailureStage::None
    };
    std::string last_mutation_path_;
    std::string last_mutation_target_;
    std::string last_previous_value_;
    bool last_path_was_dirty_before_{false};
    bool last_candidate_from_retained_{false};
    // Factory values recovered from a previous run's journal. Discovery after a
    // crash reads the mutated governor, so these must override the profile.
    MutationJournal::Entries recovered_factory_;
};

} // namespace coreflow
