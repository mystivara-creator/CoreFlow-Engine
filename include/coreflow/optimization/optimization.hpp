#pragma once
#include "coreflow/model/types.hpp"
#include <map>
#include <optional>
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
private:
    std::map<std::string, int> direction_by_capability_;
    std::map<std::string, double> step_fraction_by_capability_;
    std::map<std::string, std::size_t> next_choice_index_;
    std::string dependency_domain_;
    std::string dependency_trigger_attribute_;
    bool dependency_followup_pending_{false};
    std::size_t next_index_{0};
};
}
