#pragma once

#include <cerrno>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <vector>

namespace coreflow::detail {

// Open an absolute path without following symlinks in any component.
// O_PATH is intentional: Android may deny read-open of "/" or parent
// directories while still permitting path traversal. The resulting directory
// descriptors are used only as openat() anchors, never for data I/O.
inline int openNoSymlinkPath(const std::string& path, int final_flags) {
    if (path.size() < 2 || path.front() != '/' ||
        path.find('\0') != std::string::npos) {
        errno = EINVAL;
        return -1;
    }

    int dir = ::open("/", O_PATH | O_DIRECTORY | O_CLOEXEC);
    if (dir < 0) return -1;

    std::vector<std::string> parts;
    std::size_t start = 1;
    while (start < path.size()) {
        const auto slash = path.find('/', start);
        const auto end = slash == std::string::npos ? path.size() : slash;
        const auto part = path.substr(start, end - start);
        if (part.empty() || part == "." || part == "..") {
            ::close(dir);
            errno = EINVAL;
            return -1;
        }
        parts.push_back(part);
        if (slash == std::string::npos) break;
        start = slash + 1;
    }

    if (parts.empty()) {
        ::close(dir);
        errno = EINVAL;
        return -1;
    }

    for (std::size_t i = 0; i + 1 < parts.size(); ++i) {
        const int next = ::openat(dir, parts[i].c_str(),
                                  O_PATH | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        const int saved_errno = errno;
        ::close(dir);
        if (next < 0) {
            errno = saved_errno;
            return -1;
        }
        dir = next;
    }

    const int fd = ::openat(dir, parts.back().c_str(),
                            final_flags | O_NOFOLLOW | O_CLOEXEC);
    const int saved_errno = errno;
    ::close(dir);
    errno = saved_errno;
    return fd;
}

} // namespace coreflow::detail
