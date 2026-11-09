/**
 * @file recovery_monitor.hpp
 * @brief Verified, retryable recovery shared by daemon safety paths and tests.
 * @license MIT
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace coreflow {
class CentralMutationCoordinator;

class BaselineStore;

enum class RecoveryStatus : uint8_t {
    NoRecoveryRequired = 0,
    Recovered = 1,
    LiveOwnerPending = 2,
    JournalRecoveryPending = 3,
    OrphanLeasePending = 4,
    ActiveLeasePending = 5,
    CallbackDrainPending = 6,
};

struct RecoveryResult {
    RecoveryStatus status{RecoveryStatus::NoRecoveryRequired};
    size_t active_leases{0};
    size_t live_owner_pending{0};

    [[nodiscard]] bool recovered() const noexcept {
        return status == RecoveryStatus::Recovered;
    }
};

[[nodiscard]] const char* recovery_status_name(RecoveryStatus status) noexcept;

// Closes admission, requests acknowledged owner rollback, restores the durable
// journal, retires only verified detached-owner leases, and reopens admission
// only after every lease and callback has drained.
[[nodiscard]] RecoveryResult coordinated_rollback(
    CentralMutationCoordinator& coordinator,
    BaselineStore& baseline,
    std::string_view reason);

class MutationRecoveryMonitor {
public:
    MutationRecoveryMonitor(BaselineStore& baseline, CentralMutationCoordinator& coordinator) noexcept
        : baseline_(baseline), coordinator_(coordinator) {}

    // Called by the daemon timerfd safety callback. Idle ticks do not initiate a
    // global rollback; pending emergency recovery is retried on every later tick.
    [[nodiscard]] RecoveryResult on_timer_tick(
        std::string_view reason = "Detached controller owner detected by daemon monitor");

private:
    BaselineStore& baseline_;
    CentralMutationCoordinator& coordinator_;
};

} // namespace coreflow
