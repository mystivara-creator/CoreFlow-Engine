#include "coreflow/autonomous.hpp"

#include <android/log.h>
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include "coreflow/signal_state.hpp"

namespace {

constexpr const char* kLogTag = "CoreFlowAutonomous";
constexpr const char* kLockPath = "/data/adb/coreflow/coreflowd.lock";

volatile std::sig_atomic_t g_stop = 0;
volatile std::sig_atomic_t g_refresh = 0;

void handleSignal(int signal) noexcept {
    if (signal == SIGTERM || signal == SIGINT) g_stop = 1;
    else if (signal == SIGUSR1) g_refresh = 1;
}

int acquireProcessLock() {
    const int fd = open(kLockPath, O_CREAT | O_RDWR | O_CLOEXEC, 0600);
    if (fd < 0) return -1;
    if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
        close(fd);
        return -2;
    }
    return fd;
}

} // namespace

namespace coreflow::signal_state {

void install() noexcept {
    std::signal(SIGTERM, handleSignal);
    std::signal(SIGINT, handleSignal);
    std::signal(SIGUSR1, handleSignal);
}

bool stopRequested() noexcept {
    return g_stop != 0;
}

bool consumeRefresh() noexcept {
    if (g_refresh == 0) return false;
    g_refresh = 0;
    return true;
}

} // namespace coreflow::signal_state

int main() {
    umask(0077);
    (void)mkdir("/data/adb/coreflow", 0700);

    const int lock_fd = acquireProcessLock();
    if (lock_fd == -2) {
        __android_log_print(ANDROID_LOG_INFO, kLogTag,
                            "Another coreflowd instance is already running");
        return 3;
    }
    if (lock_fd < 0) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                            "Failed to acquire instance lock errno=%d", errno);
        return 4;
    }

    coreflow::signal_state::install();

    __android_log_print(ANDROID_LOG_INFO, kLogTag,
                        "CoreFlowMainEntry version=%s stage=MAIN", coreflow::kCoreFlowVersion);

    coreflow::AutonomousEngine engine;
    const int result = engine.run();

    __android_log_print(ANDROID_LOG_INFO, kLogTag,
                        "CoreFlowMainExit version=%s result=%d", coreflow::kCoreFlowVersion, result);

    close(lock_fd);
    return result;
}
