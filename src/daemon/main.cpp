#include "coreflow/types.hpp"
#include "coreflow/config.hpp"
#include "coreflow/stability_policy.hpp"
#include "coreflow/reactor.hpp"
#include "coreflow/uclamp.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/thermal_monitor.hpp"
#include "coreflow/cooling_feedback.hpp"
#include "coreflow/unique_fd.hpp"
#include "coreflow/frequency_controller.hpp"
#include "coreflow/power_bridge.hpp"
#include "coreflow/device_profiler.hpp"
#include "coreflow/hardware_longevity.hpp"
#include "coreflow/neural_predictor.hpp"
#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/vulkan_controller.hpp"
#include "coreflow-engine/src/bridge.rs.h"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <clocale>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <optional>
#include <pthread.h>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/file.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <thread>
#include <unistd.h>

#ifdef __ANDROID__
#include <android/log.h>
#endif

using namespace coreflow;

namespace {

constexpr char kStateDir[] = "/data/adb/coreflow";
constexpr char kPidPath[] = "/data/adb/coreflow/coreflowd.pid";

void emit_log(bool is_error, const std::string& message) {
    if (is_error) {
        std::cerr << message << "\n";
    } else {
        std::cout << message << "\n";
    }
#ifdef __ANDROID__
    __android_log_write(
        is_error ? ANDROID_LOG_WARN : ANDROID_LOG_INFO,
        "CoreFlow",
        message.c_str());
#endif
}

// Fatal signal handlers cannot lock mutexes, perform stream I/O, or allocate.
void on_fatal_signal(int signum) {
    constexpr char line[] = "[CoreFlow] Fatal signal. Next start will recover journal.\n";
    const ssize_t emitted = ::write(STDERR_FILENO, line, sizeof(line) - 1);
    (void)emitted;
    ::_exit(128 + signum);
}

void install_signal_hardening() noexcept {
    struct sigaction ignore_sa{};
    ignore_sa.sa_handler = SIG_IGN;
    sigemptyset(&ignore_sa.sa_mask);
    (void)::sigaction(SIGPIPE, &ignore_sa, nullptr);

    struct sigaction fatal_sa{};
    fatal_sa.sa_handler = on_fatal_signal;
    sigemptyset(&fatal_sa.sa_mask);
    fatal_sa.sa_flags = SA_RESETHAND;
    (void)::sigaction(SIGSEGV, &fatal_sa, nullptr);
    (void)::sigaction(SIGBUS, &fatal_sa, nullptr);
    (void)::sigaction(SIGABRT, &fatal_sa, nullptr);
    (void)::sigaction(SIGILL, &fatal_sa, nullptr);
    (void)::sigaction(SIGFPE, &fatal_sa, nullptr);
}

bool marker(const char* path) noexcept {
    struct stat st{};
    return ::lstat(path, &st) == 0;
}

bool safety_halt_requested() noexcept {
    return marker("/data/adb/coreflow/SAFE_MODE") ||
           marker("/data/adb/coreflow/DISABLE") ||
           marker("/data/adb/modules/cfae_framework/disable") ||
           marker("/data/adb/modules/coreflow_engine/disable");
}

std::optional<double> read_psi_avg10(const char* path) {
    UniqueFd fd(::open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return std::nullopt;

    char buf[512];
    ssize_t n = 0;
    do {
        n = ::read(fd.get(), buf, sizeof(buf));
    } while (n < 0 && errno == EINTR);
    if (n <= 0) return std::nullopt;

    std::string_view content(buf, static_cast<size_t>(n));
    size_t pos = 0;
    while (pos < content.size()) {
        const size_t eol = content.find('\n', pos);
        const std::string_view line =
            eol == std::string_view::npos ? content.substr(pos) : content.substr(pos, eol - pos);
        pos = eol == std::string_view::npos ? content.size() : eol + 1;

        if (line.rfind("some ", 0) != 0) continue;
        const size_t at = line.find("avg10=");
        if (at == std::string_view::npos) continue;
        const std::string_view rest = line.substr(at + 6);
        const size_t end_tok = rest.find_first_of(" \t\r");
        const std::string_view token =
            end_tok == std::string_view::npos ? rest : rest.substr(0, end_tok);
        double value = 0.0;
        if (parse_ascii_double(token, value) && value >= 0.0 && value <= 100.0) {
            return value;
        }
    }
    return std::nullopt;
}

const char* state_name(WorkloadState state) noexcept {
    switch (state) {
        case WorkloadState::Balanced: return "balanced";
        case WorkloadState::SustainedLoad: return "sustained-load";
        case WorkloadState::PressureGuard: return "pressure-guard";
        case WorkloadState::ThermalGuard: return "thermal-guard";
    }
    return "unknown";
}

bool process_matches(pid_t pid) {
    if (pid <= 0) return false;
    if (::kill(pid, 0) != 0 && errno != EPERM) return false;

    const std::string cmd_path = "/proc/" + std::to_string(pid) + "/cmdline";
    UniqueFd fd(::open(cmd_path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return false;

    char buf[256];
    ssize_t n = 0;
    do {
        n = ::read(fd.get(), buf, sizeof(buf));
    } while (n < 0 && errno == EINTR);
    if (n <= 0) return false;

    const std::string_view raw(buf, static_cast<size_t>(n));
    const size_t nul = raw.find('\0');
    const std::string_view argv0 = nul == std::string_view::npos ? raw : raw.substr(0, nul);
    const size_t slash = argv0.find_last_of('/');
    const std::string_view base = slash == std::string_view::npos ? argv0 : argv0.substr(slash + 1);
    return base == "coreflowd";
}

pid_t daemon_pid() {
    UniqueFd fd(::open(kPidPath, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return 0;

    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode)) return 0;

    char buf[32];
    ssize_t n = 0;
    do {
        n = ::read(fd.get(), buf, sizeof(buf));
    } while (n < 0 && errno == EINTR);
    if (n <= 0) return 0;

    const std::string trimmed = DaemonConfig::trim(std::string(buf, static_cast<size_t>(n)));
    if (trimmed.empty()) return 0;

    long value = 0;
    const auto parsed = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != trimmed.data() + trimmed.size()) return 0;
    if (value < 1 || value > 2147483647L) return 0;

    const auto pid = static_cast<pid_t>(value);
    return process_matches(pid) ? pid : 0;
}

bool journal_clear() {
    const std::string jpath = std::string(kStateDir) + "/baseline.journal";
    UniqueFd fd(::open(jpath.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) {
        return errno == ENOENT;
    }

    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0 || st.st_size > 65536) {
        return false;
    }

    char buf[4096];
    std::string raw;
    while (raw.size() <= 65536) {
        const ssize_t n = ::read(fd.get(), buf, sizeof(buf));
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) break;
        raw.append(buf, static_cast<size_t>(n));
    }
    if (raw.size() > 65536) return false;

    std::istringstream journal(raw);
    std::string header;
    if (!std::getline(journal, header) ||
        (header.rfind("CFJ1\t", 0) != 0 && header.rfind("CFJ2\t", 0) != 0 &&
         header.rfind("CFJ3\t", 0) != 0 && header.rfind("CFJ4\t", 0) != 0)) return false;
    std::string line;
    while (std::getline(journal, line)) {
        if (!DaemonConfig::trim(line).empty()) return false;
    }
    return true;
}

size_t count_journal_entries() noexcept {
    const std::string jpath = std::string(kStateDir) + "/baseline.journal";
    UniqueFd fd(::open(jpath.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return 0;

    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode) || st.st_size <= 0 || st.st_size > 65536) {
        return 0;
    }

    char buf[4096];
    std::string raw;
    while (raw.size() <= 65536) {
        const ssize_t n = ::read(fd.get(), buf, sizeof(buf));
        if (n < 0) {
            if (errno == EINTR) continue;
            return 0;
        }
        if (n == 0) break;
        raw.append(buf, static_cast<size_t>(n));
    }
    std::istringstream journal(raw);
    std::string header;
    if (!std::getline(journal, header) ||
        (header.rfind("CFJ1\t", 0) != 0 && header.rfind("CFJ2\t", 0) != 0 &&
         header.rfind("CFJ3\t", 0) != 0 && header.rfind("CFJ4\t", 0) != 0)) {
        return 0;
    }
    size_t count = 0;
    std::string line;
    while (std::getline(journal, line)) {
        if (!DaemonConfig::trim(line).empty()) ++count;
    }
    return count;
}

int stop_daemon() {
    const pid_t pid = daemon_pid();
    if (pid > 0) {
        if (::kill(pid, SIGTERM) != 0 && errno != ESRCH) return 1;
        for (int attempt = 0; attempt < 80; ++attempt) {
            if (!process_matches(pid)) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (process_matches(pid)) return 1;
    }
    // If the daemon died before receiving SIGTERM, recover its durable journal.
    if (!BaselineStore::instance().recover_pending()) return 1;
    return journal_clear() ? 0 : 1;
}

std::string json_string(std::string_view text);

double safe_json_double(double v, double fallback = 0.0) noexcept {
    return std::isfinite(v) ? v : fallback;
}

void print_status() {
    // 1. Run all hardware, kernel, AI, tuner, and Vulkan probes FIRST before writing any JSON
    const pid_t pid = daemon_pid();
    const DaemonConfig cfg_status = DaemonConfig::load_best_available();
    const auto profile = ThermalMonitor::instance().multi_sensor_profile();
    const float temp = profile.composite_soc >= 0.0f ? profile.composite_soc : ThermalMonitor::instance().hottest_celsius();
    auto thermal = ThermalMonitor::instance().assess(false);
    if (!thermal.available && (profile.total_valid_sensors > 0 || temp >= 0.0f)) {
        thermal.available = true;
        thermal.fallback_active = true;
        thermal.source = "generic_fallback";
        const double recover_deg = static_cast<double>(cfg_status.thermal_recover_celsius);
        const double trip_deg = static_cast<double>(cfg_status.thermal_trip_celsius);
        const double span = std::max(1.0, trip_deg - recover_deg);
        const double norm = std::clamp((static_cast<double>(temp) - recover_deg) / span, 0.0, 3.0);
        thermal.pressure = std::clamp(65.0 + norm * 20.0, 0.0, 130.0);
        if (temp >= cfg_status.thermal_trip_celsius + cfg_status.thermal_emergency_margin_celsius) {
            thermal.emergency = true;
        }
    }
    const bool thermal_guard_active = !thermal.fallback_active
        ? (thermal.emergency || thermal.severity >= 3 || thermal.pressure >= 85.0)
        : (thermal.emergency || temp >= cfg_status.thermal_trip_celsius);
    UclampGovernor governor;
    const auto battery = ThermalMonitor::instance().battery_celsius();
    const auto skin = ThermalMonitor::instance().skin_celsius();
    const auto cpu_psi = read_psi_avg10("/proc/pressure/cpu");
    const auto mem_psi = read_psi_avg10("/proc/pressure/memory");
    const auto io_psi = read_psi_avg10("/proc/pressure/io");
    const auto ai = NeuralPredictor::instance().infer_and_learn({
        .thermal_pressure = std::clamp(safe_json_double(thermal.pressure), 0.0, 130.0),
        .cpu_psi = std::clamp(cpu_psi.value_or(0.0), 0.0, 100.0),
        .memory_psi = std::clamp(mem_psi.value_or(0.0), 0.0, 100.0),
        .io_psi = std::clamp(io_psi.value_or(0.0), 0.0, 100.0),
        .cpu_psi_valid = cpu_psi.has_value(),
        .memory_psi_valid = mem_psi.has_value(),
        .io_psi_valid = io_psi.has_value(),
        .current_scale = 1.0,
        .elapsed_seconds = 1.0,
    });
    AutonomousTuner::instance().discover();
    const auto tuner = AutonomousTuner::instance().snapshot();
    VulkanController::instance().initialize({
        .mode = VulkanController::parse_mode(cfg_status.vulkan_layer_mode),
        .enable_skiavk_threaded = cfg_status.enable_skiavk_threaded,
        .enable_adaptive_vrs = cfg_status.enable_adaptive_vrs,
        .enable_pipeline_cache_prewarm = cfg_status.enable_pipeline_cache_prewarm,
    });
    const auto vk = VulkanController::instance().snapshot();
    const bool recovery_pending = !journal_clear();
    const size_t journal_nodes = count_journal_entries();

    // 2. Serialize complete JSON payload into an in-memory buffer so stdout is never split
    std::ostringstream out;
    out << "{\"running\":" << (pid ? "true" : "false")
        << ",\"pid\":" << pid
        << ",\"uclampMin\":" << governor.get_top_app_min()
        << ",\"uclampMax\":" << governor.get_top_app_max()
        << ",\"uclampSupported\":" << (governor.is_supported() ? "true" : "false")
        << ",\"cpuPressurePct\":" << safe_json_double(cpu_psi)
        << ",\"memoryPressurePct\":" << safe_json_double(mem_psi)
        << ",\"ioPressurePct\":" << safe_json_double(io_psi)
        << ",\"baselineNodesSnapshotted\":" << journal_nodes
        << ",\"socTempCelsius\":";
    if (std::isfinite(temp) && temp >= 0.0f) {
        char num[32];
        const int whole = static_cast<int>(temp);
        const int tenths = static_cast<int>(std::lround((temp - static_cast<float>(whole)) * 10.0f));
        if (tenths >= 10) {
            std::snprintf(num, sizeof(num), "%d.0", whole + 1);
        } else {
            std::snprintf(num, sizeof(num), "%d.%d", whole, std::max(0, tenths));
        }
        out << num;
    } else {
        out << "null";
    }
    out << ",\"thermalPolicyAvailable\":" << (thermal.available ? "true" : "false")
        << ",\"thermalGuardActive\":" << (thermal_guard_active ? "true" : "false")
        << ",\"thermalDataDegraded\":" << (thermal.degraded ? "true" : "false")
        << ",\"thermalFallbackActive\":" << (thermal.fallback_active ? "true" : "false")
        << ",\"thermalHeadroomValid\":" << (thermal.headroom_valid ? "true" : "false")
        << ",\"thermalHeadroom\":" << (thermal.headroom_valid && std::isfinite(thermal.headroom_value) ? std::to_string(thermal.headroom_value) : "null")
        << ",\"thermalSource\":" << json_string(thermal.source)
        << ",\"thermalSensor\":" << json_string(thermal.sensor)
        << ",\"thermalSeverity\":" << thermal.severity
        << ",\"thermalPressure\":" << safe_json_double(thermal.pressure)
        << ",\"socCompositeCelsius\":" << (std::isfinite(profile.composite_soc) && profile.composite_soc >= 0 ? std::to_string(profile.composite_soc) : "null")
        << ",\"cpuAverageCelsius\":" << (std::isfinite(profile.cpu_average) && profile.cpu_average >= 0 ? std::to_string(profile.cpu_average) : "null")
        << ",\"cpuPeakCelsius\":" << (std::isfinite(profile.cpu_peak) && profile.cpu_peak >= 0 ? std::to_string(profile.cpu_peak) : "null")
        << ",\"gpuTempCelsius\":" << (std::isfinite(profile.gpu_average) && profile.gpu_average >= 0 ? std::to_string(profile.gpu_average) : "null");
    out << ",\"thermalSensors\":[";
    bool first_sensor = true;
    for (const auto& reading : thermal.readings) {
        if (!first_sensor) out << ',';
        first_sensor = false;
        out << "{\"id\":" << json_string(reading.id)
            << ",\"type\":" << json_string(reading.type)
            << ",\"category\":" << reading.category
            << ",\"valid\":" << (reading.valid ? "true" : "false")
            << ",\"celsius\":" << (reading.valid && std::isfinite(reading.celsius) ? std::to_string(reading.celsius) : "null")
            << ",\"policyAvailable\":" << (reading.policy_available ? "true" : "false") << '}';
    }
    out << "]"
        << ",\"batteryTempCelsius\":" << (std::isfinite(battery) && battery >= 0 ? std::to_string(battery) : "null")
        << ",\"skinTempCelsius\":" << (std::isfinite(skin) && skin >= 0 ? std::to_string(skin) : "null")
        << ",\"aiBackend\":" << json_string(ai.backend)
        << ",\"aiOnnxActive\":" << (ai.onnx_active ? "true" : "false")
        << ",\"aiPredictedPressure10s\":" << safe_json_double(ai.predicted_pressure_10s)
        << ",\"aiPredictedVelocity\":" << safe_json_double(ai.predicted_velocity)
        << ",\"aiRecommendedScale\":" << safe_json_double(ai.recommended_scale, 1.0)
        << ",\"aiConfidence\":" << safe_json_double(ai.confidence)
        << ",\"aiSamples\":" << ai.samples_observed
        << ",\"tunerStage\":" << json_string(AutonomousTuner::stage_name(tuner.stage))
        << ",\"tunerIntent\":" << json_string(AutonomousTuner::intent_name(tuner.intent))
        << ",\"tunerWorkload\":" << json_string(tuner.workload_name)
        << ",\"tunerBottleneck\":" << json_string(tuner.bottleneck_name)
        << ",\"tunerObjective\":" << json_string(tuner.objective_name)
        << ",\"tunerCapabilityFingerprint\":" << json_string(tuner.capability_fingerprint)
        << ",\"tunerCandidateId\":" << json_string(tuner.active_candidate_id)
        << ",\"tunerCandidateLabel\":" << json_string(tuner.active_candidate_label)
        << ",\"tunerRationale\":" << json_string(tuner.rationale)
        << ",\"tunerVerdict\":" << json_string(AutonomousTuner::verdict_name(tuner.verdict))
        << ",\"tunerUtilityDelta\":" << safe_json_double(tuner.utility_delta)
        << ",\"tunerNoiseFloor\":" << safe_json_double(tuner.noise_floor, 0.05)
        << ",\"tunerTrials\":" << tuner.total_trials
        << ",\"tunerImproved\":" << tuner.improved_count
        << ",\"tunerRolledBack\":" << tuner.rolled_back_count
        << ",\"tunerInconclusive\":" << tuner.total_inconclusive
        << ",\"tunerRejected\":" << tuner.total_rejected
        << ",\"tunerThermalStabilization\":" << (tuner.thermal_stabilization_active ? "true" : "false")
        << ",\"tunerThermalRecovery\":" << (tuner.thermal_recovery_active ? "true" : "false")
        << ",\"tunerThermalPressureVelocity\":" << safe_json_double(tuner.thermal_pressure_velocity)
        << ",\"tunerTemperatureVelocity\":" << safe_json_double(tuner.temperature_velocity_celsius_per_sec)
        << ",\"tunerTemperatureVelocityValid\":" << (tuner.temperature_velocity_valid ? "true" : "false")
        << ",\"blockQueues\":[";
    bool first_queue = true;
    for (const auto& q : tuner.block_queues) {
        if (!first_queue) out << ',';
        first_queue = false;
        out << "{\"device\":" << json_string(q.device_name)
            << ",\"activeScheduler\":" << json_string(q.active_scheduler)
            << ",\"baselineScheduler\":" << json_string(q.baseline_scheduler)
            << ",\"readAheadKb\":" << q.read_ahead_kb
            << ",\"nrRequests\":" << q.nr_requests
            << ",\"rqAffinity\":" << q.rq_affinity
            << ",\"readExpireMs\":" << q.read_expire_ms
            << ",\"writesStarved\":" << q.writes_starved
            << ",\"fifoBatch\":" << q.fifo_batch
            << ",\"availableSchedulers\":[";
        bool first_sched = true;
        for (const auto& sched : q.available_schedulers) {
            if (!first_sched) out << ',';
            first_sched = false;
            out << json_string(sched);
        }
        out << "]}";
    }
    out << "],\"tunerCandidates\":[";
    bool first_cand = true;
    for (const auto& c : tuner.candidates) {
        if (!first_cand) out << ',';
        first_cand = false;
        out << "{\"id\":" << json_string(c.id)
            << ",\"label\":" << json_string(c.label)
            << ",\"domain\":" << json_string(c.domain)
            << ",\"trials\":" << c.trials
            << ",\"successes\":" << c.successes
            << ",\"rollbacks\":" << c.rollbacks
            << ",\"inconclusive\":" << c.inconclusive_count
            << ",\"rejected\":" << c.rejected_count
            << ",\"adaptiveScale\":" << safe_json_double(c.adaptive_scale, 1.0)
            << ",\"meanDelta\":" << safe_json_double(c.mean_utility_delta)
            << ",\"ucbScore\":" << safe_json_double(c.ucb_score)
            << ",\"cooldownTicks\":" << c.cooldown_ticks << '}';
    }
    out << "]"
        << ",\"vulkan\":{\"available\":" << (vk.vulkan_available ? "true" : "false")
        << ",\"layerInstalled\":" << (vk.layer_installed ? "true" : "false")
        << ",\"layerActive\":" << (vk.layer_active ? "true" : "false")
        << ",\"mode\":" << json_string(VulkanController::mode_name(vk.active_mode))
        << ",\"vendor\":" << json_string(VulkanController::vendor_name(vk.vendor))
        << ",\"deviceName\":" << json_string(vk.device_name)
        << ",\"apiVersion\":" << json_string(vk.api_version)
        << ",\"effectiveApiVersion\":" << json_string(vk.effective_api_version)
        << ",\"driverVersion\":" << json_string(vk.driver_version_str)
        << ",\"deviceExtensions\":" << vk.device_extensions_count
        << ",\"effectiveDeviceExtensions\":" << vk.effective_device_extensions_count
        << ",\"instanceExtensions\":" << vk.instance_extensions_count
        << ",\"deviceUuid\":" << json_string(vk.device_uuid)
        << ",\"driverUuid\":" << json_string(vk.driver_uuid)
        << ",\"pipelineCacheUuid\":" << json_string(vk.pipeline_cache_uuid)
        << ",\"rayTracing\":" << (vk.ray_tracing ? "true" : "false")
        << ",\"meshShaders\":" << (vk.mesh_shaders ? "true" : "false")
        << ",\"variableRateShading\":" << (vk.variable_rate_shading ? "true" : "false")
        << ",\"cooperativeMatrix\":" << (vk.cooperative_matrix ? "true" : "false")
        << ",\"rtPipeline\":" << (vk.rt_pipeline ? "true" : "false")
        << ",\"rayQuery\":" << (vk.ray_query ? "true" : "false")
        << ",\"accelerationStructure\":" << (vk.acceleration_structure ? "true" : "false")
        << ",\"portabilitySubset\":" << (vk.portability_subset ? "true" : "false")
        << ",\"robustBufferAccess\":" << (vk.robust_buffer_access ? "true" : "false")
        << ",\"samplerAnisotropy\":" << (vk.sampler_anisotropy ? "true" : "false")
        << ",\"shaderInt16\":" << (vk.shader_int16 ? "true" : "false")
        << ",\"shaderFloat64\":" << (vk.shader_float64 ? "true" : "false")
        << ",\"protectedMemory\":" << (vk.protected_memory ? "true" : "false")
        << ",\"timelineSemaphore\":" << (vk.timeline_semaphore ? "true" : "false")
        << ",\"descriptorIndexing\":" << (vk.descriptor_indexing ? "true" : "false")
        << ",\"bufferDeviceAddress\":" << (vk.buffer_device_address ? "true" : "false")
        << ",\"dynamicRendering\":" << (vk.dynamic_rendering ? "true" : "false")
        << ",\"synchronization2\":" << (vk.synchronization2 ? "true" : "false")
        << ",\"indexTypeUint8\":" << (vk.index_type_uint8 ? "true" : "false")
        << ",\"lineRasterization\":" << (vk.line_rasterization ? "true" : "false")
        << ",\"pushDescriptors\":" << (vk.push_descriptors ? "true" : "false")
        << ",\"globalPriorityQuery\":" << (vk.global_priority_query ? "true" : "false")
        << ",\"shaderFloatControls2\":" << (vk.shader_float_controls2 ? "true" : "false")
        << ",\"maxPushDescriptors\":" << vk.max_push_descriptors
        << ",\"maintenance5\":" << (vk.maintenance5 ? "true" : "false")
        << ",\"maintenance6\":" << (vk.maintenance6 ? "true" : "false")
        << ",\"maintenance7\":" << (vk.maintenance7 ? "true" : "false")
        << ",\"descriptorBuffer\":" << (vk.descriptor_buffer ? "true" : "false")
        << ",\"graphicsPipelineLibrary\":" << (vk.graphics_pipeline_library ? "true" : "false")
        << ",\"hwRayTracing\":" << (vk.hw_ray_tracing ? "true" : "false")
        << ",\"hwMeshShaders\":" << (vk.hw_mesh_shaders ? "true" : "false")
        << ",\"hwVariableRateShading\":" << (vk.hw_variable_rate_shading ? "true" : "false")
        << ",\"hwCooperativeMatrix\":" << (vk.hw_cooperative_matrix ? "true" : "false")
        << ",\"hwRtPipeline\":" << (vk.hw_rt_pipeline ? "true" : "false")
        << ",\"hwRayQuery\":" << (vk.hw_ray_query ? "true" : "false")
        << ",\"hwAccelerationStructure\":" << (vk.hw_acceleration_structure ? "true" : "false")
        << ",\"hwShaderFloat64\":" << (vk.hw_shader_float64 ? "true" : "false")
        << ",\"hwIndexTypeUint8\":" << (vk.hw_index_type_uint8 ? "true" : "false")
        << ",\"hwLineRasterization\":" << (vk.hw_line_rasterization ? "true" : "false")
        << ",\"hwPushDescriptors\":" << (vk.hw_push_descriptors ? "true" : "false")
        << ",\"hwGlobalPriorityQuery\":" << (vk.hw_global_priority_query ? "true" : "false")
        << ",\"hwShaderFloatControls2\":" << (vk.hw_shader_float_controls2 ? "true" : "false")
        << ",\"hwMaintenance5\":" << (vk.hw_maintenance5 ? "true" : "false")
        << ",\"hwMaintenance6\":" << (vk.hw_maintenance6 ? "true" : "false")
        << ",\"hwMaintenance7\":" << (vk.hw_maintenance7 ? "true" : "false")
        << ",\"hwDescriptorBuffer\":" << (vk.hw_descriptor_buffer ? "true" : "false")
        << ",\"hwGraphicsPipelineLibrary\":" << (vk.hw_graphics_pipeline_library ? "true" : "false")
        << ",\"skiavkActive\":" << (vk.skiavk_active ? "true" : "false")
        << ",\"adaptiveVrsActive\":" << (vk.adaptive_vrs_active ? "true" : "false")
        << ",\"pipelineCachePrewarmActive\":" << (vk.pipeline_cache_prewarm_active ? "true" : "false")
        << ",\"gpuKernelAvailable\":" << (vk.gpu_surface.available ? "true" : "false")
        << ",\"gpuKernelLabel\":" << json_string(vk.gpu_surface.vendor_label)
        << ",\"gpuGovernor\":" << json_string(vk.gpu_surface.active_governor)
        << ",\"gpuCurrentFreqMhz\":" << vk.gpu_surface.current_freq_mhz
        << ",\"gpuMaxFreqMhz\":" << vk.gpu_surface.max_freq_mhz
        << ",\"adrenoboostLevel\":" << vk.gpu_surface.adrenoboost_level << "}"
        << ",\"recoveryPending\":" << (recovery_pending ? "true" : "false") << "}";

    std::cout << "\n" << out.str() << "\n" << std::flush;
}

std::string json_string(std::string_view text) {
    std::string out = "\"";
    for (unsigned char c : text) {
        if (c == '\"' || c == '\\') { out.push_back('\\'); out.push_back(static_cast<char>(c)); }
        else if (c >= 32 && c < 127) out.push_back(static_cast<char>(c));
        else out.push_back('?');
    }
    return out + "\"";
}

void print_capabilities() {
    const auto topology = DeviceProfiler::instance().probe();
    const auto power = PowerBridge{}.probe();
    NeuralPredictor::instance().initialize("/data/adb/coreflow/models");
    const auto ai = NeuralPredictor::instance().last_prediction();
    const DaemonConfig cfg_cap = DaemonConfig::load_best_available();
    VulkanController::instance().initialize({
        .mode = VulkanController::parse_mode(cfg_cap.vulkan_layer_mode),
        .enable_skiavk_threaded = cfg_cap.enable_skiavk_threaded,
        .enable_adaptive_vrs = cfg_cap.enable_adaptive_vrs,
        .enable_pipeline_cache_prewarm = cfg_cap.enable_pipeline_cache_prewarm,
    });
    const auto vk = VulkanController::instance().snapshot();
    std::ostringstream out;
    out << "{\"socModel\":" << json_string(topology.soc_name)
        << ",\"kernelVersion\":" << json_string(topology.kernel_version)
        << ",\"onlineCpus\":" << json_string(topology.online_cpus_mask)
        << ",\"gpuType\":" << json_string(topology.gpu_type)
        << ",\"rustEngine\":true,\"powerHalDetected\":" << (power.aidl_power_service ? "true" : "false")
        << ",\"aiBackend\":" << json_string(ai.backend)
        << ",\"onnxRuntimeLoaded\":" << (ai.onnx_active ? "true" : "false")
        << ",\"vulkanLayerMode\":" << json_string(VulkanController::mode_name(vk.active_mode))
        << ",\"vulkanEffectiveApi\":" << json_string(vk.effective_api_version)
        << ",\"binderInterception\":false,\"npuOffload\":false,\"frameTelemetry\":false}";
    std::cout << "\n" << out.str() << "\n" << std::flush;
}

bool ensure_secure_state_dir() {
    if (::mkdir(kStateDir, 0700) != 0 && errno != EEXIST) return false;
    struct stat st{};
    if (::lstat(kStateDir, &st) != 0 || !S_ISDIR(st.st_mode)) return false;
    (void)::chmod(kStateDir, 0700);
    return true;
}

bool write_pid(UniqueFd& lock) {
    if (!ensure_secure_state_dir()) return false;
    lock.reset(::open(kPidPath, O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK, 0600));
    if (!lock.valid()) return false;

    struct stat st{};
    if (::fstat(lock.get(), &st) != 0 || !S_ISREG(st.st_mode)) return false;
    if (::flock(lock.get(), LOCK_EX | LOCK_NB) != 0) return false;
    (void)::fchmod(lock.get(), 0600);

    const auto pid = std::to_string(::getpid());
    if (::ftruncate(lock.get(), 0) != 0) return false;
    if (::lseek(lock.get(), 0, SEEK_SET) < 0) return false;
    if (::write(lock.get(), pid.data(), pid.size()) != static_cast<ssize_t>(pid.size())) return false;
    return ::fsync(lock.get()) == 0;
}

void clear_pid(const UniqueFd& lock) noexcept {
    if (!lock.valid()) return;
    if (::ftruncate(lock.get(), 0) == 0) {
        (void)::fsync(lock.get());
    }
}

void write_sys_node_quiet(const char* path, std::string_view content) noexcept {
    UniqueFd fd(::open(path, O_WRONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return;
    const ssize_t written = ::write(fd.get(), content.data(), content.size());
    (void)written;
}

void escape_app_cgroups_and_protect_oom() noexcept {
    // When launched from a root manager WebUI (KernelSU / APatch / MMRL), the child
    // inherits the WebView/Manager app's UID/PID cgroup. When the user closes the WebUI,
    // Android's libprocessgroup kills all processes remaining in that app's cgroup.
    // Migrate coreflowd into the root/system cgroups and harden oom_score_adj.
    const std::string pid_str = std::to_string(::getpid()) + "\n";
    constexpr const char* kRootCgroups[] = {
        "/sys/fs/cgroup/cgroup.procs",
        "/dev/cg2_bpf/cgroup.procs",
        "/acct/cgroup.procs",
        "/dev/cpuset/system-background/cgroup.procs",
        "/dev/cpuset/system-background/tasks",
    };
    for (const char* cg : kRootCgroups) {
        write_sys_node_quiet(cg, pid_str);
    }
    write_sys_node_quiet("/proc/self/oom_score_adj", "-900\n");
}

bool daemonize_process() noexcept {
    const pid_t first = ::fork();
    if (first < 0) return false;
    if (first > 0) ::_exit(0);

    if (::setsid() < 0) return false;

    const pid_t second = ::fork();
    if (second < 0) return false;
    if (second > 0) ::_exit(0);

    (void)::umask(0077);
    (void)::chdir("/");

    // Close all inherited descriptors (such as WebView/ksu.exec pipes) above STDERR.
    for (int fd = 3; fd < 256; ++fd) {
        (void)::close(fd);
    }

    const int null_fd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    if (null_fd >= 0) {
        (void)::dup2(null_fd, STDIN_FILENO);
        if (null_fd > STDERR_FILENO) (void)::close(null_fd);
    }

    (void)ensure_secure_state_dir();
    const std::string logs_dir = std::string(kStateDir) + "/logs";
    (void)::mkdir(logs_dir.c_str(), 0700);
    (void)::chmod(logs_dir.c_str(), 0700);

    const std::string log_path = logs_dir + "/coreflowd.log";
    const int log_fd = ::open(
        log_path.c_str(),
        O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC | O_NOFOLLOW,
        0600);
    if (log_fd >= 0) {
        (void)::dup2(log_fd, STDOUT_FILENO);
        (void)::dup2(log_fd, STDERR_FILENO);
        if (log_fd > STDERR_FILENO) (void)::close(log_fd);
    }
    return true;
}

} // namespace

int main(int argc, char* argv[]) {
    (void)::umask(0077);
    (void)::setenv("LC_ALL", "C", 1);
    (void)std::setlocale(LC_ALL, "C");

    bool fork_daemon = false;
    if (argc >= 2 && std::string_view(argv[1]) == "--set-config") {
        if (argc < 3) {
            std::cerr << "Usage: coreflowd --set-config key=value [key=value ...]\n";
            return 2;
        }
        std::vector<std::pair<std::string, std::string>> updates;
        for (int i = 2; i < argc; ++i) {
            const std::string_view arg(argv[i]);
            const size_t eq = arg.find('=');
            if (eq == std::string_view::npos || eq == 0) {
                std::cerr << "Invalid key=value argument: " << arg << "\n";
                return 2;
            }
            updates.emplace_back(
                DaemonConfig::trim(std::string(arg.substr(0, eq))),
                DaemonConfig::trim(std::string(arg.substr(eq + 1))));
        }
        (void)ensure_secure_state_dir();
        const std::string conf_path = std::string(kStateDir) + "/coreflow.conf";
        if (!DaemonConfig::update_config_file(conf_path, updates)) {
            std::cerr << "Failed to update configuration file: " << conf_path << "\n";
            return 1;
        }
        const pid_t pid = daemon_pid();
        if (pid > 0) {
            (void)::kill(pid, SIGHUP);
        }
        return 0;
    }
    if (argc == 2) {
        const std::string_view command(argv[1]);
        if (command == "--capabilities") { print_capabilities(); return 0; }
        if (command == "--status") {
            print_status();
            return 0;
        }
        if (command == "--stop" || command == "--rollback-baseline") {
            return stop_daemon();
        }
        if (command == "--daemon" || command == "-d") {
            fork_daemon = true;
        } else {
            std::cerr << "Usage: coreflowd [--daemon|--status|--stop|--rollback-baseline|--capabilities|--set-config key=value ...]\n";
            return 2;
        }
    } else if (argc != 1) {
        std::cerr << "Usage: coreflowd [--daemon|--status|--stop|--rollback-baseline|--capabilities|--set-config key=value ...]\n";
        return 2;
    }

    if (safety_halt_requested()) {
        emit_log(true, "[CoreFlow] Safety marker active; refusing runtime mutation.");
        return 2;
    }

    // If another live coreflowd instance already holds the PID file, exit cleanly.
    if (daemon_pid() > 0) {
        return 0;
    }

    if (fork_daemon) {
        if (!daemonize_process()) {
            emit_log(true, "[CoreFlow] Failed to detach background daemon session.");
            return 1;
        }
    } else {
        // Even when launched without --daemon (e.g. via service.sh or nohup),
        // detach from the caller's process group/session and close leaked descriptors.
        (void)::setsid();
        for (int fd = 3; fd < 256; ++fd) {
            (void)::close(fd);
        }
    }

    escape_app_cgroups_and_protect_oom();
    install_signal_hardening();

    UniqueFd pid_lock;
    if (!write_pid(pid_lock)) {
        emit_log(true, "[CoreFlow] Unable to acquire daemon singleton lock.");
        return 1;
    }

    auto& baseline = BaselineStore::instance();
    if (!baseline.recover_pending()) {
        emit_log(true, "[CoreFlow] Incomplete crash recovery. Refusing new mutations.");
        clear_pid(pid_lock);
        return 1;
    }

    DaemonConfig config = DaemonConfig::load_best_available();
    UclampGovernor governor;
    FrequencyController frequencies;
    frequencies.discover(config.enable_cpu_frequency_control, config.enable_gpu_frequency_control);
    auto controller = engine::new_controller({config.pid_kp, config.pid_ki, config.pid_kd, 70.0});
    HardwareLongevityManager::instance().initialize({
        config.enable_battery_step_charging, config.enable_ufs_burst_pacing,
        config.battery_step_temp_threshold, config.stepped_charge_current_ma,
        config.top_app_io_weight, config.background_io_weight});
    const auto power = PowerBridge{}.probe();
    emit_log(false, "[CoreFlow] CPU policies=" + std::to_string(frequencies.cpu_count()) +
        " GPU domains=" + std::to_string(frequencies.gpu_count()) +
        " Power HAL detected=" + std::to_string(power.aidl_power_service) +
        "; Binder interception and NPU offload unavailable without platform integration.");
    EpollReactor reactor;

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);
    if (::pthread_sigmask(SIG_BLOCK, &mask, nullptr) != 0) {
        clear_pid(pid_lock);
        return 1;
    }

    int timer_fd = -1;
    auto last_tick = std::chrono::steady_clock::now();
    bool applied = false;
    CoolingFeedback cooling_feedback;
    TemperatureTrendTracker temp_tracker(8);
    bool thermal_ready = false;
    bool unavailable_reported = false;
    WorkloadState current = WorkloadState::Balanced;
    uint32_t freq_resync_ticks = 0;
    double last_capacity_scale = 1.0;
    std::string last_tuner_candidate = "oem_baseline";
    TunerVerdict last_tuner_verdict = TunerVerdict::Pending;
    NeuralPredictor::instance().initialize("/data/adb/coreflow/models");
    AutonomousTuner::instance().discover();
    VulkanController::instance().initialize({
        .mode = VulkanController::parse_mode(config.vulkan_layer_mode),
        .enable_skiavk_threaded = config.enable_skiavk_threaded,
        .enable_adaptive_vrs = config.enable_adaptive_vrs,
        .enable_pipeline_cache_prewarm = config.enable_pipeline_cache_prewarm,
    });

    UniqueFd signal_fd(::signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC));
    if (!signal_fd.valid() ||
        !reactor.register_fd(signal_fd.get(), EPOLLIN, [&](uint32_t) {
            signalfd_siginfo info{};
            while (::read(signal_fd.get(), &info, sizeof(info)) ==
                   static_cast<ssize_t>(sizeof(info))) {
                if (info.ssi_signo == SIGHUP) {
                    (void)AutonomousTuner::instance().rollback_active("Configuration reloaded on SIGHUP");
                    (void)baseline.rollback_all();
                    config = DaemonConfig::load_best_available();
                    if (!governor.resync()) {
                        governor = UclampGovernor{};
                    }
                    controller = engine::new_controller({config.pid_kp, config.pid_ki, config.pid_kd, 70.0});
                    frequencies.discover(config.enable_cpu_frequency_control, config.enable_gpu_frequency_control);
                    HardwareLongevityManager::instance().initialize({
                        config.enable_battery_step_charging, config.enable_ufs_burst_pacing,
                        config.battery_step_temp_threshold, config.stepped_charge_current_ma,
                        config.top_app_io_weight, config.background_io_weight});
                    NeuralPredictor::instance().initialize("/data/adb/coreflow/models");
                    AutonomousTuner::instance().discover();
                    VulkanController::instance().configure({
                        .mode = VulkanController::parse_mode(config.vulkan_layer_mode),
                        .enable_skiavk_threaded = config.enable_skiavk_threaded,
                        .enable_adaptive_vrs = config.enable_adaptive_vrs,
                        .enable_pipeline_cache_prewarm = config.enable_pipeline_cache_prewarm,
                    });
                    const uint32_t clamped_ms = std::clamp(config.temp_monitor_interval_ms, 1000u, 60000u);
                    itimerspec timer{};
                    timer.it_interval.tv_sec = clamped_ms / 1000;
                    timer.it_interval.tv_nsec = (clamped_ms % 1000) * 1000000L;
                    timer.it_value = timer.it_interval;
                    (void)::timerfd_settime(timer_fd, 0, &timer, nullptr);
                    last_tick = std::chrono::steady_clock::now();
                    applied = false;
                    thermal_ready = false;
                    cooling_feedback.reset();
                    temp_tracker.reset();
                    emit_log(false, "[CoreFlow] Configuration reloaded on SIGHUP.");
                } else {
                    reactor.stop();
                }
            }
        })) {
        clear_pid(pid_lock);
        return 1;
    }

    const auto interval = std::chrono::milliseconds(
        std::clamp(config.temp_monitor_interval_ms, 1000u, 60000u));
    timer_fd = reactor.create_timerfd(interval, [&](uint32_t) {
        if (safety_halt_requested()) {
            reactor.stop();
            return;
        }

        // If an OEM PowerHAL touch boost temporarily wrote to UClamp, yield for the active boost
        // and resynchronize ownership rather than terminating the daemon.
        bool uclamp_yield_this_tick = false;
        if (config.enable_uclamp) {
            if (!governor.is_supported()) {
                (void)governor.resync();
            } else if (!governor.owned_values_intact()) {
                (void)governor.resync();
                uclamp_yield_this_tick = true;
            }
        }

        auto thermal = ThermalMonitor::instance().assess();
        const auto profile = ThermalMonitor::instance().multi_sensor_profile();
        const float temp = profile.composite_soc >= 0.0f ? profile.composite_soc : ThermalMonitor::instance().hottest_celsius();

        if (!thermal.available && (profile.total_valid_sensors > 0 || temp >= 0.0f)) {
            const double recover_deg = static_cast<double>(config.thermal_recover_celsius);
            const double trip_deg = static_cast<double>(config.thermal_trip_celsius);
            const double span = std::max(1.0, trip_deg - recover_deg);
            const double norm = std::clamp((static_cast<double>(temp) - recover_deg) / span, 0.0, 3.0);
            thermal.available = true;
            thermal.fallback_active = true;
            thermal.pressure = std::clamp(65.0 + norm * 20.0, 0.0, 130.0);
            thermal.source = "generic_fallback";
            thermal.sensor = "SoC-Composite (" + std::to_string(profile.valid_cpu_count) + " cores)";
            if (temp >= config.thermal_trip_celsius + config.thermal_emergency_margin_celsius) {
                thermal.emergency = true;
            }
        }

        if (!thermal.available) {
            last_tick = std::chrono::steady_clock::now();
            if (!unavailable_reported) {
                emit_log(true, "[CoreFlow][Thermal] OEM thermal policy unavailable; retaining baseline without synthetic thresholds.");
                unavailable_reported = true;
            }
            thermal_ready = false;
            cooling_feedback.reset();
            temp_tracker.reset();
            // Sensor-policy loss is a fail-safe condition for the autonomous tuner too;
            // do not rely on the unrelated PID applied flag to decide whether it rolls back.
            (void)AutonomousTuner::instance().emergency_rollback(
                "Thermal telemetry unavailable; restoring verified OEM baseline.");
            if (baseline.has_active_mutations()) (void)baseline.rollback_all();
            if (applied) {
                applied = false;
                controller->reset();
                (void)governor.resync();
                frequencies.discover(false, false);
            }
            return;
        }
        if (!thermal_ready) {
            frequencies.discover(config.enable_cpu_frequency_control, config.enable_gpu_frequency_control);
            controller->reset();
            thermal_ready = true; unavailable_reported = false;
        }

        // Catastrophic hardware safety check (battery >= 58C or SoC >= 95C)
        const float batt = ThermalMonitor::instance().battery_celsius();
        if ((batt > 0.0f && batt >= 58.0f) || temp >= 95.0f) {
            emit_log(true, "[CoreFlow][Safety] Extreme physical thermal threshold reached (battery=" +
                     std::to_string(batt) + "°C, soc=" + std::to_string(temp) +
                     "°C). Rolling back mutations to hardware fail-safe baseline.");
            (void)AutonomousTuner::instance().emergency_rollback(
                "Extreme physical thermal threshold reached; restoring verified OEM baseline.");
            if (baseline.has_active_mutations()) (void)baseline.rollback_all();
            applied = false;
            return;
        }

        const auto cpu_psi = read_psi_avg10("/proc/pressure/cpu");
        const auto memory_psi = read_psi_avg10("/proc/pressure/memory");
        const auto io_psi = read_psi_avg10("/proc/pressure/io");
        const PressureSnapshot pressure{
            .cpu = cpu_psi.value_or(0.0),
            .memory = memory_psi.value_or(0.0),
            .io = io_psi.value_or(0.0),
        };

        const auto now_tick = std::chrono::steady_clock::now();
        const auto now_ns = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                now_tick.time_since_epoch()).count());
        if (temp >= 0.0f && std::isfinite(temp)) {
            temp_tracker.record_sample(temp, now_ns);
        }
        const auto temp_vel = temp_tracker.evaluate_velocity();
        const double raw_elapsed = std::chrono::duration<double>(now_tick - last_tick).count();
        const double elapsed = std::clamp(raw_elapsed, 0.001, 60.0);
        last_tick = now_tick;

        // Real-time Hybrid ONNX / Online RLS-MLP 10-second horizon inference
        const auto ai_pred = NeuralPredictor::instance().infer_and_learn({
            .thermal_pressure = std::clamp(thermal.pressure, 0.0, 130.0),
            .cpu_psi = std::clamp(pressure.cpu, 0.0, 100.0),
            .memory_psi = std::clamp(pressure.memory, 0.0, 100.0),
            .io_psi = std::clamp(pressure.io, 0.0, 100.0),
            .current_scale = last_capacity_scale,
            .elapsed_seconds = elapsed,
        });

        double effective_pressure = std::clamp(thermal.pressure, 0.0, 130.0);
        if (ai_pred.valid && ai_pred.confidence >= 0.55 &&
            ai_pred.predicted_pressure_10s > effective_pressure) {
            // Proactive 10s horizon lookahead: blend predicted thermal surge into PID input
            effective_pressure = std::clamp(
                0.70 * effective_pressure + 0.30 * ai_pred.predicted_pressure_10s,
                0.0,
                130.0);
        }

        const auto decision = controller->step({
            effective_pressure, elapsed,
            std::clamp(pressure.cpu, 0.0, 100.0),
            std::clamp(pressure.memory, 0.0, 100.0),
            std::clamp(pressure.io, 0.0, 100.0),
            cpu_psi.has_value(), memory_psi.has_value(), io_psi.has_value(),
            85.0, 65.0, 15.0, config.cpu_pressure_boost_pct,
            config.memory_pressure_guard_pct, config.io_pressure_guard_pct,
            config.enable_pid_equilibrium});
        if (!decision.valid) {
            controller->reset();
            return;
        }

        // Active Thermal Mitigation:
        // When OEM/Android thermal policy is available (!thermal.fallback_active),
        // honor OEM/Android severity and normalized pressure rather than a generic 44.0°C cutoff.
        // Only use config.thermal_trip_celsius when operating in explicit generic_fallback mode.
        const bool thermal_limit_hit = !thermal.fallback_active
            ? (thermal.emergency || thermal.severity >= 3 || thermal.pressure >= 85.0 ||
               (current == WorkloadState::ThermalGuard && thermal.pressure > 65.0))
            : (thermal.emergency || temp >= config.thermal_trip_celsius ||
               (current == WorkloadState::ThermalGuard && temp > config.thermal_recover_celsius));
        const WorkloadState desired = thermal_limit_hit ? WorkloadState::ThermalGuard : static_cast<WorkloadState>(decision.state);
        double active_capacity_scale = decision.capacity_scale;
        if (ai_pred.valid && ai_pred.confidence >= 0.60 && ai_pred.predicted_pressure_10s >= 80.0) {
            // Advisory pre-cooling bound: never raises capacity above deterministic safety policy
            active_capacity_scale = std::min(active_capacity_scale, ai_pred.recommended_scale);
        }
        if (thermal_limit_hit) {
            active_capacity_scale = std::min(active_capacity_scale, 0.75);
            static auto last_guard_log = std::chrono::steady_clock::time_point::min();
            const auto now_t = std::chrono::steady_clock::now();
            if (now_t - last_guard_log > std::chrono::seconds(10)) {
                emit_log(true, "[CoreFlow][ThermalGuard] Active thermal mitigation engaged: reducing workload/power demand ("
                         "composite=" + std::to_string(temp) + "°C, source=" + thermal.source +
                         ", pressure=" + std::to_string(thermal.pressure) + ").");
                last_guard_log = now_t;
            }
        }
        last_capacity_scale = active_capacity_scale;

        // Periodically re-discover frequency domains if OEM PowerHAL temporarily retired them.
        if ((config.enable_cpu_frequency_control && frequencies.cpu_count() == 0) ||
            (config.enable_gpu_frequency_control && frequencies.gpu_count() == 0)) {
            if (++freq_resync_ticks >= 5) {
                freq_resync_ticks = 0;
                frequencies.discover(config.enable_cpu_frequency_control, config.enable_gpu_frequency_control);
            }
        } else {
            freq_resync_ticks = 0;
        }

        // The Rust budget drives real CPU/GPU caps and the scheduler ceiling.
        if (!frequencies.apply(active_capacity_scale)) {
            frequencies.discover(config.enable_cpu_frequency_control, config.enable_gpu_frequency_control);
        }
        (void)HardwareLongevityManager::instance().apply_storage_pacing();
        HardwareLongevityManager::instance().monitor_battery_thermals(temp);
        (void)VulkanController::instance().apply_workload_policy(
            desired, active_capacity_scale, thermal_limit_hit);

        // Multi-domain autonomous scheduler, VM & active thermal stabilization evaluation
        const double remaining_headroom = thermal.headroom_valid
            ? std::max(0.0, 1.0 - static_cast<double>(thermal.headroom_value))
            : -1.0;
        const auto tuner_snap = AutonomousTuner::instance().step({
            .cpu_psi = std::clamp(pressure.cpu, 0.0, 100.0),
            .memory_psi = std::clamp(pressure.memory, 0.0, 100.0),
            .io_psi = std::clamp(pressure.io, 0.0, 100.0),
            .thermal_pressure = std::clamp(thermal.pressure, 0.0, 130.0),
            .thermal_pressure_velocity = ai_pred.valid ? ai_pred.predicted_velocity : 0.0,
            .current_temperature_celsius = temp,
            .temperature_velocity_celsius_per_sec = temp_vel.valid ? temp_vel.velocity_celsius_per_sec : 0.0,
            .temperature_velocity_valid = temp_vel.valid,
            .temperature_samples_count = temp_vel.valid_samples_count,
            .temperature_sample_duration_sec = temp_vel.sample_duration_sec,
            .thermal_guard_active = thermal_limit_hit,
            .thermal_emergency = thermal.emergency,
            .thermal_headroom = remaining_headroom,
            .thermal_trend_celsius = temp_vel.valid ? temp_vel.velocity_celsius_per_sec : 0.0,
        });
        if (tuner_snap.active_candidate_id != last_tuner_candidate ||
            tuner_snap.verdict != last_tuner_verdict) {
            last_tuner_candidate = tuner_snap.active_candidate_id;
            last_tuner_verdict = tuner_snap.verdict;
            emit_log(false, "[CoreFlow][Tuner] stage=" +
                     std::string(AutonomousTuner::stage_name(tuner_snap.stage)) +
                     " intent=" + std::string(AutonomousTuner::intent_name(tuner_snap.intent)) +
                     " candidate=" + tuner_snap.active_candidate_id +
                     " verdict=" + std::string(AutonomousTuner::verdict_name(tuner_snap.verdict)) +
                     " delta_u=" + std::to_string(tuner_snap.utility_delta) +
                     " rationale=\"" + tuner_snap.rationale + "\"");
        }

        const auto capacity = static_cast<uint32_t>(std::lround(active_capacity_scale * 1024.0));
        if (config.enable_uclamp && !uclamp_yield_this_tick && governor.is_supported()) {
            if (!governor.apply_budget(desired,
                    config.top_app_interactive_min, config.top_app_interactive_max, capacity)) {
                (void)governor.resync();
            }
        }
        const auto trend = cooling_feedback.sample(thermal,
            std::chrono::duration<double>(now_tick.time_since_epoch()).count(),
            active_capacity_scale < 0.999 || desired == WorkloadState::ThermalGuard);
        if (trend) {
            emit_log(true, "[CoreFlow][Cooling] observed=" + *trend +
                " sensors=" + std::to_string(thermal.readings.size()) +
                " pressure=" + std::to_string(thermal.pressure) +
                " requested_capacity=" + std::to_string(decision.capacity_scale) +
                " applied_capacity=" + std::to_string(active_capacity_scale));
        }
        if (!applied || current != desired) {
            {
                current = desired;
                applied = true;
                std::ostringstream line;
                line << "[CoreFlow][Stability] state=" << state_name(current)
                     << " compute_temp=" << temp
                     << " thermal_source=" << thermal.source
                     << " thermal_sensor=" << thermal.sensor
                     << " thermal_pressure=" << thermal.pressure
                     << " ai_pred_10s=" << ai_pred.predicted_pressure_10s
                     << " ai_conf=" << ai_pred.confidence
                     << " cpu_psi=" << pressure.cpu
                     << " mem_psi=" << pressure.memory
                     << " io_psi=" << pressure.io;
                emit_log(true, line.str());
            }
        }
    });
    if (timer_fd < 0) {
        clear_pid(pid_lock);
        return 1;
    }

    emit_log(false, "[CoreFlow] C++23 / Rust 2024 equilibrium active; verified baseline recovery armed.");
    reactor.run();
    baseline.rollback_all();
    const bool restored = !baseline.has_active_mutations() && journal_clear();
    clear_pid(pid_lock);
    return restored ? 0 : 1;
}
