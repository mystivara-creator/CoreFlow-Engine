#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "coreflow/actuator.hpp"
#include "coreflow/config.hpp"
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
    void setJournal(MutationJournal* journal) noexcept { journal_ = journal; }
    void setGpuMinFreqPath(std::string path) { gpu_min_freq_path_ = std::move(path); }
    void setTopAppUclampPath(std::string path) { uclamp_path_ = std::move(path); }
    void addReadaheadPath(std::string path) {
        readahead_paths_.push_back(std::move(path));
    }
    // Adds every /sys/block/<dev>/queue/read_ahead_kb that is writable.
    void discoverReadaheadPaths() noexcept;

    // Applies every eligible intent in the current policy batch.
    [[nodiscard]] std::size_t apply(const std::vector<BoostIntent>& intents,
                                    const EngineConfig& config,
                                    std::uint64_t now_ms) noexcept;
    // Restores and forgets every expired boost.
    void expire(std::uint64_t now_ms) noexcept;
    // Restores everything and clears the journal (thermal guard, disarm).
    void restoreAll() noexcept;

    [[nodiscard]] std::size_t activeCount() const noexcept { return active_.size(); }
    [[nodiscard]] bool authorized(const EngineConfig& config) const noexcept;

private:
    struct ActiveBoost {
        ActuatorId id{};
        std::string baseline;
        std::string requested;
        std::uint64_t deadline_ms{0};
    };
    bool resolveTarget(const BoostIntent& intent, std::string& path,
                       std::string& requested) noexcept;
    bool commitJournal() noexcept;
    MutationJournal* journal_{nullptr};
    MutationAuthority authority_;
    ResourceActuator actuator_;
    std::map<std::string, ActiveBoost> active_;
    std::string gpu_min_freq_path_;
    std::string uclamp_path_;
    std::vector<std::string> readahead_paths_;
    std::uint16_t next_instance_{0};
};

} // namespace coreflow
