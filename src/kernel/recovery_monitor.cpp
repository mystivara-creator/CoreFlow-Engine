#include "coreflow/recovery_monitor.hpp"

#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/baseline_store.hpp"

#include <algorithm>

namespace coreflow {

const char* recovery_status_name(RecoveryStatus status) noexcept {
    switch (status) {
        case RecoveryStatus::NoRecoveryRequired: return "NO_RECOVERY_REQUIRED";
        case RecoveryStatus::Recovered: return "RECOVERED";
        case RecoveryStatus::LiveOwnerPending: return "LIVE_OWNER_PENDING";
        case RecoveryStatus::JournalRecoveryPending: return "JOURNAL_RECOVERY_PENDING";
        case RecoveryStatus::OrphanLeasePending: return "ORPHAN_LEASE_PENDING";
        case RecoveryStatus::ActiveLeasePending: return "ACTIVE_LEASE_PENDING";
        case RecoveryStatus::CallbackDrainPending: return "CALLBACK_DRAIN_PENDING";
    }
    return "UNKNOWN_RECOVERY_STATUS";
}

RecoveryResult coordinated_rollback(
    BaselineStore& baseline,
    std::string_view reason) {
    auto& coordinator = CentralMutationCoordinator::instance();

    // Close admission before taking the lease snapshot. Existing owners may
    // finish the current operation, but must acknowledge exact rollback before
    // the global journal sweep can begin.
    coordinator.begin_emergency_recovery();
    coordinator.emergency_preempt_all(reason);

    const auto remaining = coordinator.active_leases();
    const auto live_owner_pending = static_cast<size_t>(std::count_if(
        remaining.begin(), remaining.end(),
        [](const DomainLease& lease) { return !lease.owner_detached; }));
    if (live_owner_pending != 0) {
        return {RecoveryStatus::LiveOwnerPending, remaining.size(), live_owner_pending};
    }

    // Detached owners no longer have a callback. The durable journal is the
    // authority for their remaining mutations; a failed path stays journaled
    // and the recovery admission barrier remains closed for the next tick.
    if (baseline.has_active_mutations()) {
        (void)baseline.rollback_all();
    }
    if (baseline.has_active_mutations() || !baseline.verify_and_persist_clear_state()) {
        return {RecoveryStatus::JournalRecoveryPending, remaining.size(), 0};
    }

    if (!remaining.empty() &&
        !coordinator.release_orphaned_leases_after_verified_recovery(baseline)) {
        return {RecoveryStatus::OrphanLeasePending, remaining.size(), 0};
    }

    const auto after_recovery = coordinator.active_leases();
    if (!after_recovery.empty()) {
        const auto live_after = static_cast<size_t>(std::count_if(
            after_recovery.begin(), after_recovery.end(),
            [](const DomainLease& lease) { return !lease.owner_detached; }));
        return {RecoveryStatus::ActiveLeasePending, after_recovery.size(), live_after};
    }

    if (!coordinator.complete_emergency_recovery()) {
        return {RecoveryStatus::CallbackDrainPending, 0, 0};
    }
    return {RecoveryStatus::Recovered, 0, 0};
}

RecoveryResult MutationRecoveryMonitor::on_timer_tick(std::string_view reason) {
    auto& coordinator = CentralMutationCoordinator::instance();
    const auto leases = coordinator.active_leases();
    const bool detached_owner_present = std::any_of(
        leases.begin(), leases.end(),
        [](const DomainLease& lease) { return lease.active && lease.owner_detached; });
    const bool recovery_retry_pending = coordinator.is_emergency_recovery_active();
    const bool preemption_pending = std::any_of(
        leases.begin(), leases.end(),
        [](const DomainLease& lease) {
            return lease.active && lease.preemption_requested && !lease.owner_detached;
        });

    if (!detached_owner_present && !recovery_retry_pending && !preemption_pending) {
        return {RecoveryStatus::NoRecoveryRequired, leases.size(), 0};
    }

    // Priority preemption is not a global rollback. Retry only exact leases
    // already marked PREEMPTION_REQUESTED, preserving unrelated active domains.
    if (!detached_owner_present && !recovery_retry_pending && preemption_pending) {
        coordinator.dispatch_pending_preemptions(reason);
        const auto remaining = coordinator.active_leases();
        const auto live_pending = static_cast<size_t>(std::count_if(
            remaining.begin(), remaining.end(),
            [](const DomainLease& lease) {
                return lease.active && lease.preemption_requested && !lease.owner_detached;
            }));
        return {
            live_pending == 0 ? RecoveryStatus::NoRecoveryRequired : RecoveryStatus::LiveOwnerPending,
            remaining.size(),
            live_pending,
        };
    }

    return coordinated_rollback(baseline_, reason);
}

} // namespace coreflow
