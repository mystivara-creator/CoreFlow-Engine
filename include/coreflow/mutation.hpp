#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>
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

    // Reject the targets from the most recent verified mutation after an efficiency regression.
    void rejectLastMutation() noexcept;

    // Explicit evidence registration for future external/device-specific
    // validation. Availability alone never creates a validated candidate.
    bool registerValidatedGovernor(
        const std::string& policyPath,
        const std::string& governor
    ) noexcept;

    std::size_t baselineSize() const noexcept { return baseline_.size(); }
    std::size_t candidateCount() const noexcept { return candidates_.size(); }
    std::size_t validatedCandidateCount() const noexcept;
    bool trialActive() const noexcept { return trial_.active; }

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
        double evidence_score{0.0};
        std::size_t completed_trials{0};
    };

    struct MutationPlanEntry {
        std::string path;
        std::string target;
        std::string baseline;
    };

    struct TrialAccumulator {
        double thermal_sum_c{0.0};
        double load_sum{0.0};
        double cpu_util_sum{0.0};
        std::size_t samples{0};

        void reset() noexcept {
            thermal_sum_c = 0.0;
            load_sum = 0.0;
            cpu_util_sum = 0.0;
            samples = 0;
        }
    };

    enum class TrialPhase {
        Idle,
        Baseline,
        Candidate
    };

    struct TrialState {
        bool active{false};
        TrialPhase phase{TrialPhase::Idle};
        std::size_t candidate_index{0};
        std::size_t samples_remaining{0};
        std::string policy_path;
        std::string governor_path;
        std::string baseline_governor;
        TrialAccumulator baseline;
        TrialAccumulator candidate;

        void reset() noexcept {
            active = false;
            phase = TrialPhase::Idle;
            candidate_index = 0;
            samples_remaining = 0;
            policy_path.clear();
            governor_path.clear();
            baseline_governor.clear();
            baseline.reset();
            candidate.reset();
        }
    };

    static constexpr std::size_t kTrialBaselineSamples = 3;
    static constexpr std::size_t kTrialCandidateSamples = 3;
    static constexpr double kTrialThermalCeilingC = 40.0;
    static constexpr double kTrialMaxThermalRegressionC = 0.75;
    static constexpr double kTrialMinEvidenceScore = 0.70;

    bool writeTextVerified(const std::string& path,
                           const std::string& value) noexcept;
    void discoverGovernorCandidates(const DeviceProfile& profile) noexcept;
    bool governorAvailable(const CpuPolicy& policy,
                           const std::string& governor) const noexcept;
    const GovernorCandidate* findValidatedCandidate(
        const std::string& policyPath
    ) const noexcept;
    GovernorCandidate* findCandidate(const std::string& policyPath,
                                     const std::string& governor) noexcept;
    bool buildPlan(
        RuntimeState state,
        const RuntimeSample& sample,
        const DeviceProfile& profile,
        std::vector<MutationPlanEntry>& plan
    ) const noexcept;
    double governorScore(const std::string& governor,
                         RuntimeState state,
                         const RuntimeSample& sample) const noexcept;
    MutationResult applyPlan(
        const std::vector<MutationPlanEntry>& plan
    ) noexcept;
    MutationResult restoreGovernors() noexcept;

    MutationResult runTrial(
        RuntimeState state,
        const RuntimeSample& sample,
        const DeviceProfile& profile,
        const EngineConfig& config
    ) noexcept;
    bool startNextTrial(const DeviceProfile& profile) noexcept;
    bool beginCandidateTrial(const DeviceProfile& profile) noexcept;
    void accumulate(TrialAccumulator& accumulator,
                    const RuntimeSample& sample) noexcept;
    bool safetyWindowValid(RuntimeState state,
                           const RuntimeSample& sample) const noexcept;
    MutationResult completeTrial() noexcept;
    void abortTrial() noexcept;
    double calculateEvidenceScore() const noexcept;

    std::unordered_map<std::string, Baseline> baseline_;
    std::vector<GovernorCandidate> candidates_;
    std::vector<std::pair<std::string, std::string>> last_applied_governors_;
    std::unordered_map<std::string, std::unordered_set<std::string>>
        rejected_governors_;
    TrialState trial_{};
    bool baseline_captured_{false};
};

} // namespace coreflow
