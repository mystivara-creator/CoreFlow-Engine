// @coreflow-platform: android

#include "coreflow/frame_telemetry.hpp"

#include "coreflow/bounded_process.hpp"

#include <cstddef>

namespace coreflow::observe {
namespace {

// The collector is bounded in time and output. A hung dumpsys can delay one
// poll by at most kDumpTimeoutMs, and it can never block the control loop
// indefinitely.
constexpr std::uint32_t kDumpTimeoutMs = 500U;
constexpr std::size_t kDumpMaxBytes = 256U * 1024U;

} // namespace

std::vector<std::uint64_t> FrameTelemetrySource::poll(std::uint64_t now_ms,
                                                      bool jank_recent) noexcept {
    if (now_ms < next_poll_ms_) return {};
    const unsigned hz = jank_recent ? active_hz_ : idle_hz_;
    next_poll_ms_ = now_ms + 1000U / (hz == 0U ? 1U : hz);

    // Direct exec, no shell: the argument list is fixed and nothing is interpreted.
    const process::BoundedResult result = process::runBounded(
        {"/system/bin/dumpsys", "SurfaceFlinger", "--latency"}, kDumpTimeoutMs, kDumpMaxBytes);
    if (!result.started || result.timed_out || result.truncated || result.exit_status != 0) {
        return {};  // unavailable telemetry yields no samples, never a stale one
    }
    last_refresh_period_ns_ = parseLatencyRefreshPeriodNs(result.output);
    return parseLatencyDurations(result.output);
}

} // namespace coreflow::observe
