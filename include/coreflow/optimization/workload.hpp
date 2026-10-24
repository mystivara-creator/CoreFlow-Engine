#pragma once

#include "coreflow/model/types.hpp"
#include <string>

namespace coreflow {

// Returns a stable signature of the signal classes currently elevated above this
// device's own noise floor. The signature is used to scope online learning to a workload.
std::string workloadSignature(const SystemObservation& observation, const NoiseFloor& noise);

// Score for a control domain under the current measurements. A zero score means
// that no relevant, sufficiently strong signal justifies experimenting with it.
double workloadScoreForDomain(const std::string& domain,
                              const SystemObservation& observation,
                              const NoiseFloor& noise);

} // namespace coreflow
