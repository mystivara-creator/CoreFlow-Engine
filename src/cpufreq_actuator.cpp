#include "coreflow/cpufreq_actuator.hpp"

#include <algorithm>
#include <fstream>
#include <unistd.h>

namespace coreflow {
namespace {

constexpr std::string_view kGovernorSuffix = "/scaling_governor";
constexpr std::uint8_t kMaxWritesPerCycle = 1;

std::string makeGovernorPath(std::string_view policyPath) {
    std::string path;
    path.reserve(policyPath.size() + kGovernorSuffix.size());
    path.append(policyPath.data(), policyPath.size());
    path.append(kGovernorSuffix.data(), kGovernorSuffix.size());
    return path;
}

} // namespace

void CpuFreqActuator::setProfile(
    const DeviceProfile* profile
) noexcept {
    if (profile_ == profile) return;

    profile_ = profile;
    baselines_.clear();
    observation_current_.clear();
    observation_baseline_.clear();
}

ActuatorCapability CpuFreqActuator::capability() const noexcept {
    ActuatorCapability capability;
    capability.id = id();
    capability.mode = ActuatorMode::Observational;
    capability.readable = profile_ != nullptr;
    capability.writable = false;
    capability.verifiable = false;
    capability.rollbackable = false;
    capability.max_writes_per_cycle = 0;

    // Autonomous mode is intentionally enabled only after discover() proves
    // every prerequisite for the specific device/profile.
    if (profile_ != nullptr) {
        bool anySafePolicy = false;

        for (const CpuPolicy& policy : profile_->cpu_policies) {
            if (!policy.path.empty() &&
                policy.readable &&
                policy.governor_writable &&
                !policy.governor.empty() &&
                !policy.available_governors.empty()) {
                anySafePolicy = true;
                break;
            }
        }

        if (anySafePolicy) {
            capability.mode = ActuatorMode::Autonomous;
            capability.writable = true;
            capability.verifiable = true;
            capability.rollbackable = true;
            capability.max_writes_per_cycle = kMaxWritesPerCycle;
        }
    }

    return capability;
}

ActuatorStatus CpuFreqActuator::discover() noexcept {
    baselines_.clear();
    observation_current_.clear();
    observation_baseline_.clear();

    if (profile_ == nullptr)
        return ActuatorStatus::Unavailable;

    bool discovered = false;

    try {
        for (const CpuPolicy& policy : profile_->cpu_policies) {
            if (policy.path.empty() ||
                !policy.readable ||
                !policy.governor_writable ||
                policy.governor.empty() ||
                policy.available_governors.empty()) {
                continue;
            }

            const std::string path = makeGovernorPath(policy.path);

            std::string current;
            if (!readText(path, current))
                continue;

            // The discovery snapshot is authoritative only if the actual
            // kernel value can be read successfully.
            if (current.empty())
                continue;

            baselines_.emplace(
                path,
                Baseline{std::move(current), true}
            );
            discovered = true;
        }
    } catch (...) {
        baselines_.clear();
        return ActuatorStatus::ReadFailed;
    }

    return discovered
        ? ActuatorStatus::Ok
        : ActuatorStatus::Unavailable;
}

ActuatorObservation CpuFreqActuator::observe(
    std::string_view target
) noexcept {
    observation_current_.clear();
    observation_baseline_.clear();

    ActuatorObservation result;
    result.id = id();
    result.target = target;

    if (profile_ == nullptr || target.empty())
        return result;

    const auto baseline = baselines_.find(std::string(target));
    if (baseline == baselines_.end() || !baseline->second.valid)
        return result;

    if (!readText(target, observation_current_))
        return result;

    observation_baseline_ = baseline->second.value;

    result.current = observation_current_;
    result.baseline = observation_baseline_;
    result.valid = true;
    result.baseline_valid = true;
    return result;
}

bool CpuFreqActuator::targetMatchesPolicy(
    const CpuPolicy& policy,
    std::string_view target
) const noexcept {
    if (policy.path.empty() || target.empty())
        return false;

    const std::string expected = makeGovernorPath(policy.path);
    return expected == target;
}

const CpuPolicy* CpuFreqActuator::findPolicy(
    std::string_view target
) const noexcept {
    if (profile_ == nullptr || target.empty())
        return nullptr;

    for (const CpuPolicy& policy : profile_->cpu_policies) {
        if (targetMatchesPolicy(policy, target))
            return &policy;
    }

    return nullptr;
}

bool CpuFreqActuator::targetAllowed(
    const CpuPolicy& policy,
    std::string_view target,
    std::string_view requested
) const noexcept {
    if (!targetMatchesPolicy(policy, target) ||
        !policy.readable ||
        !policy.governor_writable ||
        policy.available_governors.empty() ||
        requested.empty()) {
        return false;
    }

    return std::find(
        policy.available_governors.begin(),
        policy.available_governors.end(),
        requested
    ) != policy.available_governors.end();
}

ActuatorResult CpuFreqActuator::apply(
    const ActuatorMutation& mutation,
    const MutationPermit& permit
) noexcept {
    ActuatorResult result;

    if (!permit.validFor(MutationPermit::Scope::CpuFreq)) {
        result.status = ActuatorStatus::SafetyRejected;
        return result;
    }

    if (!(mutation.id == id())) {
        result.status = ActuatorStatus::Invalid;
        return result;
    }

    if (!capability().mutationSafe()) {
        result.status = ActuatorStatus::SafetyRejected;
        return result;
    }

    const CpuPolicy* policy = findPolicy(mutation.target);
    if (policy == nullptr) {
        result.status = ActuatorStatus::Unavailable;
        return result;
    }

    const auto baseline = baselines_.find(std::string(mutation.target));
    if (baseline == baselines_.end() || !baseline->second.valid) {
        result.status = ActuatorStatus::ValidationFailed;
        return result;
    }

    if (!targetAllowed(*policy, mutation.target, mutation.requested)) {
        result.status = ActuatorStatus::ValidationFailed;
        return result;
    }

    std::string current;
    if (!readText(mutation.target, current)) {
        result.status = ActuatorStatus::ReadFailed;
        return result;
    }

    if (current == mutation.requested) {
        result.status = ActuatorStatus::NoChange;
        return result;
    }

    result.writes_attempted = 1;

    if (!writeVerified(mutation.target, mutation.requested)) {
        result.status = ActuatorStatus::WriteFailed;
        return result;
    }

    std::string verified;
    if (!readText(mutation.target, verified)) {
        (void)writeVerified(mutation.target, baseline->second.value);
        result.status = ActuatorStatus::VerifyFailed;
        return result;
    }

    if (verified != mutation.requested) {
        const bool rolledBack =
            writeVerified(mutation.target, baseline->second.value);

        result.status = rolledBack
            ? ActuatorStatus::RolledBack
            : ActuatorStatus::VerifyFailed;
        return result;
    }

    result.writes_verified = 1;
    result.status = ActuatorStatus::Ok;
    return result;
}

ActuatorResult CpuFreqActuator::restore(
    std::string_view target
) noexcept {
    ActuatorResult result;

    if (profile_ == nullptr || target.empty()) {
        result.status = ActuatorStatus::Unavailable;
        return result;
    }

    const auto baseline = baselines_.find(std::string(target));
    if (baseline == baselines_.end() || !baseline->second.valid) {
        result.status = ActuatorStatus::ValidationFailed;
        return result;
    }

    return restoreTo(target, baseline->second.value);
}

ActuatorResult CpuFreqActuator::restoreTo(
    std::string_view target,
    std::string_view value
) noexcept {
    ActuatorResult result;

    if (profile_ == nullptr || target.empty() || value.empty()) {
        result.status = ActuatorStatus::Unavailable;
        return result;
    }

    const CpuPolicy* policy = findPolicy(target);
    if (policy == nullptr) {
        result.status = ActuatorStatus::Unavailable;
        return result;
    }

    // Never write a governor the policy does not advertise. This keeps a
    // stale journal entry from forcing an unsupported value.
    // Recovery is fail-closed: without an explicit advertised governor list,
    // never write a value from a stale journal.
    if (!targetAllowed(*policy, target, value)) {
        result.status = ActuatorStatus::ValidationFailed;
        return result;
    }

    std::string current;
    if (!readText(target, current)) {
        result.status = ActuatorStatus::ReadFailed;
        return result;
    }

    if (current == value) {
        result.status = ActuatorStatus::NoChange;
        return result;
    }

    result.writes_attempted = 1;

    if (!writeVerified(target, value)) {
        result.status = ActuatorStatus::WriteFailed;
        return result;
    }

    std::string verified;
    if (!readText(target, verified) || verified != value) {
        result.status = ActuatorStatus::VerifyFailed;
        return result;
    }

    result.writes_verified = 1;
    result.status = ActuatorStatus::Ok;
    return result;
}

bool CpuFreqActuator::readText(
    std::string_view path,
    std::string& value
) noexcept {
    try {
        if (path.empty())
            return false;

        std::ifstream file(
            std::string(path),
            std::ios::in
        );

        if (!file)
            return false;

        return static_cast<bool>(std::getline(file, value));
    } catch (...) {
        return false;
    }
}

bool CpuFreqActuator::writeVerified(
    std::string_view path,
    std::string_view value
) noexcept {
    try {
        if (path.empty() || value.empty())
            return false;

        const std::string pathString(path);
        const std::string valueString(value);

        if (access(pathString.c_str(), W_OK) != 0)
            return false;

        std::string current;
        if (readText(path, current) && current == valueString)
            return true;

        std::ofstream file(pathString, std::ios::out | std::ios::trunc);
        if (!file)
            return false;

        file << valueString << '\n';
        file.flush();

        if (!file.good())
            return false;

        file.close();

        std::string actual;
        return readText(path, actual) && actual == valueString;
    } catch (...) {
        return false;
    }
}

} // namespace coreflow
