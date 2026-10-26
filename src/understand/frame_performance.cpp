#include "coreflow/frame_performance.hpp"

#include <algorithm>
#include <array>
#include <limits>

namespace coreflow::graphics {

bool FramePerformanceAnalyzer::recordFrame(
    std::uint64_t duration_ns,
    std::uint64_t target_interval_ns) noexcept {
    if (duration_ns == 0 || target_interval_ns < kMinFrameIntervalNs ||
        target_interval_ns > kMaxFrameIntervalNs ||
        duration_ns > kMaxFrameIntervalNs * 10ULL) {
        return false;
    }
    durations_[next_] = duration_ns;
    targets_[next_] = target_interval_ns;
    next_ = (next_ + 1U) % kWindowSize;
    if (size_ < kWindowSize) ++size_;
    return true;
}

FramePerformanceAnalyzer::Snapshot FramePerformanceAnalyzer::snapshot() const noexcept {
    Snapshot result;
    result.samples = size_;
    if (size_ == 0) return result;

    std::array<std::uint64_t, kWindowSize> sorted{};
    std::size_t misses = 0;
    std::size_t severe = 0;
    std::size_t streak = 0;
    std::size_t longest_streak = 0;
    const std::size_t oldest = size_ == kWindowSize ? next_ : 0U;
    for (std::size_t i = 0; i < size_; ++i) {
        const std::size_t index = (oldest + i) % kWindowSize;
        sorted[i] = durations_[index];
        const bool miss = durations_[index] > targets_[index];
        if (miss) {
            ++misses;
            ++streak;
            longest_streak = std::max(longest_streak, streak);
        } else {
            streak = 0;
        }
        // Avoid overflow in target * 3 / 2.
        const std::uint64_t half = targets_[index] / 2ULL;
        const std::uint64_t severe_threshold =
            targets_[index] > std::numeric_limits<std::uint64_t>::max() - half
                ? std::numeric_limits<std::uint64_t>::max()
                : targets_[index] + half;
        if (durations_[index] > severe_threshold) ++severe;
    }
    std::sort(sorted.begin(), sorted.begin() + static_cast<std::ptrdiff_t>(size_));
    const auto percentile = [&sorted, this](std::size_t numerator) noexcept {
        // Nearest-rank percentile, with rank clamped to [1, size].
        const std::size_t rank = (size_ * numerator + 99U) / 100U;
        return sorted[std::max<std::size_t>(1U, rank) - 1U];
    };
    result.deadline_misses = misses;
    result.severe_stutters = severe;
    result.longest_miss_streak = longest_streak;
    result.deadline_miss_rate = static_cast<double>(misses) / static_cast<double>(size_);
    result.p50_frame_ns = percentile(50U);
    result.p95_frame_ns = percentile(95U);
    result.p99_frame_ns = percentile(99U);
    result.valid = true;
    return result;
}

void FramePerformanceAnalyzer::reset() noexcept {
    durations_.fill(0);
    targets_.fill(0);
    size_ = 0;
    next_ = 0;
}

} // namespace coreflow::graphics
