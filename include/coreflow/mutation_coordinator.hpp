/**
 * @file mutation_coordinator.hpp
 * @brief Lightweight single-owner domain lease coordinator for safe kernel mutations.
 * @license Apache-2.0
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

struct TamperEvent {
    uint64_t ts_ms{0};
    MutationDomain domain{MutationDomain::CpuPolicy};
    std::string node_path;
    std::string expected_value;
    std::string observed_value;
    bool reclaimed{false};
    bool circuit_open{false};
    uint32_t attempts_in_window{0};
    std::string reason;
};

enum class TamperGuardAction : uint8_t {
    ReclaimAndLock = 0,     // Reclaim node immediately (under 3 attempts / 5s limit)
    CircuitOpenQuarantine = 1, // Circuit breaker tripped (>= 3 attempts / 5s); hold 0444 + 30s cooldown
};

class CentralMutationCoordinator {
public:
    static constexpr uint32_t kTamperMaxAttempts = 3U;
    static constexpr uint64_t kTamperWindowMs = 5000ULL;
    static constexpr uint64_t kTamperQuarantineMs = 30000ULL;
    static constexpr size_t kMaxTamperHistory = 32U;

    using PreemptionHandler = std::function<bool(
        MutationDomain, std::string_view, std::string_view)>;
    using TamperListener = std::function<void(const TamperEvent&)>;

    [[nodiscard]] static std::string make_controller_identity(std::string_view controller_type);
    [[nodiscard]] static const char* domain_name(MutationDomain domain) noexcept;

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

    // Sovereign Tamper Guard (Anti Tug-of-War):
    // Evaluates sliding 5-second circuit breaker (max 3 attempts / 5s) before Reclaim-First,
    // enters 30s quarantine on trip (zero write-loop battery drain), logs TamperEvent, and notifies listener.
    void set_tamper_listener(TamperListener listener);
    [[nodiscard]] bool is_node_quarantined(const std::string& node_path, uint64_t now_ms) const noexcept;
    [[nodiscard]] TamperGuardAction evaluate_tamper_reclaim(
        MutationDomain domain,
        const std::string& node_path,
        const std::string& expected_value,
        const std::string& observed_value,
        uint64_t now_ms,
        uint32_t* out_attempts = nullptr) noexcept;
    void finalize_tamper_event(
        MutationDomain domain,
        const std::string& node_path,
        const std::string& expected_value,
        const std::string& observed_value,
        bool reclaimed,
        bool circuit_open,
        uint32_t attempts_in_window,
        const std::string& reason,
        uint64_t epoch_ms) noexcept;
    void clear_node_quarantine(const std::string& node_path) noexcept;
    [[nodiscard]] std::vector<TamperEvent> recent_tamper_events(size_t limit = 16U) const;
    [[nodiscard]] size_t quarantined_node_count(uint64_t now_ms) const noexcept;
    [[nodiscard]] uint32_t breaker_trip_count(const std::string& node_path) const noexcept;
    [[nodiscard]] uint64_t total_tamper_events() const noexcept {
        return total_tamper_events_.load(std::memory_order_relaxed);
    }

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

    struct NodeTamperTracker {
        std::vector<uint64_t> recent_attempt_ms;
        uint64_t quarantine_until_ms{0};
        uint32_t breaker_trips{0};
    };

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
    std::unordered_map<std::string, NodeTamperTracker> tamper_trackers_;
    std::vector<TamperEvent> tamper_history_;
    TamperListener tamper_listener_{};
    bool emergency_recovery_active_{false};
    std::atomic<uint64_t> conflict_rejections_{0};
    std::atomic<uint64_t> total_tamper_events_{0};
};

} // namespace coreflow
