#include "coreflow/mutation.hpp"

#include <algorithm>
#include <fstream>
#include <string>
#include <unistd.h>

namespace coreflow {

bool MutationController::writeTextVerified(
    const std::string& path,
    const std::string& value
) noexcept {
    try {
        if (access(path.c_str(), W_OK) != 0) return false;

        std::ifstream before(path);
        if (before) {
            std::string current;
            std::getline(before, current);
            if (current == value) return true;
        }

        std::ofstream file(path);
        if (!file) return false;

        file << value << '\n';
        file.flush();
        if (!file.good()) return false;
        file.close();

        std::ifstream verify(path);
        if (!verify) return false;

        std::string actual;
        std::getline(verify, actual);
        return actual == value;
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
            // A candidate is not considered mutation-eligible merely because
            // the kernel exposes it. Validation must come from a controlled
            // trial or another explicit device-specific policy layer.
            candidate.validated = false;
            candidates_.push_back(std::move(candidate));
        }
    }
}

void MutationController::captureBaseline(
    const DeviceProfile& profile
) noexcept {
    baseline_.clear();
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

    for (GovernorCandidate& candidate : candidates_) {
        if (candidate.policy_path == policyPath &&
            candidate.governor == governor &&
            candidate.available && candidate.writable) {
            candidate.validated = true;
            return true;
        }
    }

    return false;
}

const MutationController::GovernorCandidate*
MutationController::findValidatedCandidate(
    const std::string& policyPath
) const noexcept {
    for (const GovernorCandidate& candidate : candidates_) {
        if (candidate.policy_path == policyPath &&
            candidate.available && candidate.writable &&
            candidate.validated) {
            return &candidate;
        }
    }
    return nullptr;
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
    const DeviceProfile& profile,
    std::vector<MutationPlanEntry>& plan
) const noexcept {
    plan.clear();

    if (state != RuntimeState::Idle && state != RuntimeState::Normal)
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

        const GovernorCandidate* candidate =
            findValidatedCandidate(policy.path);
        if (candidate == nullptr ||
            candidate->governor == baseline->second.value) {
            continue;
        }

        if (!governorAvailable(policy, candidate->governor)) continue;

        plan.push_back({
            governorPath,
            candidate->governor,
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
    bool touched = false;
    bool failed = false;

    for (const auto& entry : baseline_) {
        if (!entry.second.valid) continue;

        if (!writeTextVerified(entry.first, entry.second.value)) {
            failed = true;
            continue;
        }

        touched = true;
    }

    if (failed) return touched ? MutationResult::RolledBack
                               : MutationResult::Failed;
    return touched ? MutationResult::Verified : MutationResult::Skipped;
}

MutationResult MutationController::apply(
    RuntimeState state,
    const RuntimeSample& sample,
    const DeviceProfile& profile,
    const EngineConfig& config
) noexcept {
    if (config.mutationMode() != MutationMode::Adaptive)
        return MutationResult::Skipped;

    if (!config.allowCpuGovernor())
        return MutationResult::Skipped;

    if (sample.confidence < config.minConfidence())
        return MutationResult::Skipped;

    if (!baseline_captured_)
        captureBaseline(profile);

    // Protected states never experiment with a governor. They restore the
    // known baseline instead of guessing that another governor is safer.
    if (state == RuntimeState::ThermalGuard ||
        state == RuntimeState::Pressure) {
        return restoreGovernors();
    }

    // Warming/Elevated remain observation-only until a validated candidate
    // exists. This prevents load/thermal events from becoming arbitrary writes.
    if (state == RuntimeState::Warming ||
        state == RuntimeState::Elevated) {
        return MutationResult::Skipped;
    }

    std::vector<MutationPlanEntry> plan;
    if (!buildPlan(state, profile, plan))
        return MutationResult::Skipped;

    return applyPlan(plan);
}

bool MutationController::restoreAll() noexcept {
    if (!baseline_captured_) return true;

    const MutationResult result = restoreGovernors();
    return result != MutationResult::Failed &&
           result != MutationResult::RolledBack;
}

} // namespace coreflow
