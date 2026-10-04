#include "coreflow/mutation.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>

namespace coreflow {

bool MutationController::writeTextVerified(const std::string& path,
                                           const std::string& value) noexcept {
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
        file << value;
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

bool MutationController::isGovernorSupported(
    const CpuPolicy& policy,
    const std::string& governor
) const noexcept {
    for (const auto& available : policy.available_governors) {
        if (available == governor) return true;
    }
    return false;
}

void MutationController::captureBaseline(const DeviceProfile& profile) noexcept {
    baseline_.clear();

    for (const CpuPolicy& policy : profile.cpu_policies) {
        if (!policy.path.empty() && policy.readable && policy.governor_writable && !policy.governor.empty()) {
            baseline_[policy.path + "/scaling_governor"] = {policy.governor, true};
        }
    }

    baseline_captured_ = true;
}

MutationResult MutationController::restoreGovernors() noexcept {
    bool touched = false;
    for (const auto& entry : baseline_) {
        if (!writeTextVerified(entry.first, entry.second.value)) {
            return touched ? MutationResult::RolledBack : MutationResult::Failed;
        }
        touched = true;
    }
    return touched ? MutationResult::Verified : MutationResult::Skipped;
}

MutationResult MutationController::applyThermalGuardGovernor(
    const DeviceProfile& profile,
    const EngineConfig& config
) noexcept {
    if (!config.allowCpuGovernor()) return MutationResult::Skipped;

    bool attempted = false;
    bool changed = false;

    for (const CpuPolicy& policy : profile.cpu_policies) {
        if (!policy.governor_writable || policy.path.empty()) continue;
        if (!isGovernorSupported(policy, "schedutil")) continue;
        if (policy.governor == "schedutil") continue;

        attempted = true;
        const std::string path = policy.path + "/scaling_governor";
        if (!writeTextVerified(path, "schedutil")) {
            restoreGovernors();
            return MutationResult::RolledBack;
        }
        changed = true;
    }

    if (!attempted) return MutationResult::Skipped;
    return changed ? MutationResult::Verified : MutationResult::Skipped;
}

MutationResult MutationController::apply(
    RuntimeState state,
    const RuntimeSample& sample,
    const DeviceProfile& profile,
    const EngineConfig& config
) noexcept {
    if (config.mutationMode() != MutationMode::Adaptive) return MutationResult::Skipped;
    if (sample.confidence < config.minConfidence()) return MutationResult::Skipped;
    if (!baseline_captured_) captureBaseline(profile);

    // Conservative production mutation: only a validated governor fallback
    // is applied during ThermalGuard. Other system controls remain read-only
    // until their semantics are proven for the target device/kernel.
    if (state == RuntimeState::ThermalGuard || state == RuntimeState::Pressure) {
        return applyThermalGuardGovernor(profile, config);
    }

    if (state == RuntimeState::Idle || state == RuntimeState::Normal) {
        return restoreGovernors();
    }

    return MutationResult::Skipped;
}

bool MutationController::restoreAll() noexcept {
    if (!baseline_captured_) return true;
    const MutationResult result = restoreGovernors();
    return result != MutationResult::Failed && result != MutationResult::RolledBack;
}

} // namespace coreflow
