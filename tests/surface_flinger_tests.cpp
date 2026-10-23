#include "coreflow/observation/surface_flinger.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <sstream>
#include <string>

namespace {
// histogramConfig from the cited AOSP TimeStatsHelper implementation. These
// are bucket lower bounds, so the spacing intentionally becomes non-uniform.
constexpr std::array<int, 85> kAospBuckets{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
    34, 36, 38, 40, 42, 44, 46, 48, 50, 54, 58, 62, 66, 70, 74, 78, 82,
    86, 90, 94, 98, 102, 106, 110, 114, 118, 122, 126, 130, 134, 138, 142,
    146, 150, 200, 250, 300, 350, 400, 450, 500, 550, 600, 650, 700, 750,
    800, 850, 900, 950, 1000
};

std::string makeAospStats(unsigned long long total_frames,
                          unsigned long long count_16,
                          unsigned long long count_17,
                          unsigned long long count_36,
                          int omitted_bucket = -1) {
    std::ostringstream dump;
    dump << "SurfaceFlinger TimeStats:\n"
         << "layerName = SurfaceView[com.example.game/com.example.game.MainActivity]@0(BLAST)#4\n"
         << "packageName = com.example.game\n"
         << "totalFrames = " << total_frames << "\n"
         << "averageFPS = 60.000\n"
         << "present2present histogram is as below:\n";

    for (const int bucket : kAospBuckets) {
        if (bucket == omitted_bucket) continue;
        const auto count = bucket == 16 ? count_16
                         : bucket == 17 ? count_17
                         : bucket == 36 ? count_36 : 0ULL;
        dump << bucket << "ms=" << count << ' ';
    }
    dump << '\n';
    return dump.str();
}
} // namespace

int main() {
    using coreflow::FrameTimingMetrics;
    using coreflow::SurfaceFlingerObserver;

    const std::string activity =
        "topResumedActivity=ActivityRecord{123 u0 com.example.game/.MainActivity t12}\n";
    const std::string stats =
        "SurfaceFlinger TimeStats:\n"
        "layerName = SurfaceView[com.example.game/com.example.game.MainActivity]@0(BLAST)#4\n"
        "packageName = com.example.game\n"
        "totalFrames = 100\n"
        "averageFPS = 60.000\n"
        "present2present histogram is as below:\n"
        "16ms=50 17ms=30 35ms=20\n";

    const FrameTimingMetrics metrics = SurfaceFlingerObserver::parse(activity, stats);
    assert(metrics.valid);
    assert(metrics.package_name == "com.example.game");
    assert(metrics.total_frames == 100);
    assert(std::abs(metrics.average_fps - 60.0) < 0.001);
    assert(metrics.p95_frame_interval_ms == 35.0);
    assert(metrics.p99_frame_interval_ms == 35.0);
    assert(std::abs(metrics.slow_frame_ratio - 0.20) < 0.001);

    const std::string previous_stats =
        "SurfaceFlinger TimeStats:\n"
        "layerName = SurfaceView[com.example.game/com.example.game.MainActivity]@0(BLAST)#4\n"
        "packageName = com.example.game\n"
        "totalFrames = 60\n"
        "averageFPS = 60.000\n"
        "present2present histogram is as below:\n"
        "16ms=40 17ms=15 35ms=5\n";
    const auto delta = SurfaceFlingerObserver::parseDelta(activity, stats, previous_stats, 2.0 / 3.0);
    assert(delta.valid && delta.total_frames == 40);
    assert(std::abs(delta.average_fps - 60.0) < 0.001);
    assert(delta.p95_frame_interval_ms == 35.0);
    assert(std::abs(delta.slow_frame_ratio - 0.375) < 0.001);
    const auto reset_counter = SurfaceFlingerObserver::parseDelta(activity, previous_stats, stats, 1.0);
    assert(!reset_counter.valid);

    // AOSP prints all 85 histogram bucket lower bounds, including zero-count
    // buckets. Several later intervals are spaced by more than one millisecond.
    const auto aosp_current = makeAospStats(100, 50, 30, 20);
    const auto aosp_previous = makeAospStats(60, 40, 15, 5);
    const auto aosp_snapshot = SurfaceFlingerObserver::parse(activity, aosp_current);
    assert(aosp_snapshot.valid && aosp_snapshot.total_frames == 100);
    assert(aosp_snapshot.p95_frame_interval_ms == 36.0);
    assert(aosp_snapshot.p99_frame_interval_ms == 36.0);
    assert(std::abs(aosp_snapshot.slow_frame_ratio - 0.20) < 0.001);

    const auto aosp_delta = SurfaceFlingerObserver::parseDelta(
        activity, aosp_current, aosp_previous, 2.0 / 3.0);
    assert(aosp_delta.valid && aosp_delta.total_frames == 40);
    assert(std::abs(aosp_delta.average_fps - 60.0) < 0.001);
    assert(aosp_delta.p95_frame_interval_ms == 36.0);
    assert(std::abs(aosp_delta.slow_frame_ratio - 0.375) < 0.001);

    // The earlier regression fixture used literal backslash-n sequences and
    // therefore never built a multi-line dump. This fixture is a real dump and
    // specifically omits a bucket that was present (with count zero) before.
    const auto missing_zero_bucket = makeAospStats(100, 50, 30, 20, 2);
    const auto incomplete_delta = SurfaceFlingerObserver::parseDelta(
        activity, missing_zero_bucket, aosp_previous, 2.0 / 3.0);
    assert(!incomplete_delta.valid);

    const auto wrong_app = SurfaceFlingerObserver::parse(
        "topResumedActivity=ActivityRecord{123 u0 com.other.app/.MainActivity t13}\n", stats);
    assert(!wrong_app.valid);

    const std::string negative_histogram =
        "layerName = SurfaceView[com.example.game/com.example.game.MainActivity]@0(BLAST)#4\n"
        "packageName = com.example.game\n"
        "totalFrames = 100\n"
        "averageFPS = 60.000\n"
        "present2present histogram is as below:\n"
        "16ms=-1 17ms=30\n";
    const auto malformed_count = SurfaceFlingerObserver::parse(activity, negative_histogram);
    assert(!malformed_count.valid);

    const auto unavailable = SurfaceFlingerObserver::parse(activity, "TimeStats is disabled\n");
    assert(!unavailable.valid);
    return 0;
}
