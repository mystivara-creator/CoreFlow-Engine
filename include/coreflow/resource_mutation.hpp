#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "coreflow/config.hpp"
#include "coreflow/control.hpp"
#include "coreflow/decision_agent.hpp"
#include "coreflow/experience.hpp"
#include "coreflow/mutation_journal.hpp"
#include "coreflow/resource_actuator.hpp"
#include "coreflow/resource_model.hpp"
#include "coreflow/types.hpp"

namespace coreflow {

// Autonomous tuning for *discovered* VM/I/O/Scheduler resources.
// Candidate generation is capability-driven via EffectModel — not a fixed
// allow-list of resource names. Discovery decides what exists and is writable;
// EffectModel scores whether a bounded change improves stability; this
// controller applies at most one verified mutation per cycle with journal
// and rollback.
class ResourceMutationController final {
public:
    void setJournal(MutationJournal* journal) noexcept;
    void setExperienceMemory(const ExperienceMemory* memory) noexcept;

    // Capture factory baselines only for explicitly policy-authorized,
    // mutation-ready resources. Production block queue controls are quarantined;
    // legacy journal restore is still permitted for recovery.
    bool captureBaseline(const DeviceProfile& profile) noexcept;

    bool restoreAll() noexcept;

    MutationResult apply(const RuntimeState state,
                         const RuntimeSample& sample,
                         const DeviceProfile& profile,
                         const EngineConfig& config,
                         const PolicyPlan& policy,
                         const MutationPermit& permit) noexcept;

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

    bool mutated() const noexcept { return mutated_; }
    // True when every change currently held was made under a stabilizing-only
    // plan. Such a change may stay in place through a thermal/memory safety
    // state; any other change must be restored there.
    bool stabilizingEpoch() const noexcept { return mutated_ && stabilizing_epoch_; }
    // A candidate whose outcome regressed is not selected again for this many
    // controller cycles.
    static constexpr std::uint64_t kRejectionCooldownCycles = 120;
    std::size_t baselineSize() const noexcept { return baseline_.size(); }

private:
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
        Candidate& out) const noexcept;
    MutationResult applyCandidate(const Candidate& candidate,
                                  const MutationPermit& permit) noexcept;
    MutationJournal::Entries factorySnapshot() const;
    bool verifySchedulerToken(const std::string& path,
                              const std::string& token,
                              std::string& resolved) const noexcept;

    MutationJournal* journal_{nullptr};
    const ExperienceMemory* experience_memory_{nullptr};
    ResourceActuator actuator_;
    ScoringDecisionAgent decision_agent_;
    std::unordered_map<std::string, Baseline> baseline_;
    std::unordered_set<std::string> dirty_resources_;
    std::vector<std::pair<std::string, std::string>> last_applied_resources_;
    bool recovery_pending_{false};
    bool baseline_captured_{false};
    bool journal_committed_{false};
    bool mutated_{false};
    bool restore_failed_{false};
    bool stabilizing_epoch_{false};
    std::uint64_t cycle_{0};
    // "path\nrequested" -> cycle at which the candidate was rejected.
    std::unordered_map<std::string, std::uint64_t> rejected_;
    bool isRejected(const std::string& path,
                    const std::string& requested) const noexcept;
};

} // namespace coreflow
