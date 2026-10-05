#include "coreflow/mutation.hpp"

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

void MutationController::captureBaseline(
    const DeviceProfile& profile
) noexcept {
    baseline_.clear();

    for (const CpuPolicy& policy : profile.cpu_policies) {
        if (!policy.path.empty() &&
            policy.readable &&
            policy.governor_writable &&
            !policy.governor.empty()) {
            baseline_[policy.path + "/scaling_governor"] =
                {policy.governor, true};
        }
    }

    baseline_captured_ = true;
}

MutationResult MutationController::restoreGovernors() noexcept {
    bool touched = false;

    for (const auto& entry : baseline_) {
        if (!entry.second.valid) continue;

        if (!writeTextVerified(entry.first, entry.second.value)) {
            return touched ? MutationResult::RolledBack
                           : MutationResult::Failed;
        }

        touched = true;
    }

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

    if (sample.confidence < config.minConfidence())
        return MutationResult::Skipped;

    if (!baseline_captured_)
        captureBaseline(profile);

    /*
     * Foundation safety rule:
     *
     * THERMAL_GUARD / PRESSURE means "reduce intervention", not
     * "force a different governor". The previous implementation could
     * replace a vendor governor such as WALT with schedutil here.
     *
     * A governor change is a policy decision that requires device/driver
     * validation first. Until that policy exists, the only mutation allowed
     * in a constrained state is restoring CoreFlow's captured baseline.
     */
    if (state == RuntimeState::ThermalGuard ||
        state == RuntimeState::Pressure) {
        return restoreGovernors();
    }

    if (state == RuntimeState::Idle ||
        state == RuntimeState::Normal) {
        return restoreGovernors();
    }

    return MutationResult::Skipped;
}

bool MutationController::restoreAll() noexcept {
    if (!baseline_captured_) return true;

    const MutationResult result = restoreGovernors();
    return result != MutationResult::Failed &&
           result != MutationResult::RolledBack;
}

} // namespace coreflow
