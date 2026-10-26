#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace coreflow {

inline constexpr const char* kCoreFlowVersion = "3.0.1";

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
    // Default-deny. A domain is writable only when its grant_<domain>=yes key
    // is present in the current config snapshot. allow_cpu_governor is retained
    // for compatibility/diagnostics but is not mutation authority. Missing keys
    // grant nothing; capability discovery alone never authorises a write.
    bool allowCpuGovernor() const noexcept { return allow_cpu_governor_; }
    bool mutationArmed() const noexcept { return mutation_armed_; }
    bool runtimeRefreshEnabled() const noexcept { return runtime_refresh_enabled_; }
    bool grantMemory() const noexcept { return grants_.memory; }
    bool grantBlockQueue() const noexcept { return grants_.block_queue; }
    bool grantScheduler() const noexcept { return grants_.scheduler; }
    bool grantGpu() const noexcept { return grants_.gpu; }
    bool grantCgroup() const noexcept { return grants_.cgroup; }
    bool grantCpuFreq() const noexcept { return grants_.cpufreq; }
    bool grantKernel() const noexcept { return grants_.kernel; }
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
    void setAllowCpuGovernor(bool enabled) noexcept { allow_cpu_governor_ = enabled; }
    void setMutationArmed(bool enabled) noexcept { mutation_armed_ = enabled; }
    void setRuntimeRefreshEnabled(bool enabled) noexcept { runtime_refresh_enabled_ = enabled; }
    void setGrantMemory(bool enabled) noexcept { grants_.memory = enabled; }
    void setGrantBlockQueue(bool enabled) noexcept { grants_.block_queue = enabled; }
    void setGrantScheduler(bool enabled) noexcept { grants_.scheduler = enabled; }
    void setGrantGpu(bool enabled) noexcept { grants_.gpu = enabled; }
    void setGrantCgroup(bool enabled) noexcept { grants_.cgroup = enabled; }
    void setGrantCpuFreq(bool enabled) noexcept { grants_.cpufreq = enabled; }
    void setGrantKernel(bool enabled) noexcept { grants_.kernel = enabled; }
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
    bool allow_cpu_governor_{false};
    struct GrantFlags {
        bool memory{false};
        bool block_queue{false};
        bool scheduler{false};
        bool gpu{false};
        bool cgroup{false};
        bool cpufreq{false};
        bool kernel{false};
    };
    GrantFlags grants_{};
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
