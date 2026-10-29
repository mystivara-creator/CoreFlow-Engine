#include "coreflow/foreground_tracker.hpp"

#include <array>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

namespace coreflow::observe {
namespace {
ssize_t readBounded(const std::string& path, char* buffer, std::size_t size) noexcept {
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return -1;
    ssize_t count;
    do { count = ::read(fd, buffer, size); } while (count < 0 && errno == EINTR);
    ::close(fd);
    return count;
}
}

bool ForegroundTracker::poll(std::string& package_out) noexcept {
    available_ = false;
    try {
        std::array<char, 4096> buffer{};
        const auto n = readBounded(procs_path_, buffer.data(), buffer.size());
        if (n < 0 || static_cast<std::size_t>(n) == buffer.size()) return false;
        if (n == 0) { last_pid_ = 0U; identity_.clear(); return false; }
        const std::string_view members(buffer.data(), static_cast<std::size_t>(n));
        std::uint32_t first = 0;
        if (!parseFirstPid(members, first) || first == 0U) return false;
        std::uint32_t pid = first;
        std::size_t pos = 0;
        // cgroup row order is not an activity ordering. Preserve a known member.
        while (last_pid_ != 0U && pos < members.size()) {
            auto end = members.find('\n', pos);
            if (end == std::string_view::npos) end = members.size();
            std::uint32_t member = 0;
            if (parseFirstPid(members.substr(pos, end - pos), member) && member == last_pid_) {
                pid = member; break;
            }
            pos = end + 1U;
        }
        const std::string path = proc_root_ + "/" + std::to_string(pid) + "/cmdline";
        std::array<char, 256> cmdline{};
        const auto count = readBounded(path, cmdline.data(), cmdline.size());
        if (count <= 0) return false;
        const std::string_view raw(cmdline.data(), static_cast<std::size_t>(count));
        const auto end = raw.find('\0');
        if (end == 0U || end == std::string_view::npos) return false;
        const auto identity = raw.substr(0, end);
        const bool changed = pid != last_pid_ || identity != identity_;
        if (changed) {
            // All allocations precede committing the observed identity.
            std::string resolved(identity);
            package_out = resolved;
            identity_.swap(resolved);
            last_pid_ = pid;
        }
        available_ = true;
        return changed;
    } catch (...) {
        return false; // optional telemetry must never terminate the daemon
    }
}
} // namespace coreflow::observe
