/**
 * @file unique_fd.hpp
 * @brief Safe RAII File Descriptor Wrapper for Linux/Android Native Systems
 * @author mystivara-creator
 * @license Apache-2.0
 */

#pragma once

#include <cerrno>
#include <unistd.h>
#include <utility>

namespace coreflow {

class UniqueFd {
public:
    constexpr UniqueFd() noexcept : fd_(-1) {}
    explicit UniqueFd(int fd) noexcept : fd_(fd) {}

    ~UniqueFd() noexcept {
        reset();
    }

    // Disable copy semantics to prevent double close
    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    // Enable move semantics
    UniqueFd(UniqueFd&& other) noexcept : fd_(other.release()) {}

    UniqueFd& operator=(UniqueFd&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    [[nodiscard]] int get() const noexcept {
        return fd_;
    }

    [[nodiscard]] bool valid() const noexcept {
        return fd_ >= 0;
    }

    explicit operator bool() const noexcept {
        return valid();
    }

    int release() noexcept {
        int old = fd_;
        fd_ = -1;
        return old;
    }

    void reset(int new_fd = -1) noexcept {
        if (fd_ == new_fd) return;
        if (fd_ >= 0) {
            const int saved_errno = errno;
            // On Linux, close() always releases the descriptor even if EINTR is returned;
            // never retry close() to avoid closing a concurrently reused descriptor.
            (void)::close(fd_);
            errno = saved_errno;
        }
        fd_ = new_fd;
    }

private:
    int fd_{-1};
};

} // namespace coreflow
