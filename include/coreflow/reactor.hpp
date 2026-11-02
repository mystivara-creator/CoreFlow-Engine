/**
 * @file reactor.hpp
 * @brief High-Performance Zero-Polling Epoll Event Reactor
 * @author mystivara-creator
 * @license Apache-2.0
 * 
 * Replaces traditional CPU-wasting 'while(true) sleep(100ms)' polling loops
 * with kernel-driven epoll_wait(), consuming 0.00% CPU when idle.
 */

#pragma once

#include "types.hpp"
#include <sys/epoll.h>
#include <unistd.h>
#include <functional>
#include <unordered_map>
#include <memory>
#include <span>

namespace coreflow {

class UniqueFd {
public:
    constexpr UniqueFd() noexcept : fd_(-1) {}
    explicit UniqueFd(int fd) noexcept : fd_(fd) {}
    ~UniqueFd() { reset(); }

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    UniqueFd(UniqueFd&& other) noexcept : fd_(other.release()) {}
    UniqueFd& operator=(UniqueFd&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    [[nodiscard]] int get() const noexcept { return fd_; }
    [[nodiscard]] bool valid() const noexcept { return fd_ >= 0; }
    explicit operator bool() const noexcept { return valid(); }

    int release() noexcept {
        int temp = fd_;
        fd_ = -1;
        return temp;
    }

    void reset(int new_fd = -1) noexcept {
        if (fd_ >= 0) {
            ::close(fd_);
        }
        fd_ = new_fd;
    }

private:
    int fd_;
};

using EventCallback = std::function<void(uint32_t events)>;

class EpollReactor {
public:
    EpollReactor();
    ~EpollReactor();

    // Prevent copying
    EpollReactor(const EpollReactor&) = delete;
    EpollReactor& operator=(const EpollReactor&) = delete;

    /// Register a file descriptor with epoll interest mask
    bool register_fd(int fd, uint32_t events, EventCallback callback);

    /// Modify existing fd registration
    bool modify_fd(int fd, uint32_t events);

    /// Unregister fd
    bool unregister_fd(int fd);

    /// Create high-resolution kernel timerfd for periodic safety tick
    int create_timerfd(std::chrono::milliseconds interval, EventCallback callback);

    /// Run reactor event loop until stop requested
    void run();

    /// Stop reactor loop cleanly from signal handler or thread
    void stop();

    [[nodiscard]] bool is_running() const noexcept { return running_; }

private:
    UniqueFd epoll_fd_;
    bool running_{false};
    int stop_event_fd_{-1};
    std::unordered_map<int, EventCallback> callbacks_;
    static constexpr size_t MAX_EPOLL_EVENTS = 32;
};

} // namespace coreflow
