#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

#include "coreflow/effect_model.hpp"
#include "coreflow/environment.hpp"

namespace coreflow {

// StrategyCatalog sits between Outcome Evaluator / Baseline Intelligence and
// EffectModel candidate ranking in the closed-loop pipeline:
//
//   Outcome Evaluator -> Strategy Catalog -> Effect Model & Candidate Ranking
//
// It contains zero hardcoded kernel write values. Instead, it inspects the
// live discovered EnvironmentCapabilityMatrix and current resource baselines
// to derive:
//   1. The active operating strategy (IdleConservation, DailyMultitasking,
//      SustainedGaming, MemoryRelief, ThermalStabilization).
//   2. Cross-domain parameter coupling posture (active I/O scheduler family,
//      active CPU governor family, UClamp/CpuSet/GPU domain presence) so a
//      change in one domain coherently steers coupled domains on subsequent
//      cycles while cooperating with Android Power HAL / Performance Hint API.

enum class StrategyProfile : std::uint8_t {
    IdleConservation = 0,
    DailyMultitasking,
    SustainedGaming,
    MemoryRelief,
    ThermalStabilization,
};

enum class IoSchedulerPosture : std::uint8_t {
    Unknown = 0,
    LowOverheadDirect,   // none / noop
    DeadlineLatency,     // mq-deadline / deadline / kyber
    FairThroughput,      // bfq / cfq / other
};

enum class CpuGovernorPosture : std::uint8_t {
    Unknown = 0,
    EnergyAwareDynamic,  // schedutil / walt / sugov
    PerformancePinned,   // performance
    PowerSavePinned,     // powersave / conservative
};

struct CouplingSnapshot {
    StrategyProfile profile{StrategyProfile::IdleConservation};
    IoSchedulerPosture io_posture{IoSchedulerPosture::Unknown};
    CpuGovernorPosture cpu_posture{CpuGovernorPosture::Unknown};
    bool warm_equilibrium{false};   // 48.0C <= thermal < 52.0C: trim background first
    bool has_io_domain{false};
    bool has_cpufreq_domain{false};
    bool has_uclamp_domain{false};
    bool has_cpuset_domain{false};
    bool has_gpu_domain{false};
    bool has_memory_domain{false};
    std::size_t active_domain_count{0};
};

struct StrategyAdjustment {
    double benefit_delta{0.0};
    double risk_delta{0.0};
    const char* coupling_tag{nullptr};
};

const char* strategyProfileName(StrategyProfile profile) noexcept;

class StrategyCatalog final {
public:
    static CouplingSnapshot analyze(
        const EnvironmentCapabilityMatrix& matrix,
        const std::unordered_map<std::string, std::string>& baselines,
        const EffectContext& ctx
    ) noexcept;

    static StrategyAdjustment evaluateCandidate(
        const ResourceCapability& capability,
        std::string_view baseline_value,
        std::string_view candidate_value,
        EffectDirection direction,
        const CouplingSnapshot& snapshot,
        const EffectContext& ctx
    ) noexcept;

private:
    static std::string extractActiveToken(std::string_view raw) noexcept;
    static IoSchedulerPosture classifyIoScheduler(std::string_view token) noexcept;
    static CpuGovernorPosture classifyCpuGovernor(std::string_view token) noexcept;
};

} // namespace coreflow
