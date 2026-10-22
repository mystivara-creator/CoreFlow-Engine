#pragma once
#include "coreflow/model/types.hpp"
namespace coreflow {
class OutcomeVerifier {
public:
    Outcome verify(const SystemObservation& before,
                   const SystemObservation& after) const;
};
class OutcomePolicy {
public:
    Disposition decide(const Outcome&) const;
};
}
