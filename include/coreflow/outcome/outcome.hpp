#pragma once
#include "coreflow/model/types.hpp"
namespace coreflow {
class OutcomeVerifier {
public:
    // Every threshold comes from the learned noise floor of the observation that
    // preceded the change. Signals without a learned floor are inconclusive.
    Outcome verify(const SystemObservation& before, const SystemObservation& after,
                   const NoiseFloor& noise) const;
    Outcome verify(const SystemObservation& before, const SystemObservation& after,
                   const ChangeProposal& proposal, const NoiseFloor& noise) const;
};
class OutcomePolicy {
public:
    Disposition decide(const Outcome&) const;
};
}
