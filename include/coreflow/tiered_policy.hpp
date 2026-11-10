#pragma once

#include "coreflow/platform_context.hpp"
#include "coreflow/workload_intelligence.hpp"

#include <cstdint>
#include <string_view>

namespace coreflow {

enum class OptimizationTier : uint8_t {
    Tier0Equilibrium = 0,
    Tier1InteractiveResponse = 1,
    Tier2SustainedOptimization = 2,
    Tier3ThermalRecovery = 3
};

enum class PolicyAction : uint8_t {
    ObserveOnly,
    CpuInteractiveResponse,
    CpuFrequencyStep,
    MemoryWorkingSetWarmup,
    ThermalRecovery
};

struct PolicyEvidence final {
    WorkloadFeatureSnapshot telemetry{};
    WorkloadClassification classification{};
    uint64_t evaluated_at_ns{0};
    uint64_t max_telemetry_age_ns{0};
    bool telemetry_fresh{false};
    bool event_stream_saturated{false};
    bool thermal_headroom_valid{false};
    double thermal_headroom_c{0.0};
    bool thermal_recovery_required{false};
    bool cpu_frequency_capability_valid{false};
    bool memory_warmup_capability_valid{false};
    bool working_set_validated{false};
    bool mutation_runtime_ready{false};
};

struct PolicyDecision final {
    OptimizationTier tier{OptimizationTier::Tier0Equilibrium};
    PolicyAction action{PolicyAction::ObserveOnly};
    double requested_capacity_scale{1.0};
    uint64_t max_prefetch_bytes{0};
    uint32_t settle_window_ms{0};
    uint32_t evidence_mask{FeatureNone};
    float confidence{0.0F};
    std::string_view rationale{"insufficient evidence"};
    bool requires_mutation_admission{false};
};

class OptimizationPolicy {
public:
    virtual ~OptimizationPolicy() = default;
    [[nodiscard]] virtual PolicyDecision evaluate(const PolicyEvidence&) const noexcept = 0;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
};

// Universal policy. It never assumes vendor-specific nodes or privileges.
class CoreFlowBase final : public OptimizationPolicy {
public:
    [[nodiscard]] PolicyDecision evaluate(const PolicyEvidence&) const noexcept override;
    [[nodiscard]] std::string_view name() const noexcept override { return "coreflow_base"; }
};

// HyperOS specialization changes response thresholds/budgets only. It cannot
// bypass telemetry validity, thermal admission, CDDP, ownership, or rollback.
class HyperOS4Inspired final : public OptimizationPolicy {
public:
    [[nodiscard]] PolicyDecision evaluate(const PolicyEvidence&) const noexcept override;
    [[nodiscard]] std::string_view name() const noexcept override { return "hyperos4_inspired"; }
};

struct TieredPolicySelection final {
    const OptimizationPolicy* policy{nullptr};
    bool platform_policy_eligible{false};
};

class TieredPolicyRouter final {
public:
    TieredPolicyRouter(const CoreFlowBase& base, const HyperOS4Inspired& hyperos) noexcept
        : base_(base), hyperos_(hyperos) {}

    [[nodiscard]] TieredPolicySelection select(const PlatformContextSnapshot&) const noexcept;
    [[nodiscard]] PolicyDecision evaluate(const PlatformContextSnapshot&,
                                          const PolicyEvidence&) const noexcept;

private:
    const CoreFlowBase& base_;
    const HyperOS4Inspired& hyperos_;
};

} // namespace coreflow
