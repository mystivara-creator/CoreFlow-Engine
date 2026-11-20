/**
 * @file mutation_coordinator.hpp
 * @brief Lightweight single-owner domain lease coordinator for safe kernel mutations.
 * @license MIT
 */
#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace coreflow {

class BaselineStore;

enum class MutationDomain : uint8_t {
    CpuPolicy = 0,
    BlockIo = 1,
    VmMemory = 2,
    UclampCgroup = 3,
    GpuDevfreq = 4,
    PowerLongevity = 5,
    ThermalCore = 6,
};

enum class ControllerPriority : uint8_t {
    BackgroundEfficiency = 20,
    WorkloadOptimization = 40,
    PressureGuard = 60,
    ThermalStabilization = 80,
    EmergencySafety = 100,
};

struct DomainLease {
    MutationDomain domain{MutationDomain::CpuPolicy};
    std::string owner_controller;
    std::string transaction_id;
    ControllerPriority priority{ControllerPriority::WorkloadOptimization};
    bool active{false};
    bool preemption_requested{false};
    bool preemption_dispatch_in_flight{false};
    bool owner_detached{false};
    bool external_ownership_lost{false};
    uint32_t contention_retries{0};
};

class CentralMutationCoordinator {
public:
    using PreemptionHandler = std::function<bool(
        MutationDomain, std::string_view, std::string_view)>;

    [[nodiscard]] static std::string make_controller_identity(std::string_view controller_type);

    bool acquire_lease(MutationDomain domain,
                       std::string_view controller_name,
                       std::string_view transaction_id,
                       ControllerPriority priority,
                       std::string* conflict_reason = nullptr);

    void release_lease(
        MutationDomain domain,
        std::string_view controller_name,
        std::string_view transaction_id) noexcept;
    [[nodiscard]] bool mark_external_ownership_lost(
        MutationDomain domain, std::string_view controller_name,
        std::string_view transaction_id) noexcept;
    [[nodiscard]] bool clear_external_ownership_lost_after_verified_recovery(
        MutationDomain domain, std::string_view controller_name,
        std::string_view transaction_id) noexcept;
    [[nodiscard]] bool lease_is_live(
        MutationDomain domain, std::string_view controller_name,
        std::string_view transaction_id) const noexcept;
    [[nodiscard]] bool reserve_contention_retry(
        MutationDomain domain,
        std::string_view controller_name,
        std::string_view transaction_id,
        uint32_t max_retries,
        uint32_t* attempt_number = nullptr) noexcept;
    [[nodiscard]] bool is_preemption_requested(
        MutationDomain domain,
        std::string_view controller_name,
        std::string_view transaction_id) const noexcept;
    void dispatch_pending_preemptions(std::string_view reason) noexcept;
    bool register_preemption_handler(std::string_view controller_name, PreemptionHandler handler);
    void unregister_preemption_handler(std::string_view controller_name) noexcept;
    void emergency_preempt_all(std::string_view reason) noexcept;
    [[nodiscard]] bool release_orphaned_leases_after_verified_recovery(
        BaselineStore& baseline_store) noexcept;

    void begin_emergency_recovery() noexcept;
    [[nodiscard]] bool is_emergency_recovery_active() const noexcept;
    [[nodiscard]] bool complete_emergency_recovery() noexcept;

    [[nodiscard]] bool is_domain_locked_by_other(MutationDomain domain,
                                                 std::string_view controller_name,
                                                 ControllerPriority requester_priority) const noexcept;
    [[nodiscard]] std::vector<DomainLease> active_leases() const;
    [[nodiscard]] uint64_t conflict_rejections() const noexcept {
        return conflict_rejections_.load(std::memory_order_relaxed);
    }
    void reset() noexcept;

    CentralMutationCoordinator() = default;
    CentralMutationCoordinator(const CentralMutationCoordinator&) = delete;
    CentralMutationCoordinator& operator=(const CentralMutationCoordinator&) = delete;

private:
    bool dispatch_preemption(
        MutationDomain domain,
        std::string_view controller_name,
        std::string_view transaction_id,
        std::string_view reason) noexcept;

    mutable std::mutex mutex_;
    std::vector<DomainLease> leases_;
    struct PreemptionHandlerEntry {
        PreemptionHandler handler;
        size_t in_flight{0};
        bool callback_dispatch_in_flight{false};
        bool unregistering{false};
    };
    std::condition_variable preemption_cv_;
    std::unordered_map<std::string, PreemptionHandlerEntry> preemption_handlers_;
    std::unordered_set<std::string> retired_controller_ids_;
    bool emergency_recovery_active_{false};
    std::atomic<uint64_t> conflict_rejections_{0};
};

} // namespace coreflow
