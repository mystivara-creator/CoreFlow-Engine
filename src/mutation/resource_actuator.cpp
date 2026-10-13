#include "coreflow/resource_actuator.hpp"
#include "coreflow/fatal_surface.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {
namespace {

std::string trimmed(std::string_view raw) {
    std::size_t first = 0;
    std::size_t last = raw.size();
    while (first < last && (raw[first] == ' ' || raw[first] == '\t' ||
                            raw[first] == '\r' || raw[first] == '\n')) {
        ++first;
    }
    while (last > first && (raw[last - 1] == ' ' || raw[last - 1] == '\t' ||
                            raw[last - 1] == '\r' || raw[last - 1] == '\n')) {
        --last;
    }
    return std::string(raw.substr(first, last - first));
}

bool writeAndVerify(std::string_view path, std::string_view requested) noexcept {
    try {
        std::ofstream file(std::string(path), std::ios::out | std::ios::trunc);
        if (!file) return false;
        file << requested;
        file.flush();
        if (!file.good()) return false;
        file.close();

        std::string observed;
        if (!ResourceActuator::readComparable(path, observed)) return false;
        return observed == ResourceActuator::comparableValue(requested);
    } catch (...) {
        return false;
    }
}

} // namespace

std::string ResourceActuator::comparableValue(std::string_view raw) {
    const std::string text = trimmed(raw);
    std::istringstream stream(text);
    std::string part;
    while (stream >> part) {
        if (part.size() >= 3 && part.front() == '[' && part.back() == ']') {
            return part.substr(1, part.size() - 2);
        }
    }
    return text;
}

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

bool ResourceActuator::readComparable(std::string_view path, std::string& value) noexcept {
    std::string raw;
    if (!read(path, raw)) return false;
    value = comparableValue(raw);
    return !value.empty();
}

bool ResourceActuator::writable(std::string_view path) noexcept {
    const std::string target(path);
    if (access(target.c_str(), W_OK) != 0) return false;
    // Root passes access(W_OK) on read-only sysfs/procfs nodes because of
    // CAP_DAC_OVERRIDE; the kernel exposes real writability via the mode bits.
    struct stat st {};
    if (::stat(target.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) return false;
    return (st.st_mode & 0222) != 0;
}

bool ResourceActuator::mutationTargetAllowed(std::string_view path) noexcept {
    // Capability-driven: there is no per-parameter allow-list. Any kernel
    // tunable is admissible unless it belongs to a universally fatal class
    // (see FatalSurfaceGuard). The CPU governor node is owned by the separate
    // MutationController authority and is never written by this actuator.
    if (isFatalSurface(path)) return false;
    const auto slash = path.rfind('/');
    const std::string_view leaf = slash == std::string_view::npos ? path : path.substr(slash + 1);
    if (leaf == "scaling_governor") return false;
    return true;
}

ActuatorCapability ResourceActuator::capability(
    ActuatorId id, std::string_view target
) const noexcept {
    ActuatorCapability capability;
    capability.id = id;
    if (target.empty() || !mutationTargetAllowed(target) || !writable(target)) {
        return capability;
    }
    std::string current;
    if (!readComparable(target, current) || current.empty()) {
        return capability;
    }
    capability.mode = ActuatorMode::Autonomous;
    capability.readable = true;
    capability.writable = true;
    capability.verifiable = true;
    capability.rollbackable = true;
    capability.max_writes_per_cycle = 1;
    return capability;
}

ActuatorResult ResourceActuator::apply(const ActuatorMutation& mutation,
                                       const MutationPermit& permit) noexcept {
    ActuatorResult result;
    if (!permit.validFor(mutation) ||
        mutation.target.empty() || mutation.requested.empty() ||
        !mutationTargetAllowed(mutation.target)) {
        result.status = ActuatorStatus::SafetyRejected;
        return result;
    }

    const std::string requested = comparableValue(mutation.requested);

    std::string current;
    if (!readComparable(mutation.target, current)) {
        result.status = ActuatorStatus::ReadFailed;
        return result;
    }

    if (current == requested) {
        result.status = ActuatorStatus::NoChange;
        return result;
    }

    if (!writable(mutation.target) || isFatalValue(current, requested)) {
        result.status = ActuatorStatus::SafetyRejected;
        return result;
    }

    result.writes_attempted = 1;
    if (!writeAndVerify(mutation.target, requested)) {
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
    // Journals written before this fix may hold the raw bracketed line for
    // selector nodes. Normalize so restore always writes a value the kernel
    // accepts and verifies against the same comparable form.
    const std::string target = comparableValue(baseline);
    if (path.empty() || target.empty()) {
        result.status = ActuatorStatus::Invalid;
        return result;
    }

    std::string current;
    if (!readComparable(path, current)) {
        result.status = ActuatorStatus::ReadFailed;
        return result;
    }

    if (current == target) {
        result.status = ActuatorStatus::NoChange;
        return result;
    }

    if (!writable(path)) {
        result.status = ActuatorStatus::SafetyRejected;
        return result;
    }

    result.writes_attempted = 1;
    if (!writeAndVerify(path, target)) {
        result.status = ActuatorStatus::VerifyFailed;
        return result;
    }

    result.writes_verified = 1;
    result.status = ActuatorStatus::RolledBack;
    return result;
}

} // namespace coreflow
