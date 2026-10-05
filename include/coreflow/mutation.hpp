#pragma once

#include <string>
#include <unordered_map>

#include "coreflow/config.hpp"
#include "coreflow/types.hpp"

namespace coreflow {

class MutationController {
public:
    MutationController() = default;

    void captureBaseline(const DeviceProfile& profile) noexcept;
    MutationResult apply(
        RuntimeState state,
        const RuntimeSample& sample,
        const DeviceProfile& profile,
        const EngineConfig& config
    ) noexcept;
    bool restoreAll() noexcept;

    std::size_t baselineSize() const noexcept { return baseline_.size(); }

private:
    struct Baseline {
        std::string value;
        bool valid{false};
    };

    bool writeTextVerified(const std::string& path,
                           const std::string& value) noexcept;
    MutationResult restoreGovernors() noexcept;

    std::unordered_map<std::string, Baseline> baseline_;
    bool baseline_captured_{false};
};

} // namespace coreflow
