/**
 * @file reactor.cpp
 * @brief Event-driven Linux epoll reactor implementation
 * @author mystivara-creator
 * @license MIT
 */

#include "coreflow/reactor.hpp"
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <utility>

namespace coreflow {

EpollReactor::EpollReactor() {
    epoll_fd_.reset(::epoll_create1(EPOLL_CLOEXEC));
    if (!epoll_fd_.valid()) {
        std::cerr << "[CoreFlow][Reactor] Failed to create epoll instance: "
                  << std::strerror(errno) << "\n";
        return;
    }

    // Wakeup eventfd for lock-free stop signaling. Fail closed if unavailable.
    stop_event_fd_.reset(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC));
    if (!stop_event_fd_.valid() ||
        !register_fd(stop_event_fd_.get(), EPOLLIN, [this](uint32_t) {
            uint64_t val = 0;
            while (::read(stop_event_fd_.get(), &val, sizeof(val)) == static_cast<ssize_t>(sizeof(val))) {}
            running_.store(false, std::memory_order_release);
        })) {
        stop_event_fd_.reset();
        epoll_fd_.reset();
    }
}

EpollReactor::~EpollReactor() {
    stop();
}

bool EpollReactor::register_fd(int fd, uint32_t events, EventCallback callback) {
    if (!epoll_fd_.valid() || fd < 0 || !callback) return false;

    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;

    if (callbacks_.find(fd) != callbacks_.end()) {
        if (::epoll_ctl(epoll_fd_.get(), EPOLL_CTL_MOD, fd, &ev) < 0) {
            return false;
        }
    } else if (::epoll_ctl(epoll_fd_.get(), EPOLL_CTL_ADD, fd, &ev) < 0) {
        std::cerr << "[CoreFlow][Reactor] EPOLL_CTL_ADD failed for fd " << fd
                  << ": " << std::strerror(errno) << "\n";
        return false;
    }

    callbacks_[fd] = std::move(callback);
    return true;
}

bool EpollReactor::modify_fd(int fd, uint32_t events) {
    if (!epoll_fd_.valid() || fd < 0 || callbacks_.find(fd) == callbacks_.end()) return false;

    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;

    return ::epoll_ctl(epoll_fd_.get(), EPOLL_CTL_MOD, fd, &ev) == 0;
}

bool EpollReactor::unregister_fd(int fd) {
    if (!epoll_fd_.valid() || fd < 0) return false;

    (void)::epoll_ctl(epoll_fd_.get(), EPOLL_CTL_DEL, fd, nullptr);
    callbacks_.erase(fd);
    return true;
}

int EpollReactor::create_timerfd(std::chrono::milliseconds interval, EventCallback callback) {
    if (!epoll_fd_.valid() || interval.count() <= 0 || !callback) return -1;

    UniqueFd tfd(::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC));
    if (!tfd.valid()) return -1;

    itimerspec ts{};
    const auto sec = std::chrono::duration_cast<std::chrono::seconds>(interval);
    const auto nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(interval - sec);

    ts.it_interval.tv_sec = sec.count();
    ts.it_interval.tv_nsec = nsec.count();
    ts.it_value = ts.it_interval;

    if (::timerfd_settime(tfd.get(), 0, &ts, nullptr) < 0) {
        return -1;
    }

    const int raw_fd = tfd.get();
    auto wrapped = [raw_fd, cb = std::move(callback)](uint32_t events) {
        uint64_t expirations = 0;
        bool expired = false;
        while (::read(raw_fd, &expirations, sizeof(expirations)) ==
               static_cast<ssize_t>(sizeof(expirations))) {
            expired = expirations > 0;
        }
        if (expired) {
            cb(events);
        }
    };

    if (!register_fd(raw_fd, EPOLLIN, std::move(wrapped))) {
        return -1;
    }

    timer_fds_.push_back(std::move(tfd));
    return raw_fd;
}

void EpollReactor::run() {
    if (!epoll_fd_.valid()) return;

    running_.store(true, std::memory_order_release);
    epoll_event events[MAX_EPOLL_EVENTS];

    std::cout << "[CoreFlow][Reactor] epoll reactor started; waiting on registered kernel events.\n";

    while (running_.load(std::memory_order_acquire)) {
        const int nfds = ::epoll_wait(epoll_fd_.get(), events, static_cast<int>(MAX_EPOLL_EVENTS), -1);
        if (nfds < 0) {
            if (errno == EINTR) continue;
            std::cerr << "[CoreFlow][Reactor] epoll_wait error: " << std::strerror(errno) << "\n";
            break;
        }

        for (int i = 0; i < nfds && running_.load(std::memory_order_acquire); ++i) {
            const int fd = events[i].data.fd;
            const auto it = callbacks_.find(fd);
            if (it != callbacks_.end() && it->second) {
                // Copy callback so re-entrant register_fd/unregister_fd cannot invalidate closure.
                EventCallback cb = it->second;
                cb(events[i].events);
            }
        }
    }

    running_.store(false, std::memory_order_release);
    std::cout << "[CoreFlow][Reactor] Event loop terminated gracefully.\n";
}

void EpollReactor::stop() noexcept {
    const bool was_running = running_.exchange(false, std::memory_order_acq_rel);
    if (!was_running) return;
    if (stop_event_fd_.valid()) {
        const uint64_t val = 1;
        const ssize_t signaled = ::write(stop_event_fd_.get(), &val, sizeof(val));
        (void)signaled;
    }
}

} // namespace coreflow
