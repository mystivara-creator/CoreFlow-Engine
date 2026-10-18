#include "coreflow/autonomous.hpp"
#include "coreflow/thermal_limits.hpp"
#include "coreflow/thermal_guard.hpp"
#include "coreflow/experience.hpp"
#include "coreflow/thermal_predictor.hpp"
#include "coreflow/decision_trace.hpp"
#include "coreflow/decision_agent.hpp"
#include "coreflow/frame_telemetry.hpp"
#include "coreflow/foreground_tracker.hpp"
#include "coreflow/gpu_busy_source.hpp"
#include "coreflow/interactivity_boost.hpp"
#include "coreflow/interactivity_model.hpp"
#include "coreflow/interactivity_policy.hpp"
#include "coreflow/launch_detector.hpp"
#include "coreflow/render_collector.hpp"

#include <android/log.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <memory>
#include <limits>
#include <unistd.h>
#include <cstdarg>
#include <cmath>
#include <exception>
#include <thread>
#include <utility>
#include <filesystem>
#include <fstream>

#include "coreflow/signal_state.hpp"
#include "coreflow/runtime_paths.hpp"

namespace coreflow {
namespace {

// ============================================================================
// CONFIGURATION & CONSTANTS
// ============================================================================

const RuntimePaths kPaths = RuntimePaths::defaults();

// Resolve interactivity targets from the current capability matrix. A path is
// eligible only when discovery says it exists and is writable; the actuator
// still performs its own authorization, safety, journal, and read-back checks.
struct InteractivityPaths final {
    std::string gpu_min_freq;
    std::string uclamp_min;
    std::string foreground_procs;
};

InteractivityPaths discoverInteractivityPaths(
    const EnvironmentCapabilityMatrix& matrix) {
    InteractivityPaths result;
    // Prefer actual foreground cgroups, not paths that merely contain words
    // like "perf". Candidates must be fully discovered and policy-authorized;
    // the mutation authority/actuator gates still apply at write time.
    auto pathScore = [](const std::string& p) -> int {
        if (p.find("/system-background/") != std::string::npos) return -50;
        if (p.find("/background/") != std::string::npos) return -30;
        if (p.find("/top-app/") != std::string::npos) return 50;
        if (p.find("/foreground/") != std::string::npos) return 30;
        return 0;
    };
    int best_gpu = -1000;
    int best_uclamp = -1000;
    for (const auto& resource : matrix.resources) {
        if (!resource.exists || !resource.readable || !resource.writable ||
            !resource.permission_granted || !resource.policy_authorized ||
            !resource.runtime_verified || !resource.mutation_ready ||
            resource.path.empty()) continue;
        const std::filesystem::path path(resource.path);
        const std::string filename = path.filename().string();
        const int score = pathScore(resource.path);
        if (resource.domain == ResourceDomain::Gpu && filename == "min_freq") {
            if (score > best_gpu) {
                best_gpu = score;
                result.gpu_min_freq = resource.path;
            }
        }
        if (resource.domain == ResourceDomain::UClamp &&
            (filename == "uclamp.min" || filename == "cpu.uclamp.min")) {
            // An interactivity boost must never be aimed at a background group
            // just because it is the only writable clamp discovered.
            if (score < 0) continue;
            if (score > best_uclamp) {
                best_uclamp = score;
                result.uclamp_min = resource.path;
            }
        }
    }
    // cgroup.procs is observation-only; derive from the chosen uclamp parent.
    if (!result.uclamp_min.empty()) {
        const auto sibling = std::filesystem::path(result.uclamp_min).parent_path() /
                             "cgroup.procs";
        std::ifstream probe(sibling);
        if (probe) result.foreground_procs = sibling.string();
    }
    return result;
}

// Log tag shared with main.cpp so both daemon entry and engine log together.
constexpr const char* kLogTag = "CoreFlowAutonomous";

// Bounded runtime sample window. Must hold at least
// kThermalMinimumHistorySamples so the thermal model can run; trends use its
// oldest entry.
constexpr std::size_t kHistorySize = 8U;

// Notification throttling: prevents repeated events
constexpr std::uint64_t kNotificationCooldownSamples = 12;

// Periodic diagnostics: balance between detail and storage efficiency
// Optimized: every 10 samples (less frequently than 6, reduces storage wear)
constexpr std::uint64_t kPeriodicLogSamples = 10;

// ============================================================================
// EFFICIENCY & SAFETY CONSTANTS
// ============================================================================

// ============================================================================
// THERMAL STABILITY (HYSTERESIS & PREDICTION)
// ============================================================================

// Thresholds are shared with the policy layer (see policy.hpp), so the
// engine and the policy can never disagree about when the guard exits.
constexpr double kThermalGuardThresholdC = kThermalGuardEnterC;

// After a measured regression, give the runtime a short settling window before
// considering another mutation. Candidate-specific rejection is handled by
// MutationController; this cooldown prevents immediate re-entry churn.
constexpr std::uint64_t kRegressionCooldownSamples = 4;

// After a beneficial or neutral outcome, hold the current decision for a
// short settling window instead of immediately starting another mutation.
// Balance: 3 samples keeps one-epoch safety without starving further experiments.
constexpr std::uint64_t kOutcomeHoldSamples = 3;

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

constexpr long kThermalMinMillidegrees = kThermalPlausibleMinMillidegrees;
constexpr long kThermalMaxMillidegrees = kThermalPlausibleMaxMillidegrees;
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




} // namespace

// ============================================================================
// AUTONOMOUS ENGINE IMPLEMENTATION
// ============================================================================

AutonomousEngine::AutonomousEngine()
    : journal_(kPaths.mutation_journal_cpu),
      resource_journal_(kPaths.mutation_journal_resource),
      cpu_retained_store_(kPaths.retained_cpu),
      resource_retained_store_(kPaths.retained_resource) {}

AutonomousEngine::~AutonomousEngine() {
    try {
        // Double-restore protection (from HARDENED)
        // Prevents calling restore twice if already restored
        if (!has_restored_) {
            const bool cpu_restored = controller_.restoreAll();
            const bool resource_restored = resource_controller_.restoreAll();
            has_restored_ = cpu_restored && resource_restored;
        }
        (void)experience_memory_.flush(kPaths.experience_db);
    } catch (const std::exception& e) {
        logError("Exception during destructor: %s", e.what());
    } catch (...) {
        logError("Unknown exception during destructor");
    }
}

bool AutonomousEngine::initialize() {
    controller_.setJournal(&journal_);
    resource_controller_.setJournal(&resource_journal_);
    controller_.setRetainedStateStore(&cpu_retained_store_);
    resource_controller_.setRetainedStateStore(&resource_retained_store_);
    controller_.setExperienceMemory(&experience_memory_);
    resource_controller_.setExperienceMemory(&experience_memory_);
    logInfo("CoreFlowInitializeEnter version=%s", kCoreFlowVersion);

    if (!config_.load(kPaths.config_ini)) {
        // A missing or unreadable config keeps the observe-only defaults.
        logWarn("config.ini not loaded; using observe-only defaults");
    }

    configured_mode_ = config_.mutationMode();
    configured_armed_ = config_.mutationArmed();

    snapshot_.profile = discovery_.discover();
    resource_model_.reset(snapshot_.profile.capabilities);
    logInfo("CAPABILITY_SURFACE vendor_files=%zu vendor_knobs=%zu walked=%zu tunables=%zu fatal_skipped=%zu",
            snapshot_.profile.vendor_files_parsed, snapshot_.profile.vendor_knobs,
            snapshot_.profile.tunables_walked, snapshot_.profile.tunables_added,
            snapshot_.profile.tunables_fatal_skipped);
    snapshot_.state = RuntimeState::Idle;
    history_.clear();
    sample_count_ = 0;
    last_notification_sample_ = 0;
    last_notification_ = NotificationEvent::None;
    has_restored_ = false;
    baseline_intelligence_.reset();
    std::error_code mkdir_error;
    std::filesystem::create_directories("/data/adb/coreflow", mkdir_error);
    experience_memory_.clear();
    const std::string scope = snapshot_.profile.environment.manufacturer + ":" +
                              snapshot_.profile.environment.device + ":" +
                              snapshot_.profile.environment.soc_model + ":" +
                              snapshot_.profile.kernel_release;
    experience_memory_.setScope(scope);
    (void)experience_memory_.load(kPaths.experience_db);
    mutation_cooldown_until_sample_ = 0;

    const bool ml_ready = thermal_predictor_.initialize(kPaths.thermal_model.c_str());
    logInfo("THERMAL_ML status=%s backend=%s",
            ml_ready ? "READY" : "FALLBACK",
            thermal_predictor_.usingModel() ? "ONNX" : "HEURISTIC");

    if (!controller_.captureBaseline(snapshot_.profile)) {
        logWarn("Factory baseline not established; mutation stays blocked until recovery succeeds");
    }
    if (!resource_controller_.captureBaseline(snapshot_.profile)) {
        logWarn("Resource baseline not established; ecosystem mutation stays blocked");
    } else {
        (void)resource_controller_.syncPolicyModel(resource_model_);
    }
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
    refresh_pending_ = false;
    controller_.setRetainedStateStore(&cpu_retained_store_);
    resource_controller_.setRetainedStateStore(&resource_retained_store_);
    controller_.setExperienceMemory(&experience_memory_);
    resource_controller_.setExperienceMemory(&experience_memory_);
    if (!config_.runtimeRefreshEnabled()) return false;

    logInfo("Runtime discovery refresh requested");

    // Refresh must never re-baseline a mutated system. If the factory restore
    // is incomplete, keep the current profile and retry on the next cycle.
    const bool cpu_restored = controller_.restoreAll();
    const bool resource_restored = resource_controller_.restoreAll();
    if (!cpu_restored || !resource_restored) {
        refresh_pending_ = true;
        logWarn("Refresh deferred: owned mutation restore incomplete, will retry");
        return false;
    }

    DeviceProfile refreshed = discovery_.discover();
    snapshot_.profile = std::move(refreshed);
    resource_model_.reset(snapshot_.profile.capabilities);
    logInfo("CAPABILITY_SURFACE vendor_files=%zu vendor_knobs=%zu walked=%zu tunables=%zu fatal_skipped=%zu",
            snapshot_.profile.vendor_files_parsed, snapshot_.profile.vendor_knobs,
            snapshot_.profile.tunables_walked, snapshot_.profile.tunables_added,
            snapshot_.profile.tunables_fatal_skipped);
    snapshot_.state = RuntimeState::Idle;
    history_.clear();
    sample_count_ = 0;
    last_notification_sample_ = 0;
    last_notification_ = NotificationEvent::None;
    has_restored_ = false;
    baseline_intelligence_.reset();
    experience_memory_.clear();
    const std::string scope = snapshot_.profile.environment.manufacturer + ":" +
                              snapshot_.profile.environment.device + ":" +
                              snapshot_.profile.environment.soc_model + ":" +
                              snapshot_.profile.kernel_release;
    experience_memory_.setScope(scope);
    (void)experience_memory_.load(kPaths.experience_db);
    mutation_cooldown_until_sample_ = 0;

    const bool ml_ready = thermal_predictor_.initialize(kPaths.thermal_model.c_str());
    logInfo("THERMAL_ML status=%s backend=%s",
            ml_ready ? "READY" : "FALLBACK",
            thermal_predictor_.usingModel() ? "ONNX" : "HEURISTIC");

    if (!controller_.captureBaseline(snapshot_.profile)) {
        logWarn("Factory baseline not re-established after refresh; mutation blocked");
    }
    if (!resource_controller_.captureBaseline(snapshot_.profile)) {
        logWarn("Resource baseline not re-established after refresh; ecosystem mutation blocked");
    } else {
        (void)resource_controller_.syncPolicyModel(resource_model_);
    }

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
    logInfo(
        "ENV     android=%s | sdk=%s | device=%s | soc=%s | cgroup_v2=%s | uclamp=%s | cpuset=%s | devfreq=%s",
        snapshot_.profile.environment.release.c_str(),
        snapshot_.profile.environment.sdk_level.c_str(),
        snapshot_.profile.environment.device.c_str(),
        snapshot_.profile.environment.soc_model.empty()
            ? snapshot_.profile.environment.hardware.c_str()
            : snapshot_.profile.environment.soc_model.c_str(),
        snapshot_.profile.environment.cgroup_v2 ? "yes" : "no",
        snapshot_.profile.environment.uclamp_available ? "yes" : "no",
        snapshot_.profile.environment.cpuset_available ? "yes" : "no",
        snapshot_.profile.environment.devfreq_available ? "yes" : "no"
    );
    logInfo(
        "CAPS    resources=%zu | mutation_ready=%zu | scheduler=%s",
        snapshot_.profile.capabilities.resources.size(),
        snapshot_.profile.capabilities.mutationReadyCount(),
        snapshot_.profile.environment.scheduler_controls_available ? "available" : "unavailable"
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
    if (sample.thermal_available) {
        logInfo("ALERT   %-18s | temp=%.2fC | trend=%s | chg=%s",
                notificationEventName(event),
                static_cast<double>(sample.thermal_millidegrees) / 1000.0,
                trendName(sample.thermal_trend),
                sample.charging ? "YES" : "NO");
    } else {
        logInfo("ALERT   %-18s | temp=n/a | trend=%s | chg=%s",
                notificationEventName(event),
                trendName(sample.thermal_trend),
                sample.charging ? "YES" : "NO");
    }
}

void AutonomousEngine::tick() {
    try {
        // SAFETY HOLD: an operator kill switch or a guard-raised SAFE_MODE forces
        // observe-only behaviour. Restores happen through the normal relax path,
        // so only a governor this daemon itself changed is ever written back.
        const HoldReason hold = evaluateSafetyHold({kPaths.kill_switch, kPaths.safe_mode});
        if (hold != hold_reason_) {
            logWarn("SAFETY_HOLD %s -> %s", holdReasonName(hold_reason_), holdReasonName(hold));
            hold_reason_ = hold;
        }
        const bool held = hold != HoldReason::None;
        config_.setMutationMode(held ? MutationMode::Disabled : configured_mode_);
        config_.setMutationArmed(held ? false : configured_armed_);

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

        // Autonomous safety hysteresis and predictive thermal guard are
        // applied after policy evaluation.
        if (!sample.thermal_available) {
            // Confirmation streaks count consecutive ticks only.
            thermal_guard_state_ = ThermalGuardState{};
        } else {
            // Safety uses the hottest valid policy-eligible sensor, not only
            // the representative sensor selected for trend/model features.
            ThermalGuardInputs guard_in;
            guard_in.selected_c =
                static_cast<double>(sample.thermal_millidegrees) / 1000.0;
            guard_in.hottest_c =
                static_cast<double>(sample.hottest_thermal_millidegrees) / 1000.0;
            guard_in.predicted_c =
                thermal_predictor_.predict(history_, sample, 3);
            guard_in.rising = sample.thermal_trend == Trend::Rising;

            if (previous == RuntimeState::ThermalGuard &&
                next != RuntimeState::ThermalGuard &&
                thermalGuardShouldHold(guard_in, kThermalGuardExitC)) {
                next = RuntimeState::ThermalGuard;
            } else if (next != RuntimeState::ThermalGuard &&
                       thermalGuardShouldEnter(guard_in,
                                               kThermalGuardThresholdC,
                                               thermal_guard_state_)) {
                next = RuntimeState::ThermalGuard;
                logWarn(
                    "THERMAL_GUARD_TRIGGER: selected=%.2fC hottest=%.2fC "
                    "predicted_3ticks=%.2fC",
                    guard_in.selected_c, guard_in.hottest_c,
                    guard_in.predicted_c);
            }
        }

        const Decision decision = policy_.decide(sample, next);

        // STEP 4c (v2.4.0): interactivity pipeline. Sensing and understanding
        // run every tick; adaptation stays behind the same authority, journal
        // and read-back discipline as every other write, and is suppressed by
        // the safety hold and the thermal guard.
        if (!interactivity_sources_ready_) {
            gpu_busy_source_.discoverDefaultRoots();
            const InteractivityPaths paths =
                discoverInteractivityPaths(snapshot_.profile.capabilities);
            // Empty means discovery did not find a safe observation path; do
            // not silently fall back to a vendor-specific default.
            foreground_tracker_.setProcsPath(paths.foreground_procs);
            interactivity_journal_ = std::make_unique<FileMutationJournal>(
                "/data/adb/coreflow/interactivity.journal");
            boost_controller_.setJournal(interactivity_journal_.get());
            boost_controller_.setGpuMinFreqPath(paths.gpu_min_freq);
            boost_controller_.setTopAppUclampPath(paths.uclamp_min);
            // Prefer fully admitted read-ahead nodes from capability discovery.
            // Accept both /sys/block/<device>/... and discovered /sys/devices/
            // .../block/<device>/... paths; avoid a brittle device-name blacklist.
            {
                std::size_t added = 0;
                for (const auto& resource : snapshot_.profile.capabilities.resources) {
                    if (resource.domain != ResourceDomain::Io ||
                        !resource.exists || !resource.readable || !resource.writable ||
                        !resource.permission_granted || !resource.policy_authorized ||
                        !resource.runtime_verified || !resource.mutation_ready) continue;
                    if (resource.path.find("/queue/read_ahead_kb") == std::string::npos) continue;
                    const bool sys_block = resource.path.find("/sys/block/") != std::string::npos;
                    const bool device_block = resource.path.find("/sys/devices/") != std::string::npos &&
                                              resource.path.find("/block/") != std::string::npos;
                    if (!sys_block && !device_block) continue;
                    boost_controller_.addReadaheadPath(resource.path);
                    ++added;
                }
                if (added == 0) boost_controller_.discoverReadaheadPaths();
            }
            // A previous run may have died while a boost was active. Adopt its
            // journal so the next pass restores the true baseline.
            (void)boost_controller_.recoverFromJournal();
            interactivity_sources_ready_ = true;
        }
        understand::InteractivityThresholds ia_thresholds;
        ia_thresholds.launch_window_ms =
            static_cast<std::uint64_t>(config_.launchWindowMs());
        ia_thresholds.jank_miss_rate = config_.jankMissRate();
        interactivity_model_.setThresholds(ia_thresholds);

        const std::uint64_t ia_now_ms = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch())
                .count());
        const bool ia_jank_recent =
            last_interactivity_state_ == understand::InteractionState::Stuttering;
        for (const std::uint64_t ia_duration_ns :
             frame_telemetry_.poll(ia_now_ms, ia_jank_recent)) {
            interactivity_model_.ingestFrame(ia_duration_ns, 16'666'667ULL);
        }
        // The launch detector and render collector are now part of the live
        // loop. Their signal is corroborative: ForegroundTracker remains the
        // PID-change signal, while process-count growth plus PSI can extend a
        // bounded launch window. Missing observation files fail closed.
        const std::string launch_procs_path = foreground_tracker_.procsPath();
        observe::LaunchObservation launch_observation;
        constexpr std::uint64_t kNsPerMs = 1'000'000ULL;
        const bool timestamp_convertible =
            ia_now_ms <= std::numeric_limits<std::uint64_t>::max() / kNsPerMs;
        const std::uint64_t ia_now_ns = timestamp_convertible
            ? ia_now_ms * kNsPerMs : 0U;
        if (timestamp_convertible && !launch_procs_path.empty() &&
            observe::readLaunchObservation(launch_procs_path, "/proc/pressure/io",
                                           ia_now_ns, launch_observation)) {
            const observe::LaunchPhase previous_phase = launch_detector_.phase();
            const observe::LaunchPhase phase = launch_detector_.observe(launch_observation);
            if (previous_phase == observe::LaunchPhase::Idle &&
                phase == observe::LaunchPhase::LaunchWindow) {
                interactivity_model_.onForegroundChanged(ia_now_ms);
                logInfo("INTERACTIVITY launch_window source=process_count_psi");
            }
        }
        std::string ia_foreground_package;
        if (foreground_tracker_.poll(ia_foreground_package)) {
            interactivity_model_.onForegroundChanged(ia_now_ms);
            logInfo("INTERACTIVITY foreground_change pkg=%s",
                    ia_foreground_package.empty() ? "unknown"
                                                  : ia_foreground_package.c_str());
        }
        understand::InteractivitySample ia_sample;
        ia_sample.now_ms = ia_now_ms;
        ia_sample.gpu_busy_available =
            gpu_busy_source_.poll(ia_sample.gpu_busy_ratio);
        ia_sample.io_full_avg10 = sample.psi_io_full_avg10;
        ia_sample.memory_some_avg10 = sample.psi_memory_some_avg10;
        ia_sample.thermal_guard = next == RuntimeState::ThermalGuard;
        const understand::InteractivitySnapshot ia_snap =
            interactivity_model_.evaluate(ia_sample);
        if (ia_snap.state != last_interactivity_state_) {
            logInfo("INTERACTIVITY_STATE %s -> %s gpu_busy=%.2f io_full=%.3f",
                    understand::interactionStateName(last_interactivity_state_),
                    understand::interactionStateName(ia_snap.state),
                    ia_snap.gpu_busy_ratio, ia_snap.io_full_avg10);
            last_interactivity_state_ = ia_snap.state;
        }

        boost_controller_.expire(ia_now_ms);
        if (next == RuntimeState::ThermalGuard || hold != HoldReason::None) {
            boost_controller_.restoreAll();
        } else if (boost_controller_.authorized(config_)) {
            InteractivityPolicy::BoostConfig ia_boost_config;
            ia_boost_config.boost_window_ms =
                static_cast<std::uint64_t>(config_.boostWindowMs());
            ia_boost_config.uclamp_launch = config_.boostUclampLaunch();
            ia_boost_config.uclamp_jank = config_.boostUclampJank();
            ia_boost_config.readahead_kb = config_.boostReadaheadKb();
            ia_boost_config.gpu_steps = config_.boostGpuSteps();
            interactivity_policy_.setConfig(ia_boost_config);
            const std::size_t ia_boosted = boost_controller_.apply(
                interactivity_policy_.evaluate(ia_snap, ia_now_ms),
                config_, ia_now_ms);
            if (ia_boosted > 0U) {
                logInfo("INTERACTIVITY_BOOST applied=%zu state=%s",
                        ia_boosted, understand::interactionStateName(ia_snap.state));
            }
        }

        // v1.9 foundation: classify context and build a plan without granting
        // an actuator permit. MutationController remains the only owner of the
        // final mutation gate.
        const SystemContext context = context_engine_.evaluate(sample, next);
        const PolicyPlan ecosystem_plan = policy_engine_.evaluate(context, resource_model_);
        // Log on eligibility change, and at a low fixed rate otherwise, so the
        // log is not flooded once per monitor tick.
        if (ecosystem_plan.mutation_eligible != last_context_eligible_ ||
            sample_count_ % 60 == 0) {
            logInfo(
                "CONTEXT workload=%s confidence=%.2f thermal_headroom=%s "
                "memory_headroom=%s power_headroom=%s mutation_eligible=%s "
                "stabilizing_only=%s candidates=%zu",
                workloadClassName(context.workload), context.confidence,
                context.thermal_headroom ? "yes" : "no",
                context.memory_headroom ? "yes" : "no",
                context.power_headroom ? "yes" : "no",
                ecosystem_plan.mutation_eligible ? "yes" : "no",
                ecosystem_plan.stabilizing_only ? "yes" : "no",
                ecosystem_plan.candidates.size());
            last_context_eligible_ = ecosystem_plan.mutation_eligible;
        }

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

        // STEP 9: Maintain the rolling efficiency baseline. It is refreshed with
        // every sample while no candidate is pending, so the next candidate is
        // always compared with the workload immediately before it (not with a
        // window captured once at daemon start). It is frozen while observing.
        // It is deliberately separate from MutationController's restore baseline.
        if (!baseline_intelligence_.observing() &&
            !controller_.hasPendingOutcome() &&
            !resource_controller_.hasPendingOutcome()) {
            const bool was_ready = baseline_intelligence_.baselineReady();
            const bool ready = baseline_intelligence_.captureBaseline(sample);
            if (ready && !was_ready) {
                logInfo("EFFICIENCY_BASELINE_READY samples=%zu",
                        BaselineIntelligence::kBaselineSamples);
            } else if (!ready) {
                logInfo("EFFICIENCY_BASELINE_CAPTURE %zu/%zu",
                        baseline_intelligence_.baselineSampleCount(),
                        BaselineIntelligence::kBaselineSamples);
            }
        }

        // Experience is keyed by the context in which the candidate was chosen,
        // because that is the context the controllers look it up with.
        const auto decisionContext = [&]() noexcept {
            ExperienceMemory::Context c;
            c.state = next;
            c.workload = context.workload;
            c.charging = sample.charging;
            c.thermal_trend = sample.thermal_trend;
            c.memory_trend = sample.memory_trend;
            c.load_trend = sample.load_trend;
            return c;
        };

        // The event context links actual milestones within this cycle. Event
        // records are diagnostic only and never participate in authorization.
        MutationResult mutation = MutationResult::Skipped;
        // Keep the CPU actuator result separate from outcome-resolution results,
        // which may refer to a resource candidate from the previous epoch.
        MutationResult cpu_mutation_for_trace = MutationResult::Skipped;
        DecisionTraceContext trace_context{
            sample_count_, previous, next, decision, sample.confidence,
            &context, &ecosystem_plan, hold, config_.mutationMode(),
            config_.mutationArmed(), config_.allowCpuGovernor(), mutation,
            MutationResult::Skipped};
        appendDecisionTraceEvent(trace_context, {
            "policy_evaluated", "policy",
            (ecosystem_plan.mutation_eligible &&
             ecosystem_plan.action == PolicyAction::Candidate) ? "ELIGIBLE" : "BLOCKED",
            ecosystem_plan.reason.empty() ? "policy_plan_has_no_explanation"
                                          : ecosystem_plan.reason.c_str()});

        // STEP 10: Observe a verified mutation without allowing another
        // mutation to alter the candidate during the observation window.
        if (baseline_intelligence_.observing()) {
            const bool complete = baseline_intelligence_.observe(sample);
            if (complete) {
                const auto& evaluation = baseline_intelligence_.result();
                logInfo(
                    "EFFICIENCY_OUTCOME outcome=%d score=%.3f confidence=%.3f "
                    "baseline=%zu observation=%zu",
                    static_cast<int>(evaluation.outcome),
                    evaluation.overall_score,
                    evaluation.confidence,
                    evaluation.baseline_samples,
                    evaluation.observation_samples);

                // STEP 10b: Persist the verified outcome as advisory experience,
                // under the decision-time context captured at observation begin.
                const ExperienceMemory::Context experience_context = observation_context_;

                for (const auto& applied : controller_.lastAppliedGovernors()) {
                    ExperienceMemory::CandidateIdentity candidate;
                    candidate.key = "cpufreq:" + applied.first + ":" + applied.second;

                    if (experience_memory_.recordEvaluation(
                            candidate,
                            experience_context,
                            evaluation)) {
                        logInfo(
                            "EXPERIENCE_RECORDED candidate=%s score=%.3f "
                            "confidence=%.3f observations=%llu",
                            candidate.key.c_str(),
                            evaluation.overall_score,
                            evaluation.confidence,
                            static_cast<unsigned long long>(
                                evaluation.observation_samples));
                    }
                }
                for (const auto& applied : resource_controller_.lastAppliedResources()) {
                    ExperienceMemory::CandidateIdentity candidate;
                    candidate.key = "resource:" + applied.first + ":" + applied.second;
                    (void)experience_memory_.recordEvaluation(
                        candidate, experience_context, evaluation);
                }

                // Outcomes are rare (one per baseline+observation epoch), so
                // persist each one. Gating on sample_count_ % N here almost never
                // matched and left experience only in RAM until shutdown.
                (void)experience_memory_.flush(kPaths.experience_db);

                switch (evaluation.outcome) {
                    case BaselineIntelligence::Outcome::Beneficial: {
                        mutation_cooldown_until_sample_ =
                            sample_count_ + kOutcomeHoldSamples + 1U;
                        // Both controllers may hold a pending candidate from the
                        // same observation epoch. Handle each independently so a
                        // dual-pending window never leaves one controller active
                        // while the other is retained or rolled back.
                        const bool cpu_pending = controller_.hasPendingOutcome();
                        const bool resource_pending =
                            resource_controller_.hasPendingOutcome();
                        // A single observation score cannot attribute benefit to
                        // two actuator classes. Dual-pending violates the
                        // one-actuator-per-epoch invariant, so do not retain either
                        // candidate based on ambiguous evidence.
                        if (cpu_pending && resource_pending) {
                            bool rolled_back = controller_.rollbackLastMutation();
                            if (!resource_controller_.rollbackLastMutation()) {
                                rolled_back = false;
                            }
                            if (!rolled_back) {
                                const bool cpu_recovered = controller_.restoreAll();
                                const bool resource_recovered = resource_controller_.restoreAll();
                                rolled_back = cpu_recovered && resource_recovered;
                            }
                            refresh_pending_ = !rolled_back;
                            mutation = rolled_back ? MutationResult::RolledBack
                                                   : MutationResult::Failed;
                            logError("DUAL_PENDING_OUTCOME action=ROLLBACK result=%s",
                                     rolled_back ? "verified" : "failed");
                            break;
                        }
                        bool retained = false;
                        if (cpu_pending) {
                            retained = controller_.retainLastMutation(snapshot_.profile);
                        } else if (resource_pending) {
                            retained = resource_controller_.retainLastMutation(
                                snapshot_.profile);
                        }
                        if ((cpu_pending || resource_pending) && !retained) {
                            const bool cpu_retention_failed = cpu_pending;
                            const RetainedStateFailureStage retention_stage =
                                cpu_retention_failed
                                    ? controller_.lastRetainedStateFailureStage()
                                    : resource_controller_.lastRetainedStateFailureStage();
                            const char* retention_stage_name =
                                retainedStateFailureStageName(retention_stage);
                            appendDecisionTraceEvent(trace_context, {
                                "retained_state_commit_failed",
                                cpu_retention_failed ? "cpu_freq" : "resource",
                                "FAILED",
                                retention_stage_name,
                                false, false,
                                retention_stage_name});

                            // Never leave a candidate active as "beneficial" if
                            // its durable retained record could not be committed.
                            bool rolled_back = true;
                            if (controller_.hasPendingOutcome()) {
                                if (!controller_.rollbackLastMutation()) {
                                    rolled_back = false;
                                }
                            }
                            if (resource_controller_.hasPendingOutcome()) {
                                if (!resource_controller_.rollbackLastMutation()) {
                                    rolled_back = false;
                                }
                            }
                            if (!rolled_back) {
                                const bool cpu_recovered = controller_.restoreAll();
                                const bool resource_recovered = resource_controller_.restoreAll();
                                const bool recovered = cpu_recovered && resource_recovered;
                                refresh_pending_ = !recovered;
                                rolled_back = recovered;
                            }
                            mutation = rolled_back ? MutationResult::RolledBack
                                                   : MutationResult::Failed;
                            logError("RETAINED_STATE_COMMIT_FAILED recovery=%s",
                                     rolled_back ? "verified" : "failed");
                        } else if (cpu_pending || resource_pending) {
                            logInfo("EFFICIENCY_BENEFICIAL action=RETAINED "
                                    "hold_until_sample=%llu",
                                    static_cast<unsigned long long>(
                                        mutation_cooldown_until_sample_));
                        }
                        break;
                    }

                    case BaselineIntelligence::Outcome::Neutral:
                    case BaselineIntelligence::Outcome::Inconclusive:
                    case BaselineIntelligence::Outcome::Unknown: {
                        mutation_cooldown_until_sample_ =
                            sample_count_ + kOutcomeHoldSamples + 1U;
                        const bool cpu_pending = controller_.hasPendingOutcome();
                        const bool resource_pending =
                            resource_controller_.hasPendingOutcome();
                        const bool had_pending_candidate =
                            cpu_pending || resource_pending;
                        bool rolled_back = true;
                        if (cpu_pending) {
                            if (!controller_.rollbackLastMutation()) {
                                rolled_back = false;
                            }
                        }
                        if (resource_pending) {
                            if (!resource_controller_.rollbackLastMutation()) {
                                rolled_back = false;
                            }
                        }
                        if (!had_pending_candidate) {
                            rolled_back = false;
                        }
                        if (!rolled_back) {
                            const bool cpu_recovered = controller_.restoreAll();
                            const bool resource_recovered = resource_controller_.restoreAll();
                            const bool recovered = cpu_recovered && resource_recovered;
                            refresh_pending_ = !recovered;
                            rolled_back = recovered;
                        }
                        mutation = rolled_back ? MutationResult::RolledBack
                                               : MutationResult::Failed;
                        logInfo("EFFICIENCY_UNPROVEN action=%s result=%s "
                                "until_sample=%llu",
                                had_pending_candidate ? "ROLLBACK" : "RECOVERY_NO_PENDING",
                                rolled_back ? "verified" : "failed",
                                static_cast<unsigned long long>(
                                    mutation_cooldown_until_sample_));
                        break;
                    }

                    case BaselineIntelligence::Outcome::Regression: {
                        mutation_cooldown_until_sample_ =
                            sample_count_ + kRegressionCooldownSamples + 1U;
                        const bool cpu_pending = controller_.hasPendingOutcome();
                        const bool resource_pending =
                            resource_controller_.hasPendingOutcome();
                        const bool had_pending_candidate =
                            cpu_pending || resource_pending;
                        bool restored = true;
                        if (cpu_pending) {
                            controller_.rejectLastMutation();
                            if (!controller_.rollbackLastMutation()) {
                                restored = false;
                            }
                        }
                        if (resource_pending) {
                            resource_controller_.rejectLastMutation();
                            if (!resource_controller_.rollbackLastMutation()) {
                                restored = false;
                            }
                        }
                        if (!had_pending_candidate) {
                            restored = false;
                        }
                        if (!restored) {
                            const bool cpu_recovered = controller_.restoreAll();
                            const bool resource_recovered = resource_controller_.restoreAll();
                            const bool recovered = cpu_recovered && resource_recovered;
                            refresh_pending_ = !recovered;
                            restored = recovered;
                        }
                        mutation = restored ? MutationResult::RolledBack
                                            : MutationResult::Failed;
                        logInfo(
                            "EFFICIENCY_REGRESSION action=%s result=%s "
                            "suppression=ACTIVE until_sample=%llu",
                            had_pending_candidate ? "ROLLBACK" : "RECOVERY_NO_PENDING",
                            restored ? "VERIFIED" : "FAILED",
                            static_cast<unsigned long long>(
                                mutation_cooldown_until_sample_));
                        break;
                    }

                }

                const char* outcome_reason = "observation_outcome_unknown";
                switch (evaluation.outcome) {
                    case BaselineIntelligence::Outcome::Beneficial:
                        outcome_reason = (mutation == MutationResult::RolledBack ||
                                          mutation == MutationResult::Failed)
                            ? "beneficial_outcome_retention_failed_recovery_attempted"
                            : "beneficial_outcome_candidate_retained";
                        break;
                    case BaselineIntelligence::Outcome::Neutral:
                        outcome_reason = "neutral_outcome_candidate_rollback_attempted";
                        break;
                    case BaselineIntelligence::Outcome::Inconclusive:
                        outcome_reason = "inconclusive_outcome_candidate_rollback_attempted";
                        break;
                    case BaselineIntelligence::Outcome::Unknown:
                        outcome_reason = "unknown_outcome_candidate_rollback_attempted";
                        break;
                    case BaselineIntelligence::Outcome::Regression:
                        outcome_reason = "regression_outcome_candidate_rejection_and_rollback_attempted";
                        break;
                }
                appendDecisionTraceEvent(trace_context, {
                    "outcome_resolved", "pending_mutation_epoch",
                    mutationResultName(mutation), outcome_reason});
            }
        } else if (baseline_intelligence_.baselineReady()) {
            const bool mutationCooldownActive =
                sample_count_ < mutation_cooldown_until_sample_;

            if (!mutationCooldownActive && !controller_.hasPendingOutcome() &&
                !resource_controller_.hasPendingOutcome()) {
                const char* cpu_gate_reason = mutationPermitBlockReason(
                    ecosystem_plan, config_, next, sample.confidence,
                    MutationPermit::Scope::CpuFreq);
                const MutationPermit cpu_permit = mutation_authority_.authorize(
                    ecosystem_plan, config_, next, sample.confidence,
                    MutationPermit::Scope::CpuFreq);
                appendDecisionTraceEvent(trace_context, {
                    "scope_gate_checked", "cpu_freq_coarse",
                    cpu_permit.valid() ? "ALLOW" : "DENY",
                    cpu_permit.valid()
                        ? "coarse_scope_passed_target_capability_still_required"
                        : cpu_gate_reason,
                    true, cpu_permit.valid()});
                mutation = controller_.apply(
                    next, sample, snapshot_.profile, config_, ecosystem_plan, cpu_permit,
                    &trace_context);
                cpu_mutation_for_trace = mutation;
                trace_context.cpu_mutation = cpu_mutation_for_trace;
                trace_context.cpu_mutation_known = true;
                appendDecisionTraceEvent(trace_context, {
                    "mutation_controller_returned", "cpu_freq",
                    mutationResultName(mutation), mutationTraceOutcomeReason(mutation)});
            } else {
                const char* held_reason = mutationCooldownActive
                    ? "mutation_cooldown_active"
                    : "pending_outcome_blocks_new_cpu_candidate";
                appendDecisionTraceEvent(trace_context, {
                    "mutation_held", "cpu_freq", "HELD", held_reason});
                logInfo(
                    "EFFICIENCY_MUTATION_HELD until_sample=%llu",
                    static_cast<unsigned long long>(mutation_cooldown_until_sample_));
            }

            if (mutation == MutationResult::Verified) {
                if (baseline_intelligence_.beginObservation()) {
                    observation_context_ = decisionContext();
                    appendDecisionTraceEvent(trace_context, {
                        "observation_window_started", "cpu_freq", "STARTED",
                        "verified_mutation_has_measurement_window"});
                    logInfo("EFFICIENCY_OBSERVATION_BEGIN samples=%zu",
                            BaselineIntelligence::kObservationSamples);
                } else {
                    // A verified write without a measurable observation window
                    // would leave pending_outcome stuck and block all later
                    // candidates. Roll back immediately (fail-closed).
                    appendDecisionTraceEvent(trace_context, {
                        "observation_window_failed", "cpu_freq", "FAILED",
                        "verified_mutation_could_not_open_observation_window"});
                    const bool rolled = controller_.rollbackLastMutation() ||
                                       controller_.restoreAll();
                    mutation = rolled ? MutationResult::RolledBack
                                     : MutationResult::Failed;
                    cpu_mutation_for_trace = mutation;
                    trace_context.cpu_mutation = cpu_mutation_for_trace;
                    trace_context.cpu_mutation_known = true;
                    appendDecisionTraceEvent(trace_context, {
                        "rollback_completed", "cpu_freq", mutationResultName(mutation),
                        rolled ? "observation_window_failure_rollback_succeeded"
                               : "observation_window_failure_rollback_failed"});
                    logError("EFFICIENCY_OBSERVATION_BEGIN_FAILED action=ROLLBACK "
                             "result=%s",
                             rolled ? "verified" : "failed");
                }
            }
        } else {
            appendDecisionTraceEvent(trace_context, {
                "mutation_held", "cpu_freq", "HELD", "baseline_capture_not_ready"});
        }

        // Accepted settings persist until a real safety/authority transition.
        // Cooldown throttles new candidates only; it never expires an accepted
        // mutation merely because a number of samples elapsed.
        const bool mutation_epoch_active =
            controller_.mutated() || resource_controller_.mutated();
        EpochRestoreInputs epoch_inputs;
        epoch_inputs.state = next;
        epoch_inputs.mode_adaptive = config_.mutationMode() == MutationMode::Adaptive;
        epoch_inputs.armed = config_.mutationArmed();
        epoch_inputs.cpu_mutated = controller_.mutated();
        epoch_inputs.resource_epoch_stabilizing = resource_controller_.stabilizingEpoch();
        epoch_inputs.plan_stabilizing_only = ecosystem_plan.stabilizing_only;
        epoch_inputs.plan_mutation_eligible = ecosystem_plan.mutation_eligible;
        const bool safety_restore = shouldRestoreEpochNow(epoch_inputs);
        if (mutation_epoch_active && safety_restore) {
            const bool cpu_restored = controller_.restoreAll();
            const bool resource_restored = resource_controller_.restoreAll();
            trace_context.cpu_mutation = cpu_mutation_for_trace;
            appendDecisionTraceEvent(trace_context, {
                "safety_restore_completed", "mutation_epoch",
                (cpu_restored && resource_restored) ? "RESTORED" : "FAILED",
                cpu_restored
                    ? (resource_restored ? "cpu_restore_ok_resource_restore_ok"
                                         : "cpu_restore_ok_resource_restore_failed")
                    : (resource_restored ? "cpu_restore_failed_resource_restore_ok"
                                         : "cpu_restore_failed_resource_restore_failed")});
            if (!cpu_restored || !resource_restored) {
                logError("MUTATION_EPOCH_RESTORE_FAILED cpu=%s resource=%s",
                         cpu_restored ? "ok" : "failed",
                         resource_restored ? "ok" : "failed");
                refresh_pending_ = true;
            } else {
                baseline_intelligence_.reset();
                logInfo("MUTATION_EPOCH_RESTORED reason=safety");
            }
        }

        // Exactly one actuator class may mutate in a decision cycle. This keeps
        // the efficiency measurement causal and attributable to one candidate.
        MutationResult resource_mutation = MutationResult::Skipped;
        const bool resource_mutation_cooldown = sample_count_ < mutation_cooldown_until_sample_;
        // A candidate may only be applied when its outcome can be measured: a
        // resource write without a ready baseline could never begin observation,
        // stayed pending forever and blocked every later candidate.
        if (!resource_mutation_cooldown && mutation != MutationResult::Verified &&
            baseline_intelligence_.baselineReady() &&
            !baseline_intelligence_.observing() && !controller_.hasPendingOutcome() &&
            !resource_controller_.hasPendingOutcome()) {
            const char* resource_gate_reason = mutationPermitBlockReason(
                ecosystem_plan, config_, next, sample.confidence,
                MutationPermit::Scope::Resource);
            const MutationPermit resource_permit = mutation_authority_.authorize(
                ecosystem_plan, config_, next, sample.confidence,
                MutationPermit::Scope::Resource);
            appendDecisionTraceEvent(trace_context, {
                "scope_gate_checked", "resource_coarse",
                resource_permit.valid() ? "ALLOW" : "DENY",
                resource_permit.valid()
                    ? "coarse_scope_passed_target_capability_still_required"
                    : resource_gate_reason,
                true, resource_permit.valid()});
            resource_mutation = resource_controller_.apply(
                next, sample, snapshot_.profile, config_, ecosystem_plan, resource_permit,
                &trace_context);
            trace_context.resource_mutation = resource_mutation;
            trace_context.resource_mutation_known = true;
            appendDecisionTraceEvent(trace_context, {
                "mutation_controller_returned", "resource",
                mutationResultName(resource_mutation),
                (resource_mutation == MutationResult::Failed ||
                 resource_mutation == MutationResult::RolledBack)
                    ? resourceMutationFailureStageName(resource_controller_.lastFailureStage())
                    : mutationTraceOutcomeReason(resource_mutation),
                false, false,
                resourceMutationFailureStageName(resource_controller_.lastFailureStage()),
                actuatorStatusName(resource_controller_.lastActuatorStatus()).data()});
            if (resource_mutation == MutationResult::Verified) {
                if (baseline_intelligence_.beginObservation()) {
                    observation_context_ = decisionContext();
                    appendDecisionTraceEvent(trace_context, {
                        "observation_window_started", "resource", "STARTED",
                        "verified_mutation_has_measurement_window"});
                    logInfo("EFFICIENCY_OBSERVATION_BEGIN resource samples=%zu",
                            BaselineIntelligence::kObservationSamples);
                } else {
                    // Same fail-closed rule as CPU: never leave a verified
                    // resource write without an observation epoch.
                    appendDecisionTraceEvent(trace_context, {
                        "observation_window_failed", "resource", "FAILED",
                        "verified_mutation_could_not_open_observation_window"});
                    const bool rolled =
                        resource_controller_.rollbackLastMutation() ||
                        resource_controller_.restoreAll();
                    resource_mutation = rolled ? MutationResult::RolledBack
                                               : MutationResult::Failed;
                    trace_context.resource_mutation = resource_mutation;
                    appendDecisionTraceEvent(trace_context, {
                        "rollback_completed", "resource", mutationResultName(resource_mutation),
                        rolled ? "observation_window_failure_rollback_succeeded"
                               : "observation_window_failure_rollback_failed",
                        false, false,
                        resourceMutationFailureStageName(resource_controller_.lastFailureStage()),
                        actuatorStatusName(resource_controller_.lastActuatorStatus()).data()});
                    logError("EFFICIENCY_OBSERVATION_BEGIN_FAILED resource "
                             "action=ROLLBACK result=%s",
                             rolled ? "verified" : "failed");
                }
            }
        } else {
            const char* held_reason = !baseline_intelligence_.baselineReady()
                ? "baseline_capture_not_ready"
                : baseline_intelligence_.observing()
                    ? "baseline_outcome_observation_active"
                    : resource_mutation_cooldown
                        ? "mutation_cooldown_active"
                        : mutation == MutationResult::Verified
                            ? "cpu_mutation_already_verified_this_cycle"
                            : "pending_outcome_blocks_new_resource_candidate";
            appendDecisionTraceEvent(trace_context, {
                "mutation_held", "resource", "HELD", held_reason});
        }
        if (resource_mutation != MutationResult::Skipped) {
            logInfo("RESOURCE_AUTONOMOUS result=%s",
                    mutationResultName(resource_mutation));
            const auto failure_stage = resource_controller_.lastFailureStage();
            if (failure_stage != ResourceMutationFailureStage::None &&
                (resource_mutation == MutationResult::Failed ||
                 resource_mutation == MutationResult::RolledBack)) {
                logError("RESOURCE_MUTATION_DIAG result=%s stage=%s actuator_status=%u",
                         mutationResultName(resource_mutation),
                         resourceMutationFailureStageName(failure_stage),
                         static_cast<unsigned>(
                             resource_controller_.lastActuatorStatus()));
            }
        } else if (!ecosystem_plan.mutation_eligible) {
            logInfo("RESOURCE_AUTONOMOUS result=SKIPPED reason=POLICY_BLOCKED state=%s",
                    stateName(next));
        }

        logMutation(mutation, next);

        // Observation-only journal: records the completed evaluation/results;
        // it does not grant actuator authority or alter any safety gate.
        appendDecisionTrace(
            sample_count_, previous, next, decision, sample.confidence,
            context, ecosystem_plan, hold, config_.mutationMode(),
            config_.mutationArmed(), config_.allowCpuGovernor(),
            cpu_mutation_for_trace, resource_mutation);

        // STEP 11: Periodic detailed logging (reduced frequency for storage efficiency)
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
                "       cpu %5.1f%% | load %5.2f %-7s | mem %5.1f%% %-7s | temp %5.2fC | io %.0f/%.0fKB/s | batt=%d%%",
                sample.cpu_utilization * 100.0,
                sample.load1,
                trendName(sample.load_trend),
                sample.mem_available_ratio * 100.0,
                trendName(sample.memory_trend),
                static_cast<double>(sample.thermal_millidegrees) / 1000.0,
                sample.io_read_kb_per_sec,
                sample.io_write_kb_per_sec,
                sample.battery_level_percent
            );
            logInfo("       thermal-sensors total=%u valid=%u selected=%.2fC hottest=%.2fC",
                    sample.thermal_sensor_count,
                    sample.thermal_valid_sensor_count,
                    static_cast<double>(sample.thermal_millidegrees) / 1000.0,
                    static_cast<double>(sample.hottest_thermal_millidegrees) / 1000.0);
            if (periodic) {
                for (const ThermalReading& reading : sample.thermal_readings) {
                    const char* status = "UNREADABLE";
                    switch (reading.status) {
                        case ThermalReadingStatus::Valid: status = "VALID"; break;
                        case ThermalReadingStatus::Unreadable: status = "UNREADABLE"; break;
                        case ThermalReadingStatus::InvalidRange: status = "INVALID_RANGE"; break;
                        case ThermalReadingStatus::ExcludedFromPolicy: status = "EXCLUDED_POLICY"; break;
                    }
                    if (reading.readable) {
                        logInfo("       thermal-zone=%s type=%s temp=%.3fC status=%s policy=%s",
                                reading.zone.c_str(), reading.type.c_str(),
                                static_cast<double>(reading.temperature_millidegrees) / 1000.0,
                                status, reading.policy_eligible ? "yes" : "no");
                    } else {
                        logInfo("       thermal-zone=%s type=%s status=%s",
                                reading.zone.c_str(), reading.type.c_str(), status);
                    }
                }
            }
            if (sample.process_profile_available) {
                logInfo("       top-process=%s cpu=%.1f%% rss=%llukB count=%u",
                        sample.top_process_name.c_str(),
                        sample.top_process_cpu_ratio * 100.0,
                        static_cast<unsigned long long>(sample.top_process_memory_kb),
                        sample.process_count);
            }
        }

        // STEP 13: Update history
        history_.push_back(sample);
        while (history_.size() > kHistorySize) {
            history_.pop_front();
        }

        // STEP 14: Export live status for the WebUI (best-effort, no authority).
        publishStatus(sample, previous, next, decision, context, ecosystem_plan,
                      hold, mutation, resource_mutation);

    } catch (const std::exception& e) {
        logError("Exception during tick: %s", e.what());
    } catch (...) {
        logError("Unknown exception during tick");
    }

    ++sample_count_;
}

void AutonomousEngine::publishStatus(
    const RuntimeSample& sample,
    const RuntimeState previous,
    const RuntimeState next,
    const Decision decision,
    const SystemContext& context,
    const PolicyPlan& plan,
    const HoldReason hold,
    const MutationResult cpu,
    const MutationResult resource
) noexcept {
    try {
        const auto tally = [this](const MutationResult r, const char* kind) {
            if (r == MutationResult::Verified) ++status_verified_;
            else if (r == MutationResult::RolledBack) ++status_rolled_back_;
            else if (r == MutationResult::Failed) ++status_failed_;
            if (r != MutationResult::Skipped) {
                last_change_sample_ = sample_count_;
                last_change_kind_ = kind;
                last_change_result_ = mutationResultName(r);
            }
        };
        tally(cpu, "cpu");
        tally(resource, "resource");

        EngineStatus s;
        s.version = kCoreFlowVersion;
        s.pid = static_cast<long>(::getpid());
        s.sample = sample_count_;
        s.updated_epoch = static_cast<std::int64_t>(std::time(nullptr));
        s.interval_s = config_.monitorIntervalSeconds();
        s.running = true;

        const AndroidEnvironment& env = snapshot_.profile.environment;
        s.manufacturer = env.manufacturer;
        s.model = env.model;
        s.device = env.device;
        s.product = env.product;
        s.board = env.board;
        s.hardware = env.hardware;
        s.soc_manufacturer = env.soc_manufacturer;
        s.soc_model = env.soc_model;
        s.android_release = env.release;
        s.sdk_level = env.sdk_level;
        s.kernel_release = env.kernel_release;
        s.abi = env.abi;
        s.proc_available = snapshot_.profile.proc_available;
        s.sys_available = snapshot_.profile.sys_available;
        s.cgroup_v2 = env.cgroup_v2;
        s.cpuset_available = env.cpuset_available;
        s.uclamp_available = env.uclamp_available;
        s.scheduler_controls_available = env.scheduler_controls_available;
        s.devfreq_available = env.devfreq_available;
        s.discovered_resources = snapshot_.profile.capabilities.resources.size();
        s.mutation_ready_resources = snapshot_.profile.capabilities.mutationReadyCount();
        s.expanded_domains = snapshot_.profile.expanded_domains;
        // Scheduler tokens remain quarantined; read_ahead/nr_requests on real
        // devices are now BoundedTune. Flag reflects residual scheduler quarantine.
        s.block_queue_mutations_quarantined = false;

        s.thermal_available = sample.thermal_available;
        s.thermal_c = static_cast<double>(sample.thermal_millidegrees) / 1000.0;
        s.hottest_c = static_cast<double>(sample.hottest_thermal_millidegrees) / 1000.0;
        s.thermal_trend = trendName(sample.thermal_trend);
        s.mem_available_ratio = sample.mem_available_ratio;
        s.memory_trend = trendName(sample.memory_trend);
        s.cpu_available = sample.cpu_utilization_available;
        s.cpu_utilization = sample.cpu_utilization;
        s.load1 = sample.load1;
        s.load_trend = trendName(sample.load_trend);
        s.io_available = sample.io_activity_available;
        s.io_read_kbs = sample.io_read_kb_per_sec;
        s.io_write_kbs = sample.io_write_kb_per_sec;
        s.battery_percent = sample.battery_level_percent;
        s.charging = sample.charging;
        s.confidence = sample.confidence;

        s.state = stateName(next);
        s.previous_state = stateName(previous);
        s.decision = decisionName(decision);
        s.workload = workloadClassName(context.workload);
        s.thermal_headroom = context.thermal_headroom;
        s.memory_headroom = context.memory_headroom;
        s.power_headroom = context.power_headroom;
        s.proactive_allowed = context.mutation_allowed_by_context;
        s.stabilizing_allowed = context.stabilizing_allowed_by_context;
        s.plan_action = policyActionName(plan.action);
        s.intervention = interventionLevelName(plan.intervention);
        s.mutation_eligible = plan.mutation_eligible;
        s.stabilizing_only = plan.stabilizing_only;
        s.candidates = plan.candidates.size();
        s.plan_reason = plan.reason;
        s.agent_mode = agentReasoningModeName(AgentReasoningMode::ReActLite);
        s.agent_reason = resource_controller_.lastAgentReason();
        s.consultation_ledger = resource_controller_.consultationLedger();
        s.hold_active = hold != HoldReason::None;
        s.safety_hold = holdReasonName(hold);
        // WebUI / primaryBlocker expect config-style tokens, not log labels.
        s.mode = (config_.mutationMode() == MutationMode::Adaptive)
                     ? "adaptive"
                     : "observe";
        s.armed = config_.mutationArmed();
        s.allow_cpu_governor = config_.allowCpuGovernor();
        // Re-evaluate the single authority gate for display. authorize() is const
        // and has no side effects; this is not a permit anyone can use.
        s.permit_resource = mutation_authority_.authorize(
            plan, config_, next, sample.confidence,
            MutationPermit::Scope::Resource).valid();
        s.permit_cpu = mutation_authority_.authorize(
            plan, config_, next, sample.confidence,
            MutationPermit::Scope::CpuFreq).valid();
        s.baseline_ready = baseline_intelligence_.baselineReady();
        s.baseline_samples = baseline_intelligence_.baselineSampleCount();
        s.baseline_target = BaselineIntelligence::kBaselineSamples;
        s.observation_samples = baseline_intelligence_.observationSampleCount();
        s.observation_target = BaselineIntelligence::kObservationSamples;
        s.observing = baseline_intelligence_.observing();
        s.cooldown_remaining = sample_count_ < mutation_cooldown_until_sample_
            ? mutation_cooldown_until_sample_ - sample_count_ : 0U;

        s.cpu_mutated = controller_.mutated();
        s.resource_mutated = resource_controller_.mutated();
        s.stabilizing_epoch = resource_controller_.stabilizingEpoch();

        // Human-readable why this cycle did not open a new mutation epoch.
        if (s.hold_active) {
            s.skip_reason = "safety hold active";
        } else if (s.observing) {
            s.skip_reason = "observing outcome epoch " +
                std::to_string(s.observation_samples) + "/" +
                std::to_string(s.observation_target);
        } else if (s.cpu_mutated || s.resource_mutated) {
            s.skip_reason = "change held; one mutation epoch at a time";
        } else if (s.cooldown_remaining > 0) {
            s.skip_reason = "outcome/regression cooldown " +
                std::to_string(s.cooldown_remaining) + " sample(s) left";
        } else if (!s.baseline_ready) {
            s.skip_reason = "baseline capture " +
                std::to_string(s.baseline_samples) + "/" +
                std::to_string(s.baseline_target);
        } else if (!s.mutation_eligible) {
            s.skip_reason = s.plan_reason.empty() ? "policy not eligible" : s.plan_reason;
        } else if (s.candidates == 0) {
            s.skip_reason = "no ranked candidates this cycle";
        } else if (cpu == MutationResult::Skipped &&
                   resource == MutationResult::Skipped) {
            s.skip_reason = "candidates present; authority or score declined write";
        } else {
            s.skip_reason.clear();
        }
        for (const auto& applied : controller_.activeGovernorSettings()) {
            s.cpu_applied.emplace_back(applied.first, applied.second);
        }
        for (const auto& applied : resource_controller_.activeResourceSettings()) {
            s.resource_applied.emplace_back(applied.first, applied.second);
        }
        s.last_cpu_result = mutationResultName(cpu);
        s.last_resource_result = mutationResultName(resource);
        s.last_change_kind = last_change_kind_;
        s.last_change_result = last_change_result_;
        s.last_change_sample = last_change_sample_;
        s.verified = status_verified_;
        s.rolled_back = status_rolled_back_;
        s.failed = status_failed_;

        last_status_ = s;
        // When Idle, throttle status.json writes to reduce storage wakeups
        // that would otherwise prevent deep sleep. Always write on the first
        // sample and every 3rd idle cycle thereafter; non-idle always writes.
        const bool idle = (next == RuntimeState::Idle);
        const bool should_write = !idle || (sample_count_ % 3U) == 0U;
        if (should_write) {
            (void)writeStatusFile(kPaths.status_json, statusToJson(s));
        }
    } catch (...) {
        // Status is advisory. Never let it disturb the control loop.
    }
}

void AutonomousEngine::publishStoppedStatus() noexcept {
    try {
        EngineStatus s = last_status_;
        s.running = false;
        s.updated_epoch = static_cast<std::int64_t>(std::time(nullptr));
        s.cpu_mutated = controller_.mutated();
        s.resource_mutated = resource_controller_.mutated();
        s.stabilizing_epoch = false;
        if (!s.cpu_mutated) s.cpu_applied.clear();
        if (!s.resource_mutated) s.resource_applied.clear();
        if (s.version.empty()) s.version = kCoreFlowVersion;
        s.pid = static_cast<long>(::getpid());
        (void)writeStatusFile(kPaths.status_json, statusToJson(s));
    } catch (...) {
    }
}

int AutonomousEngine::run() {
    if (!initialize()) {
        logError("Initialization failed");
        return 2;
    }

    logStartup();

    auto next_tick_time = std::chrono::steady_clock::now();

    while (!signal_state::stopRequested()) {
        // Refresh requests are latched in refresh_pending_ so a deferred
        // refresh (incomplete restore) is retried rather than silently lost.
        if (refresh_pending_ || signal_state::consumeRefresh()) {
            refreshEnvironment();
            next_tick_time = std::chrono::steady_clock::now();
        }

        tick();

        const int configured = std::max(1, config_.monitorIntervalSeconds());
        int sleep_seconds = configured;
        // Slice between stop/refresh checks. Idle uses a coarse slice so the
        // process does not interrupt deep sleep with 250 ms wakeups.
        int slice_ms = 1000;

        switch (snapshot_.state) {
            case RuntimeState::Idle:
                // Deep-sleep friendly: long quiescent period. Factory settings
                // are already restored on Idle (shouldRestoreEpochNow), so the
                // daemon only needs a rare heartbeat while the SoC sleeps.
                sleep_seconds = std::max(180, configured * 12);
                if (sleep_seconds > 300) sleep_seconds = 300;
                slice_ms = 10000;
                break;

            case RuntimeState::Warming:
            case RuntimeState::Elevated:
            case RuntimeState::Pressure:
                sleep_seconds = std::max(1, configured / 2);
                slice_ms = 500;
                break;

            case RuntimeState::ThermalGuard:
                sleep_seconds = 1;
                slice_ms = 250;
                break;

            case RuntimeState::Normal:
            default:
                slice_ms = 1000;
                break;
        }

        next_tick_time += std::chrono::seconds(sleep_seconds);

        while (!signal_state::stopRequested()) {
            const auto now = std::chrono::steady_clock::now();
            if (now >= next_tick_time) break;

            if (config_.runtimeRefreshEnabled() && signal_state::consumeRefresh()) {
                refresh_pending_ = true;
                break;
            }

            const auto remaining = next_tick_time - now;
            const auto sleep_for = std::min(
                remaining,
                std::chrono::steady_clock::duration{
                    std::chrono::milliseconds(slice_ms)});

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
            // Keep either journal if restore is incomplete: the next start recovers it.
            const bool cpu_restored = controller_.restoreAll();
            const bool resource_restored = resource_controller_.restoreAll();
            has_restored_ = cpu_restored && resource_restored;
        }
    } catch (const std::exception& e) {
        logError("Exception during shutdown restore: %s", e.what());
    } catch (...) {
        logError("Unknown exception during shutdown restore");
    }

    publishStoppedStatus();

    logInfo("CoreFlow Autonomous shutdown complete");
    return 0;
}

} // namespace coreflow
