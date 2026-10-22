#include "coreflow/mutation/mutation.hpp"
#include "coreflow/internal/safe_path.hpp"
#include <cerrno>
#include <cmath>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {
namespace {
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
    AppliedChange a; a.proposal = p;
    double previous{}, target{};
    if (p.capability_id.empty() || p.path.empty() || p.path.front() != '/' ||
        p.target_value == p.previous_value ||
        (p.allowed_values.empty() &&
         (!parseFinite(p.previous_value, previous) || !parseFinite(p.target_value, target) ||
          !std::isfinite(p.minimum) || !std::isfinite(p.maximum) ||
          !(p.minimum < p.maximum) || previous < p.minimum || previous > p.maximum ||
          target < p.minimum || target > p.maximum || target == previous)) ||
        (!p.allowed_values.empty() &&
         (std::find(p.allowed_values.begin(), p.allowed_values.end(), p.previous_value) == p.allowed_values.end() ||
          std::find(p.allowed_values.begin(), p.allowed_values.end(), p.target_value) == p.allowed_values.end()))) {
        a.readback_value = "proposal rejected before opening resource";
        return a;
    }
    const int read_fd = detail::openNoSymlinkPath(p.path, O_RDONLY | O_NONBLOCK);
    if (read_fd < 0) return a;
    struct stat st{};
    if (::fstat(read_fd, &st) != 0 || !S_ISREG(st.st_mode)) { ::close(read_fd); return a; }
    std::string current_value;
    if (!readValueFd(read_fd, current_value) || !valueMatches(current_value, p.previous_value, p.allowed_values)) {
        a.readback_value = current_value; ::close(read_fd); return a;
    }
    const int write_fd = detail::openNoSymlinkPath(p.path, O_WRONLY | O_NONBLOCK);
    if (write_fd < 0 || !sameObject(read_fd, write_fd)) {
        if (write_fd >= 0) ::close(write_fd);
        ::close(read_fd); return a;
    }
    std::string final_check;
    if (!readValueFd(read_fd, final_check) || !valueMatches(final_check, p.previous_value, p.allowed_values)) {
        a.readback_value = final_check; ::close(write_fd); ::close(read_fd); return a;
    }
    a.write_attempted = true;
    a.write_succeeded = writeValueFd(write_fd, p.target_value);
    ::close(write_fd);
    if (!readValueFd(read_fd, a.readback_value)) a.write_succeeded = false;
    a.write_succeeded = a.write_succeeded && valueMatches(a.readback_value, p.target_value, p.allowed_values);
    ::close(read_fd);
    return a;
}

bool ResourceMutation::rollback(const AppliedChange& a, std::string& reason) const {
    if (!a.write_attempted) { reason = "No write was attempted; rollback not required"; return true; }
    const int read_fd = detail::openNoSymlinkPath(a.proposal.path, O_RDONLY | O_NONBLOCK);
    if (read_fd < 0) { reason = "Rollback could not safely open resource"; return false; }
    std::string current_value;
    if (!readValueFd(read_fd, current_value)) {
        ::close(read_fd); reason = "Rollback could not read current resource value"; return false;
    }
    if (valueMatches(current_value, a.proposal.previous_value, a.proposal.allowed_values)) {
        ::close(read_fd);
        reason = "Original value is already present; rollback not required";
        return true;
    }
    if (!valueMatches(current_value, a.proposal.target_value, a.proposal.allowed_values) &&
        !valueMatches(current_value, a.readback_value, a.proposal.allowed_values)) {
        ::close(read_fd); reason = "Rollback skipped because resource changed independently"; return false;
    }
    const int write_fd = detail::openNoSymlinkPath(a.proposal.path, O_WRONLY | O_NONBLOCK);
    if (write_fd < 0 || !sameObject(read_fd, write_fd)) {
        if (write_fd >= 0) ::close(write_fd);
        ::close(read_fd); reason = "Rollback refused: resource identity changed or path is unsafe"; return false;
    }
    std::string rechecked;
    if (!readValueFd(read_fd, rechecked) ||
        (!valueMatches(rechecked, a.proposal.target_value, a.proposal.allowed_values) &&
         !valueMatches(rechecked, a.readback_value, a.proposal.allowed_values))) {
        ::close(write_fd); ::close(read_fd);
        reason = "Rollback skipped because resource changed independently"; return false;
    }
    const bool wrote = writeValueFd(write_fd, a.proposal.previous_value);
    ::close(write_fd);
    std::string restored;
    const bool read_ok = readValueFd(read_fd, restored);
    ::close(read_fd);
    if (!wrote || !read_ok || !valueMatches(restored, a.proposal.previous_value, a.proposal.allowed_values)) {
        reason = "Rollback write or read-back verification failed"; return false;
    }
    reason = "Previous value restored and verified";
    return true;
}
}
