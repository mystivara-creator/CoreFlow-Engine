#pragma once

#include "coreflow/model/types.hpp"
#include <string>

namespace coreflow {

// Returns a stable signature of pressure classes currently supported by valid
// measurements. The signature is used to scope online learning to a workload.
std::string workloadSignature(const SystemObservation& observation);

// Score for a control domain under the current measurements. A zero score means
// that no relevant, sufficiently strong signal justifies experimenting with it.
double workloadScoreForDomain(const std::string& domain,
                              const SystemObservation& observation);

} // namespace coreflow
