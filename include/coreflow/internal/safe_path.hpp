#pragma once

#include <cerrno>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <vector>

namespace coreflow::detail {
namespace {

class ScopedFd final {
public:
    explicit ScopedFd(int fd = -1) noexcept : fd_(fd) {}

    ~ScopedFd() noexcept {
        reset();
    }

    ScopedFd(const ScopedFd&) = delete;
    ScopedFd& operator=(const ScopedFd&) = delete;

    [[nodiscard]] int get() const noexcept {
        return fd_;
    }

    [[nodiscard]] int release() noexcept {
        const int result = fd_;
        fd_ = -1;
        return result;
    }

    void reset(int next = -1) noexcept {
        if (fd_ >= 0) {
            // Do not retry close() after EINTR: descriptor state can be
            // platform-dependent, and retrying may close a reused descriptor.
            (void)::close(fd_);
        }
        fd_ = next;
    }

private:
    int fd_;
};

} // namespace

// Open an absolute path without following symlinks in any component.
// Path parsing completes before descriptors are opened. ScopedFd owns every
// directory descriptor, including during stack unwinding.
inline int openNoSymlinkPath(const std::string& path, int final_flags) {
    if (path.size() < 2 || path.front() != '/' ||
        path.find('\0') != std::string::npos) {
        errno = EINVAL;
        return -1;
    }

    std::vector<std::string> parts;
    std::size_t start = 1;

    while (start < path.size()) {
        const auto slash = path.find('/', start);
        const auto end = slash == std::string::npos ? path.size() : slash;
        const auto part = path.substr(start, end - start);

        if (part.empty() || part == "." || part == "..") {
            errno = EINVAL;
            return -1;
        }

        parts.push_back(part);

        if (slash == std::string::npos) {
            break;
        }
        start = slash + 1;
    }

    if (parts.empty()) {
        errno = EINVAL;
        return -1;
    }

    ScopedFd dir(::open("/", O_PATH | O_DIRECTORY | O_CLOEXEC));
    if (dir.get() < 0) {
        return -1;
    }

    for (std::size_t i = 0; i + 1 < parts.size(); ++i) {
        const int next = ::openat(
            dir.get(), parts[i].c_str(),
            O_PATH | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);

        if (next < 0) {
            return -1; // dir closes automatically
        }

        dir.reset(next); // closes the previous directory descriptor
    }

    const int fd = ::openat(
        dir.get(), parts.back().c_str(),
        final_flags | O_NOFOLLOW | O_CLOEXEC);

    if (fd < 0) {
        return -1;
    }

    return fd; // caller owns the final descriptor
}

} // namespace coreflow::detail
