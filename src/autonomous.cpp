#include "coreflow/autonomous.hpp"

#include <android/log.h>
#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cmath>
#include <exception>
#include <thread>

#include "coreflow/signal_state.hpp"

namespace coreflow {
namespace {

constexpr const char* kLogTag = "CoreFlowAutonomous";
constexpr const char* kConfigPath = "/data/adb/coreflow/config.ini";
constexpr std::size_t kHistorySize = 6;
constexpr std::uint64_t kNotificationCooldownSamples = 12;
constexpr std::uint64_t kPeriodicLogSamples = 6;

void logInfo(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_INFO, kLogTag, fmt, args);
    va_end(args);
}

void logWarn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_WARN, kLogTag, fmt, args);
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
    if (delta > deadband) return Trend::Rising;
    if (delta < -deadband) return Trend::Falling;
    return Trend::Stable;
}

} // namespace

AutonomousEngine::AutonomousEngine() = default;

AutonomousEngine::~AutonomousEngine() {
    controller_.restoreAll();
}

bool AutonomousEngine::initialize() {
    logInfo("CoreFlowInitializeEnter version=%s", kCoreFlowVersion);

    if (!config_.load(kConfigPath)) {
        logWarn("Config unavailable; using built-in production defaults path=%s", kConfigPath);
    }

    snapshot_.profile = discovery_.discover();
    snapshot_.state = RuntimeState::Idle;
    history_.clear();
    sample_count_ = 0;
    last_notification_sample_ = 0;
    last_notification_ = NotificationEvent::None;

    controller_.captureBaseline(snapshot_.profile);

    const ChargingCapability& charging = snapshot_.profile.charging;
    logInfo(
        "ChargingCapability battery=%s status=%s telemetry=%s input_limit=%s "
        "charge_current=%s control_limit=%s charging_enabled=%s charge_disable=%s",
        charging.battery_available ? "YES" : "NO",
        charging.status_readable ? "YES" : "NO",
        charging.telemetry_readable ? "YES" : "NO",
        charging.has_input_current_limit ? "YES" : "NO",
        charging.has_charge_current_limit ? "YES" : "NO",
        charging.has_charge_control_limit ? "YES" : "NO",
        charging.has_charging_enabled ? "YES" : "NO",
        charging.has_charge_disable ? "YES" : "NO"
    );

    if (charging.has_charge_control_limit) {
        logInfo(
            "ChargingControlValidation path=%s readable=%s writable=%s numeric=%s value=%lld "
            "min=%s:%lld max=%s:%lld range_valid=%s semantics_validated=%s mutation_ready=%s",
            charging.charge_control_limit_path.c_str(),
            charging.charge_control_limit_readable ? "YES" : "NO",
            charging.charge_control_limit_writable ? "YES" : "NO",
            charging.charge_control_limit_numeric ? "YES" : "NO",
            charging.charge_control_limit_value,
            charging.charge_control_limit_min_available ? "YES" : "NO",
            charging.charge_control_limit_min,
            charging.charge_control_limit_max_available ? "YES" : "NO",
            charging.charge_control_limit_max,
            charging.charge_control_limit_range_valid ? "YES" : "NO",
            charging.charge_control_limit_semantics_validated ? "YES" : "NO",
            charging.charge_control_limit_mutation_ready ? "YES" : "NO"
        );
    } else {
        logInfo("ChargingControlValidation available=NO");
    }

    return snapshot_.profile.proc_available || snapshot_.profile.sys_available;
}

bool AutonomousEngine::refreshEnvironment() {
    if (!config_.runtimeRefreshEnabled()) return false;

    logInfo("Runtime discovery refresh requested");
    const bool restored = controller_.restoreAll();
    if (!restored) logWarn("Baseline restore during refresh was incomplete");

    DeviceProfile refreshed = discovery_.discover();
    snapshot_.profile = std::move(refreshed);
    snapshot_.state = RuntimeState::Idle;
    history_.clear();
    sample_count_ = 0;
    last_notification_sample_ = 0;
    last_notification_ = NotificationEvent::None;
    controller_.captureBaseline(snapshot_.profile);

    logInfo(
        "Runtime discovery refresh complete cpu_policies=%zu thermal_zones=%zu io_devices=%zu",
        snapshot_.profile.cpu_policies.size(),
        snapshot_.profile.thermal_zones.size(),
        snapshot_.profile.io_devices.size()
    );
    return true;
}

void AutonomousEngine::logStartup() const {
    logInfo(
        "CoreFlow Autonomous %s | abi=%s kernel=%s cpu_policies=%zu thermal_zones=%zu io_devices=%zu "
        "mutation=%s confidence_floor=%.2f",
        kCoreFlowVersion,
        snapshot_.profile.abi.c_str(),
        snapshot_.profile.kernel_release.c_str(),
        snapshot_.profile.cpu_policies.size(),
        snapshot_.profile.thermal_zones.size(),
        snapshot_.profile.io_devices.size(),
        mutationModeName(config_.mutationMode()),
        config_.minConfidence()
    );
}

void AutonomousEngine::logStateTransition(RuntimeState oldState, RuntimeState newState) const {
    logInfo("state: %s -> %s", stateName(oldState), stateName(newState));
}

void AutonomousEngine::logMutation(MutationResult result, RuntimeState state) const {
    if (result == MutationResult::Skipped) return;
    logInfo("Mutation result=%s state=%s baseline=%zu",
            mutationResultName(result), stateName(state), controller_.baselineSize());
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
            static_cast<double>(sample.thermal_millidegrees), 500.0);
    }
    if (sample.mem_total_kb > 0 && oldest.mem_total_kb > 0) {
        sample.memory_trend = calculateTrend(
            oldest.mem_available_ratio, sample.mem_available_ratio, 0.015);
    }
    sample.load_trend = calculateTrend(oldest.load1, sample.load1, 0.20);
}

double AutonomousEngine::calculateConfidence(const RuntimeSample& sample) const {
    double score = 0.0;
    if (sample.thermal_available) score += 0.30;
    if (sample.mem_total_kb > 0) score += 0.20;
    if (sample.cpu_utilization_available) score += 0.15;
    if (sample.uptime_seconds > 0) score += 0.05;
    if (sample.charging_telemetry_available) score += 0.15;
    if (sample.thermal_trend != Trend::Unknown) score += 0.10;
    if (sample.memory_trend != Trend::Unknown) score += 0.05;
    return std::min(1.0, score);
}

NotificationEvent AutonomousEngine::selectNotification(
    const RuntimeSample& sample,
    RuntimeState state,
    RuntimeState previous) const {
    return policy_.notification(sample, state, previous);
}

void AutonomousEngine::emitNotification(NotificationEvent event, const RuntimeSample& sample) const {
    if (event == NotificationEvent::None) return;
    logInfo("NOTIFICATION_EVENT=%s thermal=%.2fC trend=%s charging=%s",
            notificationEventName(event),
            static_cast<double>(sample.thermal_millidegrees) / 1000.0,
            trendName(sample.thermal_trend), sample.charging ? "YES" : "NO");
}

void AutonomousEngine::tick() {
    try {
        RuntimeSample sample = observer_.sample(snapshot_.profile);
        updateTrends(sample);
        sample.confidence = calculateConfidence(sample);

        const RuntimeState previous = snapshot_.state;
        const RuntimeState next = policy_.evaluate(sample, previous);
        const Decision decision = policy_.decide(sample, next);

        if (next != previous) {
            snapshot_.state = next;
            logStateTransition(previous, next);
        }

        const NotificationEvent event = selectNotification(sample, next, previous);
        const bool cooldownExpired =
            sample_count_ >= last_notification_sample_ + kNotificationCooldownSamples;
        if (event != NotificationEvent::None &&
            (event != last_notification_ || cooldownExpired)) {
            emitNotification(event, sample);
            last_notification_ = event;
            last_notification_sample_ = sample_count_;
        }

        const MutationResult mutation =
            controller_.apply(next, sample, snapshot_.profile, config_);
        logMutation(mutation, next);

        const bool periodic = (sample_count_ % kPeriodicLogSamples) == 0;
        if (decision != Decision::NoAction || periodic) {
            logInfo(
                "sample=%llu state=%s decision=%s load=%.2f load_trend=%s cpu_util=%.1f%% "
                "mem=%llu/%lluKB mem_avail=%.1f%% mem_trend=%s thermal=%.2fC thermal_max=%.2fC "
                "thermal_trend=%s thermal_source=%s charging=%s battery_temp=%.2fC confidence=%.2f",
                static_cast<unsigned long long>(sample_count_),
                stateName(snapshot_.state), decisionName(decision),
                sample.load1, trendName(sample.load_trend), sample.cpu_utilization * 100.0,
                static_cast<unsigned long long>(sample.mem_available_kb),
                static_cast<unsigned long long>(sample.mem_total_kb),
                sample.mem_available_ratio * 100.0, trendName(sample.memory_trend),
                static_cast<double>(sample.thermal_millidegrees) / 1000.0,
                static_cast<double>(sample.hottest_thermal_millidegrees) / 1000.0,
                trendName(sample.thermal_trend),
                sample.thermal_source == ThermalSource::Primary ? "PRIMARY" :
                    sample.thermal_source == ThermalSource::Fallback ? "FALLBACK" : "UNKNOWN",
                sample.charging ? "YES" : "NO",
                static_cast<double>(sample.battery_temperature_millidegrees) / 1000.0,
                sample.confidence
            );
        }

        history_.push_back(sample);
        while (history_.size() > kHistorySize) history_.pop_front();
    } catch (const std::exception& e) {
        logError("Exception caught during tick processing: %s", e.what());
    } catch (...) {
        logError("Unknown exception caught during tick processing");
    }
    ++sample_count_;
}

int AutonomousEngine::run() {
    if (!initialize()) return 2;
    logStartup();

    while (!signal_state::stopRequested()) {
        if (signal_state::consumeRefresh()) refreshEnvironment();
        tick();

        int sleep_seconds = config_.monitorIntervalSeconds();
        if (snapshot_.state == RuntimeState::Idle) sleep_seconds = std::min(sleep_seconds * 4, 60);

        for (int i = 0; i < sleep_seconds * 10; ++i) {
            if (signal_state::stopRequested()) break;
            if (config_.runtimeRefreshEnabled() && signal_state::consumeRefresh()) {
                refreshEnvironment();
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    controller_.restoreAll();
    logInfo("CoreFlow Autonomous shutdown complete");
    return 0;
}

} // namespace coreflow
