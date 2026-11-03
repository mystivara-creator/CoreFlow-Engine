/**
 * @file reactor.hpp
 * @brief High-Performance Zero-Polling Epoll Event Reactor
 * @author mystivara-creator
 * @license Apache-2.0
 */

#pragma once

#include "types.hpp"
#include "unique_fd.hpp"
#include <sys/epoll.h>
#include <unistd.h>
#include <functional>
#include <unordered_map>
#include <memory>
#include <span>

namespace coreflow {

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
    UniqueFd stop_event_fd_;
    std::unordered_map<int, EventCallback> callbacks_;
    static constexpr size_t MAX_EPOLL_EVENTS = 32;
};

} // namespace coreflow
