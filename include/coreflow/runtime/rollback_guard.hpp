#pragma once

// Compatibility include for callers that used the original runtime path.
// The guard itself lives in coreflow::internal so lower-level modules such as
// mutation can use transaction rollback without depending on the runtime layer.
#include "coreflow/internal/rollback_guard.hpp"

namespace coreflow::runtime {
template <typename RollbackFn>
using RollbackGuard = coreflow::internal::RollbackGuard<RollbackFn>;
} // namespace coreflow::runtime
