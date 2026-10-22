#include "coreflow/validation/validation.hpp"
#include "coreflow/internal/safe_path.hpp"
#include <cerrno>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {
namespace {
bool sameObject(int a, int b) {
    struct stat x{}, y{};
    return ::fstat(a, &x) == 0 && ::fstat(b, &y) == 0 &&
           S_ISREG(x.st_mode) && S_ISREG(y.st_mode) &&
           x.st_dev == y.st_dev && x.st_ino == y.st_ino;
}
bool parseFinite(const std::string& text, double& value) {
    char* end = nullptr;
    errno = 0;
    value = std::strtod(text.c_str(), &end);
    return errno == 0 && end != text.c_str() && end && *end == '\0' && std::isfinite(value);
}
}

bool CapabilitySafetyValidator::validate(const ChangeProposal& p, std::string& reason) const {
    if (p.capability_id.empty() || p.path.empty() || p.path.front() != '/') {
        reason = "Invalid resource identity"; return false;
    }

    const auto in_allowed_values = [&p](const std::string& value) {
        return !p.allowed_values.empty() &&
            std::find(p.allowed_values.begin(), p.allowed_values.end(), value) != p.allowed_values.end();
    };
    if (!p.allowed_values.empty()) {
        if (!in_allowed_values(p.previous_value) || !in_allowed_values(p.target_value)) {
            reason = "Discrete value is not advertised by the live kernel"; return false;
        }
    } else {
        double previous{}, target{};
        if (!parseFinite(p.previous_value, previous) || !parseFinite(p.target_value, target)) {
            reason = "Non-numeric mutation value"; return false;
        }
        if (!std::isfinite(p.minimum) || !std::isfinite(p.maximum) || p.minimum >= p.maximum ||
            !(p.minimum <= target && target <= p.maximum &&
              p.minimum <= previous && previous <= p.maximum)) {
            reason = "Requested change falls outside discovered bounds"; return false;
        }
        if (target == previous) { reason = "No-op change"; return false; }
    }
    if (p.target_value == p.previous_value) { reason = "No-op change"; return false; }

    const int read_fd = detail::openNoSymlinkPath(p.path, O_RDONLY | O_NONBLOCK);
    if (read_fd < 0) {
        reason = "Resource cannot be opened safely without following symlinks"; return false;
    }
    struct stat st{};
    if (::fstat(read_fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        ::close(read_fd); reason = "Resource is not a regular file"; return false;
    }
    const int write_fd = detail::openNoSymlinkPath(p.path, O_WRONLY | O_NONBLOCK);
    if (write_fd < 0 || !sameObject(read_fd, write_fd)) {
        if (write_fd >= 0) ::close(write_fd);
        ::close(read_fd); reason = "Resource is not safely writable or its identity changed"; return false;
    }
    ::close(write_fd);
    ::close(read_fd);

    reason = "Validated bounded target and no-symlink resource path; mutation rechecks object identity";
    return true;
}
}
