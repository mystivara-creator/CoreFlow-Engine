/**
 * @file decision_engine.hpp
 * @brief Global Evidence-Driven Autonomous Optimization Engine Components:
 *        EnvironmentDiscovery, EvidenceCorrelator, Independent CandidateProviders,
 *        GlobalCandidateEvaluator, and Compatible InterventionPlanner.
 * @license MIT
 */
#pragma once

#include "coreflow/autonomous_tuner.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace coreflow {

class BoundedWorkerPool;

enum class CandidateActionKind : uint8_t {
    SysfsTransaction = 0,
    TieredFrequencyPolicy = 1,
    GpuFrequencyPolicy = 2,
};

struct PlannedMutationStep final {
    std::string path;
    std::string target_value;
    std::string relationship;
    StepRelation relation{StepRelation::RequiredDependent};
};

struct EnvironmentDiscoveryInput final {
    std::string sys_root;
    std::string proc_root;
    bool zram_active{false};
    std::string vm_page_cluster_path;
    std::string vm_vfs_cache_pressure_path;
    const std::vector<BlockQueueSurface>* block_queues{nullptr};
    const std::vector<std::string>* cpufreq_governor_dirs{nullptr};
    const std::vector<NodeCapability>* kernel_node_capabilities{nullptr};
    DeviceProfileHandle device_profile;
    bool frequency_actuator_configured{false};
    bool gpu_actuator_ready{false};
    std::optional<uint32_t> frequency_policy_index;
    PolicyDecision tiered_policy_decision{};
    bool tiered_policy_ready{false};
    bool tiered_cpu_policy_owned{false};
    bool uclamp_ready{false};
};

struct EnvironmentCapabilities final {
    bool zram_active{false};
    std::string vm_page_cluster_path;
    std::string vm_vfs_cache_pressure_path;
    std::vector<BlockQueueSurface> block_queues;
    std::vector<std::string> cpufreq_governor_dirs;
    std::vector<NodeCapability> runtime_tunable_nodes;
    size_t gpu_domain_count{0U};
    size_t thermal_sensor_count{0U};
    bool frequency_actuator_ready{false};
    bool gpu_actuator_ready{false};
    std::optional<uint32_t> frequency_policy_index;
    PolicyDecision tiered_policy_decision{};
    bool tiered_policy_ready{false};
    bool tiered_cpu_policy_owned{false};
    bool uclamp_ready{false};

    [[nodiscard]] bool has_runtime_node(std::string_view path) const noexcept;
    [[nodiscard]] uint64_t lookup_factory_u64(
        std::string_view path,
        uint64_t fallback_baseline) const noexcept;
};

struct CorrelatedEvidence final {
    TunerTelemetryInput telemetry{};
    WorkloadClass workload{WorkloadClass::Idle};
    DynamicWorkloadRegime regime{DynamicWorkloadRegime::IdleDeep};
    uint8_t thermal_bucket{0U};

    // Normalized [0.0 .. 1.0] independent subsystem severities
    double cpu_severity{0.0};
    double memory_severity{0.0};
    double io_severity{0.0};
    double thermal_severity{0.0};
    double gpu_severity{0.0};

    // Cross-domain evidence correlations [0.0 .. 1.0]
    double concurrent_cpu_io{0.0};
    double swap_io_thrash{0.0};
    double interactive_cpu_stall{0.0};
    double thermal_ramp_pressure{0.0};

    // Overall observability & admission state
    double evidence_confidence{0.0};
    double remaining_thermal_headroom{1.0};
    bool psi_complete{false};
    bool thermal_emergency{false};
    bool thermal_stabilization_required{false};
    bool in_thermal_recovery{false};
    BottleneckKind diagnosed_bottleneck{BottleneckKind::None};
};

struct CandidateProposal final {
    std::string id;
    std::string label;
    std::string domain_label; // "io", "vm", "cpufreq", "gpu", "thermal"
    MutationDomain mutation_domain{MutationDomain::BlockIo};
    EvaluationDomain evaluation_domain{EvaluationDomain::Unavailable};
    ControllerPriority priority{ControllerPriority::WorkloadOptimization};
    CandidateActionKind action_kind{CandidateActionKind::SysfsTransaction};
    std::string intent;
    std::string rationale;
    std::vector<PlannedMutationStep> steps;
    std::vector<std::string> resource_keys;
    double requested_capacity_scale{1.0};
    uint32_t frequency_policy_index{0U};

    // Evidence-derived metrics [0.0 .. 1.0]
    double relevance{0.0};
    double inherent_risk{0.2};
    double thermal_cost{0.0};
    bool stabilizing{false};
};

struct EvaluatedCandidate final {
    CandidateProposal proposal{};
    double utility_score{0.0};       // Comparable [0.0 .. 100.0]
    double normalized_utility{0.0};  // Comparable [0.0 .. 1.0]
    double empirical_score{0.5};     // Domain-normalized contextual UCB [0.0 .. 1.0]
    double confidence{0.0};          // Combined evidence + sample maturity [0.0 .. 1.0]
    double risk_penalty{0.0};        // [0.0 .. 1.0]
    double thermal_penalty{0.0};     // [0.0 .. 1.0]
    bool eligible{false};
    std::string rejection_reason;
};

struct ActiveDomainReservation final {
    MutationDomain domain{MutationDomain::BlockIo};
    std::string candidate_id;
    std::vector<std::string> resource_keys;
};

struct InterventionPlan final {
    std::vector<EvaluatedCandidate> selected;
    std::vector<EvaluatedCandidate> ranked_all;
    std::string planning_rationale;
};

class EnvironmentDiscovery final {
public:
    [[nodiscard]] static EnvironmentCapabilities discover(
        const EnvironmentDiscoveryInput& input);
};

class EvidenceCorrelator final {
public:
    [[nodiscard]] static CorrelatedEvidence correlate(
        const TunerTelemetryInput& telemetry,
        const EnvironmentCapabilities& env,
        bool in_thermal_recovery) noexcept;
};

class ICandidateProvider {
public:
    virtual ~ICandidateProvider() = default;
    [[nodiscard]] virtual std::string_view provider_name() const noexcept = 0;
    virtual void generate(
        const EnvironmentCapabilities& env,
        const CorrelatedEvidence& evidence,
        const std::vector<CandidateStats>& stats,
        std::vector<CandidateProposal>& out) const = 0;
};

class BlockIoCandidateProvider final : public ICandidateProvider {
public:
    [[nodiscard]] std::string_view provider_name() const noexcept override {
        return "BlockIoCandidateProvider";
    }
    void generate(
        const EnvironmentCapabilities& env,
        const CorrelatedEvidence& evidence,
        const std::vector<CandidateStats>& stats,
        std::vector<CandidateProposal>& out) const override;
};

class MemoryVmCandidateProvider final : public ICandidateProvider {
public:
    [[nodiscard]] std::string_view provider_name() const noexcept override {
        return "MemoryVmCandidateProvider";
    }
    void generate(
        const EnvironmentCapabilities& env,
        const CorrelatedEvidence& evidence,
        const std::vector<CandidateStats>& stats,
        std::vector<CandidateProposal>& out) const override;
};

class CpuGovernorCandidateProvider final : public ICandidateProvider {
public:
    [[nodiscard]] std::string_view provider_name() const noexcept override {
        return "CpuGovernorCandidateProvider";
    }
    void generate(
        const EnvironmentCapabilities& env,
        const CorrelatedEvidence& evidence,
        const std::vector<CandidateStats>& stats,
        std::vector<CandidateProposal>& out) const override;
};

class TieredFrequencyCandidateProvider final : public ICandidateProvider {
public:
    [[nodiscard]] std::string_view provider_name() const noexcept override {
        return "TieredFrequencyCandidateProvider";
    }
    void generate(
        const EnvironmentCapabilities& env,
        const CorrelatedEvidence& evidence,
        const std::vector<CandidateStats>& stats,
        std::vector<CandidateProposal>& out) const override;
};

class GpuCandidateProvider final : public ICandidateProvider {
public:
    [[nodiscard]] std::string_view provider_name() const noexcept override {
        return "GpuCandidateProvider";
    }
    void generate(const EnvironmentCapabilities& env, const CorrelatedEvidence& evidence,
        const std::vector<CandidateStats>& stats, std::vector<CandidateProposal>& out) const override;
};

class ThermalStabilizationCandidateProvider final : public ICandidateProvider {
public:
    [[nodiscard]] std::string_view provider_name() const noexcept override {
        return "ThermalStabilizationCandidateProvider";
    }
    void generate(
        const EnvironmentCapabilities& env,
        const CorrelatedEvidence& evidence,
        const std::vector<CandidateStats>& stats,
        std::vector<CandidateProposal>& out) const override;
};

using CandidateEligibilityFn = std::function<bool(const CandidateProposal&, std::string& reason_out)>;

class GlobalCandidateEvaluator final {
public:
    [[nodiscard]] static std::vector<EvaluatedCandidate> evaluate_all(
        std::vector<CandidateProposal> proposals,
        const CorrelatedEvidence& evidence,
        const std::vector<CandidateStats>& stats,
        uint32_t total_reward_samples,
        const CandidateEligibilityFn& eligibility_check);
};

class InterventionPlanner final {
public:
    static constexpr size_t kMaxConcurrentActiveNormal = 2U;
    static constexpr size_t kMaxConcurrentActiveTightThermal = 1U;
    static constexpr size_t kMaxConcurrentActiveStabilizing = 2U;
    static constexpr double kMinimumUtilityThreshold = 0.28;

    [[nodiscard]] static InterventionPlan plan(
        std::vector<EvaluatedCandidate> evaluated,
        const CorrelatedEvidence& evidence,
        const std::vector<ActiveDomainReservation>& active_reservations,
        const CentralMutationCoordinator& coordinator,
        std::string_view controller_name);
};

// Orchestrates all independent providers (optionally in parallel across worker pool),
// evaluates all candidates globally in a single comparable utility model, and produces
// a conflict-free InterventionPlan.
class GlobalDecisionEngine final {
public:
    GlobalDecisionEngine();

    [[nodiscard]] InterventionPlan run_cycle(
        const EnvironmentCapabilities& env,
        const CorrelatedEvidence& evidence,
        const std::vector<CandidateStats>& stats,
        uint32_t total_reward_samples,
        const std::vector<ActiveDomainReservation>& active_reservations,
        const CentralMutationCoordinator& coordinator,
        std::string_view controller_name,
        const CandidateEligibilityFn& eligibility_check,
        BoundedWorkerPool* pool = nullptr) const;

private:
    std::vector<std::unique_ptr<ICandidateProvider>> providers_;
};

} // namespace coreflow
