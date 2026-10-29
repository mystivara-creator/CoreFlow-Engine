// @coreflow-platform: any

#include "coreflow/frame_telemetry.hpp"

#include "coreflow/bounded_process.hpp"
#include "coreflow/render_target.hpp"

#include <cstddef>

namespace coreflow::observe {
namespace {

// The collector is bounded in time and output. A hung dumpsys can delay one
// poll by at most kDumpTimeoutMs, and it can never block the control loop
// indefinitely.
constexpr std::uint32_t kDumpTimeoutMs = 500U;
constexpr std::size_t kDumpMaxBytes = 256U * 1024U;

} // namespace

void FrameTelemetrySource::setForeground(std::string_view process) {
    const auto separator = process.find(':');
    const auto package = process.substr(0, separator);
    if (package == foreground_) return;
    reset();
    foreground_.assign(package);
}

std::vector<std::uint64_t> FrameTelemetrySource::poll(std::uint64_t now_ms,
                                                      bool jank_recent) noexcept {
    try {
        if (foreground_.empty()) { availability_ = "foreground_unavailable"; return {}; }
        if (now_ms < next_poll_ms_) return {};
        const unsigned hz = jank_recent ? active_hz_ : idle_hz_;
        next_poll_ms_ = now_ms + 1000U / (hz == 0U ? 1U : hz);
        if (layer_.empty()) {
            if (now_ms < next_discovery_ms_) return {};
            next_discovery_ms_ = now_ms + 5000U;
            const auto list = runner_(
                {"/system/bin/dumpsys", "SurfaceFlinger", "--list"}, kDumpTimeoutMs, kDumpMaxBytes);
            if (!list.started || list.timed_out || list.truncated || list.exit_status != 0) {
                availability_ = "layer_discovery_unavailable"; return {};
            }
            layer_ = selectRenderLayer(list.output, foreground_);
            if (layer_.empty()) { availability_ = "layer_unresolved_or_ambiguous"; return {}; }
            // Discovery consumes this cycle's subprocess budget.
            availability_ = "warming_up";
            return {};
        }
        const auto result = runner_(
            {"/system/bin/dumpsys", "SurfaceFlinger", "--latency", layer_},
            kDumpTimeoutMs, kDumpMaxBytes);
        if (!result.started || result.timed_out || result.truncated || result.exit_status != 0) {
            availability_ = "collection_unavailable";
            layer_.clear(); presentations_.reset(); primed_ = false;
            return {};
        }
        const auto period = parseLatencyRefreshPeriodNs(result.output, false);
        if (period == 0U) {
            availability_ = "invalid_refresh_period";
            layer_.clear(); presentations_.reset(); primed_ = false;
            return {};
        }
        if (period != last_refresh_period_ns_) {
            presentations_.reset();
            primed_ = false;
        }
        last_refresh_period_ns_ = period;
        auto frames = presentations_.consume(result.output);
        if (!primed_) {
            primed_ = true; availability_ = "warming_up";
            return {}; // historical frames predate this observation epoch
        }
        availability_ = frames.empty() ? "no_fresh_presentations" : "available";
        if (frames.empty()) {
            // Surfaces may be replaced without a process transition. Re-resolve
            // at a bounded cadence; do not reuse intervals across surface epochs.
            if (now_ms >= next_discovery_ms_) {
                layer_.clear(); presentations_.reset(); primed_ = false;
                next_discovery_ms_ = now_ms + 5000U;
            }
        }
        return frames;
    } catch (...) {
        availability_ = "collection_unavailable";
        layer_.clear(); presentations_.reset(); primed_ = false;
        return {};
    }
}

} // namespace coreflow::observe
