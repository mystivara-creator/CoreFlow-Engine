#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

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

    // Registers evidence produced by a future controlled-trial engine.
    // The controller still verifies that the governor is actually exposed
    // by the target CPUFreq policy before accepting the registration.
    bool registerValidatedGovernor(
        const std::string& policyPath,
        const std::string& governor
    ) noexcept;

    std::size_t baselineSize() const noexcept { return baseline_.size(); }
    std::size_t candidateCount() const noexcept { return candidates_.size(); }
    std::size_t validatedCandidateCount() const noexcept;

private:
    struct Baseline {
        std::string value;
        bool valid{false};
    };

    struct GovernorCandidate {
        std::string policy_path;
        std::string governor;
        bool available{false};
        bool writable{false};
        bool validated{false};
    };

    struct MutationPlanEntry {
        std::string path;
        std::string target;
        std::string baseline;
    };

    bool writeTextVerified(const std::string& path,
                           const std::string& value) noexcept;
    void discoverGovernorCandidates(const DeviceProfile& profile) noexcept;
    bool governorAvailable(const CpuPolicy& policy,
                           const std::string& governor) const noexcept;
    const GovernorCandidate* findValidatedCandidate(
        const std::string& policyPath
    ) const noexcept;
    bool buildPlan(
        RuntimeState state,
        const DeviceProfile& profile,
        std::vector<MutationPlanEntry>& plan
    ) const noexcept;
    MutationResult applyPlan(
        const std::vector<MutationPlanEntry>& plan
    ) noexcept;
    MutationResult restoreGovernors() noexcept;

    std::unordered_map<std::string, Baseline> baseline_;
    std::vector<GovernorCandidate> candidates_;
    bool baseline_captured_{false};
};

} // namespace coreflow
