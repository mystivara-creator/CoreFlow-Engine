#pragma once

#include <cstddef>
#include <string>

namespace coreflow {

inline constexpr const char* kCoreFlowVersion = "2.3.0";

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
    // User permission per control namespace. A device need never grants this.
    bool grantMemory() const noexcept { return grant_memory_; }
    bool grantBlockQueue() const noexcept { return grant_block_queue_; }
    bool grantScheduler() const noexcept { return grant_scheduler_; }
    bool grantGpu() const noexcept { return grant_gpu_; }
    bool grantCgroup() const noexcept { return grant_cgroup_; }
    bool grantCpuFreq() const noexcept { return grant_cpufreq_; }
    bool grantKernel() const noexcept { return grant_kernel_; }

    void setMonitorIntervalSeconds(int seconds) noexcept;
    void setMinConfidence(double confidence) noexcept;
    void setMutationMode(MutationMode mode) noexcept { mutation_mode_ = mode; }
    void setAllowCpuGovernor(bool enabled) noexcept { allow_cpu_governor_ = enabled; }
    void setMutationArmed(bool enabled) noexcept { mutation_armed_ = enabled; }
    void setRuntimeRefreshEnabled(bool enabled) noexcept { runtime_refresh_enabled_ = enabled; }
    void setGrantMemory(bool enabled) noexcept { grant_memory_ = enabled; }
    void setGrantBlockQueue(bool enabled) noexcept { grant_block_queue_ = enabled; }
    void setGrantScheduler(bool enabled) noexcept { grant_scheduler_ = enabled; }
    void setGrantGpu(bool enabled) noexcept { grant_gpu_ = enabled; }
    void setGrantCgroup(bool enabled) noexcept { grant_cgroup_ = enabled; }
    void setGrantCpuFreq(bool enabled) noexcept { grant_cpufreq_ = enabled; }
    void setGrantKernel(bool enabled) noexcept { grant_kernel_ = enabled; }

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
    // Per-namespace grants are opt-OUT: Adaptive + armed already is the user's
    // consent, so every namespace is permitted unless grant_<ns>=no. An invalid
    // mode still clears all grants (fail closed).
    bool grant_memory_{true};
    bool grant_block_queue_{true};
    bool grant_scheduler_{true};
    bool grant_gpu_{true};
    bool grant_cgroup_{true};
    bool grant_cpufreq_{true};
    bool grant_kernel_{true};
};

const char* mutationModeName(MutationMode mode) noexcept;

} // namespace coreflow
