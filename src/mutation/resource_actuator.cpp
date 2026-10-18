#include "coreflow/resource_actuator.hpp"
#include "coreflow/fatal_surface.hpp"

#include <cerrno>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>

namespace coreflow {
namespace {

bool kernelTunableFilesystem(int fd) noexcept {
#ifdef COREFLOW_HOST_TEST_FIXTURES
    // Host tests emulate sysfs/procfs nodes with ordinary temporary files.
    (void)fd;
    return true;
#else
    struct statfs fs {};
    if (::fstatfs(fd, &fs) != 0) return false;
    const auto type = static_cast<unsigned long>(fs.f_type);
    // The observer walks mounted procfs (/proc/sys), sysfs, and cgroup
    // control trees. Restrict the final write gate to those kernel interfaces;
    // a syntactically valid journal must not grant authority over /data files.
    constexpr unsigned long kProcfsMagic = 0x00009FA0UL;
    constexpr unsigned long kSysfsMagic = 0x62656572UL;
    constexpr unsigned long kCgroupMagic = 0x0027E0EBUL;
    constexpr unsigned long kCgroup2Magic = 0x63677270UL;
    return type == kProcfsMagic || type == kSysfsMagic ||
           type == kCgroupMagic || type == kCgroup2Magic;
#endif
}

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
    if (path.empty() || requested.empty() || requested.size() > 4096U) return false;
    try {
        const std::string target(path);
        const int fd = ::open(target.c_str(), O_WRONLY | O_CLOEXEC | O_NOFOLLOW);
        if (fd < 0) return false;

        struct stat st {};
        const bool target_is_writable_regular_file =
            ::fstat(fd, &st) == 0 && S_ISREG(st.st_mode) &&
            (st.st_mode & 0222) != 0 && kernelTunableFilesystem(fd);
        if (!target_is_writable_regular_file) {
            (void)::close(fd);
            return false;
        }

        // Host fixtures emulate sysfs attributes with ordinary files, which
        // retain old trailing bytes unless truncated. This test-only behavior
        // occurs only after descriptor validation; production kernel pseudo-files
        // are written as a single attribute write without ftruncate.
#ifdef COREFLOW_HOST_TEST_FIXTURES
        if (::ftruncate(fd, 0) != 0) {
            (void)::close(fd);
            return false;
        }
#endif
        // Keep the privileged write on the descriptor that was checked. Do not
        // reopen the pathname through an ofstream after the earlier path check:
        // the final component could otherwise be replaced by a symlink in the gap.
        ssize_t written = -1;
        do {
            written = ::write(fd, requested.data(), requested.size());
        } while (written < 0 && errno == EINTR);
        const bool write_ok = written >= 0 &&
            static_cast<std::size_t>(written) == requested.size();
        const bool close_ok = ::close(fd) == 0;
        if (!write_ok || !close_ok) return false;

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
    // Do not follow a final-component symlink: privileged mutation must target
    // the discovered kernel node itself, not an arbitrary file redirected to
    // by a stale or substituted path.
    if (::lstat(target.c_str(), &st) != 0 || !S_ISREG(st.st_mode) ||
        (st.st_mode & 0222) == 0) return false;
    const int fd = ::open(target.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return false;
    struct stat opened {};
    const bool valid = ::fstat(fd, &opened) == 0 && S_ISREG(opened.st_mode) &&
                       (opened.st_mode & 0222) != 0 && kernelTunableFilesystem(fd);
    (void)::close(fd);
    return valid;
}

bool ResourceActuator::mutationTargetAllowed(std::string_view path) noexcept {
    // Capability-driven path-level gate: no per-parameter allow-list. The
    // final filesystem/descriptor gate independently requires a procfs, sysfs,
    // or cgroup control node. The CPU governor remains separately owned by the
    // MutationController authority.
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

    // Recovery is a write path too. A syntactically valid journal must never
    // bypass the same surface guard used by normal mutations; otherwise a
    // stale or tampered journal could write a fatal control or the separately
    // owned CPU governor. Validate before reading or writing the target.
    if (!mutationTargetAllowed(path)) {
        result.status = ActuatorStatus::SafetyRejected;
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
