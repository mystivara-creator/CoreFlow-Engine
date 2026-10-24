#pragma once

#include <chrono>
#include <mutex>
#include <string>

namespace coreflow {

struct FrameTimingMetrics {
    bool valid{false};
    double average_fps{};
    double average_fps_resolution{};
    double p95_frame_interval_ms{};
    double p99_frame_interval_ms{};
    double slow_frame_ratio{};
    double interval_resolution_ms{};
    double slow_frame_ratio_resolution{};
    unsigned long long total_frames{};
    std::string package_name;
};

// Read-only SurfaceFlinger observer. It consumes existing timestats only and
// never enables, clears, or otherwise changes SurfaceFlinger diagnostics.
class SurfaceFlingerObserver {
public:
    FrameTimingMetrics observe() const;

    // Parser is public to allow deterministic testing with captured dumpsys text.
    static FrameTimingMetrics parse(const std::string& activity_dump,
                                    const std::string& timestats_dump);
    static FrameTimingMetrics parseDelta(const std::string& activity_dump,
                                         const std::string& current_timestats_dump,
                                         const std::string& previous_timestats_dump,
                                         double elapsed_seconds);

private:
    mutable std::mutex mutex_;
    mutable std::chrono::steady_clock::time_point last_attempt_{};
    mutable std::chrono::steady_clock::time_point last_sample_time_{};
    mutable std::string last_activity_package_;
    mutable std::string last_timestats_dump_;
    mutable FrameTimingMetrics last_metrics_{};
};

} // namespace coreflow
