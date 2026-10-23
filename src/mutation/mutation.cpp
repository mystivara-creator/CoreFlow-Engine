#include "coreflow/mutation/mutation.hpp"
#include "coreflow/internal/safe_path.hpp"
#include "coreflow/internal/rollback_guard.hpp"
#include <cerrno>
#include <cmath>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>

namespace coreflow {
namespace {

class ScopedFd final {
public:
    explicit ScopedFd(int fd = -1) noexcept : fd_(fd) {}
    ~ScopedFd() noexcept {
        if (fd_ >= 0) {
            // Do not retry close() after EINTR: descriptor state may vary
            // by platform, and retrying can close a reused descriptor.
            (void)::close(fd_);
        }
    }

    ScopedFd(const ScopedFd&) = delete;
    ScopedFd& operator=(const ScopedFd&) = delete;

    [[nodiscard]] int get() const noexcept { return fd_; }

private:
    int fd_;
};
bool parseFinite(const std::string& text, double& value) {
    char* end = nullptr;
    errno = 0;
    value = std::strtod(text.c_str(), &end);
    return errno == 0 && end != text.c_str() && end && *end == '\0' && std::isfinite(value);
}
bool equivalentValue(const std::string& a, const std::string& b) {
    double da{}, db{};
    if (parseFinite(a, da) && parseFinite(b, db)) return da == db;
    return a == b;
}
bool sameObject(int a, int b) {
    struct stat x{}, y{};
    return ::fstat(a, &x) == 0 && ::fstat(b, &y) == 0 &&
           S_ISREG(x.st_mode) && S_ISREG(y.st_mode) &&
           x.st_dev == y.st_dev && x.st_ino == y.st_ino;
}
bool readValueFd(int fd, std::string& value) {
    if (::lseek(fd, 0, SEEK_SET) < 0 && errno != ESPIPE) return false;
    char buf[257];
    ssize_t n;
    do { n = ::read(fd, buf, sizeof(buf) - 1); } while (n < 0 && errno == EINTR);
    if (n <= 0 || n >= static_cast<ssize_t>(sizeof(buf) - 1)) return false;
    buf[n] = '\0';
    value.assign(buf);
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return false;
    const auto last = value.find_last_not_of(" \t\r\n");
    value = value.substr(first, last - first + 1);
    return true;
}

std::string normalizeObservedValue(const std::string& raw,
                                  const std::vector<std::string>& allowed_values) {
    if (allowed_values.empty()) return raw;
    const auto left = raw.find('[');
    if (left != std::string::npos) {
        const auto right = raw.find(']', left + 1);
        if (right != std::string::npos && right > left + 1) {
            const auto active = raw.substr(left + 1, right - left - 1);
            if (std::find(allowed_values.begin(), allowed_values.end(), active) != allowed_values.end())
                return active;
        }
    }
    std::size_t end = raw.find_first_of(" \t\r\n");
    const auto first = raw.substr(0, end);
    return std::find(allowed_values.begin(), allowed_values.end(), first) != allowed_values.end()
        ? first : raw;
}
bool valueMatches(const std::string& observed, const std::string& expected,
                  const std::vector<std::string>& allowed_values) {
    return equivalentValue(normalizeObservedValue(observed, allowed_values), expected);
}

bool writeValueFd(int fd, const std::string& value) {
    // Ordinary files retain trailing bytes after shorter writes. Kernel control
    // files have their own semantics; never ftruncate sysfs/procfs/debugfs/tracefs.
    // Fail closed if filesystem type cannot be determined.
    struct statfs fs{};
    if (::fstatfs(fd, &fs) != 0) return false;
    constexpr long kProcSuperMagic = 0x9fa0;
    constexpr long kSysfsMagic = 0x62656572;
    constexpr long kDebugfsMagic = 0x64626720;
    constexpr long kTracefsMagic = 0x74726163;
    const long fs_type = static_cast<long>(fs.f_type);
    const bool kernel_control_fs = fs_type == kProcSuperMagic || fs_type == kSysfsMagic ||
                                   fs_type == kDebugfsMagic || fs_type == kTracefsMagic;
    if (!kernel_control_fs && ::ftruncate(fd, 0) != 0) return false;
    if (::lseek(fd, 0, SEEK_SET) < 0 && errno != ESPIPE) return false;
    std::size_t offset = 0;
    while (offset < value.size()) {
        const auto n = ::write(fd, value.data() + offset, value.size() - offset);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        offset += static_cast<std::size_t>(n);
    }
    return true;
}
}

AppliedChange ResourceMutation::apply(const ChangeProposal& p) const {
    AppliedChange a;
    a.proposal = p;

    // The guard starts disarmed and is armed immediately before the first write.
    // This closes the exception-safety gap between a successful kernel write and
    // the later read-back allocation/verification. A normal return disarms it so
    // the caller remains responsible for handling a reported write failure.
    internal::RollbackGuard rollback_guard(
        [this](const AppliedChange& change, std::string& reason) {
            return rollback(change, reason);
        }, a, false);

    double previous{}, target{};
    if (p.capability_id.empty() || p.path.empty() || p.path.front() != '/' ||
        p.target_value == p.previous_value ||
        (p.allowed_values.empty() &&
         (!parseFinite(p.previous_value, previous) ||
          !parseFinite(p.target_value, target) ||
          !std::isfinite(p.minimum) || !std::isfinite(p.maximum) ||
          !(p.minimum < p.maximum) ||
          previous < p.minimum || previous > p.maximum ||
          target < p.minimum || target > p.maximum ||
          target == previous)) ||
        (!p.allowed_values.empty() &&
         (std::find(p.allowed_values.begin(), p.allowed_values.end(),
                    p.previous_value) == p.allowed_values.end() ||
          std::find(p.allowed_values.begin(), p.allowed_values.end(),
                    p.target_value) == p.allowed_values.end()))) {
        a.readback_value = "proposal rejected before opening resource";
        return a;
    }

    ScopedFd read_fd(detail::openNoSymlinkPath(
        p.path, O_RDONLY | O_NONBLOCK));
    if (read_fd.get() < 0) return a;

    struct stat st{};
    if (::fstat(read_fd.get(), &st) != 0 || !S_ISREG(st.st_mode)) {
        return a;
    }

    std::string current_value;
    if (!readValueFd(read_fd.get(), current_value) ||
        !valueMatches(current_value, p.previous_value, p.allowed_values)) {
        a.readback_value = current_value;
        return a;
    }

    std::string final_check;
    {
        ScopedFd write_fd(detail::openNoSymlinkPath(
            p.path, O_WRONLY | O_NONBLOCK));
        if (write_fd.get() < 0 ||
            !sameObject(read_fd.get(), write_fd.get())) {
            return a;
        }

        if (!readValueFd(read_fd.get(), final_check) ||
            !valueMatches(final_check, p.previous_value, p.allowed_values)) {
            a.readback_value = final_check;
            return a;
        }

        a.write_attempted = true;
        rollback_guard.arm();
        a.write_succeeded = writeValueFd(write_fd.get(), p.target_value);
    }

    if (!readValueFd(read_fd.get(), a.readback_value)) {
        a.write_succeeded = false;
    }
    a.write_succeeded =
        a.write_succeeded &&
        valueMatches(a.readback_value, p.target_value, p.allowed_values);

    // The caller receives the complete transaction record and handles any
    // ordinary write/read-back failure. RAII rollback is reserved for exceptions.
    rollback_guard.dismiss();
    return a;
}

bool ResourceMutation::rollback(const AppliedChange& a,
                                std::string& reason) const {
    if (!a.write_attempted) {
        reason = "No write was attempted; rollback not required";
        return true;
    }

    ScopedFd read_fd(detail::openNoSymlinkPath(
        a.proposal.path, O_RDONLY | O_NONBLOCK));
    if (read_fd.get() < 0) {
        reason = "Rollback could not safely open resource";
        return false;
    }

    std::string current_value;
    if (!readValueFd(read_fd.get(), current_value)) {
        reason = "Rollback could not read current resource value";
        return false;
    }

    if (valueMatches(current_value, a.proposal.previous_value,
                     a.proposal.allowed_values)) {
        reason = "Original value is already present; rollback not required";
        return true;
    }

    if (!valueMatches(current_value, a.proposal.target_value,
                      a.proposal.allowed_values)) {
        reason = "Rollback skipped because resource changed independently";
        return false;
    }

    bool wrote = false;
    {
        ScopedFd write_fd(detail::openNoSymlinkPath(
            a.proposal.path, O_WRONLY | O_NONBLOCK));
        if (write_fd.get() < 0 ||
            !sameObject(read_fd.get(), write_fd.get())) {
            reason = "Rollback refused: resource identity changed or path is unsafe";
            return false;
        }

        std::string rechecked;
        if (!readValueFd(read_fd.get(), rechecked) ||
            !valueMatches(rechecked, a.proposal.target_value,
                          a.proposal.allowed_values)) {
            reason = "Rollback skipped because resource changed independently";
            return false;
        }

        wrote = writeValueFd(write_fd.get(), a.proposal.previous_value);
    }

    std::string restored;
    if (!wrote || !readValueFd(read_fd.get(), restored) ||
        !valueMatches(restored, a.proposal.previous_value,
                      a.proposal.allowed_values)) {
        reason = "Rollback write or read-back verification failed";
        return false;
    }

    reason = "Previous value restored and verified";
    return true;
}
}
