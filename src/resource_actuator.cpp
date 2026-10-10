#include "coreflow/resource_actuator.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
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
    return access(std::string(path).c_str(), W_OK) == 0;
}

bool ResourceActuator::mutationTargetAllowed(std::string_view path) noexcept {
    // Production allow-list: only known VM controls currently have reviewed
    // semantics and bounded effect priors. A path being writable, discovered,
    // or suggested by a model is not authorization to mutate it.
    static constexpr std::string_view approvedVmPaths[] = {
        "/proc/sys/vm/swappiness",
        "/proc/sys/vm/dirty_ratio",
        "/proc/sys/vm/dirty_background_ratio",
        "/proc/sys/vm/vfs_cache_pressure",
        "/proc/sys/vm/min_free_kbytes",
        "/proc/sys/vm/dirty_expire_centisecs",
        "/proc/sys/vm/dirty_writeback_centisecs",
    };
    for (const auto approved : approvedVmPaths) {
        if (path == approved) return true;
    }

#ifdef COREFLOW_HOST_TEST_FIXTURES
    // Tests use temporary files to model kernel nodes. This exception is
    // compiled only into the host-test library; it is absent from coreflowd.
    if (path.size() >= 6U && path.substr(0U, 5U) == "/tmp/") return true;
#endif
    // This also quarantines all block queue controls, including loop/dm and
    // physical storage nodes, without relying on their names or writability.
    return false;
}

ActuatorResult ResourceActuator::apply(const ActuatorMutation& mutation,
                                       const MutationPermit& permit) noexcept {
    ActuatorResult result;
    if (!permit.validFor(MutationPermit::Scope::Resource) ||
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

    if (!writable(mutation.target)) {
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
