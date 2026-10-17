#include "coreflow/interactivity_policy.hpp"

namespace coreflow {

std::vector<BoostIntent> InteractivityPolicy::evaluate(
    const understand::InteractivitySnapshot& snapshot,
    std::uint64_t now_ms) const noexcept {
    std::vector<BoostIntent> out;
    switch (snapshot.state) {
        case understand::InteractionState::Launching: {
            const std::uint64_t remaining =
                snapshot.launch_deadline_ms > now_ms
                    ? snapshot.launch_deadline_ms - now_ms
                    : config_.boost_window_ms;
            out.push_back({BoostKind::TopAppUclampMin, remaining,
                           config_.uclamp_launch, "launch window"});
            out.push_back({BoostKind::GpuMinFreqStep, remaining,
                           static_cast<long>(config_.gpu_steps), "launch window"});
            if (config_.readahead_kb > 0L) {
                out.push_back({BoostKind::IoReadaheadKb, remaining,
                               config_.readahead_kb, "launch io prefetch"});
            }
            break;
        }
        case understand::InteractionState::Stuttering: {
            out.push_back({BoostKind::TopAppUclampMin, config_.boost_window_ms,
                           config_.uclamp_jank, "jank streak"});
            if (snapshot.gpu_busy_available && snapshot.gpu_busy_ratio >= 0.80) {
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
}

} // namespace coreflow
