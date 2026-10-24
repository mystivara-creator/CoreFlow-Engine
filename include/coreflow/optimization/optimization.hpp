#pragma once
#include "coreflow/model/types.hpp"
#include <deque>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>
namespace coreflow {
class OptimizationEngine {
public:
    std::optional<ChangeProposal> propose(const std::vector<Capability>&,
                                          const ObservationFrame&);
    // Only conclusive outcomes update exploration. Inconclusive evidence is neutral.
    void recordOutcome(const std::string& capability_id, bool improved);
    void recordOutcome(const std::string& capability_id, const std::string& domain,
                       const std::string& attribute, bool improved, bool retained);
    void recordOutcome(const ChangeProposal& proposal, bool improved, bool retained,
                       const ObservationFrame& frame);
    // Outcome-aware entry point: inconclusive measurements never train learning.
    void recordOutcome(const ChangeProposal& proposal, const Outcome& outcome,
                       Disposition disposition, const ObservationFrame& frame);
    void defer(const ChangeProposal& proposal, const ObservationFrame& frame);

    // A retained selector change (I/O elevator, CPU governor, devfreq governor)
    // creates new kernel controls beneath it. The runtime must rediscover before
    // proposing their follow-ups. Returns true once per request.
    [[nodiscard]] bool consumeRediscoveryRequest() noexcept;

    // Forgets all learned direction, step, and dependency state (operator reset).
    void reset();

private:
    // True when `candidate` is a legitimate follow-up of the last retained selector:
    // a same-subsystem sibling, or a tunable beneath the newly selected directory.
    [[nodiscard]] bool isDependencyFollowup(const Capability& candidate) const;

    void trackContext(const std::string& key);
    void trackCapability(const std::string& capability_id);

    std::map<std::string, int> direction_by_context_;
    std::map<std::string, double> step_fraction_by_context_;
    // Legacy feedback without workload context is retained only for API compatibility.
    std::map<std::string, int> legacy_direction_by_capability_;
    std::map<std::string, double> legacy_step_fraction_by_capability_;
    std::map<std::string, std::size_t> next_choice_index_;
    std::map<std::string, std::set<std::string>> tried_choices_by_context_;
    std::set<std::string> retained_choice_contexts_;
    std::set<std::string> deferred_numeric_contexts_;
    std::map<std::string, unsigned> non_improving_by_context_;
    std::map<std::string, unsigned> numeric_trials_by_context_;
    std::deque<std::string> context_history_order_;
    std::set<std::string> tracked_context_keys_;
    std::deque<std::string> capability_history_order_;
    std::set<std::string> tracked_capability_ids_;
    std::string dependency_parent_;
    std::string dependency_domain_;
    std::string dependency_trigger_attribute_;
    std::string dependency_workload_signature_;
    // Directory whose children are re-evaluated after the selector changed, for
    // example <queue>/iosched or <policy>/<new-governor>. Empty when none applies.
    std::string dependency_child_parent_;
    bool dependency_followup_pending_{false};
    bool rediscovery_requested_{false};
    std::size_t next_index_{0};
};
}
