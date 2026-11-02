/**
 * @file reactor.cpp
 * @brief Zero-polling Linux Epoll Reactor Implementation
 * @author mystivara-creator
 * @license Apache-2.0
 */

#include "coreflow/reactor.hpp"
#include <sys/timerfd.h>
#include <sys/eventfd.h>
#include <iostream>
#include <cstring>

namespace coreflow {

EpollReactor::EpollReactor() {
    epoll_fd_.reset(::epoll_create1(EPOLL_CLOEXEC));
    if (!epoll_fd_.valid()) {
        std::cerr << "[CoreFlow][Reactor] Failed to create epoll instance: "
                  << std::strerror(errno) << std::endl;
        return;
    }

    // Wakeup eventfd for lock-free stop signaling
    stop_event_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (stop_event_fd_ >= 0) {
        register_fd(stop_event_fd_, EPOLLIN, [this](uint32_t) {
            uint64_t val = 0;
            (void)::read(stop_event_fd_, &val, sizeof(val));
            running_ = false;
        });
    }
}

EpollReactor::~EpollReactor() {
    stop();
    if (stop_event_fd_ >= 0) {
        ::close(stop_event_fd_);
        stop_event_fd_ = -1;
    }
}

bool EpollReactor::register_fd(int fd, uint32_t events, EventCallback callback) {
    if (!epoll_fd_.valid() || fd < 0) return false;

    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;

    if (::epoll_ctl(epoll_fd_.get(), EPOLL_CTL_ADD, fd, &ev) < 0) {
        std::cerr << "[CoreFlow][Reactor] EPOLL_CTL_ADD failed for fd " << fd 
                  << ": " << std::strerror(errno) << std::endl;
        return false;
    }

    callbacks_[fd] = std::move(callback);
    return true;
}

bool EpollReactor::modify_fd(int fd, uint32_t events) {
    if (!epoll_fd_.valid() || fd < 0) return false;

    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;

    return ::epoll_ctl(epoll_fd_.get(), EPOLL_CTL_MOD, fd, &ev) == 0;
}

bool EpollReactor::unregister_fd(int fd) {
    if (!epoll_fd_.valid() || fd < 0) return false;

    ::epoll_ctl(epoll_fd_.get(), EPOLL_CTL_DEL, fd, nullptr);
    callbacks_.erase(fd);
    return true;
}

int EpollReactor::create_timerfd(std::chrono::milliseconds interval, EventCallback callback) {
    int tfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (tfd < 0) return -1;

    itimerspec ts{};
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(interval);
    auto nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(interval - sec);

    ts.it_interval.tv_sec = sec.count();
    ts.it_interval.tv_nsec = nsec.count();
    ts.it_value = ts.it_interval;

    if (::timerfd_settime(tfd, 0, &ts, nullptr) < 0) {
        ::close(tfd);
        return -1;
    }

    // Wrap callback to consume 8-byte uint64_t timer expiration count
    auto wrapped = [tfd, cb = std::move(callback)](uint32_t events) {
        uint64_t expirations = 0;
        (void)::read(tfd, &expirations, sizeof(expirations));
        cb(events);
    };

    if (!register_fd(tfd, EPOLLIN, std::move(wrapped))) {
        ::close(tfd);
        return -1;
    }

    return tfd;
}

void EpollReactor::run() {
    if (!epoll_fd_.valid()) return;

    running_ = true;
    epoll_event events[MAX_EPOLL_EVENTS];

    std::cout << "[CoreFlow][Reactor] Zero-polling epoll reactor started (native kernel wait)." << std::endl;

    while (running_) {
        // -1 timeout: daemon sleeps deeply in kernel until a registered event or timer fires
        int nfds = ::epoll_wait(epoll_fd_.get(), events, MAX_EPOLL_EVENTS, -1);
        if (nfds < 0) {
            if (errno == EINTR) continue; // Signal received, resume or check running_
            std::cerr << "[CoreFlow][Reactor] epoll_wait error: " << std::strerror(errno) << std::endl;
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;
            auto it = callbacks_.find(fd);
            if (it != callbacks_.end() && it->second) {
                it->second(events[i].events);
            }
        }
    }

    std::cout << "[CoreFlow][Reactor] Event loop terminated gracefully." << std::endl;
}

void EpollReactor::stop() {
    if (!running_) return;
    if (stop_event_fd_ >= 0) {
        uint64_t val = 1;
        (void)::write(stop_event_fd_, &val, sizeof(val));
    }
    running_ = false;
}

} // namespace coreflow
