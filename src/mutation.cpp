#include "coreflow/mutation.hpp"

#include <algorithm>
#include <fstream>
#include <string>
#include <utility>
#include <unistd.h>

namespace coreflow {
namespace {

constexpr double kThermalStartC = 35.0;
constexpr double kThermalGuardC = 43.0;

struct GovernorTraits {
    double workload_affinity;
    double efficiency_affinity;
    double thermal_affinity;
};

bool governorTraits(const std::string& governor, GovernorTraits& traits) noexcept {
    if (governor == "performance") {
        traits = {1.00, 0.10, 0.10};
        return true;
    }
    if (governor == "walt") {
        traits = {0.90, 0.50, 0.55};
        return true;
    }
    if (governor == "schedutil") {
        traits = {0.78, 0.68, 0.78};
        return true;
    }
    if (governor == "conservative") {
        traits = {0.48, 0.86, 0.95};
        return true;
    }
    if (governor == "powersave") {
        traits = {0.18, 1.00, 1.00};
        return true;
    }
    return false;
}

double workloadSignal(const RuntimeSample& sample) noexcept {
    if (sample.cpu_utilization_available)
        return std::clamp(sample.cpu_utilization, 0.0, 1.0);

    // Load average is a fallback only. Scale it conservatively rather than
    // treating an absolute load value as a CPU percentage.
    return std::clamp(sample.load1 / 4.0, 0.0, 1.0);
}

double thermalPressure(const RuntimeSample& sample) noexcept {
    if (!sample.thermal_available) return 0.0;
    const double temperature =
        static_cast<double>(sample.thermal_millidegrees) / 1000.0;
    return std::clamp(
        (temperature - kThermalStartC) / (kThermalGuardC - kThermalStartC),
        0.0, 1.0);
}

bool mutationState(RuntimeState state) noexcept {
    return state == RuntimeState::Normal ||
           state == RuntimeState::Warming ||
           state == RuntimeState::Elevated ||
           state == RuntimeState::ThermalGuard;
}


bool readTextValue(
    const std::string& path,
    std::string& value
) noexcept {
    try {
        std::ifstream file(path);
        if (!file) return false;
        return static_cast<bool>(std::getline(file, value));
    } catch (...) {
        return false;
    }
}

} // namespace

bool MutationController::writeTextVerified(
    const std::string& path,
    const std::string& value
) noexcept {
    try {
        if (access(path.c_str(), W_OK) != 0) return false;

        std::string current;
        if (readTextValue(path, current) && current == value)
            return true;

        std::ofstream file(path);
        if (!file) return false;

        file << value << '\n';
        file.flush();
        if (!file.good()) return false;
        file.close();

        std::string actual;
        return readTextValue(path, actual) && actual == value;
    } catch (...) {
        return false;
    }
}

bool MutationController::governorAvailable(
    const CpuPolicy& policy,
    const std::string& governor
) const noexcept {
    return std::find(
        policy.available_governors.begin(),
        policy.available_governors.end(),
        governor
    ) != policy.available_governors.end();
}

void MutationController::discoverGovernorCandidates(
    const DeviceProfile& profile
) noexcept {
    candidates_.clear();

    for (const CpuPolicy& policy : profile.cpu_policies) {
        if (policy.path.empty() || !policy.readable ||
            !policy.governor_writable || policy.available_governors.empty()) {
            continue;
        }

        for (const std::string& governor : policy.available_governors) {
            if (governor.empty()) continue;

            GovernorCandidate candidate;
            candidate.policy_path = policy.path;
            candidate.governor = governor;
            candidate.available = true;
            candidate.writable = true;
            candidate.validated = false;
            candidates_.push_back(std::move(candidate));
        }
    }
}

void MutationController::captureBaseline(
    const DeviceProfile& profile
) noexcept {
    baseline_.clear();
    trial_.reset();
    baseline_captured_ = false;

    for (const CpuPolicy& policy : profile.cpu_policies) {
        if (!policy.path.empty() &&
            policy.readable &&
            policy.governor_writable &&
            !policy.governor.empty()) {
            baseline_[policy.path + "/scaling_governor"] =
                {policy.governor, true};
        }
    }

    discoverGovernorCandidates(profile);
    baseline_captured_ = true;
}

bool MutationController::registerValidatedGovernor(
    const std::string& policyPath,
    const std::string& governor
) noexcept {
    if (policyPath.empty() || governor.empty()) return false;

    GovernorCandidate* candidate = findCandidate(policyPath, governor);
    if (candidate == nullptr || !candidate->available || !candidate->writable)
        return false;

    candidate->validated = true;
    candidate->evidence_score = std::max(candidate->evidence_score, 1.0);
    return true;
}

MutationController::GovernorCandidate*
MutationController::findCandidate(
    const std::string& policyPath,
    const std::string& governor
) noexcept {
    for (GovernorCandidate& candidate : candidates_) {
        if (candidate.policy_path == policyPath &&
            candidate.governor == governor) {
            return &candidate;
        }
    }
    return nullptr;
}

double MutationController::scoreGovernor(
    const GovernorCandidate& candidate,
    const RuntimeSample& sample,
    RuntimeState state
) const noexcept {
    GovernorTraits traits {};
    if (!governorTraits(candidate.governor, traits))
        return -1.0;

    const double workload = workloadSignal(sample);
    const double thermal = thermalPressure(sample);

    // Low workload rewards efficiency; high workload rewards responsiveness.
    double score =
        (workload * traits.workload_affinity) +
        ((1.0 - workload) * traits.efficiency_affinity);

    // Thermal pressure increasingly dominates the decision as temperature
    // rises. This is a bounded heuristic, not empirical proof of superiority.
    score = (score * (1.0 - thermal)) +
            (traits.thermal_affinity * thermal);

    // State-specific safety/latency bias.
    switch (state) {
        case RuntimeState::ThermalGuard:
            if (candidate.governor == "performance") score -= 0.30;
            if (candidate.governor == "powersave" ||
                candidate.governor == "conservative") score += 0.10;
            break;
        case RuntimeState::Warming:
            if (candidate.governor == "performance") score -= 0.12;
            if (candidate.governor == "powersave" ||
                candidate.governor == "conservative") score += 0.05;
            break;
        case RuntimeState::Elevated:
            if (candidate.governor == "performance" ||
                candidate.governor == "walt") score += 0.06;
            break;
        case RuntimeState::Normal:
        case RuntimeState::Idle:
        case RuntimeState::Pressure:
            break;
    }

    // Explicitly validated candidates may contribute prior device-specific
    // evidence, but Adaptive mode never requires that evidence to act.
    if (candidate.validated)
        score += std::clamp(candidate.evidence_score, 0.0, 1.0) * 0.10;

    return std::clamp(score, 0.0, 1.0);
}

const MutationController::GovernorCandidate* MutationController::selectGovernor(
    const CpuPolicy& policy,
    const RuntimeSample& sample,
    RuntimeState state
) const noexcept {
    const GovernorCandidate* best = nullptr;
    double bestScore = -1.0;

    for (const GovernorCandidate& candidate : candidates_) {
        if (candidate.policy_path != policy.path ||
            !candidate.available ||
            !candidate.writable) {
            continue;
        }

        const double score = scoreGovernor(candidate, sample, state);
        if (score < 0.0) continue;

        if (best == nullptr ||
            score > bestScore ||
            (score == bestScore && candidate.governor < best->governor)) {
            best = &candidate;
            bestScore = score;
        }
    }

    return best;
}

std::size_t MutationController::validatedCandidateCount() const noexcept {
    return static_cast<std::size_t>(std::count_if(
        candidates_.begin(),
        candidates_.end(),
        [](const GovernorCandidate& candidate) {
            return candidate.validated;
        }
    ));
}

bool MutationController::buildPlan(
    RuntimeState state,
    const RuntimeSample& sample,
    const DeviceProfile& profile,
    std::vector<MutationPlanEntry>& plan
) const noexcept {
    plan.clear();

    if (!mutationState(state))
        return false;

    for (const CpuPolicy& policy : profile.cpu_policies) {
        if (policy.path.empty() || !policy.readable ||
            !policy.governor_writable || policy.governor.empty()) {
            continue;
        }

        const std::string governorPath = policy.path + "/scaling_governor";
        const auto baseline = baseline_.find(governorPath);
        if (baseline == baseline_.end() || !baseline->second.valid)
            continue;

        const GovernorCandidate* best =
            selectGovernor(policy, sample, state);
        if (best == nullptr) continue;

        std::string current;
        if (!readTextValue(governorPath, current))
            current = policy.governor;

        if (current == best->governor)
            continue;

        // Hysteresis: do not switch governors for a marginal score gain.
        const GovernorCandidate* currentCandidate = nullptr;
        for (const GovernorCandidate& candidate : candidates_) {
            if (candidate.policy_path == policy.path &&
                candidate.governor == current &&
                candidate.available &&
                candidate.writable) {
                currentCandidate = &candidate;
                break;
            }
        }

        if (currentCandidate != nullptr) {
            const double bestScore = scoreGovernor(*best, sample, state);
            const double currentScore =
                scoreGovernor(*currentCandidate, sample, state);
            if (bestScore <= currentScore + kAdaptiveHysteresisMargin)
                continue;
        }

        plan.push_back({
            governorPath,
            best->governor,
            baseline->second.value
        });
    }

    return !plan.empty();
}

MutationResult MutationController::applyPlan(
    const std::vector<MutationPlanEntry>& plan
) noexcept {
    if (plan.empty()) return MutationResult::Skipped;

    std::vector<std::string> touched;
    touched.reserve(plan.size());

    for (const MutationPlanEntry& entry : plan) {
        if (!writeTextVerified(entry.path, entry.target)) {
            bool rollbackOk = true;
            for (const std::string& path : touched) {
                const auto baseline = baseline_.find(path);
                if (baseline == baseline_.end() || !baseline->second.valid ||
                    !writeTextVerified(path, baseline->second.value)) {
                    rollbackOk = false;
                }
            }
            return rollbackOk && !touched.empty()
                ? MutationResult::RolledBack
                : MutationResult::Failed;
        }
        touched.push_back(entry.path);
    }

    return MutationResult::Verified;
}

MutationResult MutationController::restoreGovernors() noexcept {
    bool restoredAny = false;
    bool failed = false;

    for (const auto& entry : baseline_) {
        if (!entry.second.valid) continue;

        std::string current;
        if (readTextValue(entry.first, current) &&
            current == entry.second.value) {
            // Already at baseline; avoid an unnecessary sysfs write.
            continue;
        }

        if (!writeTextVerified(entry.first, entry.second.value)) {
            failed = true;
            continue;
        }

        restoredAny = true;
    }

    if (failed) {
        return restoredAny
            ? MutationResult::RolledBack
            : MutationResult::Failed;
    }

    return restoredAny
        ? MutationResult::Verified
        : MutationResult::Skipped;
}

void MutationController::accumulate(
    TrialAccumulator& accumulator,
    const RuntimeSample& sample
) noexcept {
    if (sample.thermal_available) {
        accumulator.thermal_sum_c +=
            static_cast<double>(sample.thermal_millidegrees) / 1000.0;
    }
    accumulator.load_sum += sample.load1;
    accumulator.cpu_util_sum += sample.cpu_utilization;
    ++accumulator.samples;
}

bool MutationController::safetyWindowValid(
    RuntimeState state,
    const RuntimeSample& sample
) const noexcept {
    if (state != RuntimeState::Idle && state != RuntimeState::Normal)
        return false;
    if (sample.confidence < 0.70) return false;
    if (sample.thermal_available &&
        static_cast<double>(sample.thermal_millidegrees) / 1000.0 >=
            kTrialThermalCeilingC) {
        return false;
    }
    return true;
}

bool MutationController::startNextTrial(
    const DeviceProfile& profile
) noexcept {
    if (candidates_.empty()) return false;

    for (std::size_t i = 0; i < candidates_.size(); ++i) {
        GovernorCandidate& candidate = candidates_[i];
        if (!candidate.available || !candidate.writable) continue;

        const auto baseline = baseline_.find(
            candidate.policy_path + "/scaling_governor");
        if (baseline == baseline_.end() || !baseline->second.valid) continue;

        if (candidate.governor == baseline->second.value) continue;
        if (candidate.validated) continue;

        bool policyStillPresent = false;
        for (const CpuPolicy& policy : profile.cpu_policies) {
            if (policy.path == candidate.policy_path &&
                policy.governor_writable &&
                governorAvailable(policy, candidate.governor)) {
                policyStillPresent = true;
                break;
            }
        }
        if (!policyStillPresent) continue;

        trial_.active = true;
        trial_.phase = TrialPhase::Baseline;
        trial_.candidate_index = i;
        trial_.samples_remaining = kTrialBaselineSamples;
        trial_.policy_path = candidate.policy_path;
        trial_.governor_path = candidate.policy_path + "/scaling_governor";
        trial_.baseline_governor = baseline->second.value;
        trial_.baseline.reset();
        trial_.candidate.reset();
        return true;
    }

    return false;
}

bool MutationController::beginCandidateTrial(
    const DeviceProfile& profile
) noexcept {
    if (!trial_.active || trial_.candidate_index >= candidates_.size())
        return false;

    const GovernorCandidate& candidate = candidates_[trial_.candidate_index];
    for (const CpuPolicy& policy : profile.cpu_policies) {
        if (policy.path == trial_.policy_path &&
            governorAvailable(policy, candidate.governor) &&
            policy.governor_writable) {
            if (!writeTextVerified(trial_.governor_path, candidate.governor))
                return false;

            trial_.phase = TrialPhase::Candidate;
            trial_.samples_remaining = kTrialCandidateSamples;
            trial_.candidate.reset();
            return true;
        }
    }
    return false;
}

double MutationController::calculateEvidenceScore() const noexcept {
    if (trial_.baseline.samples == 0 || trial_.candidate.samples == 0)
        return 0.0;

    const double baseThermal =
        trial_.baseline.thermal_sum_c /
        static_cast<double>(trial_.baseline.samples);
    const double trialThermal =
        trial_.candidate.thermal_sum_c /
        static_cast<double>(trial_.candidate.samples);
    const double baseLoad =
        trial_.baseline.load_sum /
        static_cast<double>(trial_.baseline.samples);
    const double trialLoad =
        trial_.candidate.load_sum /
        static_cast<double>(trial_.candidate.samples);

    const double thermalDelta = trialThermal - baseThermal;
    const double loadDelta = trialLoad - baseLoad;

    const double thermalScore =
        std::clamp(1.0 - std::max(0.0, thermalDelta) / 2.0, 0.0, 1.0);
    const double loadScore =
        std::clamp(1.0 - std::max(0.0, loadDelta) / 2.0, 0.0, 1.0);

    return (thermalScore * 0.60) + (loadScore * 0.40);
}

MutationResult MutationController::completeTrial() noexcept {
    if (!trial_.active || trial_.candidate_index >= candidates_.size())
        return MutationResult::TrialAborted;

    const double score = calculateEvidenceScore();
    const double baseThermal = trial_.baseline.samples == 0
        ? 0.0
        : trial_.baseline.thermal_sum_c /
            static_cast<double>(trial_.baseline.samples);
    const double trialThermal = trial_.candidate.samples == 0
        ? 0.0
        : trial_.candidate.thermal_sum_c /
            static_cast<double>(trial_.candidate.samples);
    const double thermalDelta = trialThermal - baseThermal;

    const bool safe = thermalDelta <= kTrialMaxThermalRegressionC &&
                      score >= kTrialMinEvidenceScore;

    const bool restored =
        writeTextVerified(trial_.governor_path, trial_.baseline_governor);

    GovernorCandidate& candidate = candidates_[trial_.candidate_index];
    candidate.evidence_score = score;
    ++candidate.completed_trials;
    candidate.validated = safe && restored;

    const MutationResult result =
        safe && restored ? MutationResult::TrialCompleted
                         : MutationResult::TrialRejected;
    trial_.reset();
    return result;
}

void MutationController::abortTrial() noexcept {
    if (!trial_.active) return;
    writeTextVerified(trial_.governor_path, trial_.baseline_governor);
    trial_.reset();
}

// Experimental path retained for controlled validation. Production Adaptive
// mode does not depend on this state machine.
MutationResult MutationController::runTrial(
    RuntimeState state,
    const RuntimeSample& sample,
    const DeviceProfile& profile,
    const EngineConfig& config
) noexcept {
    if (!config.allowCpuGovernor() || sample.confidence < config.minConfidence())
        return MutationResult::Skipped;

    if (!baseline_captured_) captureBaseline(profile);

    if (!safetyWindowValid(state, sample)) {
        abortTrial();
        return MutationResult::TrialAborted;
    }

    if (!trial_.active && !startNextTrial(profile))
        return MutationResult::Skipped;

    if (trial_.phase == TrialPhase::Baseline) {
        accumulate(trial_.baseline, sample);
        if (--trial_.samples_remaining > 0)
            return MutationResult::TrialObserving;

        if (!beginCandidateTrial(profile)) {
            abortTrial();
            return MutationResult::Failed;
        }
        return MutationResult::TrialApplied;
    }

    if (trial_.phase == TrialPhase::Candidate) {
        accumulate(trial_.candidate, sample);
        if (--trial_.samples_remaining > 0)
            return MutationResult::TrialObserving;
        return completeTrial();
    }

    (void)config;
    return MutationResult::TrialAborted;
}

MutationResult MutationController::apply(
    RuntimeState state,
    const RuntimeSample& sample,
    const DeviceProfile& profile,
    const EngineConfig& config
) noexcept {
    if (config.mutationMode() == MutationMode::Trial)
        return runTrial(state, sample, profile, config);

    if (config.mutationMode() != MutationMode::Adaptive)
        return MutationResult::Skipped;

    if (!baseline_captured_)
        captureBaseline(profile);

    // Idle and memory-pressure states do not justify CPU-governor mutation.
    // Restore the captured baseline instead of using a governor switch as a
    // proxy for memory management.
    if (state == RuntimeState::Idle || state == RuntimeState::Pressure)
        return restoreGovernors();

    // Fail closed whenever mutation is not permitted or telemetry confidence
    // is below the configured floor.
    if (!config.allowCpuGovernor() ||
        sample.confidence < config.minConfidence()) {
        return restoreGovernors();
    }

    std::vector<MutationPlanEntry> plan;
    if (!buildPlan(state, sample, profile, plan))
        return MutationResult::Skipped;

    return applyPlan(plan);
}

bool MutationController::restoreAll() noexcept {
    if (trial_.active) abortTrial();
    if (!baseline_captured_) return true;

    const MutationResult result = restoreGovernors();
    return result != MutationResult::Failed &&
           result != MutationResult::RolledBack;
}

} // namespace coreflow
