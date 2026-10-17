#include "coreflow/frame_latency.hpp"
#include "coreflow/frame_performance.hpp"
#include "coreflow/launch_detector.hpp"

#include <cstdio>
#include <string>

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", \
                     __FILE__, __LINE__, #condition); \
        return __LINE__; \
    } \
} while (false)

using coreflow::graphics::FrameLatencyTrace;
using coreflow::graphics::FramePerformanceAnalyzer;
using coreflow::graphics::parseSurfaceFlingerLatency;
using coreflow::observe::LaunchBurstDetector;
using coreflow::observe::LaunchObservation;
using coreflow::observe::LaunchPhase;

namespace {

// Five presents: four intervals, the third one (66.7 ms) misses a 60 Hz deadline.
const char* kTrace =
    "16666667\n"
    "1000000000 1000000000 999000000\n"
    "1000000000 1016666667 1000100000\n"
    "1016666667 1033333334 1016700000\n"
    "1033333334 1100000000 1033400000\n"
    "1100000000 1116666667 1100100000\n";

int testParsesIntervalsAndPending() {
    const std::string text = std::string(kTrace) + "1000 9223372036854775807 1000\n";
    const FrameLatencyTrace trace = parseSurfaceFlingerLatency(text);
    CHECK(trace.valid);
    CHECK(trace.refresh_period_ns == 16666667U);
    CHECK(trace.frame_intervals_ns.size() == 4U);
    CHECK(trace.frame_intervals_ns[0] == 16666667U);
    CHECK(trace.frame_intervals_ns[2] == 66666666U);
    CHECK(trace.rows_pending == 1U);
    CHECK(trace.rows_rejected == 0U);
    return 0;
}

int testRejectsMalformedAndNonMonotonicRows() {
    const std::string text =
        "16666667\n"
        "1000 1000 1000\n"
        "1100 1100 1100\n"
        "1050 1050 1050\n"      // non-monotonic: rejected, never a negative interval
        "garbage here\n"        // malformed: rejected
        "1200 1200 1200 extra\n" // too many fields: rejected
        "1300 1300 1300\n";
    const FrameLatencyTrace trace = parseSurfaceFlingerLatency(text);
    CHECK(trace.valid);
    CHECK(trace.rows_rejected == 3U);
    CHECK(trace.frame_intervals_ns.size() == 2U);
    for (const auto interval : trace.frame_intervals_ns) CHECK(interval > 0U);
    return 0;
}

int testFailsClosedOnForeignFormat() {
    CHECK(!parseSurfaceFlingerLatency("").valid);
    CHECK(!parseSurfaceFlingerLatency("not a number\n1 2 3\n4 5 6\n").valid);
    CHECK(!parseSurfaceFlingerLatency("0\n1 2 3\n4 5 6\n").valid);
    CHECK(!parseSurfaceFlingerLatency("16666667\n").valid);  // header only
    return 0;
}

int testBoundsInputAndRows() {
    const std::string huge(FrameLatencyTrace::kMaxInputBytes + 1U, 'x');
    CHECK(!parseSurfaceFlingerLatency(huge).valid);

    std::string many = "16666667\n";
    std::uint64_t t = 1000000000ULL;
    for (int i = 0; i < 200; ++i) {
        many += std::to_string(t) + " " + std::to_string(t) + " " + std::to_string(t) + "\n";
        t += 16666667ULL;
    }
    const FrameLatencyTrace trace = parseSurfaceFlingerLatency(many);
    CHECK(trace.valid);
    CHECK(trace.rows_total == FrameLatencyTrace::kMaxRows);
    return 0;
}

int testParsedFramesFeedTheAnalyzer() {
    const FrameLatencyTrace trace = parseSurfaceFlingerLatency(kTrace);
    CHECK(trace.valid);
    FramePerformanceAnalyzer analyzer;
    for (const auto interval : trace.frame_intervals_ns) {
        (void)analyzer.recordFrame(interval, trace.refresh_period_ns);
    }
    const auto snapshot = analyzer.snapshot();
    CHECK(snapshot.samples == 4U);
    CHECK(snapshot.deadline_misses == 1U);
    return 0;
}

int testLaunchWindowOpensOnForegroundSwitch() {
    LaunchBurstDetector detector;
    CHECK(detector.observe({1'000'000'000ULL, 1, -1.0}) == LaunchPhase::Idle);  // no predecessor
    CHECK(detector.observe({2'000'000'000ULL, 1, -1.0}) == LaunchPhase::Idle);  // steady
    CHECK(detector.observe({3'000'000'000ULL, 2, -1.0}) == LaunchPhase::LaunchWindow);
    return 0;
}

int testLaunchWindowClosesWhenQuiet() {
    LaunchBurstDetector detector;
    (void)detector.observe({1'000'000'000ULL, 1, -1.0});
    (void)detector.observe({2'000'000'000ULL, 2, -1.0});  // switch at t=2 s
    CHECK(detector.observe({4'000'000'000ULL, 2, 0.0}) == LaunchPhase::LaunchWindow);
    CHECK(detector.observe({5'000'000'000ULL, 2, 0.0}) == LaunchPhase::Idle);  // 3 s quiet
    return 0;
}

int testIoActivityExtendsButNeverPastCap() {
    LaunchBurstDetector detector;
    (void)detector.observe({1'000'000'000ULL, 1, -1.0});
    (void)detector.observe({2'000'000'000ULL, 2, 10.0});  // opened at 2 s
    for (std::uint64_t s = 3; s <= 7; ++s) {
        CHECK(detector.observe({s * 1'000'000'000ULL, 2, 10.0}) == LaunchPhase::LaunchWindow);
    }
    // Storage stays busy, but the window hits its hard cap of 8 s after opening.
    CHECK(detector.observe({10'000'000'000ULL, 2, 10.0}) == LaunchPhase::Idle);
    return 0;
}

int testInvalidObservationsAreIgnored() {
    LaunchBurstDetector detector;
    (void)detector.observe({1'000'000'000ULL, 1, -1.0});
    CHECK(detector.observe({0ULL, 5, -1.0}) == LaunchPhase::Idle);           // zero time
    CHECK(detector.observe({500'000'000ULL, 9, -1.0}) == LaunchPhase::Idle);  // going back
    CHECK(detector.observe({2'000'000'000ULL, 2, 0.0}) == LaunchPhase::LaunchWindow);
    return 0;
}

} // namespace

int main() {
    int failures = 0;
    failures += testParsesIntervalsAndPending() != 0;
    failures += testRejectsMalformedAndNonMonotonicRows() != 0;
    failures += testFailsClosedOnForeignFormat() != 0;
    failures += testBoundsInputAndRows() != 0;
    failures += testParsedFramesFeedTheAnalyzer() != 0;
    failures += testLaunchWindowOpensOnForegroundSwitch() != 0;
    failures += testLaunchWindowClosesWhenQuiet() != 0;
    failures += testIoActivityExtendsButNeverPastCap() != 0;
    failures += testInvalidObservationsAreIgnored() != 0;
    if (failures != 0) {
        std::fprintf(stderr, "CoreFlow render signal tests: FAIL (%d)\n", failures);
        return 1;
    }
    std::puts("CoreFlow render signal tests: PASS");
    return 0;
}
