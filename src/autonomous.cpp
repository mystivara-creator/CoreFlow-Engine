#include "coreflow/autonomous.hpp"

#include <android/log.h>
#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cmath>
#include <exception>
#include <thread>
#include <utility>

#include "coreflow/signal_state.hpp"

namespace coreflow {
namespace {

// ============================================================================
// CONFIGURATION & CONSTANTS
// ============================================================================

constexpr const char* kLogTag = "CoreFlowAutonomous";
constexpr const char* kConfigPath = "/data/adb/coreflow/config.ini";
constexpr std::size_t kHistorySize = 6;

// Notification throttling: prevents repeated events
constexpr std::uint64_t kNotificationCooldownSamples = 12;

// Periodic diagnostics: balance between detail and storage efficiency
// Optimized: every 10 samples (less frequently than 6, reduces storage wear)
constexpr std::uint64_t kPeriodicLogSamples = 10;

// ============================================================================
// EFFICIENCY & SAFETY CONSTANTS
// ============================================================================

// Sensor staleness threshold: if thermal data older than this, distrust it
constexpr int kSensorStalenessMs = 2000;


// Policy cache: skip re-evaluation in steady state (efficiency optimization)

// ============================================================================
// THERMAL STABILITY (HYSTERESIS & PREDICTION)
// ============================================================================

// Threshold aligned with repository policy: enter ThermalGuard at 43°C
constexpr double kThermalGuardThresholdC = 43.0;

// Hysteresis: Prevent state bouncing (yo-yo effect)
// Once in ThermalGuard, stay there until suhu < 41.5°C (43.0 - 1.5)
constexpr double kThermalGuardHysteresisC = 1.5;

// Predictive buffer: enter protection 0.5°C earlier if rising trend detected
// Prevents overshoot during rapid heating
constexpr double kRisingThermalBufferC = 0.5;

// Adaptive thermal buffer scaling (efficiency feature)
constexpr double kRisingThermalBufferMaxC = 2.0;

// ============================================================================
// DEADBAND CONSTANTS (Trend Detection)
// ============================================================================

// Thermal: 500 millidegrees = 0.5°C (prevents false trend oscillation)
constexpr double kThermalDeadband = 500.0;

// Memory ratio: 1.5% (filters noise in memory utilization)
constexpr double kMemoryDeadband = 0.015;

// Load average: 0.2 (prevents reaction to minor load fluctuations)
constexpr double kLoadDeadband = 0.20;

// ============================================================================
// CONFIDENCE SCORING WEIGHTS (Must sum to 1.0)
// ============================================================================

constexpr double kThermalWeight = 0.30;        // Most critical
constexpr double kMemoryWeight = 0.20;         // System stability
constexpr double kCpuUtilWeight = 0.15;        // Workload intensity
constexpr double kChargingWeight = 0.15;       // Affects thermal behavior
constexpr double kUptimeWeight = 0.05;         // System stability
constexpr double kThermalTrendWeight = 0.10;   // Predictive
constexpr double kMemoryTrendWeight = 0.05;    // Predictive

// ============================================================================
// SAMPLE VALIDATION BOUNDS
// ============================================================================

constexpr long kThermalMinMillidegrees = -50000;   // -50°C
constexpr long kThermalMaxMillidegrees = 150000;   // 150°C
constexpr double kMemoryRatioMin = 0.0;
constexpr double kMemoryRatioMax = 1.0;
constexpr double kLoadAverageMax = 100.0;

// ============================================================================
// LOGGING UTILITIES
// ============================================================================

#if defined(__clang__) || defined(__GNUC__)
#define COREFLOW_PRINTF_FORMAT(format_index, argument_index) \
    __attribute__((format(printf, format_index, argument_index)))
#else
#define COREFLOW_PRINTF_FORMAT(format_index, argument_index)
#endif

void logMessage(int priority, const char* fmt, va_list args) {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif
    __android_log_vprint(priority, kLogTag, fmt, args);
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
}

void logInfo(const char* fmt, ...) COREFLOW_PRINTF_FORMAT(1, 2);
void logWarn(const char* fmt, ...) COREFLOW_PRINTF_FORMAT(1, 2);
void logError(const char* fmt, ...) COREFLOW_PRINTF_FORMAT(1, 2);

void logInfo(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logMessage(ANDROID_LOG_INFO, fmt, args);
    va_end(args);
}

void logWarn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logMessage(ANDROID_LOG_WARN, fmt, args);
    va_end(args);
}

void logError(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logMessage(ANDROID_LOG_ERROR, fmt, args);
    va_end(args);
}

#undef COREFLOW_PRINTF_FORMAT

// ============================================================================
// TREND ANALYSIS
// ============================================================================

/**
 * Calculate trend with deadband filtering.
 * Delta must exceed deadband to register as true trend.
 */
Trend calculateTrend(double oldest, double newest, double deadband) {
    const double delta = newest - oldest;
    if (delta > deadband) return Trend::Rising;
    if (delta < -deadband) return Trend::Falling;
    return Trend::Stable;
}

/**
 * Compute adaptive thermal buffer based on rise rate.
 * Faster rises get larger buffer for more responsive control.
 * Efficiency feature from OPTIMIZED version.
 * 
 * @param newTemp Current temperature in millidegrees
 * @param oldTemp Previous temperature in millidegrees
 * @param timeDeltaMs Time since previous sample in milliseconds
 * @return Adaptive buffer in Celsius (0.5-2.0°C)
 */
double computeAdaptiveThermalBuffer(long newTemp, long oldTemp, int timeDeltaMs) {
    if (timeDeltaMs <= 0) return kRisingThermalBufferC;
    
    // Calculate rise rate in °C per second
    double riseRateCPerS = ((newTemp - oldTemp) / 1000.0) / (timeDeltaMs / 1000.0);
    
    // Scale buffer: 0.5°C/s rise → 0.5°C buffer, 2°C/s → 2.0°C buffer
    double adaptiveBuffer = std::clamp(riseRateCPerS * 1.0,
                                      kRisingThermalBufferC,
                                      kRisingThermalBufferMaxC);
    
    return adaptiveBuffer;
}

} // namespace

// ============================================================================
// AUTONOMOUS ENGINE IMPLEMENTATION
// ============================================================================

AutonomousEngine::AutonomousEngine() = default;

AutonomousEngine::~AutonomousEngine() {
    try {
        // Double-restore protection (from HARDENED)
        // Prevents calling restore twice if already restored
        if (!has_restored_) {
            controller_.restoreAll();
            has_restored_ = true;
        }
    } catch (const std::exception& e) {
        logError("Exception during destructor: %s", e.what());
    } catch (...) {
        logError("Unknown exception during destructor");
    }
}

bool AutonomousEngine::initialize() {
    logInfo("CoreFlowInitializeEnter version=%s", kCoreFlowVersion);

    if (!config_.load(kConfigPath)) {
        logWarn("Config unavailable; using built-in defaults path=%s", kConfigPath);
    }

    snapshot_.profile = discovery_.discover();
    snapshot_.state = RuntimeState::Idle;
    history_.clear();
    sample_count_ = 0;
    last_notification_sample_ = 0;
    last_notification_ = NotificationEvent::None;
    has_restored_ = false;
    
    // Initialize efficiency cache members

    controller_.captureBaseline(snapshot_.profile);
    has_restored_ = false;

    // Log charging capabilities
    const ChargingCapability& charging = snapshot_.profile.charging;
    logInfo(
        "ChargingCapability battery=%s status=%s telemetry=%s "
        "control_limit=%s charging_enabled=%s",
        charging.battery_available ? "YES" : "NO",
        charging.status_readable ? "YES" : "NO",
        charging.telemetry_readable ? "YES" : "NO",
        charging.has_charge_control_limit ? "YES" : "NO",
        charging.has_charging_enabled ? "YES" : "NO"
    );

    if (charging.has_charge_control_limit && !charging.charge_control_limit_range_valid) {
        logWarn("Charge control limit has invalid range");
    }

    return snapshot_.profile.proc_available || snapshot_.profile.sys_available;
}

bool AutonomousEngine::refreshEnvironment() {
    if (!config_.runtimeRefreshEnabled()) return false;

    logInfo("Runtime discovery refresh requested");
    const bool restored = controller_.restoreAll();
    if (!restored) logWarn("Baseline restore during refresh incomplete");

    DeviceProfile refreshed = discovery_.discover();
    snapshot_.profile = std::move(refreshed);
    snapshot_.state = RuntimeState::Idle;
    history_.clear();
    sample_count_ = 0;
    last_notification_sample_ = 0;
    last_notification_ = NotificationEvent::None;
    has_restored_ = false;

    controller_.captureBaseline(snapshot_.profile);

    logInfo(
        "Runtime discovery refresh complete cpu_policies=%zu thermal_zones=%zu",
        snapshot_.profile.cpu_policies.size(),
        snapshot_.profile.thermal_zones.size()
    );
    return true;
}

void AutonomousEngine::logStartup() const {
    // Better logging format (from HARDENED)
    logInfo(
        "CORE    v%s | mode=%s | interval=%ds | conf>=%.2f",
        kCoreFlowVersion,
        mutationModeName(config_.mutationMode()),
        config_.monitorIntervalSeconds(),
        config_.minConfidence()
    );
    logInfo(
        "DEVICE  cpu=%zu | thermal=%zu | io=%zu | kernel=%s",
        snapshot_.profile.cpu_policies.size(),
        snapshot_.profile.thermal_zones.size(),
        snapshot_.profile.io_devices.size(),
        snapshot_.profile.kernel_release.c_str()
    );
}

void AutonomousEngine::logStateTransition(RuntimeState oldState, RuntimeState newState) const {
    logInfo("STATE_TRANSITION: %s -> %s", stateName(oldState), stateName(newState));
}

void AutonomousEngine::logMutation(MutationResult result, RuntimeState state) const {
    if (result == MutationResult::Skipped) return;
    logInfo("ACTION  %-12s | state=%s", mutationResultName(result), stateName(state));
}

/**
 * Validate sample data integrity.
 * Comprehensive validation from HARDENED version with isfinite() checks.
 * Prevents NaN/Infinity from crashing the daemon.
 */
bool AutonomousEngine::validateSample(const RuntimeSample& sample) const {
    if (sample.thermal_available) {
        if (sample.thermal_millidegrees < kThermalMinMillidegrees ||
            sample.thermal_millidegrees > kThermalMaxMillidegrees) {
            logWarn("Thermal out of range: %ld", sample.thermal_millidegrees);
            return false;
        }

        if (sample.thermal_age_ms < 0) {
            logWarn("Thermal sensor age invalid: %d ms", sample.thermal_age_ms);
            return false;
        }

        if (sample.thermal_age_ms > kSensorStalenessMs) {
            logWarn("Thermal sensor stale: %d ms old", sample.thermal_age_ms);
        }
    }

    if (sample.mem_total_kb > 0) {
        if (!std::isfinite(sample.mem_available_ratio) ||
            sample.mem_available_ratio < kMemoryRatioMin ||
            sample.mem_available_ratio > kMemoryRatioMax) {
            logWarn("Memory ratio invalid: %.3f", sample.mem_available_ratio);
            return false;
        }
    }

    if (!std::isfinite(sample.load1) ||
        sample.load1 < 0.0 ||
        sample.load1 > kLoadAverageMax) {
        logWarn("Load average invalid: %.3f", sample.load1);
        return false;
    }

    if (sample.cpu_utilization_available) {
        if (!std::isfinite(sample.cpu_utilization) ||
            sample.cpu_utilization < 0.0 ||
            sample.cpu_utilization > 1.0) {
            logWarn("CPU utilization invalid: %.3f", sample.cpu_utilization);
            return false;
        }
    }

    return true;
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
            kThermalDeadband);
    }
    
    if (sample.mem_total_kb > 0 && oldest.mem_total_kb > 0) {
        sample.memory_trend = calculateTrend(
            oldest.mem_available_ratio,
            sample.mem_available_ratio,
            kMemoryDeadband);
    }
    
    sample.load_trend = calculateTrend(oldest.load1, sample.load1, kLoadDeadband);
}

double AutonomousEngine::calculateConfidence(const RuntimeSample& sample) const {
    double score = 0.0;
    
    if (sample.thermal_available) score += kThermalWeight;
    if (sample.mem_total_kb > 0) score += kMemoryWeight;
    if (sample.cpu_utilization_available) score += kCpuUtilWeight;
    if (sample.charging_telemetry_available) score += kChargingWeight;
    if (sample.uptime_seconds > 0) score += kUptimeWeight;
    if (sample.thermal_trend != Trend::Unknown) score += kThermalTrendWeight;
    if (sample.memory_trend != Trend::Unknown) score += kMemoryTrendWeight;
    
    return std::min<double>(1.0, score);
}

NotificationEvent AutonomousEngine::selectNotification(
    const RuntimeSample& sample,
    RuntimeState state,
    RuntimeState previous) const {
    return policy_.notification(sample, state, previous);
}

void AutonomousEngine::emitNotification(NotificationEvent event, const RuntimeSample& sample) const {
    if (event == NotificationEvent::None) return;
    
    // Better format from HARDENED
    logInfo("ALERT   %-18s | temp=%.2fC | trend=%s | chg=%s",
            notificationEventName(event),
            static_cast<double>(sample.thermal_millidegrees) / 1000.0,
            trendName(sample.thermal_trend),
            sample.charging ? "YES" : "NO");
}

void AutonomousEngine::tick() {
    try {
        // STEP 1: Sample runtime environment
        RuntimeSample sample = observer_.sample(snapshot_.profile);

        // STEP 2: Validate sample (comprehensive from HARDENED)
        if (!validateSample(sample)) {
            logWarn("Skipping tick: validation failed");
            ++sample_count_;
            return;
        }

        // STEP 3: Update trends
        updateTrends(sample);
        
        // STEP 4: Calculate confidence
        sample.confidence = calculateConfidence(sample);
        // STEP 5: Evaluate state from fresh telemetry.
        // Policy evaluation is intentionally uncached: runtime conditions can
        // change between samples and the decision layer is inexpensive.
        const RuntimeState previous = snapshot_.state;
        RuntimeState next = policy_.evaluate(sample, previous);

        // Autonomous safety hysteresis is applied after policy evaluation.
        if (sample.thermal_available) {
            const double thermal_c =
                static_cast<double>(sample.thermal_millidegrees) / 1000.0;

            if (previous == RuntimeState::ThermalGuard &&
                next != RuntimeState::ThermalGuard &&
                thermal_c >
                    (kThermalGuardThresholdC - kThermalGuardHysteresisC)) {
                next = RuntimeState::ThermalGuard;
            } else if (next != RuntimeState::ThermalGuard &&
                       sample.thermal_trend == Trend::Rising &&
                       thermal_c >=
                           (kThermalGuardThresholdC - kRisingThermalBufferC)) {
                next = RuntimeState::ThermalGuard;
            }
        }

        const Decision decision = policy_.decide(sample, next);

        // STEP 7: Handle state transitions
        if (next != snapshot_.state) {
            snapshot_.state = next;
            logStateTransition(previous, next);
        }

        // STEP 8: Handle notifications
        const NotificationEvent event = selectNotification(sample, next, previous);
        const bool cooldownExpired =
            sample_count_ >= last_notification_sample_ &&
            (sample_count_ - last_notification_sample_) >= kNotificationCooldownSamples;

        if (event != NotificationEvent::None &&
            (event != last_notification_ || cooldownExpired)) {
            emitNotification(event, sample);
            last_notification_ = event;
            last_notification_sample_ = sample_count_;
        }

        // STEP 9: Apply mutations
        const MutationResult mutation = controller_.apply(next, sample, snapshot_.profile, config_);
        logMutation(mutation, next);

        // STEP 10: Periodic detailed logging (reduced frequency for storage efficiency)
        const bool periodic = (sample_count_ % kPeriodicLogSamples) == 0;
        if (decision != Decision::NoAction || periodic) {
            // Better format from HARDENED, efficiency from OPTIMIZED
            logInfo(
                "[%04llu] %-13s | %-18s | conf %.2f",
                static_cast<unsigned long long>(sample_count_),
                stateName(snapshot_.state),
                decisionName(decision),
                sample.confidence
            );
            logInfo(
                "       cpu %5.1f%% | load %5.2f %-7s | mem %5.1f%% %-7s | temp %5.2fC",
                sample.cpu_utilization * 100.0,
                sample.load1,
                trendName(sample.load_trend),
                sample.mem_available_ratio * 100.0,
                trendName(sample.memory_trend),
                static_cast<double>(sample.thermal_millidegrees) / 1000.0
            );
        }

        // STEP 12: Update history
        history_.push_back(sample);
        while (history_.size() > kHistorySize) {
            history_.pop_front();
        }

    } catch (const std::exception& e) {
        logError("Exception during tick: %s", e.what());
    } catch (...) {
        logError("Unknown exception during tick");
    }

    ++sample_count_;
}

int AutonomousEngine::run() {
    if (!initialize()) {
        logError("Initialization failed");
        return 2;
    }

    logStartup();

    auto next_tick_time = std::chrono::steady_clock::now();

    while (!signal_state::stopRequested()) {
        if (signal_state::consumeRefresh()) {
            refreshEnvironment();
            next_tick_time = std::chrono::steady_clock::now();
        }

        tick();

        const int configured =
            std::max(1, config_.monitorIntervalSeconds());

        int sleep_seconds = configured;

        switch (snapshot_.state) {
            case RuntimeState::Idle:
                sleep_seconds = configured > 15 ? 60 : configured * 4;
                break;

            case RuntimeState::Warming:
            case RuntimeState::Elevated:
            case RuntimeState::Pressure:
                sleep_seconds = std::max(1, configured / 2);
                break;

            case RuntimeState::ThermalGuard:
                sleep_seconds = 1;
                break;

            case RuntimeState::Normal:
            default:
                break;
        }

        next_tick_time += std::chrono::seconds(sleep_seconds);

        while (!signal_state::stopRequested()) {
            const auto now = std::chrono::steady_clock::now();
            if (now >= next_tick_time) {
                break;
            }

            if (config_.runtimeRefreshEnabled() &&
                signal_state::consumeRefresh()) {
                refreshEnvironment();
                next_tick_time = std::chrono::steady_clock::now();
                break;
            }

            const auto remaining = next_tick_time - now;
            const auto sleep_for =
                std::min(remaining, std::chrono::milliseconds(250));

            std::this_thread::sleep_for(sleep_for);
        }

        // Avoid a burst of catch-up ticks after a long stall.
        if (std::chrono::steady_clock::now() >
            next_tick_time + std::chrono::seconds(60)) {
            next_tick_time = std::chrono::steady_clock::now();
        }
    }

    try {
        if (!has_restored_) {
            controller_.restoreAll();
            has_restored_ = true;
        }
    } catch (const std::exception& e) {
        logError("Exception during shutdown restore: %s", e.what());
    } catch (...) {
        logError("Unknown exception during shutdown restore");
    }

    logInfo("CoreFlow Autonomous shutdown complete");
    return 0;
}

} // namespace coreflow
