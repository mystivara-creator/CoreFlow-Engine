#include "coreflow/interactivity_policy.hpp"
#include <algorithm>
#include <cmath>

namespace coreflow {

std::vector<BoostIntent> InteractivityPolicy::evaluate(
    const understand::InteractivitySnapshot& snapshot,
    std::uint64_t now_ms) const noexcept {
    std::vector<BoostIntent> out;
    try {
        const auto ratio = [](double value) noexcept {
            return std::isfinite(value) && value >= 0.0 && value <= 1.0;
        };
        const bool gpu_pressure = snapshot.gpu_busy_available &&
            ratio(snapshot.gpu_busy_ratio) && snapshot.gpu_busy_ratio >= 0.80;
        const bool cpu_pressure = snapshot.cpu_pressure_available &&
            ratio(snapshot.cpu_pressure_ratio) && snapshot.cpu_pressure_ratio >= 0.05;
        switch (snapshot.state) {
            case understand::InteractionState::Launching: {
                if (snapshot.launch_deadline_ms <= now_ms) break;
                const std::uint64_t remaining = std::min(config_.boost_window_ms,
                    snapshot.launch_deadline_ms - now_ms);
                out.push_back({BoostKind::TopAppUclampMin, remaining,
                               config_.uclamp_launch, "launch window"});
                if (gpu_pressure) out.push_back({BoostKind::GpuMinFreqStep, remaining,
                               static_cast<long>(config_.gpu_steps), "launch gpu pressure"});
                if (config_.readahead_kb > 0L && ratio(snapshot.io_full_avg10) &&
                    snapshot.io_full_avg10 >= 0.01 && ratio(snapshot.memory_some_avg10) &&
                    snapshot.memory_some_avg10 < 0.05) {
                    out.push_back({BoostKind::IoReadaheadKb, remaining,
                                   config_.readahead_kb, "launch io prefetch"});
                }
                break;
            }
            case understand::InteractionState::Stuttering: {
                if (!snapshot.frame.valid) break;
                if (cpu_pressure) out.push_back({BoostKind::TopAppUclampMin, config_.boost_window_ms,
                               config_.uclamp_jank, "frame disturbance with cpu pressure"});
                if (gpu_pressure) {
                    out.push_back({BoostKind::GpuMinFreqStep, config_.boost_window_ms,
                                   static_cast<long>(config_.gpu_steps),
                                   "gpu bound jank"});
                }
                break;
            }
            case understand::InteractionState::Idle:
            case understand::InteractionState::Interactive:
                break;
        }
        return out;
    } catch (...) { return {}; }
}

} // namespace coreflow
