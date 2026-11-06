/**
 * @file mutation_coordinator.cpp
 * @brief Thread-safe cross-controller mutation ownership and preemption dispatch.
 * @license MIT
 */
#include "coreflow/autonomous_tuner.hpp"

#include <algorithm>
#include <mutex>
#include <string>
#include <utility>

namespace coreflow {

CentralMutationCoordinator& CentralMutationCoordinator::instance() {
    static CentralMutationCoordinator coordinator;
    return coordinator;
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
        }
    }
}

bool CentralMutationCoordinator::register_preemption_handler(
    std::string_view controller_name,
    PreemptionHandler handler) {
    if (controller_name.empty() || !handler) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string key(controller_name);
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
    std::unique_lock<std::mutex> lock(mutex_);
    const std::string key(controller_name);
    auto it = preemption_handlers_.find(key);
    if (it == preemption_handlers_.end()) return;
    it->second.unregistering = true;
    preemption_cv_.wait(lock, [&] {
        const auto current = preemption_handlers_.find(key);
        return current == preemption_handlers_.end() || current->second.in_flight == 0;
    });
    preemption_handlers_.erase(key);
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

    bool acknowledged = false;
    try {
        // Invoke controller code outside the coordinator mutex. The in-flight
        // reference keeps unregister/destruction from invalidating its target.
        acknowledged = handler(domain, transaction_id, reason);
    } catch (...) {
        acknowledged = false;
    }

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
    conflict_rejections_.store(0, std::memory_order_relaxed);
}

// ============================================================================

} // namespace coreflow
