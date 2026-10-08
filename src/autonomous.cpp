#include "coreflow/autonomous.hpp"
#include "coreflow/experience.hpp"
#include "coreflow/thermal_predictor.hpp"

#include <android/log.h>
#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cmath>
#include <exception>
#include <thread>
#include <utility>
#include <filesystem>

#include "coreflow/signal_state.hpp"

namespace coreflow {
namespace {

// ============================================================================
// CONFIGURATION & CONSTANTS
// ============================================================================

constexpr const char* kLogTag = "CoreFlowAutonomous";
constexpr const char* kConfigPath = "/data/adb/coreflow/config.ini";
constexpr const char* kMutationJournalPath = "/data/adb/coreflow/mutation_journal.txt";
constexpr const char* kExperiencePath = "/data/adb/coreflow/experience.db";
constexpr const char* kKillSwitchPath = "/data/adb/coreflow/DISABLE";
constexpr const char* kSafeModePath = "/data/adb/coreflow/SAFE_MODE";
constexpr const char* kThermalModelPath = "/data/adb/modules/coreflow_autonomous/system/etc/coreflow/thermal_predictor.onnx";
constexpr std::size_t kHistorySize = 6;

// Notification throttling: prevents repeated events
constexpr std::uint64_t kNotificationCooldownSamples = 12;

// Periodic diagnostics: balance between detail and storage efficiency
// Optimized: every 10 samples (less frequently than 6, reduces storage wear)
constexpr std::uint64_t kPeriodicLogSamples = 10;
constexpr std::uint64_t kExperienceFlushSamples = 60;

// ============================================================================
// EFFICIENCY & SAFETY CONSTANTS
// ============================================================================

// Sensor staleness threshold: if thermal data older than this, distrust it


// Policy cache: skip re-evaluation in steady state (efficiency optimization)

// ============================================================================
// THERMAL STABILITY (HYSTERESIS & PREDICTION)
// ============================================================================

// Thresholds are shared with the policy layer (see policy.hpp), so the
// engine and the policy can never disagree about when the guard exits.
constexpr double kThermalGuardThresholdC = kThermalGuardEnterC;

// Predictive buffer: enter protection 0.5°C earlier if rising trend detected
// Prevents overshoot during rapid heating
constexpr double kRisingThermalBufferC = 0.5;

// Adaptive thermal buffer scaling (efficiency feature)

// After a measured regression, give the runtime a short settling window before
// considering another mutation. Candidate-specific rejection is handled by
// MutationController; this cooldown prevents immediate re-entry churn.
constexpr std::uint64_t kRegressionCooldownSamples = 5;

// After a beneficial or neutral outcome, hold the current decision for a
// short settling window instead of immediately starting another mutation.
constexpr std::uint64_t kOutcomeHoldSamples = 5;

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


} // namespace

// ============================================================================
// AUTONOMOUS ENGINE IMPLEMENTATION
// ============================================================================

AutonomousEngine::AutonomousEngine()
    : journal_(kMutationJournalPath),
      resource_journal_("/data/adb/coreflow_resource_mutation_journal.txt") {}

AutonomousEngine::~AutonomousEngine() {
    try {
        // Double-restore protection (from HARDENED)
        // Prevents calling restore twice if already restored
        if (!has_restored_) {
            const bool cpu_restored = controller_.restoreAll();
            const bool resource_restored = resource_controller_.restoreAll();
            has_restored_ = cpu_restored && resource_restored;
        }
        (void)experience_memory_.flush(kExperiencePath);
    } catch (const std::exception& e) {
        logError("Exception during destructor: %s", e.what());
    } catch (...) {
        logError("Unknown exception during destructor");
    }
}

bool AutonomousEngine::initialize() {
    controller_.setJournal(&journal_);
    resource_controller_.setJournal(&resource_journal_);
    controller_.setExperienceMemory(&experience_memory_);
    logInfo("CoreFlowInitializeEnter version=%s", kCoreFlowVersion);

    if (!config_.load(kConfigPath)) {
        logWarn("Config unavailable; using built-in defaults path=%s", kConfigPath);
    }
    configured_mode_ = config_.mutationMode();
    configured_armed_ = config_.mutationArmed();

    snapshot_.profile = discovery_.discover();
    resource_model_.reset(snapshot_.profile.capabilities);
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
    (void)experience_memory_.load(kExperiencePath);
    mutation_cooldown_until_sample_ = 0;

    const bool ml_ready = thermal_predictor_.initialize(kThermalModelPath);
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
    controller_.setExperienceMemory(&experience_memory_);
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
    (void)experience_memory_.load(kExperiencePath);
    mutation_cooldown_until_sample_ = 0;

    const bool ml_ready = thermal_predictor_.initialize(kThermalModelPath);
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
        const HoldReason hold = evaluateSafetyHold({kKillSwitchPath, kSafeModePath});
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
        if (sample.thermal_available) {
            const double thermal_c =
                static_cast<double>(sample.thermal_millidegrees) / 1000.0;
            const double predicted_thermal_c =
                thermal_predictor_.predict(history_, sample, 3);

            if (previous == RuntimeState::ThermalGuard &&
                next != RuntimeState::ThermalGuard &&
                thermal_c > kThermalGuardExitC) {
                next = RuntimeState::ThermalGuard;
            } else if (
                next != RuntimeState::ThermalGuard &&
                (predicted_thermal_c >= kThermalGuardThresholdC ||
                 (sample.thermal_trend == Trend::Rising &&
                  thermal_c >=
                      (kThermalGuardThresholdC -
                       kRisingThermalBufferC)))) {
                next = RuntimeState::ThermalGuard;
                logWarn(
                    "PREDICTIVE_THERMAL_TRIGGER: current=%.2fC, "
                    "predicted_3ticks=%.2fC",
                    thermal_c, predicted_thermal_c);
            }
        }

        const Decision decision = policy_.decide(sample, next);

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
                "memory_headroom=%s power_headroom=%s mutation_eligible=%s candidates=%zu",
                workloadClassName(context.workload), context.confidence,
                context.thermal_headroom ? "yes" : "no",
                context.memory_headroom ? "yes" : "no",
                context.power_headroom ? "yes" : "no",
                ecosystem_plan.mutation_eligible ? "yes" : "no",
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

        // STEP 9: Build the efficiency baseline before allowing mutations.
        // The first baseline window represents the observed factory/OEM runtime
        // state; it is deliberately separate from MutationController's restore
        // baseline.
        if (!baseline_intelligence_.baselineReady()) {
            const bool ready = baseline_intelligence_.captureBaseline(sample);
            if (ready) {
                logInfo("EFFICIENCY_BASELINE_READY samples=%zu",
                        BaselineIntelligence::kBaselineSamples);
            } else {
                logInfo("EFFICIENCY_BASELINE_CAPTURE %zu/%zu",
                        sample_count_ + 1U,
                        BaselineIntelligence::kBaselineSamples);
            }
        }

        // STEP 10: Observe a verified mutation without allowing another
        // mutation to alter the candidate during the observation window.
        MutationResult mutation = MutationResult::Skipped;
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

                // STEP 10b: Persist the verified outcome as advisory experience.
                ExperienceMemory::Context experience_context;
                experience_context.state = next;
                experience_context.workload = context.workload;
                experience_context.charging = sample.charging;
                experience_context.thermal_trend = sample.thermal_trend;
                experience_context.memory_trend = sample.memory_trend;
                experience_context.load_trend = sample.load_trend;

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

                if ((sample_count_ % kExperienceFlushSamples) == 0U) {
                    (void)experience_memory_.flush(kExperiencePath);
                }

                switch (evaluation.outcome) {
                    case BaselineIntelligence::Outcome::Beneficial:
                        mutation_cooldown_until_sample_ =
                            sample_count_ + kOutcomeHoldSamples + 1U;
                        logInfo(
                            "EFFICIENCY_BENEFICIAL action=KEEP "
                            "hold_until_sample=%llu",
                            static_cast<unsigned long long>(
                                mutation_cooldown_until_sample_));
                        break;

                    case BaselineIntelligence::Outcome::Neutral:
                        mutation_cooldown_until_sample_ =
                            sample_count_ + kOutcomeHoldSamples + 1U;
                        logInfo(
                            "EFFICIENCY_NEUTRAL action=HOLD "
                            "hold_until_sample=%llu",
                            static_cast<unsigned long long>(
                                mutation_cooldown_until_sample_));
                        break;

                    case BaselineIntelligence::Outcome::Regression: {
                        controller_.rejectLastMutation();
                        resource_controller_.rejectLastMutation();
                        mutation_cooldown_until_sample_ =
                            sample_count_ + kRegressionCooldownSamples + 1U;

                        const bool cpu_restored = controller_.restoreAll();
                        const bool resource_restored = resource_controller_.restoreAll();
                        const bool restored = cpu_restored && resource_restored;
                        mutation = restored ? MutationResult::RolledBack
                                            : MutationResult::Failed;
                        logInfo(
                            "EFFICIENCY_REGRESSION restore=%s "
                            "suppression=ACTIVE until_sample=%llu",
                            restored ? "OK" : "FAILED",
                            static_cast<unsigned long long>(
                                mutation_cooldown_until_sample_));
                        break;
                    }

                    default:
                        break;
                }
            }
        } else if (baseline_intelligence_.baselineReady()) {
            const bool mutationCooldownActive =
                sample_count_ < mutation_cooldown_until_sample_;

            if (!mutationCooldownActive && !controller_.mutated() &&
                !resource_controller_.mutated()) {
                const MutationPermit cpu_permit = mutation_authority_.authorize(
                    ecosystem_plan, config_, next, sample.confidence,
                    MutationPermit::Scope::CpuFreq);
                mutation = controller_.apply(
                    next, sample, snapshot_.profile, config_, ecosystem_plan, cpu_permit);
            } else {
                logInfo(
                    "EFFICIENCY_MUTATION_HELD until_sample=%llu",
                    static_cast<unsigned long long>(mutation_cooldown_until_sample_));
            }

            if (mutation == MutationResult::Verified) {
                if (baseline_intelligence_.beginObservation()) {
                    logInfo("EFFICIENCY_OBSERVATION_BEGIN samples=%zu",
                            BaselineIntelligence::kObservationSamples);
                }
            }
        }

        // End an explored mutation epoch before selecting another candidate.
        // Safety states restore immediately; beneficial/neutral epochs restore
        // after their short hold so the next experiment starts from OEM state.
        const bool mutation_epoch_active =
            controller_.mutated() || resource_controller_.mutated();
        const bool hold_expired = sample_count_ >= mutation_cooldown_until_sample_;
        const bool safety_restore =
            next == RuntimeState::Idle || next == RuntimeState::Pressure ||
            next == RuntimeState::ThermalGuard ||
            config_.mutationMode() != MutationMode::Adaptive ||
            !config_.mutationArmed();
        if (mutation_epoch_active && !baseline_intelligence_.observing() &&
            (hold_expired || safety_restore)) {
            const bool cpu_restored = controller_.restoreAll();
            const bool resource_restored = resource_controller_.restoreAll();
            if (!cpu_restored || !resource_restored) {
                logError("MUTATION_EPOCH_RESTORE_FAILED cpu=%s resource=%s",
                         cpu_restored ? "ok" : "failed",
                         resource_restored ? "ok" : "failed");
                refresh_pending_ = true;
            } else {
                logInfo("MUTATION_EPOCH_RESTORED reason=%s",
                        safety_restore ? "safety" : "hold_expired");
            }
        }

        // Exactly one actuator class may mutate in a decision cycle. This keeps
        // the efficiency measurement causal and attributable to one candidate.
        MutationResult resource_mutation = MutationResult::Skipped;
        const bool resource_mutation_cooldown = sample_count_ < mutation_cooldown_until_sample_;
        if (!resource_mutation_cooldown && mutation != MutationResult::Verified &&
            !baseline_intelligence_.observing() && !controller_.mutated() &&
            !resource_controller_.mutated()) {
            const MutationPermit resource_permit = mutation_authority_.authorize(
                ecosystem_plan, config_, next, sample.confidence,
                MutationPermit::Scope::Resource);
            resource_mutation = resource_controller_.apply(
                next, sample, snapshot_.profile, config_, ecosystem_plan, resource_permit);
            if (resource_mutation == MutationResult::Verified &&
                baseline_intelligence_.beginObservation()) {
                logInfo("EFFICIENCY_OBSERVATION_BEGIN resource samples=%zu",
                        BaselineIntelligence::kObservationSamples);
            }
        }
        if (resource_mutation != MutationResult::Skipped) {
            logInfo("RESOURCE_AUTONOMOUS result=%s", mutationResultName(resource_mutation));
        }

        logMutation(mutation, next);

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
        // Refresh requests are latched in refresh_pending_ so a deferred
        // refresh (incomplete restore) is retried rather than silently lost.
        if (refresh_pending_ || signal_state::consumeRefresh()) {
            refreshEnvironment();
            next_tick_time = std::chrono::steady_clock::now();
        }

        tick();

        const int configured = std::max(1, config_.monitorIntervalSeconds());
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
            if (now >= next_tick_time) break;

            if (config_.runtimeRefreshEnabled() && signal_state::consumeRefresh()) {
                refresh_pending_ = true;
                break;
            }

            const auto remaining = next_tick_time - now;
            const auto sleep_for = std::min(
                remaining,
                std::chrono::steady_clock::duration{std::chrono::milliseconds(250)});

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

    logInfo("CoreFlow Autonomous shutdown complete");
    return 0;
}

} // namespace coreflow
