/**
 * @file thermal_envelope_actuator.hpp
 * @brief CoreFlow v5 Pillar 3C: Smooth CPU/GPU governor & thermal envelope actuator driven by Rust PID + Context.
 * @license MIT
 */
#pragma once

#include "coreflow/baseline_store.hpp"
#include "coreflow/v5/context_engine.hpp"
#include "coreflow/v5/hardware_probe.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace coreflow::v5 {

struct ThermalEnvelopeStats {
    uint32_t managed_nodes{0};
    uint32_t contended_nodes{0};
    double last_capacity_scale{1.0};
};

struct EnvelopeTelemetry {
    double cpu_psi{0.0};
    double gpu_busy_pct{0.0};
};

class ThermalEnvelopeActuator {
public:
    explicit ThermalEnvelopeActuator(BaselineStore& baseline) noexcept;

    void apply(const HardwareTopology& topo,
               ContextMode mode,
               PerformanceProfile profile,
               double capacity_scale,
               bool thermal_guard,
               bool enabled,
               const EnvelopeTelemetry& telemetry = {}) noexcept;

    void reset_contention() noexcept;
    [[nodiscard]] ThermalEnvelopeStats stats() const noexcept { return stats_; }

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
    bool write_managed_node(const std::string& path, const std::string& target) noexcept;
    void rollback_all_managed() noexcept;

    BaselineStore& baseline_;
    std::unordered_map<std::string, std::string> last_written_;
    std::unordered_set<std::string> contended_paths_;
    ThermalEnvelopeStats stats_{};
};

} // namespace coreflow::v5
