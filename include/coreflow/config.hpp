#pragma once

#include <cstddef>
#include <string>

namespace coreflow {

inline constexpr const char* kCoreFlowVersion = "1.3.0";

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
    bool runtimeRefreshEnabled() const noexcept { return runtime_refresh_enabled_; }

    void setMonitorIntervalSeconds(int seconds) noexcept;
    void setMinConfidence(double confidence) noexcept;
    void setMutationMode(MutationMode mode) noexcept { mutation_mode_ = mode; }
    void setAllowCpuGovernor(bool enabled) noexcept { allow_cpu_governor_ = enabled; }
    void setRuntimeRefreshEnabled(bool enabled) noexcept { runtime_refresh_enabled_ = enabled; }

private:
    int monitor_interval_seconds_{5};
    double min_confidence_{0.70};
    MutationMode mutation_mode_{MutationMode::Disabled};
    bool allow_cpu_governor_{true};
    bool runtime_refresh_enabled_{true};
};

const char* mutationModeName(MutationMode mode) noexcept;

} // namespace coreflow
