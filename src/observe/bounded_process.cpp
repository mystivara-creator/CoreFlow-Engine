#include "coreflow/bounded_process.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>

namespace coreflow::process {
namespace {

std::uint64_t monotonicMs() noexcept {
    timespec ts{};
    if (::clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0U;
    return static_cast<std::uint64_t>(ts.tv_sec) * 1000U +
           static_cast<std::uint64_t>(ts.tv_nsec) / 1000000U;
}

void closeFd(int& fd) noexcept {
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

void killAndReap(pid_t pid) noexcept {
    int status = 0;
    ::kill(pid, SIGKILL);
    while (::waitpid(pid, &status, 0) < 0 && errno == EINTR) {
    }
}

} // namespace

BoundedResult runBounded(const std::vector<std::string>& argv, std::uint32_t timeout_ms,
                         std::size_t max_output_bytes) noexcept {
    BoundedResult result;
    if (argv.empty() || argv[0].empty() || argv[0].front() != '/' || timeout_ms == 0U ||
        max_output_bytes == 0U) {
        return result;
    }

    try {
        // Build every pointer before fork(). After fork, the child calls only
        // async-signal-safe functions until exec.
        std::vector<char*> args;
        args.reserve(argv.size() + 1U);
        for (const auto& arg : argv) args.push_back(const_cast<char*>(arg.c_str()));
        args.push_back(nullptr);

        int pipe_fds[2] = {-1, -1};
        if (::pipe(pipe_fds) != 0) return result;
        int read_fd = pipe_fds[0];
        int write_fd = pipe_fds[1];
        (void)::fcntl(read_fd, F_SETFD, FD_CLOEXEC);
        (void)::fcntl(write_fd, F_SETFD, FD_CLOEXEC);

        const pid_t pid = ::fork();
        if (pid < 0) {
            closeFd(read_fd);
            closeFd(write_fd);
            return result;
        }
        if (pid == 0) {
            const int null_fd = ::open("/dev/null", O_RDWR);
            if (null_fd >= 0) {
                ::dup2(null_fd, STDIN_FILENO);
                ::dup2(null_fd, STDERR_FILENO);
            }
            ::dup2(write_fd, STDOUT_FILENO);
            ::execv(args[0], args.data());
            ::_exit(127);
        }

        closeFd(write_fd);
        result.started = true;

        const std::uint64_t deadline = monotonicMs() + timeout_ms;
        bool eof = false;
        std::array<char, 4096> buffer{};
        while (!eof && !result.timed_out && !result.truncated) {
            const std::uint64_t now = monotonicMs();
            if (now >= deadline) {
                result.timed_out = true;
                break;
            }
            pollfd watch{read_fd, POLLIN, 0};
            const int ready = ::poll(&watch, 1, static_cast<int>(deadline - now));
            if (ready < 0) {
                if (errno == EINTR) continue;
                break;  // unexpected poll failure: killed below
            }
            if (ready == 0) continue;  // the deadline check above decides

            const ssize_t n = ::read(read_fd, buffer.data(), buffer.size());
            if (n < 0) {
                if (errno == EINTR) continue;
                break;
            }
            if (n == 0) {
                eof = true;
                break;
            }
            const std::size_t room = max_output_bytes - result.output.size();
            const std::size_t take = std::min(room, static_cast<std::size_t>(n));
            result.output.append(buffer.data(), take);
            if (take < static_cast<std::size_t>(n)) result.truncated = true;
        }
        closeFd(read_fd);  // a child still writing now gets SIGPIPE

        if (!eof || result.timed_out || result.truncated) {
            killAndReap(pid);
            result.exit_status = -1;
            return result;
        }

        // The child closed stdout. Reap it within the same budget, then kill it
        // if it is still running, so a child that closes stdout cannot hang us.
        const std::uint64_t reap_deadline = monotonicMs() + timeout_ms;
        int status = 0;
        for (;;) {
            const pid_t done = ::waitpid(pid, &status, WNOHANG);
            if (done == pid) {
                result.exit_status = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
                return result;
            }
            if (done < 0 && errno != EINTR) {
                result.exit_status = -1;
                return result;
            }
            if (monotonicMs() >= reap_deadline) {
                killAndReap(pid);
                result.timed_out = true;
                result.exit_status = -1;
                return result;
            }
            const timespec pause{0, 1000000};  // 1 ms
            ::nanosleep(&pause, nullptr);
        }
    } catch (...) {
        return BoundedResult{};
    }
}

} // namespace coreflow::process
