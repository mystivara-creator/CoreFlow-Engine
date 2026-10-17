// @coreflow-platform: android

#include "coreflow/frame_telemetry.hpp"

#include <array>
#include <cstdio>
#include <string>

namespace coreflow::observe {

std::vector<std::uint64_t> FrameTelemetrySource::poll(std::uint64_t now_ms,
                                                      bool jank_recent) noexcept {
    if (now_ms < next_poll_ms_) return {};
    const unsigned hz = jank_recent ? active_hz_ : idle_hz_;
    next_poll_ms_ = now_ms + 1000U / (hz == 0U ? 1U : hz);

    FILE* pipe = ::popen("dumpsys SurfaceFlinger --latency 2>/dev/null", "r");
    if (pipe == nullptr) return {};
    std::string dump;
    dump.reserve(16384);
    std::array<char, 1024> buffer{};
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        dump.append(buffer.data());
        if (dump.size() >= 262144U) break;
    }
    ::pclose(pipe);
    return parseLatencyDurations(dump);
}

} // namespace coreflow::observe
