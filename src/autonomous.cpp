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

// Log tag for Android logcat
constexpr const char* kLogTag = "CoreFlowAutonomous";

// Configuration file path
constexpr const char* kConfigPath = "/data/adb/coreflow/config.ini";

// History buffer size for trend analysis (circular buffer)
constexpr std::size_t kHistorySize = 6;

// Notification throttling: prevents notification spam during continuous issues
// With 1 sample/second monitoring, this is ~12 seconds
constexpr std::uint64_t kNotificationCooldownSamples = 12;

// Periodic logging: logs full system state every N samples for diagnostics
// With 1 sample/second monitoring, this is ~6 seconds
constexpr std::uint64_t kPeriodicLogSamples = 6;

// ============================================================================
// DEADBAND CONSTANTS (Trend Detection)
// ============================================================================

// Thermal deadband: 500 millidegrees = 0.5°C
// Prevents false trend oscillation from sensor noise
constexpr double kThermalDeadband = 500.0;

// Memory ratio deadband: 1.5%
// Filters noise in memory utilization readings
constexpr double kMemoryDeadband = 0.015;

// Load average deadband: 0.2
// Prevents reaction to minor load fluctuations
constexpr double kLoadDeadband = 0.20;

// ============================================================================
// CONFIDENCE SCORING WEIGHTS (Must sum to 1.0)
// ============================================================================

// Thermal telemetry: 30% - Most critical for thermal management
constexpr double kThermalWeight = 0.30;

// Memory metrics: 20% - Important for system stability and OOM prevention
constexpr double kMemoryWeight = 0.20;

// CPU utilization: 15% - Indicates workload intensity
constexpr double kCpuUtilWeight = 0.15;

// Charging telemetry: 15% - Affects thermal behavior significantly
constexpr double kChargingWeight = 0.15;

// System uptime: 5% - Indicates system stability
constexpr double kUptimeWeight = 0.05;

// Thermal trend: 10% - Predictive indicator of future thermal state
constexpr double kThermalTrendWeight = 0.10;

// Memory trend: 5% - Predictive indicator of memory pressure
constexpr double kMemoryTrendWeight = 0.05;

// ============================================================================
// THERMAL PREDICTION
// ============================================================================

// Current repository policy enters THERMAL_GUARD at 43°C.
// Keep prediction aligned with that policy rather than inventing a second
// vendor-independent thermal state machine inside AutonomousEngine.
constexpr double kThermalGuardThresholdC = 43.0;

// Predictive guard buffer: only 0.5°C early when a sustained rising trend
// is visible. The mutation layer remains confidence-gated and fail-closed.
constexpr double kRisingThermalBufferC = 0.5;

// ============================================================================
// SAMPLE VALIDATION BOUNDS
// ============================================================================

// Thermal: Reasonable range for Android device temperature
constexpr long kThermalMinMillidegrees = -50000;  // -50°C
constexpr long kThermalMaxMillidegrees = 150000;  // 150°C

// Memory ratio: Must be between 0 and 100%
constexpr double kMemoryRatioMin = 0.0;
constexpr double kMemoryRatioMax = 1.0;

// Load average: Reasonable upper bound for mobile device
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

/**
 * Android logcat logging with compile-time format validation.
 * Uses variadic arguments for flexible formatting.
 */
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

// Forward declarations
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
// TREND ANALYSIS & PREDICTION
// ============================================================================

/**
 * Calculate trend based on delta and deadband threshold.
 * Deadband prevents oscillation at boundaries.
 * 
 * Algorithm:
 * - Rising: delta > deadband (significant increase)
 * - Falling: delta < -deadband (significant decrease)
 * - Stable: within deadband range (noise)
 * 
 * @param oldest Historical value (older sample)
 * @param newest Current value (new sample)
 * @param deadband Threshold to avoid false positives
 * @return Trend::Rising, Trend::Falling, or Trend::Stable
 */
Trend calculateTrend(double oldest, double newest, double deadband) {
    const double delta = newest - oldest;
    
    if (delta > deadband) {
        return Trend::Rising;
    }
    if (delta < -deadband) {
        return Trend::Falling;
    }
    return Trend::Stable;
}

/**
 * Predict if system will reach critical thermal state.
 * Enables proactive throttling before crisis conditions.
 * 
 * Strategy:
 * 1. Already critical? Return true immediately
 * 2. Rising trend? Apply safety buffer
 * 3. Otherwise stable or falling? Return false
 * 
 * @param currentTemp Current temperature in °C
 * @param trend Current thermal trend
 * @param threshold Critical temperature threshold in °C
 * @return true if should throttle as if critical
 */
bool predictThermalCritical(double currentTemp, const Trend& trend, 
                           double threshold) {
    // Already at or above critical threshold
    if (currentTemp >= threshold) {
        return true;
    }
    
    // Rising trend detected - apply preventive buffer
    if (trend == Trend::Rising) {
        // Start throttling at threshold - buffer when rising
        // This provides smooth, anticipatory control
        return currentTemp >= (threshold - kRisingThermalBufferC);
    }
    
    // Stable or falling - no need to throttle yet
    return false;
}

} // namespace

// ============================================================================
// AUTONOMOUS ENGINE IMPLEMENTATION
// ============================================================================

AutonomousEngine::AutonomousEngine() = default;

/**
 * Destructor with exception safety.
 * CRITICAL: Never throw from destructor
 */
AutonomousEngine::~AutonomousEngine() {
    try {
        controller_.restoreAll();
    } catch (const std::exception& e) {
        logError("Exception during destructor cleanup: %s", e.what());
        // Do NOT re-throw: destructors must not throw
    } catch (...) {
        logError("Unknown exception during destructor cleanup");
    }
}

/**
 * Initialize the autonomous engine.
 * 
 * Steps:
 * 1. Load configuration (file-based or defaults)
 * 2. Discover device capabilities
 * 3. Capture baseline system state
 * 4. Validate essential subsystems available
 * 5. Log detailed capability information
 * 
 * @return true if initialization succeeded and device is supported
 */
bool AutonomousEngine::initialize() {
    logInfo("CoreFlowInitializeEnter version=%s", kCoreFlowVersion);

    // Load configuration with fallback to defaults
    if (!config_.load(kConfigPath)) {
        logWarn("Config unavailable; using built-in production defaults path=%s", kConfigPath);
    }

    // Discover device capabilities (thermal zones, CPU policies, etc.)
    snapshot_.profile = discovery_.discover();
    snapshot_.state = RuntimeState::Idle;
    history_.clear();
    sample_count_ = 0;
    last_notification_sample_ = 0;
    last_notification_ = NotificationEvent::None;

    // Capture baseline for mutation restoration
    controller_.captureBaseline(snapshot_.profile);

    // Log charging capabilities for diagnostics
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

    // Validate charge control limit if available
    if (charging.has_charge_control_limit) {
        if (!charging.charge_control_limit_range_valid) {
            logWarn("Charge control limit has invalid range - mutations may be unreliable");
        }
        
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

    // Validate essential subsystems
    const bool subsystemAvailable = snapshot_.profile.proc_available || snapshot_.profile.sys_available;
    
    if (!subsystemAvailable) {
        logError("No essential subsystems available (proc or sys)");
    }
    
    return subsystemAvailable;
}

/**
 * Refresh device discovery at runtime.
 * Handles hotplug events or configuration changes.
 * 
 * @return true if refresh succeeded
 */
bool AutonomousEngine::refreshEnvironment() {
    if (!config_.runtimeRefreshEnabled()) {
        return false;
    }

    logInfo("Runtime discovery refresh requested");
    
    // Restore all mutations before refreshing
    const bool restored = controller_.restoreAll();
    if (!restored) {
        logWarn("Baseline restore during refresh was incomplete");
    }

    // Rediscover device capabilities
    DeviceProfile refreshed = discovery_.discover();
    snapshot_.profile = std::move(refreshed);
    snapshot_.state = RuntimeState::Idle;
    history_.clear();
    sample_count_ = 0;
    last_notification_sample_ = 0;
    last_notification_ = NotificationEvent::None;
    
    // Recapture baseline
    controller_.captureBaseline(snapshot_.profile);

    logInfo(
        "Runtime discovery refresh complete cpu_policies=%zu thermal_zones=%zu io_devices=%zu",
        snapshot_.profile.cpu_policies.size(),
        snapshot_.profile.thermal_zones.size(),
        snapshot_.profile.io_devices.size()
    );
    
    return true;
}

/**
 * Log complete startup information for debugging and telemetry.
 */
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

/**
 * Log state transitions for system tracking.
 */
void AutonomousEngine::logStateTransition(RuntimeState oldState, RuntimeState newState) const {
    logInfo("STATE_TRANSITION: %s -> %s", stateName(oldState), stateName(newState));
}

/**
 * Log mutation application results.
 */
void AutonomousEngine::logMutation(MutationResult result, RuntimeState state) const {
    if (result == MutationResult::Skipped) return;
    
    logInfo("MUTATION result=%s state=%s baseline_actions=%zu",
            mutationResultName(result), stateName(state), controller_.baselineSize());
}

/**
 * Validate sample data integrity before processing.
 * Detects sensor failures and anomalies.
 * 
 * Checks:
 * - Thermal: -50°C to 150°C (reasonable for Android)
 * - Memory: 0% to 100% ratio
 * - Load: Non-negative and reasonable bounds
 * 
 * @param sample Runtime sample to validate
 * @return true if sample is valid and usable
 */
bool AutonomousEngine::validateSample(const RuntimeSample& sample) const {
    // Thermal sanity check: -50°C to 150°C in millidegrees
    if (sample.thermal_available) {
        if (sample.thermal_millidegrees < kThermalMinMillidegrees || 
            sample.thermal_millidegrees > kThermalMaxMillidegrees) {
            logWarn("Thermal sample out of valid range: %ld millidegrees",
                    sample.thermal_millidegrees);
            return false;
        }
    }
    
    // Memory sanity check: ratio must be [0.0, 1.0]
    if (sample.mem_total_kb > 0) {
        if (sample.mem_available_ratio < kMemoryRatioMin || 
            sample.mem_available_ratio > kMemoryRatioMax) {
            logWarn("Memory ratio out of range: %.2f", sample.mem_available_ratio);
            return false;
        }
    }
    
    // Load average sanity check
    if (sample.load1 < 0.0 || sample.load1 > kLoadAverageMax) {
        logWarn("Load average out of range: %.2f", sample.load1);
        return false;
    }
    
    return true;
}

/**
 * Update trend indicators based on historical data.
 * 
 * Algorithm:
 * 1. Need at least 2 samples for trend calculation
 * 2. Compare oldest vs newest sample
 * 3. Apply deadband threshold
 * 4. Set trend indicator (Rising/Falling/Stable/Unknown)
 * 
 * Trends are used for predictive decisions and smoothing.
 */
void AutonomousEngine::updateTrends(RuntimeSample& sample) const {
    // Need at least 2 samples to calculate trend
    if (history_.size() < 2) {
        sample.thermal_trend = Trend::Unknown;
        sample.memory_trend = Trend::Unknown;
        sample.load_trend = Trend::Unknown;
        return;
    }

    const RuntimeSample& oldest = history_.front();
    
    // THERMAL TREND: 0.5°C deadband
    if (sample.thermal_available && oldest.thermal_available) {
        sample.thermal_trend = calculateTrend(
            static_cast<double>(oldest.thermal_millidegrees),
            static_cast<double>(sample.thermal_millidegrees),
            kThermalDeadband);
    }
    
    // MEMORY TREND: 1.5% ratio deadband
    if (sample.mem_total_kb > 0 && oldest.mem_total_kb > 0) {
        sample.memory_trend = calculateTrend(
            oldest.mem_available_ratio,
            sample.mem_available_ratio,
            kMemoryDeadband);
    }
    
    // LOAD TREND: 0.2 load average deadband
    sample.load_trend = calculateTrend(oldest.load1, sample.load1, kLoadDeadband);
}

/**
 * Calculate confidence score for decision-making.
 * 
 * Confidence represents data reliability for thermal decisions.
 * Higher confidence = more aggressive control.
 * Lower confidence = conservative approach.
 * 
 * Scoring breakdown (must sum to 1.0):
 * - Thermal: 30% (most critical)
 * - Memory: 20%
 * - CPU: 15%
 * - Charging: 15%
 * - Uptime: 5%
 * - Thermal trend: 10%
 * - Memory trend: 5%
 * 
 * @param sample Current runtime sample
 * @return Confidence score [0.0, 1.0]
 */
double AutonomousEngine::calculateConfidence(const RuntimeSample& sample) const {
    double score = 0.0;
    
    // Critical thermal sensor
    if (sample.thermal_available) {
        score += kThermalWeight;
    }
    
    // Memory information
    if (sample.mem_total_kb > 0) {
        score += kMemoryWeight;
    }
    
    // CPU utilization data
    if (sample.cpu_utilization_available) {
        score += kCpuUtilWeight;
    }
    
    // Charging telemetry
    if (sample.charging_telemetry_available) {
        score += kChargingWeight;
    }
    
    // System uptime indicates stability
    if (sample.uptime_seconds > 0) {
        score += kUptimeWeight;
    }
    
    // Predictive trends
    if (sample.thermal_trend != Trend::Unknown) {
        score += kThermalTrendWeight;
    }
    
    if (sample.memory_trend != Trend::Unknown) {
        score += kMemoryTrendWeight;
    }
    
    // Cap confidence at 1.0 (100%)
    return std::min(1.0, score);
}

/**
 * Select appropriate notification event based on system state.
 * Uses policy engine to determine notification strategy.
 * 
 * @param sample Current runtime sample
 * @param state Current runtime state
 * @param previous Previous runtime state
 * @return Notification event to emit
 */
NotificationEvent AutonomousEngine::selectNotification(
    const RuntimeSample& sample,
    RuntimeState state,
    RuntimeState previous) const {
    return policy_.notification(sample, state, previous);
}

/**
 * Emit notification event to user/system.
 * Includes relevant telemetry for diagnostic purposes.
 * 
 * @param event Notification event type
 * @param sample Runtime sample with context
 */
void AutonomousEngine::emitNotification(NotificationEvent event, const RuntimeSample& sample) const {
    if (event == NotificationEvent::None) {
        return;
    }
    
    logInfo("NOTIFICATION_EVENT=%s thermal=%.2fC trend=%s charging=%s batt_temp=%.2fC",
            notificationEventName(event),
            static_cast<double>(sample.thermal_millidegrees) / 1000.0,
            trendName(sample.thermal_trend),
            sample.charging ? "YES" : "NO",
            static_cast<double>(sample.battery_temperature_millidegrees) / 1000.0);
}

/**
 * Main control loop iteration - executes every monitor interval.
 * 
 * Execution flow:
 * 1. Sample runtime environment
 * 2. Validate sample data integrity
 * 3. Calculate trends from history
 * 4. Compute confidence score
 * 5. Evaluate new state via policy engine
 * 6. Generate control decision
 * 7. Check for state transitions
 * 8. Evaluate notification requirements
 * 9. Apply mutations (CPU/thermal changes)
 * 10. Log results
 * 11. Update history buffer
 * 
 * All operations wrapped in comprehensive exception handling.
 */
void AutonomousEngine::tick() {
    try {
        // STEP 1: Sample runtime environment
        RuntimeSample sample = observer_.sample(snapshot_.profile);
        
        // STEP 2: Validate sample data integrity
        if (!validateSample(sample)) {
            logWarn("Skipping tick: sample validation failed");
            sample_count_++;  // Still increment counter
            return;           // Skip this iteration gracefully
        }
        
        // STEP 3: Update trend indicators
        updateTrends(sample);
        
        // STEP 4: Calculate data confidence
        sample.confidence = calculateConfidence(sample);
        
        // STEP 5: Evaluate new runtime state
        const RuntimeState previous = snapshot_.state;
        RuntimeState next = policy_.evaluate(sample, previous);

        // Predictive thermal guard is intentionally aligned with the existing
        // policy threshold. It can only move the state toward protection;
        // it never creates a new mutation target or bypasses confidence gating.
        if (sample.thermal_available &&
            sample.thermal_trend != Trend::Unknown) {
            const double thermal_c =
                static_cast<double>(sample.thermal_millidegrees) / 1000.0;
            if (predictThermalCritical(
                    thermal_c, sample.thermal_trend, kThermalGuardThresholdC)) {
                if (next != RuntimeState::ThermalGuard) {
                    logInfo(
                        "PREDICTIVE_THERMAL guard=YES temp=%.2fC trend=%s threshold=%.2fC",
                        thermal_c,
                        trendName(sample.thermal_trend),
                        kThermalGuardThresholdC);
                    next = RuntimeState::ThermalGuard;
                }
            }
        }

        // STEP 6: Make control decision
        const Decision decision = policy_.decide(sample, next);

        // STEP 7: Handle state transition
        if (next != previous) {
            snapshot_.state = next;
            logStateTransition(previous, next);
        }

        // STEP 8: Evaluate notifications
        const NotificationEvent event = selectNotification(sample, next, previous);
        const bool cooldownExpired =
            sample_count_ >= last_notification_sample_ + kNotificationCooldownSamples;
        
        if (event != NotificationEvent::None &&
            (event != last_notification_ || cooldownExpired)) {
            emitNotification(event, sample);
            last_notification_ = event;
            last_notification_sample_ = sample_count_;
        }

        // STEP 9: Apply control mutations
        const MutationResult mutation =
            controller_.apply(next, sample, snapshot_.profile, config_);
        logMutation(mutation, next);

        // STEP 10: Periodic detailed logging
        const bool periodic = (sample_count_ % kPeriodicLogSamples) == 0;
        if (decision != Decision::NoAction || periodic) {
            logInfo(
                "SAMPLE=%llu state=%s decision=%s load=%.2f load_trend=%s cpu_util=%.1f%% "
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

        // STEP 11: Update history with circular buffer
        history_.push_back(sample);
        while (history_.size() > kHistorySize) {
            history_.pop_front();
        }
        
    } catch (const std::exception& e) {
        logError("Exception during tick: %s", e.what());
        // Continue operation despite error - robustness
    } catch (...) {
        logError("Unknown exception during tick processing");
        // Continue operation despite error - robustness
    }
    
    sample_count_++;
}

/**
 * Main engine loop - runs until stop signal received.
 * 
 * Features:
 * - Adaptive sleep duration based on system state
 * - Faster monitoring when system is under thermal load
 * - Slower monitoring during idle (up to 4x longer polling interval)
 * - Responsive to refresh and stop signals (100ms granularity)
 * - Clean shutdown with baseline restoration
 * 
 * @return 0 on success, 2 if initialization failed
 */
int AutonomousEngine::run() {
    // Initialize engine and validate device support
    if (!initialize()) {
        logError("Initialization failed - device may not be supported");
        return 2;
    }
    
    logStartup();

    // Main control loop
    while (!signal_state::stopRequested()) {
        // Handle runtime refresh requests
        if (signal_state::consumeRefresh()) {
            refreshEnvironment();
        }
        
        // Execute one control iteration
        tick();

        // Adaptive sleep: slower when idle, faster during activity
        int sleep_seconds = config_.monitorIntervalSeconds();
        if (snapshot_.state == RuntimeState::Idle) {
            // During idle: up to 4x slower monitoring (max 60 seconds).
            // This reduces polling work; battery savings are device-dependent.
            sleep_seconds = std::min(sleep_seconds * 4, 60);
        }

        // Sleep loop with 100ms granularity for signal responsiveness
        for (int i = 0; i < sleep_seconds * 10; ++i) {
            // Check stop signal
            if (signal_state::stopRequested()) {
                break;
            }
            
            // Handle refresh signal during sleep
            if (config_.runtimeRefreshEnabled() && signal_state::consumeRefresh()) {
                refreshEnvironment();
                break;
            }
            
            // Sleep for 100ms at a time
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // Clean shutdown: restore all mutations
    controller_.restoreAll();
    logInfo("CoreFlow Autonomous shutdown complete");
    return 0;
}

} // namespace coreflow
