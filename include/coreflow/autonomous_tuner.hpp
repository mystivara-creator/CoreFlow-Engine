/**
 * @file autonomous_tuner.hpp
 * @brief Self-Directed Multi-Domain Transaction Planner, Central Mutation Coordinator & Empirical Outcome Evaluator
 * @license MIT
 *
 * Extends CFAE beyond UClamp to autonomously discover, classify, plan, coordinate, apply, and
 * empirically evaluate kernel transactions across:
 *   1. Block I/O Schedulers (mq-deadline, kyber, bfq, none) & iosched/<tunable> queue tunables
 *   2. VM / Memory Subsystem (/proc/sys/vm/page-cluster, vfs_cache_pressure, watermark_scale_factor)
 *   3. CPUFreq Governor & rate-limit tunables (schedutil/walt up_rate_limit_us / down_rate_limit_us)
 *   4. Active Thermal Stabilization & Hysteresis Recovery (damped governor & I/O burst pacing)
 */
#pragma once
#include "coreflow/device_profile.hpp"
#include "coreflow/tiered_policy.hpp"

#include <cstdint>
#include <array>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace coreflow {

class BaselineStore;
class TransactionalFrequencyActuator;

enum class TunerVerdict : uint8_t {
    Pending = 0,
    Hold = 1,
    Improved = 2,
    Neutral = 3,
    Regressed = 4,
    Aborted = 5,
    OwnershipLost = 6,
    Inconclusive = 7,
    RecoveryRequired = 8,
    Rejected = 9,
};

enum class StepRelation : uint8_t {
    RequiredAnchor = 0,
    RequiredDependent = 1,
    OptionalIndependent = 2,
};

enum class ResourcePermissionClass : uint8_t {
    ReadOnly = 0,
    Writable = 1,
    WritableButUnsafe = 2,
    UnknownSemantic = 3,
    VerifiedConstraints = 4,
    IncompleteMetadata = 5,
};

enum class WorkloadClass : uint8_t {
    Idle = 0,
    GeneralInteraction = 1,
    ScrollingAndAnimation = 2,
    Gaming = 3,
    HeavyMultitasking = 4,
    CpuBound = 5,
    GpuBound = 6,
    MemoryPressure = 7,
    StorageIoPressure = 8,
    ThermalConstrained = 9,
    BackgroundWorkload = 10,
};

enum class BottleneckKind : uint8_t {
    None = 0,
    CpuSchedulingLatency = 1,
    IoQueueContention = 2,
    ConcurrentCpuIoContention = 3,
    MemorySwapThrashing = 4,
    ThermalHeadroomDepletion = 5,
    ThermalEmergency = 6,
};

enum class OptimizationObjective : uint8_t {
    HoldBaseline = 0,
    InteractiveLatency = 1,
    ThroughputBalance = 2,
    MemoryRelief = 3,
    IoLatencyRelief = 4,
    ThermalStabilization = 5,
    ThermalRecovery = 6,
    EmergencyMitigation = 7,
};

enum class MutationDomain : uint8_t {
    CpuPolicy = 0, BlockIo = 1, VmMemory = 2, UclampCgroup = 3,
    GpuDevfreq = 4, PowerLongevity = 5, ThermalCore = 6,
};
enum class EvaluationDomain : uint8_t { Io = 0, Memory = 1, Cpu = 2, Thermal = 3, Unavailable = 255 };

enum class ControllerPriority : uint8_t {
    BackgroundEfficiency = 20,
    WorkloadOptimization = 40,
    PressureGuard = 60,
    ThermalStabilization = 80,
    EmergencySafety = 100,
};

struct DiscoveredResourceMetadata {
    std::string path;
    MutationDomain domain{MutationDomain::BlockIo};
    ResourcePermissionClass permission_class{ResourcePermissionClass::UnknownSemantic};
    std::string unit;
    std::string range_provenance;
    std::string baseline_value;
    std::string reason;
};

struct DomainLease {
    MutationDomain domain{MutationDomain::CpuPolicy};
    std::string owner_controller;
    std::string transaction_id;
    ControllerPriority priority{ControllerPriority::WorkloadOptimization};
    bool active{false};
    // A higher-priority requester or emergency may request preemption, but the
    // current owner retains the lease until it has cancelled and verified rollback.
    bool preemption_requested{false};
    // Prevent duplicate concurrent rollback callbacks for the same lease.
    // Cleared after a failed acknowledgement so the daemon can retry.
    bool preemption_dispatch_in_flight{false};
    // Set only after unregister waits for all in-flight callbacks and the
    // controller can no longer acknowledge rollback. The lease remains active
    // until a verified global recovery explicitly retires it.
    bool owner_detached{false};
    uint32_t contention_retries{0};
};

class CentralMutationCoordinator {
public:
    using PreemptionHandler = std::function<bool(
        MutationDomain, std::string_view, std::string_view)>;

    // Process-lifetime unique identity; unlike object addresses, IDs are not reused
    // when an allocator recycles storage after controller destruction.
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
    // Retire detached-owner leases only after BaselineStore has independently
    // verified that no journaled mutations remain. Requires closed admission.
    [[nodiscard]] bool release_orphaned_leases_after_verified_recovery(
        BaselineStore& baseline_store) noexcept;

    // Close mutation admission before dispatching emergency rollback callbacks.
    // Admission reopens only after all leases and BaselineStore mutations are cleared.
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

public:
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
    // Controller identities are single-lifetime tokens and remain retired
    // after unregister, even when their final lease was already released.
    std::unordered_set<std::string> retired_controller_ids_;
    bool emergency_recovery_active_{false};
    std::atomic<uint64_t> conflict_rejections_{0};
};

struct BlockQueueSurface {
    std::string device_name;            // e.g. "sda", "mmcblk0"
    std::string queue_dir;              // e.g. "/sys/block/sda/queue"
    std::string baseline_scheduler;     // e.g. "mq-deadline" or "none"
    std::string active_scheduler;       // current active token
    std::vector<std::string> available_schedulers;
    uint64_t baseline_read_ahead_kb{128};
    uint64_t active_read_ahead_kb{128};
    uint64_t read_ahead_kb{128};
    uint64_t baseline_nr_requests{128};
    uint64_t active_nr_requests{128};
    uint64_t nr_requests{128};
    uint64_t rq_affinity{1};
    uint64_t read_expire_ms{500};
    uint64_t writes_starved{2};
    uint64_t fifo_batch{16};
    bool has_iosched_tunables{false};
};

struct CandidateStats {
    std::string id;
    std::string domain;                 // "io", "vm", "cpufreq", "thermal"
    std::string label;
    double mean_utility{0.5};           // Empirical utility score Q_k
    double mean_utility_delta{0.0};     // Empirical mean ΔU
    double ucb_score{0.65};             // Upper Confidence Bound (UCB1) score
    double adaptive_scale{1.0};         // Step-sizing factor [0.25..1.0] reduced on rollback
    uint32_t settling_ticks{1};         // Domain-specific post-mutation settling ticks
    // Attempts are not empirical rewards.
    uint32_t trials{0};
    uint32_t reward_samples{0};
    uint32_t improved_count{0};
    uint32_t successes{0};
    uint32_t regressed_count{0};
    uint32_t rollbacks{0};
    uint32_t inconclusive_count{0};
    uint32_t neutral_count{0};
    uint32_t aborted_count{0};
    uint32_t preempted_count{0};
    uint32_t recovery_required_count{0};
    uint32_t rejected_count{0};
    uint32_t cooldown_ticks{0};
    uint32_t consecutive_non_improved{0};
    // Ownership-loss quarantine is persisted and cannot be bypassed by context changes.
    uint32_t ownership_loss_count{0};
    uint32_t ownership_quarantine_ticks{0};
    WorkloadClass last_workload{WorkloadClass::Idle};
    uint8_t last_thermal_bucket{0};
    std::string last_before_summary;
    std::string last_after_summary;
    std::string last_rollback_reason;
};

struct AutonomousDecisionSnapshot {
    std::string stage{"RETAINED"};      // PLANNED, SETTLING, OBSERVING, RETAINED, ROLLED_BACK, RECOVERY_REQUIRED
    std::string active_intent{"HoldEquilibrium"};
    std::string intent{"HoldEquilibrium"};
    WorkloadClass workload_class{WorkloadClass::Idle};
    std::string workload_name{"Idle"};
    BottleneckKind bottleneck{BottleneckKind::None};
    std::string bottleneck_name{"None"};
    OptimizationObjective objective{OptimizationObjective::HoldBaseline};
    std::string objective_name{"HoldBaseline"};
    std::string capability_fingerprint;
    std::string active_candidate_id{"baseline"};
    std::string active_candidate_label{"OEM Verified Baseline"};
    std::string decision_rationale{"System in low-pressure equilibrium; no mutation required."};
    std::string rationale{"System in low-pressure equilibrium; no mutation required."};
    std::string last_verdict{"IMPROVED"}; // IMPROVED, NEUTRAL, REGRESSED, INCONCLUSIVE, OWNERSHIP_LOST, HOLD, ABORTED, RECOVERY_REQUIRED, REJECTED
    TunerVerdict verdict{TunerVerdict::Improved};
    double last_utility_delta{0.0};
    double utility_delta{0.0};
    double noise_floor{0.05};
    double before_io_psi{0.0};
    double after_io_psi{0.0};
    double before_mem_psi{0.0};
    double after_mem_psi{0.0};
    double before_cpu_psi{0.0};
    double after_cpu_psi{0.0};
    uint32_t settling_ticks_remaining{0};
    uint32_t observation_tick{0};
    uint32_t observation_window{3};
    uint32_t total_trials{0};
    uint32_t total_improved{0};
    uint32_t improved_count{0};
    uint32_t total_rolled_back{0};
    uint32_t rolled_back_count{0};
    uint32_t total_inconclusive{0};
    uint32_t total_rejected{0};
    uint32_t ownership_contention_retries{0};
    bool recovery_required{false};
    bool thermal_stabilization_active{false};
    bool thermal_recovery_active{false};
    double current_temperature_celsius{-1.0};
    double temperature_velocity_celsius_per_sec{0.0};
    bool temperature_velocity_valid{false};
    double thermal_pressure_velocity{0.0};
    std::vector<BlockQueueSurface> block_queues;
    std::vector<CandidateStats> candidates;
    std::vector<DiscoveredResourceMetadata> discovered_resources;
    std::string vm_page_cluster;
    std::string vm_vfs_cache_pressure;
};

struct TunerTelemetryInput {
    double cpu_psi{0.0};
    double memory_psi{0.0};
    double io_psi{0.0};
    // Missing PSI is not a measured zero. Defaults preserve legacy test/caller semantics;
    // production telemetry must set these from the corresponding read result.
    bool cpu_psi_valid{true};
    bool memory_psi_valid{true};
    bool io_psi_valid{true};
    bool thermal_pressure_valid{true};

    // Explicitly Separated Thermal Telemetry Domains:
    // 1. Thermal Pressure (dimensionless policy-relative index [0.0 .. 130.0])
    double thermal_pressure{45.0};

    // 2. Thermal Pressure Velocity (rate of change of pressure index points per second, [/s])
    double thermal_pressure_velocity{0.0};

    // 3. Physical Temperature Readings & Velocity in °C/s
    double current_temperature_celsius{-1.0};           // Current physical temperature in °C (< 0 if unavailable)
    double temperature_velocity_celsius_per_sec{0.0};   // Physical temperature velocity in °C/s (negative = cooling)
    bool temperature_velocity_valid{false};             // True only when backed by valid physical sensors across sufficient window
    uint32_t temperature_samples_count{0};              // Number of valid physical sensor samples in the window
    double temperature_sample_duration_sec{0.0};        // Duration in seconds over which temperature samples were collected

    bool thermal_guard_active{false};
    bool thermal_emergency{false};
    double thermal_headroom{-1.0};                      // <0 if unavailable, [0.0..1.0+] when valid

    // Telemetry trend alias (for backwards compatibility)
    double thermal_trend_celsius{0.0};

    // Independent workload classifier evidence. Defaults are deliberately
    // invalid so callers that do not provide fresh, stable windows cannot
    // trigger an autonomous frequency transaction.
    PolicyEvidence frequency_policy_evidence{};
    bool frequency_policy_evidence_valid{false};
};

class AutonomousTuner {
public:
    explicit AutonomousTuner(CentralMutationCoordinator& coordinator,
                             std::string sys_root = "/sys",
                             std::string proc_root = "/proc",
                             DeviceProfileHandle profile = {});
    ~AutonomousTuner();
    AutonomousTuner(const AutonomousTuner&) = delete;
    AutonomousTuner& operator=(const AutonomousTuner&) = delete;

    void discover(std::string_view sys_root = "/sys", std::string_view proc_root = "/proc");
    // Pins one immutable CDDP generation; CPU/GPU admission then comes only from this profile.
    void set_device_profile(DeviceProfileHandle profile);
    void configure_frequency_actuator(
        TransactionalFrequencyActuator* actuator,
        const TieredPolicyRouter* router,
        const PlatformContextSnapshot& platform) noexcept;
    [[nodiscard]] bool frequency_mutation_active() const noexcept;
    [[nodiscard]] bool frequency_actuator_configured() const noexcept;

    // Advances the autonomous reasoning, transaction coordinator, and before/after outcome evaluator.
    AutonomousDecisionSnapshot step(const TunerTelemetryInput& telemetry);

    // Forces an immediate, dependency-ordered rollback of any active tuner transaction.
    // Returns false and enters RECOVERY_REQUIRED if any node fails to restore.
    bool rollback_active(std::string_view reason = "Operator or thermal rollback");

    // Requests global emergency preemption, then rolls back this tuner's transaction.
    // Other owners retain their leases until they complete their own verified rollback.
    bool emergency_rollback(std::string_view reason = "Emergency rollback");

    // Resets all active mutations, bandit statistics, and observation state.
    void reset();

    [[nodiscard]] bool is_candidate_eligible(std::string_view id) const noexcept;
    [[nodiscard]] bool is_candidate_eligible_for_context(std::string_view id,
                                                         WorkloadClass workload,
                                                         uint8_t thermal_bucket) const noexcept;

    [[nodiscard]] AutonomousDecisionSnapshot snapshot() const;

    [[nodiscard]] static WorkloadClass classify_workload(const TunerTelemetryInput& telemetry) noexcept;
    [[nodiscard]] static BottleneckKind diagnose_bottleneck(const TunerTelemetryInput& telemetry,
                                                            WorkloadClass workload) noexcept;
    [[nodiscard]] static OptimizationObjective select_objective(const TunerTelemetryInput& telemetry,
                                                                WorkloadClass workload,
                                                                BottleneckKind bottleneck,
                                                                bool in_thermal_recovery) noexcept;
    [[nodiscard]] static uint8_t compute_thermal_bucket(const TunerTelemetryInput& telemetry) noexcept;

    static constexpr std::string_view stage_name(std::string_view stage) noexcept {
        return stage.empty() ? "PLANNED" : stage;
    }

    static constexpr std::string_view intent_name(std::string_view intent) noexcept {
        return intent.empty() ? "HoldEquilibrium" : intent;
    }

    static constexpr std::string_view verdict_name(std::string_view verdict) noexcept {
        return verdict.empty() ? "HOLD" : verdict;
    }

    static constexpr std::string_view verdict_name(TunerVerdict verdict) noexcept {
        switch (verdict) {
            case TunerVerdict::Pending: return "PENDING";
            case TunerVerdict::Hold: return "HOLD";
            case TunerVerdict::Improved: return "IMPROVED";
            case TunerVerdict::Neutral: return "NEUTRAL";
            case TunerVerdict::Regressed: return "REGRESSED";
            case TunerVerdict::Aborted: return "ABORTED";
            case TunerVerdict::OwnershipLost: return "OWNERSHIP_LOST";
            case TunerVerdict::Inconclusive: return "INCONCLUSIVE";
            case TunerVerdict::RecoveryRequired: return "RECOVERY_REQUIRED";
            case TunerVerdict::Rejected: return "REJECTED";
        }
        return "HOLD";
    }

    static constexpr std::string_view permission_class_name(ResourcePermissionClass pc) noexcept {
        switch (pc) {
            case ResourcePermissionClass::ReadOnly: return "ReadOnly";
            case ResourcePermissionClass::Writable: return "Writable";
            case ResourcePermissionClass::WritableButUnsafe: return "WritableButUnsafe";
            case ResourcePermissionClass::UnknownSemantic: return "UnknownSemantic";
            case ResourcePermissionClass::VerifiedConstraints: return "VerifiedConstraints";
            case ResourcePermissionClass::IncompleteMetadata: return "IncompleteMetadata";
        }
        return "UnknownSemantic";
    }

    static constexpr std::string_view workload_class_name(WorkloadClass wc) noexcept {
        switch (wc) {
            case WorkloadClass::Idle: return "Idle";
            case WorkloadClass::GeneralInteraction: return "GeneralInteraction";
            case WorkloadClass::ScrollingAndAnimation: return "ScrollingAndAnimation";
            case WorkloadClass::Gaming: return "Gaming";
            case WorkloadClass::HeavyMultitasking: return "HeavyMultitasking";
            case WorkloadClass::CpuBound: return "CpuBound";
            case WorkloadClass::GpuBound: return "GpuBound";
            case WorkloadClass::MemoryPressure: return "MemoryPressure";
            case WorkloadClass::StorageIoPressure: return "StorageIoPressure";
            case WorkloadClass::ThermalConstrained: return "ThermalConstrained";
            case WorkloadClass::BackgroundWorkload: return "BackgroundWorkload";
        }
        return "Idle";
    }

    static constexpr std::string_view bottleneck_kind_name(BottleneckKind bk) noexcept {
        switch (bk) {
            case BottleneckKind::None: return "None";
            case BottleneckKind::CpuSchedulingLatency: return "CpuSchedulingLatency";
            case BottleneckKind::IoQueueContention: return "IoQueueContention";
            case BottleneckKind::ConcurrentCpuIoContention: return "ConcurrentCpuIoContention";
            case BottleneckKind::MemorySwapThrashing: return "MemorySwapThrashing";
            case BottleneckKind::ThermalHeadroomDepletion: return "ThermalHeadroomDepletion";
            case BottleneckKind::ThermalEmergency: return "ThermalEmergency";
        }
        return "None";
    }

    static constexpr std::string_view objective_name(OptimizationObjective obj) noexcept {
        switch (obj) {
            case OptimizationObjective::HoldBaseline: return "HoldBaseline";
            case OptimizationObjective::InteractiveLatency: return "InteractiveLatency";
            case OptimizationObjective::ThroughputBalance: return "ThroughputBalance";
            case OptimizationObjective::MemoryRelief: return "MemoryRelief";
            case OptimizationObjective::IoLatencyRelief: return "IoLatencyRelief";
            case OptimizationObjective::ThermalStabilization: return "ThermalStabilization";
            case OptimizationObjective::ThermalRecovery: return "ThermalRecovery";
            case OptimizationObjective::EmergencyMitigation: return "EmergencyMitigation";
        }
        return "HoldBaseline";
    }

private:
    CentralMutationCoordinator& coordinator_;
    struct MutationStep {
        std::string path;
        std::string target_value;
        std::string relationship; // "Anchor (Required-before)" or "Coupled-with"
        StepRelation relation{StepRelation::RequiredDependent};
    };

    mutable std::recursive_mutex tuner_mutex_;
    std::string controller_name_;
    std::string sys_root_;
    std::string proc_root_;
    DeviceProfileHandle device_profile_;
    TransactionalFrequencyActuator* frequency_actuator_{nullptr};
    const TieredPolicyRouter* tiered_policy_router_{nullptr};
    PlatformContextSnapshot platform_context_{};
    bool frequency_mutation_active_{false};
    uint32_t frequency_policy_index_{0};
    PolicyDecision frequency_policy_decision_{};
    std::vector<BlockQueueSurface> block_queues_;
    std::vector<std::string> cpufreq_governor_dirs_;
    std::vector<CandidateStats> candidates_;
    std::vector<DiscoveredResourceMetadata> discovered_resources_;
    bool zram_active_{false};

    bool transaction_active_{false};
    bool observing_{false};
    bool recovery_required_{false};
    uint32_t contention_backoff_ticks_{0};
    uint32_t total_contention_retries_{0};
    bool thermal_stabilization_active_{false};
    uint32_t thermal_recovery_ticks_{0};
    double prev_thermal_pressure_{45.0};
    bool has_prev_thermal_{false};
    double noise_floor_{0.05};
    std::array<double, 4> evaluation_noise_floors_{{0.05, 0.05, 0.05, 0.05}};
    EvaluationDomain current_evaluation_domain_{EvaluationDomain::Unavailable};
    uint32_t settling_ticks_remaining_{0};

    WorkloadClass current_workload_{WorkloadClass::Idle};
    BottleneckKind current_bottleneck_{BottleneckKind::None};
    OptimizationObjective current_objective_{OptimizationObjective::HoldBaseline};
    std::string current_candidate_id_{"baseline"};
    std::string current_candidate_label_{"OEM Verified Baseline"};
    std::string current_intent_{"HoldEquilibrium"};
    std::string current_rationale_{"System in low-pressure equilibrium; no mutation required."};
    std::string last_verdict_{"HOLD"};
    TunerVerdict last_verdict_enum_{TunerVerdict::Hold};
    double last_utility_delta_{0.0};

    // Before/After evaluation window state
    double before_io_psi_{0.0};
    double before_mem_psi_{0.0};
    double before_cpu_psi_{0.0};
    double before_thermal_{0.0};
    double before_temperature_celsius_{-1.0};
    bool before_io_valid_{false}, before_mem_valid_{false}, before_cpu_valid_{false}, before_thermal_valid_{false};
    double sum_after_io_psi_{0.0};
    double sum_after_mem_psi_{0.0};
    double sum_after_cpu_psi_{0.0};
    double sum_after_thermal_{0.0};
    double sum_after_temp_vel_{0.0};
    uint32_t valid_thermal_samples_in_window_{0};
    uint32_t valid_io_samples_in_window_{0}, valid_mem_samples_in_window_{0};
    uint32_t valid_cpu_samples_in_window_{0}, valid_pressure_samples_in_window_{0};
    double min_after_mem_psi_{0.0}, max_after_mem_psi_{0.0};
    double min_after_cpu_psi_{0.0}, max_after_cpu_psi_{0.0};
    double min_after_thermal_{0.0}, max_after_thermal_{0.0};
    double latest_temperature_velocity_{0.0};
    bool latest_temperature_velocity_valid_{false};
    double latest_current_temperature_{-1.0};
    double latest_pressure_velocity_{0.0};
    double min_after_io_psi_{0.0};
    double max_after_io_psi_{0.0};
    uint32_t obs_ticks_{0};
    uint32_t obs_window_{3};
    uint32_t idle_streak_{0};

    uint32_t total_trials_{0}; // started transaction attempts
    uint32_t total_reward_samples_{0}; // complete, eligible evaluation windows
    uint32_t total_improved_{0};
    uint32_t total_rolled_back_{0};
    uint32_t total_inconclusive_{0};
    uint32_t total_rejected_{0};

    std::vector<MutationStep> applied_steps_;

    bool handle_preemption(MutationDomain domain, std::string_view transaction_id,
                           std::string_view reason) noexcept;
    void init_candidates();
    void update_ucb_scores();
    void refresh_surface_readings();
    void classify_discovered_resources();
    std::string compute_fingerprint() const;
    void save_persisted_stats() const;
    void load_persisted_stats();
    bool verify_active_ownership() const;
    bool retry_contended_ownership();
    MutationDomain domain_for_candidate(std::string_view candidate_id) const noexcept;
    ControllerPriority priority_for_candidate(std::string_view candidate_id) const noexcept;
    bool apply_transaction(
        std::string_view candidate_id,
        std::string_view candidate_label,
        std::string_view intent,
        std::string_view rationale,
        const std::vector<MutationStep>& steps,
        const TunerTelemetryInput& telemetry);
    void evaluate_observation_window(const TunerTelemetryInput& latest_telemetry);
    [[nodiscard]] std::optional<uint32_t> select_frequency_policy_index() const noexcept;
    CandidateStats* find_candidate(std::string_view id);
    const CandidateStats* find_candidate(std::string_view id) const;
};

} // namespace coreflow

