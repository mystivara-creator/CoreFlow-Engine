#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "coreflow/interactivity_model.hpp"

namespace coreflow {

enum class BoostKind : std::uint8_t {
    GpuMinFreqStep,   // raise GPU devfreq min frequency by N steps
    TopAppUclampMin,  // raise top-app uclamp.min
    IoReadaheadKb,    // raise block queue read_ahead_kb
};

struct BoostIntent {
    BoostKind kind{BoostKind::TopAppUclampMin};
    std::uint64_t window_ms{0};
    long value{0};
    std::string reason;
};

// Maps the understood interaction state to bounded, self-expiring boost
// intents. This layer only proposes: authority, journal and actuator
// verification still decide whether anything is written, and the thermal
// guard discards every intent before execution.
class InteractivityPolicy final {
public:
    struct BoostConfig {
        std::uint64_t boost_window_ms{3000};
        long uclamp_launch{20};
        long uclamp_jank{10};
        long readahead_kb{512};
        int gpu_steps{1};
    };
    void setConfig(const BoostConfig& config) noexcept { config_ = config; }
    [[nodiscard]] std::vector<BoostIntent> evaluate(
        const understand::InteractivitySnapshot& snapshot,
        std::uint64_t now_ms) const noexcept;

private:
    BoostConfig config_{};
};

} // namespace coreflow
