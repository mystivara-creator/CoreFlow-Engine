#include "coreflow/foreground_tracker.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <array>

namespace coreflow::observe {

bool ForegroundTracker::poll(std::string& package_out) noexcept {
    const int fd = ::open(procs_path_.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return false;
    std::array<char, 4096> buffer{};
    const ssize_t n = ::read(fd, buffer.data(), buffer.size() - 1U);
    ::close(fd);
    if (n <= 0) return false;

    std::uint32_t pid = 0;
    if (!parseFirstPid(std::string_view(buffer.data(), static_cast<std::size_t>(n)), pid)) {
        return false;
    }
    if (pid == 0U || pid == last_pid_) return false;
    last_pid_ = pid;

    package_out.clear();
    const std::string cmdline_path =
        proc_root_ + "/" + std::to_string(pid) + "/cmdline";
    const int cfd = ::open(cmdline_path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (cfd >= 0) {
        std::array<char, 256> cmdline{};
        const ssize_t cn = ::read(cfd, cmdline.data(), cmdline.size() - 1U);
        ::close(cfd);
        if (cn > 0) {
            std::string_view raw(cmdline.data(), static_cast<std::size_t>(cn));
            const std::size_t end = raw.find('\0');
            package_out.assign(end == std::string_view::npos ? raw : raw.substr(0, end));
        }
    }
    return true;
}

} // namespace coreflow::observe
