#include "coreflow/bounded_process.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", \
                     __FILE__, __LINE__, #condition); \
        return __LINE__; \
    } \
} while (false)

using coreflow::process::runBounded;
namespace fs = std::filesystem;

namespace {

long long elapsedMs(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - start).count();
}

int testCapturesOutputAndExitStatus() {
    const auto r = runBounded({"/bin/echo", "hello"}, 2000U, 1024U);
    CHECK(r.started);
    CHECK(!r.timed_out);
    CHECK(!r.truncated);
    CHECK(r.exit_status == 0);
    CHECK(r.output == "hello\n");
    return 0;
}

int testExitStatusIsReported() {
    const auto r = runBounded({"/bin/sh", "-c", "exit 3"}, 2000U, 1024U);
    CHECK(r.started);
    CHECK(r.exit_status == 3);
    return 0;
}

int testRelativeOrEmptyProgramIsRefused() {
    CHECK(!runBounded({"echo", "x"}, 1000U, 64U).started);  // no PATH lookup
    CHECK(!runBounded({}, 1000U, 64U).started);
    CHECK(!runBounded({"/bin/echo"}, 0U, 64U).started);       // zero timeout
    CHECK(!runBounded({"/bin/echo"}, 1000U, 0U).started);     // zero output cap
    return 0;
}

int testMissingProgramReportsExecFailure() {
    const auto r = runBounded({"/nonexistent/coreflow/tool"}, 1000U, 64U);
    CHECK(r.started);
    CHECK(r.exit_status == 127);
    CHECK(r.output.empty());
    return 0;
}

int testHungChildIsKilledWithinTheTimeout() {
    const auto start = std::chrono::steady_clock::now();
    const auto r = runBounded({"/bin/sleep", "30"}, 200U, 1024U);
    const long long took = elapsedMs(start);
    CHECK(r.started);
    CHECK(r.timed_out);
    CHECK(took < 5000);  // far below the 30 s the child would have taken
    return 0;
}

int testUnboundedOutputIsCappedAndChildKilled() {
    const fs::path yes = "/usr/bin/yes";
    if (!fs::exists(yes)) return 0;  // coreutils absent: nothing to prove here
    const auto start = std::chrono::steady_clock::now();
    const auto r = runBounded({yes.string()}, 5000U, 1024U);
    CHECK(r.started);
    CHECK(r.truncated);
    CHECK(r.output.size() == 1024U);
    CHECK(elapsedMs(start) < 5000);  // stopped by the cap, not by the timeout
    return 0;
}

} // namespace

int main() {
    int failures = 0;
    failures += testCapturesOutputAndExitStatus() != 0;
    failures += testExitStatusIsReported() != 0;
    failures += testRelativeOrEmptyProgramIsRefused() != 0;
    failures += testMissingProgramReportsExecFailure() != 0;
    failures += testHungChildIsKilledWithinTheTimeout() != 0;
    failures += testUnboundedOutputIsCappedAndChildKilled() != 0;
    if (failures != 0) {
        std::fprintf(stderr, "CoreFlow bounded process tests: FAIL (%d)\n", failures);
        return 1;
    }
    std::puts("CoreFlow bounded process tests: PASS");
    return 0;
}
