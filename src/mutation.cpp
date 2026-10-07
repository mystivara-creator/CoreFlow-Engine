#include "coreflow/mutation.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

namespace coreflow {
namespace {

constexpr double kAdaptiveHysteresis = 0.10;
constexpr double kThermalComfortC = 35.0;
constexpr double kThermalPressureC = 43.0;
constexpr double kLoadNormalization = 8.0;

const char* knownGovernor(const std::string& governor) noexcept {
    if (governor == "performance") return "performance";
    if (governor == "walt") return "walt";
    if (governor == "schedutil") return "schedutil";
    if (governor == "conservative") return "conservative";
    if (governor == "powersave") return "powersave";
    return nullptr;
}

} // namespace

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

    cpufreq_actuator_.setProfile(&profile);
    const ActuatorStatus actuatorStatus = cpufreq_actuator_.discover();
    actuator_ready_ = actuatorStatus == ActuatorStatus::Ok;

    baseline_captured_ = actuator_ready_ && !baseline_.empty();
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

const MutationController::GovernorCandidate*
MutationController::findValidatedCandidate(
    const std::string& policyPath
) const noexcept {
    const GovernorCandidate* best = nullptr;
    for (const GovernorCandidate& candidate : candidates_) {
        if (candidate.policy_path != policyPath ||
            !candidate.available || !candidate.writable ||
            !candidate.validated) {
            continue;
        }
        if (best == nullptr || candidate.evidence_score > best->evidence_score)
            best = &candidate;
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

double MutationController::governorScore(
    const std::string& governor,
    RuntimeState state,
    const RuntimeSample& sample
) const noexcept {
    if (knownGovernor(governor) == nullptr) return -1.0;

    const double workload = sample.cpu_utilization_available
        ? std::clamp(sample.cpu_utilization, 0.0, 1.0)
        : std::clamp(sample.load1 / kLoadNormalization, 0.0, 1.0);

    const double thermal = sample.thermal_available
        ? std::clamp(
            (static_cast<double>(sample.thermal_millidegrees) / 1000.0 -
             kThermalComfortC) / (kThermalPressureC - kThermalComfortC),
            0.0, 1.0)
        : 0.5;

    double score = 0.0;
    if (governor == "performance") {
        score = (0.70 * workload) + (0.30 * (1.0 - thermal));
    } else if (governor == "walt") {
        score = (0.60 * workload) + (0.25 * (1.0 - thermal)) + 0.15;
    } else if (governor == "schedutil") {
        score = (0.50 * workload) + (0.30 * (1.0 - thermal)) + 0.20;
    } else if (governor == "conservative") {
        score = (0.35 * (1.0 - workload)) + (0.45 * thermal) + 0.20;
    } else if (governor == "powersave") {
        score = (0.15 * (1.0 - workload)) + (0.70 * thermal) + 0.15;
    }

    switch (state) {
        case RuntimeState::ThermalGuard:
            if (governor == "powersave") score += 0.25;
            else if (governor == "conservative") score += 0.20;
            else if (governor == "schedutil") score += 0.10;
            else if (governor == "walt") score -= 0.10;
            else if (governor == "performance") score -= 0.30;
            break;
        case RuntimeState::Elevated:
            if (governor == "walt") score += 0.08;
            else if (governor == "schedutil" || governor == "performance") score += 0.05;
            else if (governor == "powersave") score -= 0.10;
            break;
        case RuntimeState::Warming:
            if (governor == "conservative" || governor == "schedutil") score += 0.08;
            else if (governor == "walt") score += 0.03;
            else if (governor == "performance") score -= 0.10;
            break;
        case RuntimeState::Normal:
            break;
        case RuntimeState::Idle:
        case RuntimeState::Pressure:
            return -1.0;
    }

    return std::clamp(score, 0.0, 1.0);
}

bool MutationController::buildPlan(
    RuntimeState state,
    const RuntimeSample& sample,
    const DeviceProfile& profile,
    std::vector<MutationPlanEntry>& plan
) const noexcept {
    plan.clear();

    // Idle and memory Pressure intentionally restore the captured baseline.
    // Governor selection is an adaptive workload/thermal decision, not a
    // direct memory-pressure remedy.
    if (state == RuntimeState::Idle || state == RuntimeState::Pressure)
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

        const double currentScore =
            governorScore(policy.governor, state, sample);
        std::string bestGovernor;
        double bestScore = -1.0;

        for (const std::string& governor : policy.available_governors) {
            double score = governorScore(governor, state, sample);

            // Experience is advisory only. Fresh runtime heuristics remain
            // dominant; historical evidence is blended only when a
            // compatible record exists for this exact candidate/context.
            if (score >= 0.0 && experience_memory_ != nullptr) {
                ExperienceMemory::CandidateIdentity identity;
                identity.key = policy.path + ":" + governor;

                ExperienceMemory::Context context;
                context.state = state;
                context.charging = sample.charging;
                context.thermal_trend = sample.thermal_trend;
                context.memory_trend = sample.memory_trend;
                context.load_trend = sample.load_trend;

                if (const ExperienceMemory::Record* record =
                        experience_memory_->find(identity, context);
                    record != nullptr && record->confidence > 0.0) {
                    const double memoryWeight =
                        std::clamp(record->confidence * 0.30, 0.0, 0.30);
                    score = (score * (1.0 - memoryWeight)) +
                            (record->score * memoryWeight);
                }
            }

            if (score > bestScore) {
                bestScore = score;
                bestGovernor = governor;
            }
        }

        if (bestGovernor.empty() || bestScore < 0.0) continue;

        // Hysteresis: keep the current governor unless the new candidate is
        // materially better. This prevents governor flapping around a score
        // boundary while still allowing clear workload/thermal changes.
        if (currentScore >= 0.0 &&
            bestScore < currentScore + kAdaptiveHysteresis) {
            continue;
        }

        if (bestGovernor == policy.governor) continue;

        plan.push_back({
            governorPath,
            bestGovernor,
            baseline->second.value
        });
    }

    return !plan.empty();
}

MutationResult MutationController::applyPlan(
    const std::vector<MutationPlanEntry>& plan
) noexcept {
    if (plan.empty() || !actuator_ready_)
        return MutationResult::Skipped;

    // v1.3 resource bound: one verified CPUFreq write per decision cycle.
    const MutationPlanEntry& entry = plan.front();

    ActuatorMutation mutation;
    mutation.id = cpufreq_actuator_.id();
    mutation.target = entry.path;
    mutation.requested = entry.target;

    const ActuatorResult result = cpufreq_actuator_.apply(mutation);

    if (!result.succeeded())
        return result.status == ActuatorStatus::RolledBack
            ? MutationResult::RolledBack
            : MutationResult::Failed;

    if (!result.changed())
        return MutationResult::Skipped;

    last_applied_governors_.clear();
    const std::string suffix = "/scaling_governor";
    if (entry.path.size() >= suffix.size() &&
        entry.path.compare(
            entry.path.size() - suffix.size(),
            suffix.size(),
            suffix
        ) == 0) {
        const std::string policyPath =
            entry.path.substr(0, entry.path.size() - suffix.size());
        last_applied_governors_.emplace_back(policyPath, entry.target);
    }

    return MutationResult::Verified;
}

void MutationController::setExperienceMemory(
    const ExperienceMemory* memory
) noexcept {
    experience_memory_ = memory;
}

void MutationController::rejectLastMutation() noexcept {
    for (const auto& entry : last_applied_governors_) {
        rejected_governors_[entry.first].insert(entry.second);
    }

    last_applied_governors_.clear();
}

MutationResult MutationController::restoreGovernors() noexcept {
    if (!actuator_ready_)
        return MutationResult::Skipped;

    bool restoredAny = false;
    bool failed = false;

    for (const auto& entry : baseline_) {
        if (!entry.second.valid)
            continue;

        const ActuatorResult result =
            cpufreq_actuator_.restore(entry.first);

        if (result.status == ActuatorStatus::NoChange)
            continue;

        if (!result.succeeded()) {
            failed = true;
            continue;
        }

        if (result.changed())
            restoredAny = true;
    }

    if (failed)
        return restoredAny
            ? MutationResult::RolledBack
            : MutationResult::Failed;

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
            if (!actuator_ready_)
                return false;

            ActuatorMutation mutation;
            mutation.id = cpufreq_actuator_.id();
            mutation.target = trial_.governor_path;
            mutation.requested = candidate.governor;

            const ActuatorResult result =
                cpufreq_actuator_.apply(mutation);
            if (!result.succeeded())
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

    const ActuatorResult restoreResult =
        actuator_ready_
            ? cpufreq_actuator_.restore(trial_.governor_path)
            : ActuatorResult{};
    const bool restored = restoreResult.succeeded();

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
    if (actuator_ready_)
        (void)cpufreq_actuator_.restore(trial_.governor_path);
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

    // Idle and memory Pressure restore the baseline. Active states use the
    // bounded adaptive selector. Confidence and permissions always gate
    // mutation before any sysfs write.
    if (state == RuntimeState::Idle || state == RuntimeState::Pressure)
        return restoreGovernors();

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
