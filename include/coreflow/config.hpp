#pragma once

#include <cstddef>
#include <string>

namespace coreflow {

inline constexpr const char* kCoreFlowVersion = "2.1.0";

enum class MutationMode {
    Disabled,
    Adaptive
};

class EngineConfig {
public:
    EngineConfig() = default;

    bool load(const std::string& path) noexcept;

    int monitorIntervalSeconds() const noexcept { return monitor_interval_seconds_; }
    double minConfidence() const noexcept { return min_confidence_; }
    MutationMode mutationMode() const noexcept { return mutation_mode_; }
    bool allowCpuGovernor() const noexcept { return allow_cpu_governor_; }
    bool mutationArmed() const noexcept { return mutation_armed_; }
    bool runtimeRefreshEnabled() const noexcept { return runtime_refresh_enabled_; }

    void setMonitorIntervalSeconds(int seconds) noexcept;
    void setMinConfidence(double confidence) noexcept;
    void setMutationMode(MutationMode mode) noexcept { mutation_mode_ = mode; }
    void setAllowCpuGovernor(bool enabled) noexcept { allow_cpu_governor_ = enabled; }
    void setMutationArmed(bool enabled) noexcept { mutation_armed_ = enabled; }
    void setRuntimeRefreshEnabled(bool enabled) noexcept { runtime_refresh_enabled_ = enabled; }

private:
    int monitor_interval_seconds_{5};
    // Balance: 0.60 lets Adaptive act on trustworthy-but-not-perfect samples.
    // Authority, journal, and regression rollback remain the hard stops.
    double min_confidence_{0.60};
    // Release default is observe-only. Kernel mutation requires explicit
    // opt-in (mutation_mode=adaptive, mutation_armed=true, allow_cpu_governor=yes).
    MutationMode mutation_mode_{MutationMode::Disabled};
    bool allow_cpu_governor_{false};
    bool runtime_refresh_enabled_{true};
    bool mutation_armed_{false};
};

const char* mutationModeName(MutationMode mode) noexcept;

} // namespace coreflow
