#include "coreflow/durable_file.hpp"
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
namespace coreflow {
namespace { constexpr std::size_t kMaxBytes = 1024U * 1024U; }
bool readBoundedPrivateFile(const std::string& path, std::string& content) noexcept {
    content.clear();
    if (path.empty() || path.find('\0') != std::string::npos) return false;
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return false;
    try {
        struct stat st {};
        bool ok = ::fstat(fd, &st) == 0 && S_ISREG(st.st_mode) && st.st_size >= 0 &&
                  static_cast<std::uint64_t>(st.st_size) <= kMaxBytes;
        char buffer[4096];
        while (ok) {
            const auto n = ::read(fd, buffer, sizeof(buffer));
            if (n < 0 && errno == EINTR) continue;
            if (n < 0) { ok = false; break; }
            if (n == 0) break;
            const auto size = static_cast<std::size_t>(n);
            if (content.size() > kMaxBytes - size) { ok = false; break; }
            content.append(buffer, size);
        }
        (void)::close(fd);
        if (!ok) content.clear();
        return ok;
    } catch (...) { (void)::close(fd); content.clear(); return false; }
}
bool writeDurablePrivateFile(const std::string& path, const std::string& content) noexcept {
    if (path.empty() || path.find('\0') != std::string::npos || content.size() > kMaxBytes) return false;
    try {
        const auto slash = path.find_last_of('/');
        const std::string dir = slash == std::string::npos ? "." : slash == 0U ? "/" : path.substr(0, slash);
        const int directory = ::open(dir.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        if (directory < 0) return false;
        std::string temp;
        try { temp = path + ".tmp.XXXXXX"; } catch (...) { (void)::close(directory); return false; }
        const int fd = ::mkstemp(temp.data());
        if (fd < 0) { (void)::close(directory); return false; }
        bool ok = ::fcntl(fd, F_SETFD, FD_CLOEXEC) == 0 && ::fchmod(fd, 0600) == 0;
        std::size_t offset = 0;
        while (ok && offset < content.size()) {
            const auto n = ::write(fd, content.data() + offset, content.size() - offset);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) { ok = false; break; }
            offset += static_cast<std::size_t>(n);
        }
        if (ok) ok = ::fsync(fd) == 0;
        if (::close(fd) != 0) ok = false;
        if (ok) ok = ::rename(temp.c_str(), path.c_str()) == 0;
        if (ok) ok = ::fsync(directory) == 0;
        if (!ok) (void)::unlink(temp.c_str());
        (void)::close(directory);
        return ok;
    } catch (...) { return false; }
}
}
