#pragma once
#include <unordered_set>

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "coreflow/actuator.hpp"
#include "coreflow/config.hpp"
#include "coreflow/runtime_intelligence.hpp"
#include "coreflow/control.hpp"
#include "coreflow/interactivity_policy.hpp"
#include "coreflow/mutation_journal.hpp"
#include "coreflow/resource_actuator.hpp"

namespace coreflow {

// Executes InteractivityPolicy intents with the same write discipline as the
// rest of the engine: capability + permit gate, journal-before-write, live
// read-back, restore on expiry or failure. Every boost has a hard deadline
// and self-restores; nothing persists beyond its window. Fail-closed: no
// journal commit means no write; a failed write restores the previous
// journal state.
class InteractivityBoostController final {
public:
    // Quarantine is shared across actuator classes; it never changes global config.
    void setExternalControlPaths(const std::unordered_set<std::string>* paths) noexcept {
        external_control_paths_ = paths;
    }
    // Withdraw only externally replaced targets, without writing their kernel nodes.
    bool reconcileExternalChanges(std::unordered_set<std::string>& paths) noexcept;

    void setJournal(MutationJournal* journal) noexcept { journal_ = journal; }
    void setGpuMinFreqPath(std::string path) { gpu_min_freq_path_ = std::move(path); }
    void setTopAppUclampPath(std::string path) { uclamp_path_ = std::move(path); }
    void addReadaheadPath(std::string path) {
        readahead_paths_.push_back(std::move(path));
    }
    void clearReadaheadPaths() noexcept { readahead_paths_.clear(); }
    // Receives dynamically discovered read-ahead resources; never synthesizes paths.

    // Adopts the entries a previous run left in the journal, so each one is
    // restored by the next expire() pass. Returns the number adopted. A corrupt
    // journal blocks new boosts and is never overwritten.
    std::size_t recoverFromJournal() noexcept;
    // Applies every eligible intent in the current policy batch.
    [[nodiscard]] std::size_t apply(const std::vector<BoostIntent>& intents,
                                    const EngineConfig& config,
                                    std::uint64_t now_ms) noexcept;
    // Restores and forgets every expired boost.
    void expire(std::uint64_t now_ms) noexcept;
    // Restores everything and clears the journal (thermal guard, disarm).
    void restoreAll() noexcept;
    [[nodiscard]] ControlContinuity inspectContinuity() const noexcept;

    [[nodiscard]] std::size_t activeCount() const noexcept { return active_.size(); }
    [[nodiscard]] std::uint64_t earliestDeadlineMs() const noexcept {
        std::uint64_t earliest = 0U;
        for (const auto& entry : active_) {
            if (entry.second.restored) continue;
            if (earliest == 0U || entry.second.deadline_ms < earliest) {
                earliest = entry.second.deadline_ms;
            }
        }
        return earliest;
    }
    [[nodiscard]] bool authorized(const EngineConfig& config) const noexcept;
    [[nodiscard]] bool recoveryRequired() const noexcept { return recovery_required_; }

private:
    const std::unordered_set<std::string>* external_control_paths_{nullptr};

    struct ActiveBoost {
        ActuatorId id{};
        std::string baseline;
        std::string requested;
        std::uint64_t deadline_ms{0};
        // True once the baseline is verified back on the device. The entry is
        // kept until the journal, without it, is durably committed.
        bool restored{false};
    };
    bool resolveTarget(const BoostIntent& intent, std::string& path,
                       std::string& requested) noexcept;
    // Writes the journal of entries not yet restored. Sets recovery_required_
    // on failure, and clears it on success.
    bool commitJournal() noexcept;
    // commitJournal(), then drops the restored entries. Returns false when the
    // journal could not be committed; the entries are then kept.
    bool settle() noexcept;
    MutationJournal* journal_{nullptr};
    MutationAuthority authority_;
    ResourceActuator actuator_;
    std::map<std::string, ActiveBoost> active_;
    std::string gpu_min_freq_path_;
    std::string uclamp_path_;
    std::vector<std::string> readahead_paths_;
    std::uint16_t next_instance_{0};
    // Set when the journal cannot be trusted or written. No new boost is
    // applied until a commit succeeds.
    bool recovery_required_{false};
    // Set when the journal on disk is corrupt. It is left untouched for diagnosis.
    bool journal_untrusted_{false};
};

} // namespace coreflow
