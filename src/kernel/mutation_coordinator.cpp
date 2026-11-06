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

CentralMutationCoordinator& CentralMutationCoordinator::instance() {
    static CentralMutationCoordinator coordinator;
    return coordinator;
}

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
        for (auto& lease : leases_) {
            if (lease.domain != domain || !lease.active) continue;
            if (lease.owner_controller == controller_name) {
                if (lease.preemption_requested || lease.transaction_id != transaction_id) {
                    conflict = true;
                    ++conflict_rejections_;
                    if (conflict_reason != nullptr) {
                        *conflict_reason = lease.preemption_requested
                            ? "Controller transaction has a pending preemption request"
                            : "Controller must release its current transaction before acquiring a new one";
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

void CentralMutationCoordinator::release_lease(
    MutationDomain domain,
    std::string_view controller_name,
    std::string_view transaction_id) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& lease : leases_) {
        if (lease.domain == domain && lease.active && lease.owner_controller == controller_name &&
            (transaction_id.empty() || lease.transaction_id == transaction_id)) {
            lease.active = false;
            lease.transaction_id.clear();
            lease.preemption_requested = false;
            lease.owner_detached = false;
        }
    }
}

void CentralMutationCoordinator::release_all_for_controller(std::string_view controller_name) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& lease : leases_) {
        if (lease.active && lease.owner_controller == controller_name) {
            lease.active = false;
            lease.transaction_id.clear();
            lease.preemption_requested = false;
            lease.owner_detached = false;
        }
    }
}

bool CentralMutationCoordinator::register_preemption_handler(
    std::string_view controller_name,
    PreemptionHandler handler) {
    if (controller_name.empty() || !handler) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string key(controller_name);
    const bool has_detached_owner_lease = std::any_of(
        leases_.begin(), leases_.end(), [&](const DomainLease& lease) {
            return lease.active && lease.owner_controller == key && lease.owner_detached;
        });
    if (has_detached_owner_lease) return false;
    const auto it = preemption_handlers_.find(key);
    if (it != preemption_handlers_.end() &&
        (it->second.unregistering || it->second.in_flight != 0)) {
        return false;
    }
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
        const bool lease_is_current = std::any_of(leases_.begin(), leases_.end(),
            [&](const DomainLease& lease) {
                return lease.domain == domain && lease.active &&
                       lease.owner_controller == controller_name &&
                       lease.transaction_id == transaction_id && lease.preemption_requested;
            });
        if (!lease_is_current) return false;
        const auto it = preemption_handlers_.find(key);
        if (it == preemption_handlers_.end() || it->second.unregistering) return false;
        ++it->second.in_flight;
        handler = it->second.handler;
    }

    // Invoke outside the coordinator mutex; the in-flight reference protects
    // handler lifetime. CFAE production builds use -fno-exceptions: every
    // registered callback is therefore required to be non-throwing and must
    // report failure through a false acknowledgement.
    const bool acknowledged = handler(domain, transaction_id, reason);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = preemption_handlers_.find(key);
        if (it != preemption_handlers_.end() && it->second.in_flight > 0) {
            --it->second.in_flight;
        }
        preemption_cv_.notify_all();
    }

    if (acknowledged) {
        // Transaction-scoped release prevents a delayed acknowledgement from
        // clearing a newer lease acquired by the same controller/domain.
        release_lease(domain, controller_name, transaction_id);
    }
    return acknowledged;
}

void CentralMutationCoordinator::begin_emergency_recovery() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    emergency_recovery_active_ = true;
}

bool CentralMutationCoordinator::complete_emergency_recovery() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    const bool leases_remaining = std::any_of(leases_.begin(), leases_.end(),
        [](const DomainLease& lease) { return lease.active; });
    if (leases_remaining) return false;
    emergency_recovery_active_ = false;
    return true;
}

bool CentralMutationCoordinator::release_orphaned_leases_after_verified_recovery(
    BaselineStore& baseline_store) noexcept {
    const auto all_active_owners_are_detached = [&]() {
        for (const auto& lease : leases_) {
            if (!lease.active) continue;
            if (!lease.owner_detached || !lease.preemption_requested) return false;
            if (preemption_handlers_.find(lease.owner_controller) != preemption_handlers_.end()) {
                return false;
            }
        }
        return true;
    };

    // Check admission and ownership before the filesystem operation, perform
    // the durable BaselineStore verification without holding the coordinator
    // mutex, then recheck because callbacks/destructors may have completed.
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!emergency_recovery_active_ || !all_active_owners_are_detached()) return false;
    }
    if (!baseline_store.verify_and_persist_clear_state()) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    if (!emergency_recovery_active_ || !all_active_owners_are_detached()) return false;
    for (auto& lease : leases_) {
        if (!lease.active || !lease.owner_detached) continue;
        lease.active = false;
        lease.transaction_id.clear();
        lease.preemption_requested = false;
        lease.owner_detached = false;
    }
    return true;
}

void CentralMutationCoordinator::emergency_preempt_all(std::string_view reason) noexcept {
    std::vector<DomainLease> pending;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& lease : leases_) {
            if (!lease.active) continue;
            // Preserve owner and transaction identity until an acknowledgement.
            lease.preemption_requested = true;
            pending.push_back(lease);
        }
    }
    for (const auto& lease : pending) {
        (void)dispatch_preemption(lease.domain, lease.owner_controller, lease.transaction_id, reason);
    }
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
