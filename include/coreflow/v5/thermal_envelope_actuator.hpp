/**
 * @file thermal_envelope_actuator.hpp
 * @brief CoreFlow v5 Pillar 3C: Smooth CPU/GPU governor & thermal envelope actuator driven by Rust PID + Context.
 * @license Apache-2.0
 */
#pragma once

#include "coreflow/baseline_store.hpp"
#include "coreflow/v5/context_engine.hpp"
#include "coreflow/v5/hardware_probe.hpp"
#include "coreflow/v6/causal_verifier.hpp"
#include "coreflow/v6/oem_collaboration_map.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace coreflow::v5 {

struct ThermalEnvelopeStats {
    uint32_t managed_nodes{0};
    uint32_t contended_nodes{0};
    double last_capacity_scale{1.0};
    uint32_t eas_top_app_cap_pct{100};
    uint32_t eas_capped_groups{0};
    bool gpu_cap_active{false};
    bool cpu_gov_active{false};
};

struct EnvelopeTelemetry {
    double cpu_psi{0.0};
    double gpu_busy_pct{0.0};
    double render_pressure{0.0};
    bool passive_media_only{false};
    double scheduler_responsiveness_scale{1.0}; // 0.70..1.00 (tightens up_rate_limit_us under render pressure)
    double thermal_stepdown_lead{0.0};          // 0.00..0.15 (shifts thermal envelope step-down earlier)
    double jank_ratio{-1.0};                    // live frame drop ratio (0..1); >=0.03 lifts top-app cap
    bool low_battery_eco{false};                // battery <=20% off-charger automatic eco pacing
};

class ThermalEnvelopeActuator {
public:
    static constexpr uint64_t kInitialContentionCooldownMs = 30000ULL;
    static constexpr uint64_t kMaxContentionCooldownMs = 300000ULL;

    explicit ThermalEnvelopeActuator(
        BaselineStore& baseline,
        v6::OemCollaborationMap* oem_map = nullptr,
        const v6::CausalVerifier* causal_verifier = nullptr) noexcept;

    void apply(const HardwareTopology& topo,
               ContextMode mode,
               PerformanceProfile profile,
               double capacity_scale,
               bool thermal_guard,
               bool enabled,
               const EnvelopeTelemetry& telemetry = {},
               uint64_t now_ms = 0U) noexcept;

    void reset_contention() noexcept;
    [[nodiscard]] ThermalEnvelopeStats stats() const noexcept { return stats_; }
    [[nodiscard]] std::vector<std::string> contended_paths() const;

    // Exposed for deterministic unit testing.
    [[nodiscard]] static uint64_t select_opp(
        const std::vector<uint64_t>& sorted_opps, double fraction) noexcept;
    [[nodiscard]] static std::pair<uint64_t, uint64_t> compute_kgsl_levels(
        uint64_t num_pwrlevels,
        ContextMode mode,
        PerformanceProfile profile,
        double capacity_scale,
        bool thermal_guard) noexcept;

private:
    struct ContentionEntry {
        uint64_t retry_after_ms{0};
        uint32_t strikes{0};
        std::string observed_value;
    };

    void record_contention(const std::string& path,
                           const std::string& observed,
                           uint64_t now_ms) noexcept;
    bool write_managed_node(const std::string& path,
                            const std::string& target,
                            uint64_t now_ms,
                            uint64_t deadband = 0U) noexcept;
    bool write_uclamp_max_node(const std::string& path,
                               double target_pct,
                               bool uses_percent,
                               uint64_t now_ms) noexcept;
    void apply_cgroup_eas_caps(const HardwareTopology& topo,
                               ContextMode mode,
                               double effective_scale,
                               bool thermal_guard,
                               const EnvelopeTelemetry& telemetry,
                               uint64_t now_ms) noexcept;
    void rollback_managed_node(const std::string& path) noexcept;
    void rollback_all_managed() noexcept;

    BaselineStore& baseline_;
    v6::OemCollaborationMap* oem_map_{nullptr};
    const v6::CausalVerifier* causal_verifier_{nullptr};
    std::unordered_map<std::string, std::string> last_written_;
    std::unordered_map<std::string, uint64_t> baseline_hispeed_khz_;
    std::unordered_map<std::string, ContentionEntry> contended_paths_;
    ThermalEnvelopeStats stats_{};
};

} // namespace coreflow::v5
