#include "coreflow/decision/decision.hpp"
#include "coreflow/optimization/workload.hpp"

namespace coreflow {
bool ChangeJustifier::justified(const ChangeProposal& proposal, const ObservationFrame& frame,
                                std::string& reason) const {
    if (proposal.path.empty() || proposal.capability_id.empty()) {
        reason = "Candidate identity is incomplete";
        return false;
    }
    if (proposal.previous_value == proposal.target_value) {
        reason = "Candidate has no effect";
        return false;
    }
    if (!(workloadScoreForDomain(proposal.domain, frame.system, frame.noise) > 0.0)) {
        reason = "No sufficiently strong, domain-relevant runtime signal justifies this change";
        return false;
    }
    reason = proposal.rationale;
    return true;
}
} // namespace coreflow
