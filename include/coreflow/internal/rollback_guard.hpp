#pragma once

#include "coreflow/model/types.hpp"

#include <iostream>
#include <string>
#include <utility>

namespace coreflow::internal {

// Best-effort rollback for exceptions escaping a mutation transaction.
// The callback must not outlive the referenced AppliedChange.
template <typename RollbackFn>
class RollbackGuard final {
public:
    RollbackGuard(RollbackFn rollback, const AppliedChange& applied,
                  bool armed = true)
        : rollback_(std::move(rollback)), applied_(applied), active_(armed) {}

    RollbackGuard(const RollbackGuard&) = delete;
    RollbackGuard& operator=(const RollbackGuard&) = delete;
    RollbackGuard(RollbackGuard&&) = delete;
    RollbackGuard& operator=(RollbackGuard&&) = delete;

    ~RollbackGuard() noexcept {
        if (!active_) {
            return;
        }

        try {
            std::string detail;
            if (!rollback_(applied_, detail)) {
                try {
                    std::clog << "[CoreFlowEngine] Exception rollback unresolved: "
                              << detail << '\n';
                } catch (...) {
                    // Logging must not escape a destructor.
                }
            }
        } catch (...) {
            // Never propagate an exception from a destructor.
        }
    }

    void arm() noexcept { active_ = true; }
    void dismiss() noexcept { active_ = false; }

private:
    RollbackFn rollback_;
    const AppliedChange& applied_;
    bool active_{true};
};

template <typename RollbackFn>
RollbackGuard(RollbackFn, const AppliedChange&, bool)
    -> RollbackGuard<RollbackFn>;

template <typename RollbackFn>
RollbackGuard(RollbackFn, const AppliedChange&)
    -> RollbackGuard<RollbackFn>;

} // namespace coreflow::internal
