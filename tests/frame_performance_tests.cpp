#include "coreflow/frame_performance.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>

namespace {
int failures = 0;
#define CHECK(expr) do { if (!(expr)) { ++failures; std::cerr << "FAIL: " #expr " line " << __LINE__ << '\n'; } } while (false)
}

int main() {
    using coreflow::graphics::FramePerformanceAnalyzer;
    constexpr std::uint64_t target = 16'666'667ULL;
    FramePerformanceAnalyzer analyzer;

    CHECK(!analyzer.recordFrame(0, target));
    CHECK(!analyzer.recordFrame(1'000'000ULL, 0));
    CHECK(!analyzer.recordFrame(1'000'000ULL, 100'000'001ULL));
    CHECK(!analyzer.snapshot().valid);

    CHECK(analyzer.recordFrame(10'000'000ULL, target));
    CHECK(analyzer.recordFrame(17'000'000ULL, target)); // deadline miss
    CHECK(analyzer.recordFrame(30'000'000ULL, target)); // severe stutter
    const auto s = analyzer.snapshot();
    CHECK(s.valid);
    CHECK(s.samples == 3U);
    CHECK(s.deadline_misses == 2U);
    CHECK(s.severe_stutters == 1U);
    CHECK(s.longest_miss_streak == 2U);
    CHECK(std::abs(s.deadline_miss_rate - (2.0 / 3.0)) < 1e-12);
    CHECK(s.p50_frame_ns == 17'000'000ULL);
    CHECK(s.p95_frame_ns == 30'000'000ULL);
    CHECK(s.p99_frame_ns == 30'000'000ULL);

    analyzer.reset();
    CHECK(!analyzer.snapshot().valid);
    for (std::size_t i = 0; i < FramePerformanceAnalyzer::kWindowSize + 5U; ++i) {
        CHECK(analyzer.recordFrame(10'000'000ULL + static_cast<std::uint64_t>(i), target));
    }
    const auto bounded = analyzer.snapshot();
    CHECK(bounded.samples == FramePerformanceAnalyzer::kWindowSize);
    CHECK(bounded.deadline_misses == 0U);

    if (failures != 0) {
        std::cerr << failures << " frame performance test(s) failed\n";
        return 1;
    }
    std::cout << "Frame performance analyzer tests passed\n";
    return 0;
}
