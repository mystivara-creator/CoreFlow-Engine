#include "coreflow/autonomous.hpp"

#include <android/log.h>
#include <chrono>
#include <cstdarg>
#include <cmath>
#include <thread>

namespace coreflow {
namespace {

constexpr const char* kLogTag = "CoreFlowAutonomous";
constexpr std::size_t kHistorySize = 6;
constexpr std::uint64_t kNotificationCooldownSamples = 12;

void logInfo(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_INFO, kLogTag, fmt, args);
    va_end(args);
}

void logError(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_ERROR, kLogTag, fmt, args);
    va_end(args);
}

Trend calculateTrend(double oldest, double newest, double deadband) {
    const double delta = newest - oldest;

    if (delta > deadband)
        return Trend::Rising;

    if (delta < -deadband)
        return Trend::Falling;

    return Trend::Stable;
}

} // namespace

AutonomousEngine::AutonomousEngine() = default;

bool AutonomousEngine::initialize() {
    snapshot_.profile = discovery_.discover();
    snapshot_.state = RuntimeState::Idle;
    history_.clear();
    sample_count_ = 0;
    last_notification_sample_ = 0;
    last_notification_ = NotificationEvent::None;

    return snapshot_.profile.proc_available ||
           snapshot_.profile.sys_available;
}

void AutonomousEngine::logStartup() const {
    logInfo(
        "CoreFlow Autonomous %s | abi=%s kernel=%s cpu_policies=%zu thermal_zones=%zu",
        CORE_FLOW_VERSION,
        snapshot_.profile.abi.c_str(),
        snapshot_.profile.kernel_release.c_str(),
        snapshot_.profile.cpu_policies.size(),
        snapshot_.profile.thermal_zones.size()
    );

    const ChargingCapability& charging = snapshot_.profile.charging;
    logInfo(
        "ChargingCapability battery=%s status=%s telemetry=%s "
        "input_limit=%s charge_current=%s control_limit=%s "
        "charging_enabled=%s charge_disable=%s",
        charging.battery_available ? "YES" : "NO",
        charging.status_readable ? "YES" : "NO",
        charging.telemetry_readable ? "YES" : "NO",
        charging.has_input_current_limit ? "YES" : "NO",
        charging.has_charge_current_limit ? "YES" : "NO",
        charging.has_charge_control_limit ? "YES" : "NO",
        charging.has_charging_enabled ? "YES" : "NO",
        charging.has_charge_disable ? "YES" : "NO"
    );
}

void AutonomousEngine::refreshDiscovery() {
    logInfo("Runtime discovery refresh requested.");

    const DeviceProfile refreshed = discovery_.discover();
    snapshot_.profile = refreshed;

    logStartup();
    logInfo("Runtime discovery refresh complete.");
}

void AutonomousEngine::logStateTransition(
    RuntimeState oldState,
    RuntimeState newState
) const {
    logInfo(
        "state: %s -> %s",
        stateName(oldState),
        stateName(newState)
    );
}

void AutonomousEngine::updateTrends(RuntimeSample& sample) const {
    if (history_.size() < 2) {
        sample.thermal_trend = Trend::Unknown;
        sample.memory_trend = Trend::Unknown;
        sample.load_trend = Trend::Unknown;
        return;
    }

    const RuntimeSample& oldest = history_.front();

    if (sample.thermal_available && oldest.thermal_available) {
        sample.thermal_trend = calculateTrend(
            static_cast<double>(oldest.thermal_millidegrees),
            static_cast<double>(sample.thermal_millidegrees),
            500.0
        );
    }

    if (sample.mem_total_kb > 0 && oldest.mem_total_kb > 0) {
        sample.memory_trend = calculateTrend(
            oldest.mem_available_ratio,
            sample.mem_available_ratio,
            0.015
        );
    }

    sample.load_trend = calculateTrend(
        oldest.load1,
        sample.load1,
        0.20
    );
}

double AutonomousEngine::calculateConfidence(
    const RuntimeSample& sample
) const {
    double score = 0.0;

    if (sample.thermal_available)
        score += 0.35;

    if (sample.mem_total_kb > 0)
        score += 0.25;

    if (sample.uptime_seconds > 0)
        score += 0.10;

    if (sample.charging_telemetry_available)
        score += 0.15;

    if (sample.thermal_trend != Trend::Unknown)
        score += 0.10;

    if (sample.memory_trend != Trend::Unknown)
        score += 0.05;

    return score > 1.0 ? 1.0 : score;
}

NotificationEvent AutonomousEngine::selectNotification(
    const RuntimeSample& sample,
    RuntimeState state,
    RuntimeState previous
) const {
    return policy_.notification(sample, state, previous);
}

void AutonomousEngine::emitNotification(
    NotificationEvent event,
    const RuntimeSample& sample
) const {
    if (event == NotificationEvent::None)
        return;

    logInfo(
        "NOTIFICATION_EVENT=%s thermal=%.2fC trend=%s charging=%s",
        notificationEventName(event),
        static_cast<double>(sample.thermal_millidegrees) / 1000.0,
        trendName(sample.thermal_trend),
        sample.charging ? "YES" : "NO"
    );
}

void AutonomousEngine::tick() {
    try {
        RuntimeSample sample =
            observer_.sample(snapshot_.profile);

        updateTrends(sample);
        sample.confidence = calculateConfidence(sample);

        const RuntimeState previous = snapshot_.state;
        const RuntimeState next =
            policy_.evaluate(sample, previous);

        const Decision decision =
            policy_.decide(sample, next);

        if (next != previous) {
            snapshot_.state = next;
            logStateTransition(previous, next);
        }

        const NotificationEvent event =
            selectNotification(sample, next, previous);

        const bool cooldownExpired =
            sample_count_ >=
            last_notification_sample_ +
            kNotificationCooldownSamples;

        if (event != NotificationEvent::None &&
            (event != last_notification_ || cooldownExpired)) {

            emitNotification(event, sample);
            last_notification_ = event;
            last_notification_sample_ = sample_count_;
        }

        if (decision != Decision::NoAction) {
            logInfo(
                "sample=%llu state=%s decision=%s "
                "load=%.2f load_trend=%s "
                "mem=%llu/%lluKB mem_avail=%.1f%% mem_trend=%s "
                "thermal=%.2fC thermal_max=%.2fC thermal_trend=%s "
                "thermal_source=%s charging=%s "
                "battery_temp=%.2fC confidence=%.2f",

                static_cast<unsigned long long>(sample_count_),
                stateName(snapshot_.state),
                decisionName(decision),

                sample.load1,
                trendName(sample.load_trend),

                static_cast<unsigned long long>(sample.mem_available_kb),
                static_cast<unsigned long long>(sample.mem_total_kb),
                sample.mem_available_ratio * 100.0,
                trendName(sample.memory_trend),

                static_cast<double>(sample.thermal_millidegrees) / 1000.0,
                static_cast<double>(sample.hottest_thermal_millidegrees) / 1000.0,
                trendName(sample.thermal_trend),

                sample.thermal_source == ThermalSource::Primary
                    ? "PRIMARY"
                    : sample.thermal_source == ThermalSource::Fallback
                        ? "FALLBACK"
                        : "UNKNOWN",

                sample.charging ? "YES" : "NO",

                static_cast<double>(
                    sample.battery_temperature_millidegrees
                ) / 1000.0,

                sample.confidence
            );

            // Observation Intelligence v1 is intentionally read-only.
            // Charging/kernel mutation will only be enabled after
            // capability discovery and validation.
            if (controller_.isReady()) {
                controller_.execute(decision, sample);
            }
        }

        history_.push_back(sample);

        while (history_.size() > kHistorySize)
            history_.pop_front();

    } catch (const std::exception& e) {
        logError(
            "Exception caught during tick processing: %s",
            e.what()
        );
    }

    ++sample_count_;
}

int AutonomousEngine::run() {
    if (!initialize())
        return 2;

    logStartup();

    while (!stop_requested_.load(std::memory_order_relaxed)) {
        if (discovery_refresh_requested_.exchange(
                false,
                std::memory_order_acq_rel)) {
            refreshDiscovery();
        }

        tick();

        int sleep_seconds =
            config_.getMonitorInterval();

        if (sleep_seconds <= 0)
            sleep_seconds = 5;

        if (snapshot_.state == RuntimeState::Idle)
            sleep_seconds *= 4;

        for (int i = 0; i < sleep_seconds * 10; ++i) {
            if (stop_requested_.load(std::memory_order_relaxed))
                break;

            if (discovery_refresh_requested_.load(
                    std::memory_order_relaxed))
                break;

            std::this_thread::sleep_for(
                std::chrono::milliseconds(100)
            );
        }
    }

    return 0;
}

void AutonomousEngine::requestStop() noexcept {
    logInfo(
        "Stop requested via signal. Initiating shutdown..."
    );

    stop_requested_.store(
        true,
        std::memory_order_relaxed
    );
}

void AutonomousEngine::requestDiscoveryRefresh() noexcept {
    discovery_refresh_requested_.store(
        true,
        std::memory_order_release
    );
}

} // namespace coreflow
