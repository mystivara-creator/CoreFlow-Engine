#include "coreflow/resource_actuator.hpp"

#include <fstream>
#include <string>
#include <string_view>
#include <unistd.h>

namespace coreflow {
namespace {

bool writeAndVerify(std::string_view path, std::string_view requested) noexcept {
    try {
        std::ofstream file(std::string(path), std::ios::out | std::ios::trunc);
        if (!file) return false;
        file << requested;
        file.flush();
        if (!file.good()) return false;
        file.close();

        std::string observed;
        if (!ResourceActuator::read(path, observed)) return false;
        return observed == requested;
    } catch (...) {
        return false;
    }
}

} // namespace

bool ResourceActuator::read(std::string_view path, std::string& value) noexcept {
    try {
        std::ifstream file{std::string(path)};
        if (!file) return false;
        std::getline(file, value);
        return !value.empty();
    } catch (...) {
        return false;
    }
}

bool ResourceActuator::writable(std::string_view path) noexcept {
    return access(std::string(path).c_str(), W_OK) == 0;
}

ActuatorResult ResourceActuator::apply(const ActuatorMutation& mutation,
                                       const MutationPermit& permit) noexcept {
    ActuatorResult result;
    if (!permit.validFor(MutationPermit::Scope::Resource) ||
        mutation.target.empty() || mutation.requested.empty()) {
        result.status = ActuatorStatus::SafetyRejected;
        return result;
    }

    std::string current;
    if (!read(mutation.target, current)) {
        result.status = ActuatorStatus::ReadFailed;
        return result;
    }

    if (current == mutation.requested) {
        result.status = ActuatorStatus::NoChange;
        return result;
    }

    if (!writable(mutation.target)) {
        result.status = ActuatorStatus::SafetyRejected;
        return result;
    }

    result.writes_attempted = 1;
    if (!writeAndVerify(mutation.target, mutation.requested)) {
        result.status = ActuatorStatus::VerifyFailed;
        return result;
    }

    result.writes_verified = 1;
    result.status = ActuatorStatus::Ok;
    return result;
}

ActuatorResult ResourceActuator::restore(std::string_view path,
                                         std::string_view baseline) noexcept {
    ActuatorResult result;
    if (path.empty() || baseline.empty()) {
        result.status = ActuatorStatus::Invalid;
        return result;
    }

    std::string current;
    if (!read(path, current)) {
        result.status = ActuatorStatus::ReadFailed;
        return result;
    }

    if (current == baseline) {
        result.status = ActuatorStatus::NoChange;
        return result;
    }

    if (!writable(path)) {
        result.status = ActuatorStatus::SafetyRejected;
        return result;
    }

    result.writes_attempted = 1;
    if (!writeAndVerify(path, baseline)) {
        result.status = ActuatorStatus::VerifyFailed;
        return result;
    }

    result.writes_verified = 1;
    result.status = ActuatorStatus::RolledBack;
    return result;
}

} // namespace coreflow
