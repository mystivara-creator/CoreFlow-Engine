#include "coreflow/decision/decision.hpp"
namespace coreflow {
bool ChangeJustifier::justified(const ChangeProposal& p, const ObservationFrame& f,
                                std::string& reason) const {
    if (p.path.empty() || p.capability_id.empty()) { reason="Candidate identity is incomplete"; return false; }
    if (p.previous_value == p.target_value) { reason="Candidate has no effect"; return false; }
    // Require at least one valid runtime signal so outcome can be evaluated.
    if (!f.system.memory_valid && !f.system.temperature_valid &&
        !f.system.cpu_pressure_valid && !f.system.memory_pressure_valid &&
        !f.system.io_pressure_valid) {
        reason="No valid system-pressure metric is available to evaluate the outcome"; return false;
    }
    reason=p.rationale;
    return true;
}
}
