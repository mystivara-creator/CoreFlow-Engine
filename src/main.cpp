#include "coreflow/autonomous.hpp"

#include <android/log.h>
#include <cerrno>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>


#include "coreflow/signal_state.hpp"

namespace {

constexpr const char* kLogTag = "CoreFlowAutonomous";
constexpr const char* kLockPath = "/data/adb/coreflow/coreflowd.lock";

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
