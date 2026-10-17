#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace coreflow {

inline constexpr const char* kCoreFlowVersion = "2.4.0";

// Interactivity pipeline (v2.4.0): launch/jank sensing and bounded boosts.
// Observe is the default; adaptation additionally requires the global
// mutation gates (mutation_mode=adaptive + mutation_armed).
enum class InteractivityMode {
    Observe,
    Adaptive
};

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
    // Legacy compatibility: CPUFreq follows the global adaptive authority.
    bool allowCpuGovernor() const noexcept { return true; }
    bool mutationArmed() const noexcept { return mutation_armed_; }
    bool runtimeRefreshEnabled() const noexcept { return runtime_refresh_enabled_; }
    // Legacy compatibility: namespace grant flags are ignored. Effective access
    // is determined by capability discovery, the global adaptive/armed gates,
    // and fatal-surface policy.
    bool grantMemory() const noexcept { return true; }
    bool grantBlockQueue() const noexcept { return true; }
    bool grantScheduler() const noexcept { return true; }
    bool grantGpu() const noexcept { return true; }
    bool grantCgroup() const noexcept { return true; }
    bool grantCpuFreq() const noexcept { return true; }
    bool grantKernel() const noexcept { return true; }
    InteractivityMode interactivityMode() const noexcept { return interactivity_mode_; }
    int launchWindowMs() const noexcept { return launch_window_ms_; }
    double jankMissRate() const noexcept { return jank_miss_rate_; }
    long boostReadaheadKb() const noexcept { return boost_readahead_kb_; }
    long boostUclampLaunch() const noexcept { return boost_uclamp_launch_; }
    long boostUclampJank() const noexcept { return boost_uclamp_jank_; }
    int boostGpuSteps() const noexcept { return boost_gpu_steps_; }
    int boostWindowMs() const noexcept { return boost_window_ms_; }

    void setMonitorIntervalSeconds(int seconds) noexcept;
    void setMinConfidence(double confidence) noexcept;
    void setMutationMode(MutationMode mode) noexcept { mutation_mode_ = mode; }
    void setAllowCpuGovernor(bool /*enabled*/) noexcept {} // Legacy no-op.
    void setMutationArmed(bool enabled) noexcept { mutation_armed_ = enabled; }
    void setRuntimeRefreshEnabled(bool enabled) noexcept { runtime_refresh_enabled_ = enabled; }
    void setGrantMemory(bool /*enabled*/) noexcept {} // Legacy no-op.
    void setGrantBlockQueue(bool /*enabled*/) noexcept {} // Legacy no-op.
    void setGrantScheduler(bool /*enabled*/) noexcept {} // Legacy no-op.
    void setGrantGpu(bool /*enabled*/) noexcept {} // Legacy no-op.
    void setGrantCgroup(bool /*enabled*/) noexcept {} // Legacy no-op.
    void setGrantCpuFreq(bool /*enabled*/) noexcept {} // Legacy no-op.
    void setGrantKernel(bool /*enabled*/) noexcept {} // Legacy no-op.
    void setInteractivityMode(InteractivityMode mode) noexcept { interactivity_mode_ = mode; }
    void setLaunchWindowMs(int ms) noexcept;
    void setJankMissRate(double rate) noexcept;
    void setBoostReadaheadKb(long kb) noexcept;
    void setBoostUclampLaunch(long value) noexcept;
    void setBoostUclampJank(long value) noexcept;
    void setBoostGpuSteps(int steps) noexcept;
    void setBoostWindowMs(int ms) noexcept;

private:
    int monitor_interval_seconds_{5};
    // Balance: 0.60 lets Adaptive act on trustworthy-but-not-perfect samples.
    // Authority, journal, and regression rollback remain the hard stops.
    double min_confidence_{0.60};
    // Release default is observe-only. Kernel mutation requires explicit
    // opt-in (mutation_mode=adaptive, mutation_armed=true).
    MutationMode mutation_mode_{MutationMode::Disabled};
    bool runtime_refresh_enabled_{true};
    bool mutation_armed_{false};
    InteractivityMode interactivity_mode_{InteractivityMode::Observe};
    int launch_window_ms_{2500};
    double jank_miss_rate_{0.05};
    long boost_readahead_kb_{512};
    long boost_uclamp_launch_{20};
    long boost_uclamp_jank_{10};
    int boost_gpu_steps_{1};
    int boost_window_ms_{3000};

};

const char* mutationModeName(MutationMode mode) noexcept;

} // namespace coreflow
