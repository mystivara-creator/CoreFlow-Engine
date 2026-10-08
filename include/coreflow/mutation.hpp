#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "coreflow/config.hpp"
#include "coreflow/cpufreq_actuator.hpp"
#include "coreflow/experience.hpp"
#include "coreflow/mutation_journal.hpp"
#include "coreflow/types.hpp"

namespace coreflow {

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
    MutationController() = default;

    void setJournal(MutationJournal* journal) noexcept;
    void setExperienceMemory(const ExperienceMemory* memory) noexcept;

    // Returns false when no trustworthy baseline exists (pending journal not
    // recovered, or governors still mutated). Callers must not mutate then.
    bool captureBaseline(const DeviceProfile& profile) noexcept;

    MutationResult apply(
        RuntimeState state,
        const RuntimeSample& sample,
        const DeviceProfile& profile,
        const EngineConfig& config
    ) noexcept;

    // True only when every touched governor has been verified at its factory value.
    bool restoreAll() noexcept;

    // Suppress the targets of the most recent verified mutation for a bounded cooldown.
    void rejectLastMutation() noexcept;

    bool mutated() const noexcept { return mutated_; }
    std::size_t baselineSize() const noexcept { return baseline_.size(); }
    const std::vector<std::pair<std::string, std::string>>& lastAppliedGovernors() const noexcept {
        return last_applied_governors_;
    }

    static constexpr std::uint64_t kRejectionCooldownCycles = 120;

private:
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
    double governorScore(
        const std::string& governor,
        RuntimeState state,
        const RuntimeSample& sample
    ) const noexcept;
    MutationResult applyPlan(const std::vector<MutationPlanEntry>& plan) noexcept;
    MutationResult restoreGovernors() noexcept;
    MutationJournal::Entries factorySnapshot() const;

    std::unordered_map<std::string, Baseline> baseline_;
    std::vector<std::pair<std::string, std::string>> last_applied_governors_;
    // policy_path -> governor -> decision cycle at which it was rejected
    std::unordered_map<std::string, std::unordered_map<std::string, std::uint64_t>> rejected_;
    const ExperienceMemory* experience_memory_{nullptr};
    MutationJournal* journal_{nullptr};
    const DeviceProfile* profile_{nullptr};
    CpuFreqActuator cpufreq_actuator_{};
    std::uint64_t decision_cycle_{0};

    bool actuator_ready_{false};
    bool baseline_captured_{false};
    bool mutated_{false};            // a write may have changed live governors
    bool journal_committed_{false};  // journal holds factory values for this epoch
    bool restore_failed_{false};     // last restore incomplete: block new mutations
    bool recovery_pending_{true};    // a previous run's journal must be recovered
    // Factory values recovered from a previous run's journal. Discovery after a
    // crash reads the mutated governor, so these must override the profile.
    MutationJournal::Entries recovered_factory_;
};

} // namespace coreflow
