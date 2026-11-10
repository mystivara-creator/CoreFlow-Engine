/**
 * @file mutation_coordinator.cpp
 * @brief Thread-safe cross-controller mutation ownership and preemption dispatch.
 * @license MIT
 */
#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/baseline_store.hpp"

#include <algorithm>
#include <mutex>
#include <string>
#include <utility>

namespace coreflow {

std::string CentralMutationCoordinator::make_controller_identity(
    std::string_view controller_type) {
    if (controller_type.empty()) return {};
    static std::atomic<uint64_t> next_identity{1};
    const uint64_t identity = next_identity.fetch_add(1, std::memory_order_relaxed);
    return std::string(controller_type) + "@" + std::to_string(identity);
}

bool CentralMutationCoordinator::acquire_lease(
    MutationDomain domain,
    std::string_view controller_name,
    std::string_view transaction_id,
    ControllerPriority priority,
    std::string* conflict_reason) {
    if (controller_name.empty() || transaction_id.empty()) {
        conflict_rejections_.fetch_add(1, std::memory_order_relaxed);
        if (conflict_reason != nullptr) {
            *conflict_reason = "Controller identity and transaction ID must be non-empty";
        }
        return false;
    }

    bool conflict = false;
    bool dispatch = false;
    std::string pending_owner;
    std::string pending_transaction;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (emergency_recovery_active_) {
            ++conflict_rejections_;
            if (conflict_reason != nullptr) {
                *conflict_reason = "Mutation admission is closed during emergency recovery";
            }
            return false;
        }
        if (retired_controller_ids_.find(std::string(controller_name)) != retired_controller_ids_.end()) {
            ++conflict_rejections_;
            if (conflict_reason != nullptr) *conflict_reason = "Controller identity has been retired";
            return false;
        }
        for (auto& lease : leases_) {
            if (lease.domain != domain || !lease.active) continue;
            if (lease.owner_controller == controller_name) {
                if (lease.preemption_requested || lease.external_ownership_lost || lease.transaction_id != transaction_id) {
                    conflict = true;
                    ++conflict_rejections_;
                    if (conflict_reason != nullptr) {
                        *conflict_reason = lease.external_ownership_lost
                            ? "External writer changed a leased node; transaction is quarantined"
                            : (lease.preemption_requested
                                ? "Controller transaction has a pending preemption request"
                                : "Controller must release its current transaction before acquiring a new one");
                    }
                    break;
                }
                lease.priority = priority;
                return true;
            }
            conflict = true;
            if (static_cast<uint8_t>(priority) > static_cast<uint8_t>(lease.priority)) {
                // A higher-priority request is delivered to the current owner after
                // releasing the coordinator lock. Ownership is never transferred here.
                lease.preemption_requested = true;
                pending_owner = lease.owner_controller;
                pending_transaction = lease.transaction_id;
                dispatch = true;
            }
            ++conflict_rejections_;
            if (conflict_reason != nullptr) {
                *conflict_reason = "Domain remains owned by controller '" +
                                   lease.owner_controller + "' (tx: " + lease.transaction_id +
                                   "); owner resolution is required before acquisition";
            }
            break;
        }
        if (!conflict) {
            const auto reusable = std::find_if(leases_.begin(), leases_.end(), [domain](const DomainLease& lease) {
                return lease.domain == domain && !lease.active;
            });
            const DomainLease next{
                .domain = domain,
                .owner_controller = std::string(controller_name),
                .transaction_id = std::string(transaction_id),
                .priority = priority,
                .active = true,
                .preemption_requested = false,
            };
            if (reusable != leases_.end()) {
                *reusable = next;
            } else {
                leases_.push_back(next);
            }
            return true;
        }
    }
    if (dispatch) {
        (void)dispatch_preemption(
            domain, pending_owner, pending_transaction, "higher-priority domain request");
    }
    return false;
}

bool CentralMutationCoordinator::reserve_contention_retry(
    MutationDomain domain,
    std::string_view controller_name,
    std::string_view transaction_id,
    uint32_t max_retries,
    uint32_t* attempt_number) noexcept {
    constexpr uint32_t kHardRetryLimit = 3;
    if (attempt_number != nullptr) *attempt_number = 0;
    if (controller_name.empty() || transaction_id.empty() || max_retries == 0) return false;
    const uint32_t retry_limit = std::min(max_retries, kHardRetryLimit);

    std::lock_guard<std::mutex> lock(mutex_);
    if (emergency_recovery_active_) return false;
    const auto it = std::find_if(leases_.begin(), leases_.end(),
        [&](const DomainLease& lease) {
            return lease.domain == domain && lease.active &&
                   lease.owner_controller == controller_name &&
                   lease.transaction_id == transaction_id;
        });
    if (it == leases_.end() || it->preemption_requested || it->external_ownership_lost ||
        it->preemption_dispatch_in_flight || it->owner_detached ||
        it->contention_retries >= retry_limit) return false;
    ++it->contention_retries;
    if (attempt_number != nullptr) *attempt_number = it->contention_retries;
    return true;
}

void CentralMutationCoordinator::release_lease(
    MutationDomain domain,
    std::string_view controller_name,
    std::string_view transaction_id) noexcept {
    if (controller_name.empty() || transaction_id.empty()) return;
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& lease : leases_) {
        if (lease.domain == domain && lease.active && lease.owner_controller == controller_name &&
            lease.transaction_id == transaction_id) {
            if (lease.external_ownership_lost) continue;
            lease.active = false;
            lease.transaction_id.clear();
            lease.preemption_requested = false;
            lease.preemption_dispatch_in_flight = false;
            lease.owner_detached = false;
        }
    }
}

bool CentralMutationCoordinator::mark_external_ownership_lost(
    MutationDomain domain, std::string_view controller_name,
    std::string_view transaction_id) noexcept {
    if (controller_name.empty() || transaction_id.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& lease : leases_) {
        if (lease.domain == domain && lease.active &&
            lease.owner_controller == controller_name && lease.transaction_id == transaction_id) {
            lease.external_ownership_lost = true;
            return true;
        }
    }
    return false;
}

bool CentralMutationCoordinator::clear_external_ownership_lost_after_verified_recovery(
    MutationDomain domain, std::string_view controller_name,
    std::string_view transaction_id) noexcept {
    if (controller_name.empty() || transaction_id.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& lease : leases_) {
        if (lease.domain == domain && lease.active &&
            lease.owner_controller == controller_name && lease.transaction_id == transaction_id) {
            lease.external_ownership_lost = false;
            return true;
        }
    }
    return false;
}

bool CentralMutationCoordinator::lease_is_live(
    MutationDomain domain, std::string_view controller_name,
    std::string_view transaction_id) const noexcept {
    if (controller_name.empty() || transaction_id.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    return std::any_of(leases_.begin(), leases_.end(), [&](const DomainLease& lease) {
        return lease.domain == domain && lease.active && !lease.preemption_requested &&
               !lease.external_ownership_lost && lease.owner_controller == controller_name &&
               lease.transaction_id == transaction_id;
    });
}

bool CentralMutationCoordinator::is_preemption_requested(
    MutationDomain domain,
    std::string_view controller_name,
    std::string_view transaction_id) const noexcept {
    if (controller_name.empty() || transaction_id.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    return std::any_of(leases_.begin(), leases_.end(), [&](const DomainLease& lease) {
        return lease.domain == domain && lease.active &&
               lease.owner_controller == controller_name &&
               lease.transaction_id == transaction_id &&
               lease.preemption_requested;
    });
}

void CentralMutationCoordinator::dispatch_pending_preemptions(std::string_view reason) noexcept {
    std::vector<DomainLease> pending;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& lease : leases_) {
            if (lease.active && lease.preemption_requested &&
                !lease.preemption_dispatch_in_flight) {
                pending.push_back(lease);
            }
        }
    }
    // dispatch_preemption serializes callbacks for each controller identity.
    // A failed/busy callback remains pending for a later recovery tick.
    for (const auto& lease : pending) {
        (void)dispatch_preemption(
            lease.domain, lease.owner_controller, lease.transaction_id, reason);
    }
}

bool CentralMutationCoordinator::register_preemption_handler(
    std::string_view controller_name,
    PreemptionHandler handler) {
    if (controller_name.empty() || !handler) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string key(controller_name);
    if (retired_controller_ids_.find(key) != retired_controller_ids_.end()) return false;
    const bool has_detached_owner_lease = std::any_of(
        leases_.begin(), leases_.end(), [&](const DomainLease& lease) {
            return lease.active && lease.owner_controller == key && lease.owner_detached;
        });
    if (has_detached_owner_lease) return false;
    const auto it = preemption_handlers_.find(key);
    // A controller identity represents one controller lifetime and exactly
    // one callback registration. Never silently replace a live owner's handler.
    if (it != preemption_handlers_.end()) return false;
    preemption_handlers_[key] = PreemptionHandlerEntry{
        .handler = std::move(handler),
        .in_flight = 0,
        .unregistering = false,
    };
    return true;
}

void CentralMutationCoordinator::unregister_preemption_handler(
    std::string_view controller_name) noexcept {
    if (controller_name.empty()) return;
    std::unique_lock<std::mutex> lock(mutex_);
    const std::string key(controller_name);
    // Tombstone before waiting: no concurrent caller may reacquire or register
    // this identity while its last callback is still draining.
    retired_controller_ids_.insert(key);
    auto it = preemption_handlers_.find(key);
    if (it != preemption_handlers_.end()) {
        it->second.unregistering = true;
        preemption_cv_.wait(lock, [&] {
            const auto current = preemption_handlers_.find(key);
            return current == preemption_handlers_.end() || current->second.in_flight == 0;
        });
        preemption_handlers_.erase(key);
    }

    // A controller destructor calls this only after its own best-effort
    // rollback. Any lease still active now has no callback capable of
    // acknowledging it. Preserve ownership as an explicit orphan; never
    // release it here because BaselineStore may still contain live mutations.
    for (auto& lease : leases_) {
        if (lease.active && lease.owner_controller == key) {
            lease.owner_detached = true;
        }
    }
}

bool CentralMutationCoordinator::dispatch_preemption(
    MutationDomain domain,
    std::string_view controller_name,
    std::string_view transaction_id,
    std::string_view reason) noexcept {
    PreemptionHandler handler;
    const std::string key(controller_name);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto lease_it = std::find_if(leases_.begin(), leases_.end(),
            [&](const DomainLease& lease) {
                return lease.domain == domain && lease.active &&
                       lease.owner_controller == controller_name &&
                       lease.transaction_id == transaction_id && lease.preemption_requested;
            });
        if (lease_it == leases_.end() || lease_it->preemption_dispatch_in_flight) {
            // Another dispatcher already owns the callback for this exact
            // lease. It will either acknowledge or clear the in-flight flag.
            return false;
        }
        const auto handler_it = preemption_handlers_.find(key);
        if (handler_it == preemption_handlers_.end() || handler_it->second.unregistering ||
            handler_it->second.callback_dispatch_in_flight) {
            // A controller has one callback object even when it owns several
            // domains. Serialize that object across all of its transactions.
            return false;
        }
        lease_it->preemption_dispatch_in_flight = true;
        handler_it->second.callback_dispatch_in_flight = true;
        ++handler_it->second.in_flight;
        handler = handler_it->second.handler;
    }

    // Invoke outside the coordinator mutex; the in-flight reference protects
    // handler lifetime. CFAE production builds use -fno-exceptions: every
    // registered callback is therefore required to be non-throwing and must
    // report failure through a false acknowledgement.
    const bool acknowledged = handler(domain, transaction_id, reason);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto handler_it = preemption_handlers_.find(key);
        if (handler_it != preemption_handlers_.end()) {
            if (handler_it->second.in_flight > 0) --handler_it->second.in_flight;
            handler_it->second.callback_dispatch_in_flight = false;
        }

        // Resolve only the exact transaction observed before callback dispatch.
        // On a failed acknowledgement, clear the dispatch marker so a later
        // monitor/emergency pass can retry. On success, release atomically under
        // this same lock so a competing dispatcher cannot enter between ack and
        // lease release.
        for (auto& lease : leases_) {
            if (lease.domain != domain || !lease.active ||
                lease.owner_controller != controller_name ||
                lease.transaction_id != transaction_id) {
                continue;
            }
            if (acknowledged && !lease.external_ownership_lost) {
                lease.active = false;
                lease.transaction_id.clear();
                lease.preemption_requested = false;
                lease.preemption_dispatch_in_flight = false;
                lease.owner_detached = false;
            } else {
                lease.preemption_dispatch_in_flight = false;
            }
            break;
        }
        preemption_cv_.notify_all();
    }
    return acknowledged;
}

void CentralMutationCoordinator::begin_emergency_recovery() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    emergency_recovery_active_ = true;
}

bool CentralMutationCoordinator::is_emergency_recovery_active() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return emergency_recovery_active_;
}

bool CentralMutationCoordinator::complete_emergency_recovery() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    const bool leases_remaining = std::any_of(leases_.begin(), leases_.end(),
        [](const DomainLease& lease) { return lease.active; });
    const bool callbacks_in_flight = std::any_of(
        preemption_handlers_.begin(), preemption_handlers_.end(),
        [](const auto& entry) {
            return entry.second.in_flight != 0 || entry.second.callback_dispatch_in_flight;
        });
    if (leases_remaining || callbacks_in_flight) return false;
    emergency_recovery_active_ = false;
    return true;
}

bool CentralMutationCoordinator::release_orphaned_leases_after_verified_recovery(
    BaselineStore& baseline_store) noexcept {
    const auto all_active_owners_are_detached = [&]() {
        for (const auto& lease : leases_) {
            if (!lease.active) continue;
            if (!lease.owner_detached || !lease.preemption_requested ||
                lease.preemption_dispatch_in_flight) return false;
            if (preemption_handlers_.find(lease.owner_controller) != preemption_handlers_.end()) {
                return false;
            }
        }
        return true;
    };
    const auto all_callbacks_are_drained = [&]() {
        return std::all_of(preemption_handlers_.begin(), preemption_handlers_.end(),
            [](const auto& entry) {
                return entry.second.in_flight == 0 &&
                       !entry.second.callback_dispatch_in_flight;
            });
    };

    // Check admission and ownership before the filesystem operation, perform
    // the durable BaselineStore verification without holding the coordinator
    // mutex, then recheck because callbacks/destructors may have completed.
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!emergency_recovery_active_ || !all_active_owners_are_detached() ||
            !all_callbacks_are_drained()) return false;
    }
    if (!baseline_store.verify_and_persist_clear_state()) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    if (!emergency_recovery_active_ || !all_active_owners_are_detached() ||
        !all_callbacks_are_drained()) return false;
    for (auto& lease : leases_) {
        if (!lease.active || !lease.owner_detached) continue;
        lease.active = false;
        lease.transaction_id.clear();
        lease.preemption_requested = false;
        lease.preemption_dispatch_in_flight = false;
        lease.owner_detached = false;
    }
    return true;
}

void CentralMutationCoordinator::emergency_preempt_all(std::string_view reason) noexcept {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& lease : leases_) {
            if (!lease.active) continue;
            lease.preemption_requested = true;
        }
    }
    dispatch_pending_preemptions(reason);
}

bool CentralMutationCoordinator::is_domain_locked_by_other(
    MutationDomain domain,
    std::string_view controller_name,
    ControllerPriority requester_priority) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& lease : leases_) {
        if (lease.domain == domain && lease.active && lease.owner_controller != controller_name) {
            if (static_cast<uint8_t>(requester_priority) <= static_cast<uint8_t>(lease.priority)) {
                return true;
            }
        }
    }
    return false;
}

std::vector<DomainLease> CentralMutationCoordinator::active_leases() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<DomainLease> out;
    for (const auto& lease : leases_) {
        if (lease.active) out.push_back(lease);
    }
    return out;
}

void CentralMutationCoordinator::reset() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    leases_.clear();
    emergency_recovery_active_ = false;
    conflict_rejections_.store(0, std::memory_order_relaxed);
}

// ============================================================================

} // namespace coreflow
