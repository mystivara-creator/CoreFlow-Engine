#pragma once

#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/frequency_target_planner.hpp"
#include "coreflow/thermal_monitor.hpp"
#include "coreflow/workload_intelligence.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace coreflow {

struct ThermalAdmission final {
    bool granted{false};
    uint64_t observed_at_ns{0};
    std::string reason{"thermal admission unavailable"};
};

class ThermalSafetyAuthority final {
public:
    explicit ThermalSafetyAuthority(
        const ThermalMonitor& monitor,
        std::vector<std::string> required_sensor_tokens = {"xo-therm", "quiet_therm"},
        uint64_t max_age_ns = 2'000'000'000ULL) noexcept;

    [[nodiscard]] ThermalAdmission evaluate(uint64_t now_ns) const noexcept;

private:
    const ThermalMonitor& monitor_;
    std::vector<std::string> required_sensor_tokens_;
    uint64_t max_age_ns_;
};

// The ceiling must be supplied by a provider independent of scaling_max_freq.
// No implicit/default provider exists: absence of an independently validated
// ceiling is a hard denial, not permission to use cpuinfo_max_freq as a proxy.
struct IndependentFrequencyCeiling final {
    uint64_t max_hz{0};
    uint64_t valid_until_ns{0};
    std::string capability_fingerprint;
    bool signature_or_kernel_contract_verified{false};
};

class IndependentFrequencyCeilingProvider {
public:
    virtual ~IndependentFrequencyCeilingProvider() = default;
    [[nodiscard]] virtual std::optional<IndependentFrequencyCeiling> current(
        std::string_view policy_path, uint64_t now_ns) const noexcept = 0;
};

struct OutcomeSample final {
    uint64_t timestamp_ns{0};
    double cpu_psi_some_pct{0.0};
    double memory_psi_some_pct{0.0};
    double io_psi_some_pct{0.0};
    double ebpf_wakeup_rate_per_sec{0.0};
    bool cpu_psi_valid{false};
    bool memory_psi_valid{false};
    bool io_psi_valid{false};
    bool ebpf_wakeup_valid{false};
    bool workload_stable{false};
};

enum class OutcomeVerdict : uint8_t {
    Pending, Improved, Neutral, Regressed, Inconclusive
};

struct OutcomeResult final {
    OutcomeVerdict verdict{OutcomeVerdict::Inconclusive};
    uint32_t samples{0};
    std::string reason{"insufficient observation"};
};

class FrequencyOutcomeEvaluator final {
public:
    [[nodiscard]] bool begin(const OutcomeSample& baseline) noexcept;
    [[nodiscard]] OutcomeResult observe(const OutcomeSample& sample) noexcept;
    void reset() noexcept;

private:
    OutcomeSample baseline_{};
    uint64_t started_at_ns_{0};
    uint32_t samples_{0};
    bool active_{false};
};

// One CPU policy transaction at a time. The coordinator lease remains held
// while the mutation is active; it is released only after BaselineStore rollback,
// readback equality with the captured baseline, and journal ownership verification.
class TransactionalFrequencyActuator final {
public:
    TransactionalFrequencyActuator(
        CentralMutationCoordinator& coordinator,
        BaselineStore& baseline_store,
        const FrequencyTargetPlanner& planner,
        const ThermalSafetyAuthority& thermal,
        const IndependentFrequencyCeilingProvider& ceiling_provider,
        std::string sys_root = "/sys");
    ~TransactionalFrequencyActuator();

    TransactionalFrequencyActuator(const TransactionalFrequencyActuator&) = delete;
    TransactionalFrequencyActuator& operator=(const TransactionalFrequencyActuator&) = delete;

    [[nodiscard]] bool apply(
        const std::string& policy_path,
        const FrequencyCapabilitySnapshot& capability,
        uint64_t current_hz,
        double requested_capacity_scale,
        const OutcomeSample& baseline_sample,
        uint64_t now_ns) noexcept;

    [[nodiscard]] OutcomeResult observe(const OutcomeSample& sample) noexcept;
    [[nodiscard]] bool rollback() noexcept;
    [[nodiscard]] bool mutation_active() const noexcept;
    [[nodiscard]] bool recovery_required() const noexcept;

private:
    CentralMutationCoordinator& coordinator_;
    BaselineStore& baseline_store_;
    const FrequencyTargetPlanner& planner_;
    const ThermalSafetyAuthority& thermal_;
    const IndependentFrequencyCeilingProvider& ceiling_provider_;
    std::string sys_root_;
    std::string controller_id_;
    std::string transaction_id_;
    std::string target_path_;
    std::string baseline_value_;
    uint64_t target_hz_{0};
    mutable std::mutex mutex_;
    FrequencyOutcomeEvaluator evaluator_;
    bool active_{false};
    bool recovery_required_{false};

    [[nodiscard]] bool rollback_locked() noexcept;
    [[nodiscard]] bool handle_preemption(
        MutationDomain domain, std::string_view transaction_id,
        std::string_view reason) noexcept;
};

} // namespace coreflow
