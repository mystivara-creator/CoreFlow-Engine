#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "coreflow/config.hpp"
#include "coreflow/control.hpp"
#include "coreflow/mutation_journal.hpp"
#include "coreflow/resource_actuator.hpp"
#include "coreflow/types.hpp"

namespace coreflow {

// Autonomous tuning for discovered VM/I/O resources. It deliberately shares
// the same MutationPermit concept as CPUFreq, while using a separate journal
// so resource recovery cannot interfere with the CPU governor journal.
class ResourceMutationController final {
public:
    void setJournal(MutationJournal* journal) noexcept;

    bool captureBaseline(const DeviceProfile& profile) noexcept;
    bool restoreAll() noexcept;

    MutationResult apply(const RuntimeState state,
                         const RuntimeSample& sample,
                         const DeviceProfile& profile,
                         const EngineConfig& config,
                         const PolicyPlan& policy,
                         const MutationPermit& permit) noexcept;

    MutationResult apply(const RuntimeState state,
                         const RuntimeSample& sample,
                         const DeviceProfile& profile,
                         const EngineConfig& config) noexcept;

    bool syncPolicyModel(ResourceStateModel& model) const noexcept;
    void rejectLastMutation() noexcept;
    const std::vector<std::pair<std::string, std::string>>& lastAppliedResources() const noexcept {
        return last_applied_resources_;
    }

    bool mutated() const noexcept { return mutated_; }
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
    };

    bool recoverIfNeeded() noexcept;
    bool selectCandidate(
        RuntimeState state,
        const RuntimeSample& sample,
        const DeviceProfile& profile,
        const PolicyPlan& policy,
        Candidate& out) const noexcept;
    bool applyCandidate(const Candidate& candidate, const MutationPermit& permit) noexcept;
    MutationJournal::Entries factorySnapshot() const;
    MutationJournal::Entries journalEntriesWith(const std::string& path) const;

    std::unordered_map<std::string, Baseline> baseline_;
    std::unordered_set<std::string> dirty_resources_;
    std::vector<std::pair<std::string, std::string>> last_applied_resources_;
    MutationJournal* journal_{nullptr};
    ResourceActuator actuator_{};
    bool recovery_pending_{true};
    bool baseline_captured_{false};
    bool journal_committed_{false};
    bool mutated_{false};
    bool restore_failed_{false};
    std::uint64_t cycle_{0};
};

} // namespace coreflow
