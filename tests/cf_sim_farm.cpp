/**
 * @file cf_sim_farm.cpp
 * @brief CoreFlow Engine CF-Sim Virtual SoC Farm — Parallel Deterministic Physics-in-the-Loop CI Gate.
 *
 * Spawns the real `coreflowd_sim --sim-loop` binary across 6 cross-OEM Android device archetypes
 * in parallel (4 worker threads, 100% isolated /tmp sysfs/procfs/state trees) and asserts all
 * physical, thermal, OEM-collaboration, non-LTPO fail-closed, universal sensor, and CFJ4 rollback
 * invariants in < 30 seconds.
 * @license Apache-2.0
 */
#include "cf_sim_plant.hpp"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <unordered_set>
#include <vector>

namespace {

using namespace cf_sim;

// Minimal zero-exception JSON value extractors for single-line coreflowd_sim protocol frames.
bool find_json_key_value(std::string_view json, std::string_view key, size_t& val_pos) noexcept {
    std::string needle = "\"";
    needle.append(key);
    needle.append("\":");
    const size_t pos = json.find(needle);
    if (pos == std::string_view::npos) return false;
    val_pos = pos + needle.size();
    while (val_pos < json.size() && (json[val_pos] == ' ' || json[val_pos] == '\t')) ++val_pos;
    return val_pos < json.size();
}

bool json_get_bool(std::string_view json, std::string_view key, bool fallback = false) noexcept {
    size_t p = 0U;
    if (!find_json_key_value(json, key, p)) return fallback;
    if (json.substr(p, 4U) == "true") return true;
    if (json.substr(p, 5U) == "false") return false;
    return fallback;
}

double json_get_double(std::string_view json, std::string_view key, double fallback = -1.0) noexcept {
    size_t p = 0U;
    if (!find_json_key_value(json, key, p)) return fallback;
    if (json.substr(p, 4U) == "null") return fallback;
    size_t end = p;
    while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != ']') ++end;
    std::string tok(json.substr(p, end - p));
    char* endp = nullptr;
    const double v = std::strtod(tok.c_str(), &endp);
    if (endp == tok.c_str() || !std::isfinite(v)) return fallback;
    return v;
}

uint64_t json_get_u64(std::string_view json, std::string_view key, uint64_t fallback = 0U) noexcept {
    const double v = json_get_double(json, key, -1.0);
    if (v < 0.0 || !std::isfinite(v)) return fallback;
    return static_cast<uint64_t>(std::llround(v));
}

std::string json_get_string(std::string_view json, std::string_view key) {
    size_t p = 0U;
    if (!find_json_key_value(json, key, p) || json[p] != '"') return {};
    ++p;
    std::string out;
    bool esc = false;
    for (; p < json.size(); ++p) {
        const char c = json[p];
        if (esc) {
            out.push_back(c);
            esc = false;
        } else if (c == '\\') {
            esc = true;
        } else if (c == '"') {
            break;
        } else {
            out.push_back(c);
        }
    }
    return out;
}

std::vector<std::string> json_get_string_array(std::string_view json, std::string_view key) {
    std::vector<std::string> out;
    size_t p = 0U;
    if (!find_json_key_value(json, key, p) || json[p] != '[') return out;
    ++p;
    while (p < json.size()) {
        while (p < json.size() && (json[p] == ' ' || json[p] == ',')) ++p;
        if (p >= json.size() || json[p] == ']') break;
        if (json[p] != '"') break;
        ++p;
        std::string item;
        bool esc = false;
        for (; p < json.size(); ++p) {
            const char c = json[p];
            if (esc) {
                item.push_back(c);
                esc = false;
            } else if (c == '\\') {
                esc = true;
            } else if (c == '"') {
                ++p;
                break;
            } else {
                item.push_back(c);
            }
        }
        out.push_back(std::move(item));
    }
    return out;
}

TickSample parse_tick_json(std::string_view json) {
    TickSample s{};
    s.now_ms = json_get_u64(json, "nowMs");
    s.mode = json_get_string(json, "mode");
    s.autonomous_regime = json_get_string(json, "autonomousRegime");
    s.boot_settle_active = json_get_bool(json, "bootSettleActive");
    s.boot_ramp_progress = json_get_double(json, "bootRampProgress", 1.0);
    s.capacity_scale = json_get_double(json, "capacityScale", 1.0);
    s.thermal_guard = json_get_bool(json, "thermalGuard");
    s.thermal_emergency = json_get_bool(json, "thermalEmergency");
    s.soc_temp = json_get_double(json, "socTemp", -1.0);
    s.battery_temp = json_get_double(json, "batteryTemp", -1.0);
    s.thermal_pressure = json_get_double(json, "thermalPressure", 0.0);
    s.thermal_velocity_cpm = json_get_double(json, "thermalVelocityCpm", 0.0);
    s.effective_target_c = json_get_double(json, "effectiveTargetC", 38.0);
    s.power_budget_mw = json_get_double(json, "powerBudgetMw", -1.0);
    s.model_valid = json_get_bool(json, "modelValid");
    s.model_confidence = static_cast<uint32_t>(json_get_u64(json, "modelConfidence"));
    s.model_closed_loop = json_get_bool(json, "modelClosedLoop");
    s.chassis_fused = json_get_bool(json, "chassisFused");
    s.battery_coarse_1c = json_get_bool(json, "batteryCoarse1C");
    s.model_ambient_c = json_get_double(json, "modelAmbientC", -1.0);
    s.model_r_theta = json_get_double(json, "modelRTheta", -1.0);
    s.eas_top_app_cap_pct = static_cast<uint32_t>(json_get_u64(json, "easTopAppCapPct", 100U));
    s.eas_capped_groups = static_cast<uint32_t>(json_get_u64(json, "easCappedGroups"));
    s.uclamp_min = static_cast<uint32_t>(json_get_u64(json, "uclampMin"));
    s.uclamp_max = static_cast<uint32_t>(json_get_u64(json, "uclampMax", 1024U));
    s.boosted_tids = static_cast<uint32_t>(json_get_u64(json, "boostedTids"));
    s.frame_pacing_available = json_get_bool(json, "framePacingAvailable");
    s.frame_pacing_verified = json_get_bool(json, "framePacingVerified");
    s.frame_pacing_source = json_get_string(json, "framePacingSource");
    s.measured_fps = json_get_double(json, "measuredFps", 0.0);
    s.jank_ratio = json_get_double(json, "jankRatio", -1.0);
    s.frame_budget_ms = json_get_double(json, "frameBudgetMs", 16.6667);
    s.p99_frame_time_ms = json_get_double(json, "p99FrameTimeMs", 0.0);
    s.p99_deadline_util = json_get_double(json, "p99DeadlineUtil", -1.0);
    s.p99_available = json_get_bool(json, "p99Available");
    s.schedstat_supported = json_get_bool(json, "schedstatSupported");
    s.runqueue_delay_ms = json_get_double(json, "runqueueDelayMs", 0.0);
    s.starvation_pressure = json_get_double(json, "starvationPressure", -1.0);
    s.vmstat_supported = json_get_bool(json, "vmstatSupported");
    s.refault_file_per_sec = json_get_double(json, "refaultFilePerSec", 0.0);
    s.direct_reclaim_per_sec = json_get_double(json, "directReclaimPerSec", 0.0);
    s.thrashing_pressure = json_get_double(json, "thrashingPressure", 0.0);
    s.direct_reclaim_storm = json_get_bool(json, "directReclaimStorm");
    s.memory_health_pct = static_cast<uint32_t>(json_get_u64(json, "memoryHealthPct", 100U));
    s.read_ahead_kb = json_get_u64(json, "readAheadKb", 128U);
    s.background_isolated = json_get_bool(json, "backgroundIsolated");
    s.preread_files = json_get_u64(json, "prereadFiles", 0U);
    s.bypass_supported = json_get_bool(json, "bypassSupported");
    s.bypass_active = json_get_bool(json, "bypassActive");
    s.bypass_reason = json_get_string(json, "bypassReason");
    s.charging_damped = json_get_bool(json, "chargingDamped");
    s.contended_nodes = static_cast<uint32_t>(json_get_u64(json, "contendedNodes"));
    s.oem_yielded_nodes = static_cast<uint32_t>(json_get_u64(json, "oemYieldedNodes"));
    s.oem_yielded_paths = json_get_string_array(json, "oemYieldedPaths");
    s.managed_envelope_nodes = static_cast<uint32_t>(json_get_u64(json, "managedEnvelopeNodes"));
    s.semantic_knobs_active = static_cast<uint32_t>(json_get_u64(json, "semanticKnobsActive"));
    s.baseline_nodes = static_cast<uint32_t>(json_get_u64(json, "baselineNodes"));
    s.journal_entries = static_cast<uint32_t>(json_get_u64(json, "journalEntries"));
    s.locked_nodes = static_cast<uint32_t>(json_get_u64(json, "lockedNodes"));
    s.quarantined_nodes = static_cast<uint32_t>(json_get_u64(json, "quarantinedNodes"));
    s.causal_effective_count = static_cast<uint32_t>(json_get_u64(json, "causalEffectiveCount"));
    s.causal_bypassed_count = static_cast<uint32_t>(json_get_u64(json, "causalBypassedCount"));
    s.causal_effective_domains = json_get_string_array(json, "causalEffectiveDomains");
    s.causal_bypassed_domains = json_get_string_array(json, "causalBypassedDomains");
    s.edp_calibrated = json_get_bool(json, "edpCalibrated");
    s.edp_confidence = static_cast<uint32_t>(json_get_u64(json, "edpConfidence"));
    s.edp_knee_scale = json_get_double(json, "edpKneeScale", 0.78);
    s.edp_power_exponent = json_get_double(json, "edpPowerExponent", 1.8);
    s.edp_effective_exponent = json_get_double(json, "edpEffectiveExponent", 1.8);
    s.edp_samples = static_cast<uint32_t>(json_get_u64(json, "edpSamples"));
    s.edp_flush_count = static_cast<uint32_t>(json_get_u64(json, "edpFlushCount"));
    return s;
}

class DaemonSimProcess {
public:
    DaemonSimProcess() = default;
    ~DaemonSimProcess() { close_and_reap(); }

    DaemonSimProcess(const DaemonSimProcess&) = delete;
    DaemonSimProcess& operator=(const DaemonSimProcess&) = delete;

    bool spawn(const std::string& bin_path, const VirtualSocPlant& plant) {
        int in_pipe[2]{-1, -1};
        int out_pipe[2]{-1, -1};
        if (::pipe(in_pipe) != 0) return false;
        if (::pipe(out_pipe) != 0) {
            ::close(in_pipe[0]);
            ::close(in_pipe[1]);
            return false;
        }
        (void)::fcntl(in_pipe[1], F_SETFD, FD_CLOEXEC);
        (void)::fcntl(out_pipe[0], F_SETFD, FD_CLOEXEC);

        // Prepare argv strictly before fork() so child executes only async-signal-safe syscalls.
        const std::string sys_arg = plant.sys_root().string();
        const std::string proc_arg = plant.proc_root().string();
        const std::string state_arg = plant.state_dir().string();
        const std::string cfg_arg = plant.config_path().string();
        char* const argv[] = {
            const_cast<char*>(bin_path.c_str()),
            const_cast<char*>("--sim-loop"),
            const_cast<char*>("--sysfs-root"), const_cast<char*>(sys_arg.c_str()),
            const_cast<char*>("--procfs-root"), const_cast<char*>(proc_arg.c_str()),
            const_cast<char*>("--state-dir"), const_cast<char*>(state_arg.c_str()),
            const_cast<char*>("--config"), const_cast<char*>(cfg_arg.c_str()),
            const_cast<char*>("--start-ms"), const_cast<char*>("1000"),
            nullptr,
        };

        const pid_t pid = ::fork();
        if (pid < 0) {
            ::close(in_pipe[0]); ::close(in_pipe[1]);
            ::close(out_pipe[0]); ::close(out_pipe[1]);
            return false;
        }
        if (pid == 0) {
            (void)::dup2(in_pipe[0], STDIN_FILENO);
            (void)::dup2(out_pipe[1], STDOUT_FILENO);
            (void)::close(in_pipe[0]);
            (void)::close(in_pipe[1]);
            (void)::close(out_pipe[0]);
            (void)::close(out_pipe[1]);
            ::execv(bin_path.c_str(), argv);
            ::_exit(127);
        }

        ::close(in_pipe[0]);
        ::close(out_pipe[1]);
        pid_ = pid;
        write_fd_ = in_pipe[1];
        read_fd_ = out_pipe[0];
        return true;
    }

    bool send_line(std::string_view line) {
        if (write_fd_ < 0) return false;
        size_t off = 0U;
        while (off < line.size()) {
            const ssize_t n = ::write(write_fd_, line.data() + off, line.size() - off);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) return false;
            off += static_cast<size_t>(n);
        }
        return true;
    }

    bool read_line(std::string& out) {
        out.clear();
        if (read_fd_ < 0) return false;
        for (;;) {
            const size_t nl = read_buf_.find('\n');
            if (nl != std::string::npos) {
                out = read_buf_.substr(0U, nl);
                read_buf_.erase(0U, nl + 1U);
                if (!out.empty() && out.back() == '\r') out.pop_back();
                return true;
            }
            char chunk[4096];
            const ssize_t n = ::read(read_fd_, chunk, sizeof(chunk));
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) {
                if (!read_buf_.empty()) {
                    out = std::move(read_buf_);
                    read_buf_.clear();
                    return true;
                }
                return false;
            }
            read_buf_.append(chunk, static_cast<size_t>(n));
        }
    }

    int close_and_reap() {
        if (write_fd_ >= 0) { ::close(write_fd_); write_fd_ = -1; }
        if (read_fd_ >= 0) { ::close(read_fd_); read_fd_ = -1; }
        if (pid_ > 0) {
            int status = 0;
            while (::waitpid(pid_, &status, 0) < 0 && errno == EINTR) {}
            pid_ = -1;
            if (WIFEXITED(status)) return WEXITSTATUS(status);
            return -1;
        }
        return 0;
    }

private:
    pid_t pid_{-1};
    int write_fd_{-1};
    int read_fd_{-1};
    std::string read_buf_;
};

struct ArchetypeResult {
    std::string name;
    bool passed{false};
    double wall_sec{0.0};
    uint32_t virtual_ticks{0};
    double peak_chassis_c{0.0};
    double peak_soc_c{0.0};
    double final_effective_target_c{38.0};
    double max_effective_target_c{38.0};
    double phase_b_min_budget_mw{0.0};
    double phase_c_peak_budget_mw{0.0};
    uint32_t max_reversals_per_min{0};
    uint32_t slew_violations{0};
    uint32_t thermal_guard_ticks{0};
    uint32_t thermal_emergency_ticks{0};
    uint32_t oem_yielded_nodes{0};
    uint32_t post_yield_write_wars{0};
    uint32_t boot_settle_sysfs_violations{0};
    uint32_t model_closed_loop_ticks{0};
    bool chassis_fused_seen{false};
    uint32_t causal_effective_count{0};
    uint32_t causal_bypassed_count{0};
    bool edp_calibrated{false};
    uint32_t edp_confidence{0};
    double edp_knee_scale{0.78};
    double edp_power_exponent{1.8};
    uint32_t p99_active_ticks{0};
    double warm_watt_savings_pct{0.0};
    bool non_ltpo_quarantined{false};
    bool ltpo_promoted{false};
    bool bypass_tested{false};
    bool rollback_clean{false};
    std::vector<std::string> failures;
};

ArchetypeResult run_archetype(const std::string& bin_path, ArchetypeConfig cfg) {
    const auto t_start = std::chrono::steady_clock::now();
    ArchetypeResult res{};
    res.name = cfg.name;
    res.virtual_ticks = cfg.total_ticks;

    auto fail = [&](const std::string& msg) {
        res.failures.push_back(msg);
    };

    VirtualSocPlant plant(std::move(cfg));
    if (!plant.initialize()) {
        fail("Failed to initialize VirtualSocPlant fixture");
        return res;
    }

    DaemonSimProcess proc;
    if (!proc.spawn(bin_path, plant)) {
        fail("Failed to spawn coreflowd_sim");
        return res;
    }

    std::string line;
    if (!proc.read_line(line) || line.rfind("READY ", 0) != 0) {
        fail("Did not receive READY handshake from coreflowd_sim (got: " + line + ")");
        return res;
    }
    const std::string ready_json = line.substr(6U);
    const bool ready_is_kgsl = json_get_bool(ready_json, "isKgsl");
    const bool ready_is_mtk_ged = json_get_bool(ready_json, "isMtkGed");
    const std::string ready_storage = json_get_string(ready_json, "storageType");
    const bool ready_mglru = json_get_bool(ready_json, "mglruAvailable");
    const bool ready_bypass_sup = json_get_bool(ready_json, "bypassSupported");
    const bool ready_boot_settle = json_get_bool(ready_json, "bootSettleActive");

    // Discovery checks per archetype
    switch (plant.config().id) {
        case ArchetypeId::SnapdragonHyperOs:
            if (!ready_is_kgsl) fail("Expected KGSL GPU discovery on Snapdragon");
            if (!ready_boot_settle) fail("Expected cold boot to start in BOOT_SETTLE sanctuary");
            break;
        case ArchetypeId::MediaTekDimensity:
            if (!ready_is_mtk_ged) fail("Expected MediaTek GED GPU discovery (isMtkGed=true)");
            break;
        case ArchetypeId::Budget4GbEmmcLegacy:
            if (ready_storage != "eMMC") fail("Expected eMMC storage discovery, got: " + ready_storage);
            if (ready_mglru) fail("Expected mglruAvailable=false on legacy kernel");
            break;
        case ArchetypeId::GamingBypassAmbientCooler:
            if (!ready_bypass_sup) fail("Expected bypassSupported=true on Gaming archetype");
            break;
        default:
            break;
    }

    // Tracking state across virtual timeline
    double prev_scale = 1.0;
    int prev_dir = 0;
    std::deque<uint32_t> reversal_times_sec;
    std::unordered_set<std::string> permanently_yielded_before_step;

    uint32_t boot_settle_ticks = 0U;
    bool exited_boot_settle = false;
    bool saw_tier_a_verified = false;
    bool saw_coarse_1c_batt = false;
    bool saw_uclamp_effective = false;
    bool saw_semantic_bypassed = false;
    uint32_t semantic_bypassed_tick = 0U;
    uint32_t post_bypass_stub_writes = 0U;
    double peak_starvation = -1.0;
    uint32_t starve_peak_uclamp_min = 0U;
    uint32_t fake_jank_ticks_non_ltpo = 0U;
    uint32_t verified_ticks_non_ltpo = 0U;
    uint32_t fake_p99_ticks_non_ltpo = 0U;
    uint32_t ltpo_idle_p99_violations = 0U;
    double warm_power_sum_w = 0.0;
    double warm_unthrottled_sum_w = 0.0;
    uint32_t warm_power_ticks = 0U;

    // Tensor LTPO checks
    uint32_t probation_verified_ticks = 0U;
    uint32_t probation_non_null_jank = 0U;
    double cadence60_max_jank = -1.0;
    double stutter_peak_jank = -1.0;
    uint32_t stutter_peak_uclamp_min = 0U;
    uint32_t smooth_peak_uclamp_min = 0U;

    // Budget 4GB eMMC Legacy checks
    bool saw_schedstat_supported = false;
    bool saw_vmstat_supported = false;
    double regime_a_peak_thrashing = 0.0;
    bool regime_b_storm_detected = false;
    uint64_t regime_b_max_ra_kb = 0U;
    uint64_t regime_b_max_preread = 0U;
    uint32_t screen_off_ticks = 0U;

    // Archetype 6 (Ambient + Cooler + Bypass + UVLO) checks
    bool phase_a_closed_loop_ok = false;
    double phase_a_end_target_c = 0.0;
    double phase_b_max_target_c = 0.0;
    double phase_b_min_budget_mw = 1e9;
    double phase_c_final_target_c = 99.0;
    double phase_c_peak_budget_mw = -1.0;
    double phase_c_min_steady_scale = 1.0;
    bool phase_d_bypass_75_ok = false;
    bool phase_d_hw_suspend_1_ok = false;
    bool phase_d_uvlo_19_ok = false;
    bool phase_d_bypass_resumed_30_ok = false;

    const uint32_t total = plant.config().total_ticks;
    for (uint32_t t = 0U; t < total; ++t) {
        const uint64_t now_ms = 1000ULL + static_cast<uint64_t>(t + 1U) * 1000ULL;
        const WorkloadStep w = plant.workload_at(t);
        bool reloaded_this_tick = false;

        if (w.trigger_reload_bypass_on) {
            (void)plant.write_config_file(true);
            if (!proc.send_line("RELOAD " + std::to_string(now_ms) + "\n") ||
                !proc.read_line(line) || line.rfind("RELOADED ", 0) != 0) {
                fail("RELOAD command failed at t=" + std::to_string(t));
                break;
            }
            reloaded_this_tick = true;
        }

        plant.step_before_daemon(t, now_ms, 1.0);

        if (w.send_touch) {
            if (!proc.send_line("TOUCH " + std::to_string(now_ms) + "\n") ||
                !proc.read_line(line) || line.rfind("TOUCHED ", 0) != 0) {
                fail("TOUCH command failed at t=" + std::to_string(t));
                break;
            }
        }

        const auto contended_before = plant.snapshot_contended_nodes();

        char step_cmd[64];
        std::snprintf(step_cmd, sizeof(step_cmd), "STEP %llu 1.0\n",
                      static_cast<unsigned long long>(now_ms));
        if (!proc.send_line(step_cmd) || !proc.read_line(line) || line.rfind("TICK ", 0) != 0) {
            fail("STEP protocol failed at t=" + std::to_string(t) + " (line=" + line + ")");
            break;
        }
        const TickSample tick = parse_tick_json(std::string_view(line).substr(5U));
        plant.note_uclamp_min_feedback(tick.uclamp_min);
        plant.set_last_uclamp_max(tick.uclamp_max);

        // Verify 0 writes to any node that was ALREADY permanently yielded before this tick!
        for (const auto& yielded_path : permanently_yielded_before_step) {
            const auto bit = contended_before.find(yielded_path);
            if (bit != contended_before.end()) {
                const std::string after_val = read_file_trimmed(yielded_path);
                if (after_val != bit->second) {
                    ++res.post_yield_write_wars;
                }
            }
        }
        for (const auto& yp : tick.oem_yielded_paths) {
            permanently_yielded_before_step.insert(yp);
        }

        // Universal telemetry accumulation
        res.peak_chassis_c = std::max(res.peak_chassis_c, plant.chassis_temp_c());
        res.peak_soc_c = std::max(res.peak_soc_c, plant.die_temp_c());
        res.final_effective_target_c = tick.effective_target_c;
        res.max_effective_target_c = std::max(res.max_effective_target_c, tick.effective_target_c);
        res.oem_yielded_nodes = std::max(res.oem_yielded_nodes, tick.oem_yielded_nodes);
        res.causal_effective_count = std::max(res.causal_effective_count, tick.causal_effective_count);
        res.causal_bypassed_count = std::max(res.causal_bypassed_count, tick.causal_bypassed_count);
        if (tick.p99_available) ++res.p99_active_ticks;
        if (tick.edp_calibrated) {
            res.edp_calibrated = true;
            res.edp_confidence = std::max(res.edp_confidence, tick.edp_confidence);
            res.edp_knee_scale = tick.edp_knee_scale;
            res.edp_power_exponent = tick.edp_power_exponent;
        }
        if (!w.screen_off && !w.charging && plant.chassis_temp_c() >= 38.0 && w.demand_power_w >= 2.5) {
            warm_power_sum_w += plant.last_power_w();
            warm_unthrottled_sum_w += plant.last_unthrottled_power_w();
            ++warm_power_ticks;
        }
        if (tick.chassis_fused) res.chassis_fused_seen = true;
        if (tick.battery_coarse_1c) saw_coarse_1c_batt = true;
        if (tick.thermal_guard) ++res.thermal_guard_ticks;
        if (tick.thermal_emergency) ++res.thermal_emergency_ticks;
        if (tick.model_valid && tick.model_closed_loop) ++res.model_closed_loop_ticks;

        for (const auto& ed : tick.causal_effective_domains) {
            if (ed == "thermal_uclamp_cap") saw_uclamp_effective = true;
        }
        for (const auto& bd : tick.causal_bypassed_domains) {
            if (bd == "semantic_cpu_gpu_boost") {
                if (!saw_semantic_bypassed) {
                    saw_semantic_bypassed = true;
                    semantic_bypassed_tick = t;
                }
            }
        }

        const char* trace_env = std::getenv("CF_SIM_TRACE");
        const bool trace_this = (trace_env != nullptr && (std::string_view(trace_env) == "all" || std::string_view(trace_env) == res.name));

        // Slew-rate & controller oscillation (reversals per 60s sliding window)
        if (t > 0U && !reloaded_this_tick) {
            const double delta = tick.capacity_scale - prev_scale;
            if (!tick.thermal_emergency && (delta < -0.085 || delta > 0.065)) {
                ++res.slew_violations;
            }
            const int dir = (delta > 0.002) ? 1 : (delta < -0.002 ? -1 : 0);
            if (dir != 0) {
                if (prev_dir != 0 && dir != prev_dir) {
                    reversal_times_sec.push_back(t);
                    while (!reversal_times_sec.empty() && reversal_times_sec.front() + 60U <= t) {
                        reversal_times_sec.pop_front();
                    }
                    res.max_reversals_per_min = std::max<uint32_t>(
                        res.max_reversals_per_min,
                        static_cast<uint32_t>(reversal_times_sec.size()));
                    if (trace_this) {
                        std::fprintf(stderr,
                            "[TRACE][%s][REV] t=%u scale=%.4f->%.4f dir=%d win=%zu bat=%.2f soc=%.2f p=%.1f cl=%d conf=%u tgt=%.2f ta=%.2f bdg=%.0f mode=%s\n",
                            res.name.c_str(), t, prev_scale, tick.capacity_scale, dir, reversal_times_sec.size(),
                            plant.chassis_temp_c(), plant.die_temp_c(), tick.thermal_pressure,
                            tick.model_closed_loop ? 1 : 0, tick.model_confidence,
                            tick.effective_target_c, tick.model_ambient_c, tick.power_budget_mw, tick.mode.c_str());
                    }
                }
                prev_dir = dir;
            }
        } else if (reloaded_this_tick) {
            prev_dir = 0;
        }
        if (trace_this && (t % 60U == 0U)) {
            std::fprintf(stderr,
                "[TRACE][%s][60s] t=%u scale=%.4f bat=%.2f soc=%.2f p=%.1f mv=%d cl=%d conf=%u fused=%d tgt=%.2f ta=%.2f r=%.2f bdg=%.0f pwr=%.2f ceff=%u cbyp=%u edp=%d(%.2f/%.2f)\n",
                res.name.c_str(), t, tick.capacity_scale, plant.chassis_temp_c(), plant.die_temp_c(),
                tick.thermal_pressure, tick.model_valid ? 1 : 0, tick.model_closed_loop ? 1 : 0,
                tick.model_confidence, tick.chassis_fused ? 1 : 0, tick.effective_target_c, tick.model_ambient_c,
                tick.model_r_theta, tick.power_budget_mw, plant.last_power_w(),
                tick.causal_effective_count, tick.causal_bypassed_count,
                tick.edp_calibrated ? 1 : 0, tick.edp_knee_scale, tick.edp_power_exponent);
        }
        prev_scale = tick.capacity_scale;

        // Archetype-specific observations
        switch (plant.config().id) {
            case ArchetypeId::SnapdragonHyperOs:
                if (tick.boot_settle_active) {
                    ++boot_settle_ticks;
                    if (tick.baseline_nodes > 0U || tick.journal_entries > 0U) {
                        ++res.boot_settle_sysfs_violations;
                    }
                } else if (boot_settle_ticks > 0U) {
                    exited_boot_settle = true;
                }
                if (tick.frame_pacing_verified) saw_tier_a_verified = true;
                break;

            case ArchetypeId::MediaTekDimensity:
                if (tick.frame_pacing_verified) saw_tier_a_verified = true;
                if (t >= 145U && t < 220U) {
                    if (tick.starvation_pressure > peak_starvation) peak_starvation = tick.starvation_pressure;
                    starve_peak_uclamp_min = std::max(starve_peak_uclamp_min, tick.uclamp_min);
                }
                // Once semantic_cpu_gpu_boost is auto-bypassed, verify ged_boost_enable & gpu_boost_level
                // roll back cleanly to "0" and stay untouched on all subsequent ticks (t > bypass + 1).
                if (saw_semantic_bypassed && t > semantic_bypassed_tick + 1U) {
                    const std::string ged_en = read_file_trimmed(plant.sys_root() / "module/ged/parameters/ged_boost_enable");
                    const std::string ged_lv = read_file_trimmed(plant.sys_root() / "module/ged/parameters/gpu_boost_level");
                    if (ged_en != "0" || ged_lv != "0") {
                        ++post_bypass_stub_writes;
                    }
                }
                break;

            case ArchetypeId::SamsungExynosSdhms:
                if (tick.frame_pacing_source == "fail_closed:fixed_hz_panel") {
                    res.non_ltpo_quarantined = true;
                }
                if (tick.frame_pacing_verified) ++verified_ticks_non_ltpo;
                if (tick.jank_ratio >= 0.0) ++fake_jank_ticks_non_ltpo;
                if (tick.p99_available) ++fake_p99_ticks_non_ltpo;
                if (t >= 125U && t < 240U) {
                    if (tick.starvation_pressure > peak_starvation) peak_starvation = tick.starvation_pressure;
                    starve_peak_uclamp_min = std::max(starve_peak_uclamp_min, tick.uclamp_min);
                }
                break;

            case ArchetypeId::GoogleTensorLtpo:
                if (t < 15U) {
                    if (tick.frame_pacing_verified) ++probation_verified_ticks;
                    if (tick.jank_ratio >= 0.0) ++probation_non_null_jank;
                }
                if (tick.frame_pacing_source == "fail_closed:fixed_hz_panel") {
                    res.non_ltpo_quarantined = true;
                }
                if (t >= 35U && tick.frame_pacing_verified) {
                    res.ltpo_promoted = true;
                }
                if (((t >= 20U && t < 38U) || t >= 1520U) && tick.p99_available) {
                    ++ltpo_idle_p99_violations;
                }
                if (t >= 50U && t < 100U) {
                    smooth_peak_uclamp_min = std::max(smooth_peak_uclamp_min, tick.uclamp_min);
                }
                if (t >= 115U && t < 180U && tick.jank_ratio >= 0.0) {
                    cadence60_max_jank = std::max(cadence60_max_jank, tick.jank_ratio);
                }
                if (t >= 195U && t < 260U) {
                    if (tick.jank_ratio > stutter_peak_jank) stutter_peak_jank = tick.jank_ratio;
                    stutter_peak_uclamp_min = std::max(stutter_peak_uclamp_min, tick.uclamp_min);
                }
                break;

            case ArchetypeId::Budget4GbEmmcLegacy:
                if (tick.schedstat_supported) saw_schedstat_supported = true;
                if (tick.vmstat_supported) saw_vmstat_supported = true;
                if (tick.frame_pacing_source == "fail_closed:fixed_hz_panel") {
                    res.non_ltpo_quarantined = true;
                }
                if (tick.frame_pacing_verified) ++verified_ticks_non_ltpo;
                if (tick.jank_ratio >= 0.0) ++fake_jank_ticks_non_ltpo;
                if (tick.p99_available) ++fake_p99_ticks_non_ltpo;
                if (t >= 100U && t < 210U) {
                    regime_a_peak_thrashing = std::max(regime_a_peak_thrashing, tick.thrashing_pressure);
                }
                if (t >= 220U && t < 360U) {
                    if (tick.direct_reclaim_storm) {
                        regime_b_storm_detected = true;
                        regime_b_max_ra_kb = std::max(regime_b_max_ra_kb, tick.read_ahead_kb);
                        regime_b_max_preread = std::max(regime_b_max_preread, tick.preread_files);
                    }
                }
                if (t >= 1510U && tick.mode == "SCREEN_OFF") {
                    ++screen_off_ticks;
                }
                break;

            case ArchetypeId::GamingBypassAmbientCooler:
                if (t >= 980U && t < 1080U) {
                    if (tick.model_valid && tick.model_closed_loop) {
                        phase_a_closed_loop_ok = true;
                        phase_a_end_target_c = tick.effective_target_c;
                    }
                }
                if (t >= 1440U && t < 1860U) {
                    phase_b_max_target_c = std::max(phase_b_max_target_c, tick.effective_target_c);
                    if (tick.power_budget_mw > 0.0) {
                        phase_b_min_budget_mw = std::min(phase_b_min_budget_mw, tick.power_budget_mw);
                    }
                }
                if (t >= 2220U && t < 2460U) {
                    phase_c_final_target_c = tick.effective_target_c;
                    if (tick.power_budget_mw > 0.0) {
                        phase_c_peak_budget_mw = std::max(phase_c_peak_budget_mw, tick.power_budget_mw);
                    }
                    if (t >= 2340U) {
                        phase_c_min_steady_scale = std::min(phase_c_min_steady_scale, tick.capacity_scale);
                    }
                }
                if (t >= 2470U && t < 2535U) {
                    if (tick.bypass_active) phase_d_bypass_75_ok = true;
                    if (read_file_trimmed(plant.sys_root() / "class/power_supply/battery/input_suspend") == "1") {
                        phase_d_hw_suspend_1_ok = true;
                    }
                }
                if (t >= 2545U && t < 2615U) {
                    const std::string susp = read_file_trimmed(plant.sys_root() / "class/power_supply/battery/input_suspend");
                    if (!tick.bypass_active && tick.bypass_reason == "soc_floor" && susp == "0") {
                        phase_d_uvlo_19_ok = true;
                    }
                }
                if (t >= 2630U && t < 2700U) {
                    const std::string susp = read_file_trimmed(plant.sys_root() / "class/power_supply/battery/input_suspend");
                    if (tick.bypass_active && susp == "1") {
                        phase_d_bypass_resumed_30_ok = true;
                    }
                }
                break;
        }
    }

    res.phase_b_min_budget_mw = (phase_b_min_budget_mw < 1e8) ? phase_b_min_budget_mw : 0.0;
    res.phase_c_peak_budget_mw = std::max(0.0, phase_c_peak_budget_mw);
    if (warm_power_ticks > 0U && warm_unthrottled_sum_w > 1e-6) {
        res.warm_watt_savings_pct = 100.0 * (1.0 - (warm_power_sum_w / warm_unthrottled_sum_w));
    }

    // Shutdown & verify 100% clean CFJ4 rollback
    bool stopped_ok = false;
    if (proc.send_line("STOP\n") && proc.read_line(line) && line.rfind("STOPPED ", 0) == 0) {
        const std::string stop_json = line.substr(8U);
        const uint64_t active_mut = json_get_u64(stop_json, "activeMutations", 99U);
        const bool j_clear = json_get_bool(stop_json, "journalClear", false);
        const uint64_t j_entries = json_get_u64(stop_json, "journalEntries", 99U);
        const uint64_t rem_tids = json_get_u64(stop_json, "remainingBoostedTids", 99U);
        const uint64_t locked = json_get_u64(stop_json, "lockedNodes", 99U);
        const bool bypass_still_on = json_get_bool(stop_json, "bypassActive", true);
        if (active_mut != 0U) fail("CFJ4 rollback left activeMutations=" + std::to_string(active_mut));
        if (!j_clear || j_entries != 0U) fail("CFJ4 baseline.journal not clean after STOP");
        if (rem_tids != 0U) fail("ThreadScheduler left boosted TIDs=" + std::to_string(rem_tids));
        if (locked != 0U) fail("BaselineStore left 0444 locked nodes=" + std::to_string(locked));
        if (bypass_still_on) fail("Bypass charging remained active after STOP");
        stopped_ok = (active_mut == 0U && j_clear && j_entries == 0U && rem_tids == 0U && locked == 0U && !bypass_still_on);
    } else {
        fail("Did not receive clean STOPPED response from coreflowd_sim");
    }

    const int exit_code = proc.close_and_reap();
    if (exit_code != 0) {
        fail("coreflowd_sim exited with non-zero code " + std::to_string(exit_code));
    }

    // Verify every non-OEM-overwritten factory sysfs/procfs node matches exact factory baseline,
    // AND verify OEM-yielded nodes were NOT overwritten back over OEM's live value!
    bool fs_restore_ok = true;
    const auto& oem_live = plant.oem_live_values();
    for (const auto& [path, factory_val] : plant.factory_baselines()) {
        const std::string actual = read_file_trimmed(path);
        const auto oit = oem_live.find(path);
        if (oit != oem_live.end()) {
            // OEM daemon actively mutated this node; shutdown rollback must NEVER clobber OEM's live value!
            if (actual != oit->second) {
                fs_restore_ok = false;
                fail("Rollback clobbered OEM-owned node " + path + " (expected OEM '" + oit->second + "', got '" + actual + "')");
            }
        } else if (actual != factory_val) {
            fs_restore_ok = false;
            fail("Node not restored to factory baseline: " + path + " (expected '" + factory_val + "', got '" + actual + "')");
        }
    }
    res.rollback_clean = stopped_ok && fs_restore_ok && (exit_code == 0);

    // Universal mandatory assertions
    if (res.thermal_guard_ticks > 0U) {
        fail("False THERMAL_GUARD triggered on " + std::to_string(res.thermal_guard_ticks) + " tick(s)");
    }
    if (res.thermal_emergency_ticks > 0U) {
        fail("THERMAL_EMERGENCY triggered on " + std::to_string(res.thermal_emergency_ticks) + " tick(s)");
    }
    if (res.max_reversals_per_min > 2U) {
        fail("Controller oscillation detected: max " + std::to_string(res.max_reversals_per_min) + " reversals/min (limit <= 2)");
    }
    if (res.slew_violations > 0U) {
        fail("Slew-rate contract violated on " + std::to_string(res.slew_violations) + " tick(s)");
    }
    if (res.post_yield_write_wars > 0U) {
        fail("OEM write-war detected: " + std::to_string(res.post_yield_write_wars) + " write(s) after permanent OEM yield");
    }
    if (res.peak_chassis_c > 42.5) {
        fail("Chassis temperature exceeded safe convergence ceiling: " + std::to_string(res.peak_chassis_c) + " C");
    }
    // Phase 3 universal EDP knee & Watt efficiency assertions
    if (!res.edp_calibrated) {
        fail("Online Silicon EDP Knee Calibrator failed to converge (confidence=" + std::to_string(res.edp_confidence) + ")");
    }
    if (res.edp_knee_scale < 0.55 || res.edp_knee_scale > 0.88) {
        fail("EDP knee scale out of safe bounds [0.55, 0.88]: " + std::to_string(res.edp_knee_scale));
    }
    if (warm_power_ticks >= 30U && res.warm_watt_savings_pct < 5.0) {
        fail("Expected measurable warm-phase Watt savings >= 5% via closed-loop EDP pacing (got " +
             std::to_string(res.warm_watt_savings_pct) + "%)");
    }

    // Archetype-specific mandatory assertions
    switch (plant.config().id) {
        case ArchetypeId::SnapdragonHyperOs:
            if (boot_settle_ticks < 30U || !exited_boot_settle) {
                fail("BootCalmSanctuary did not properly settle and exit (ticks=" + std::to_string(boot_settle_ticks) + ")");
            }
            if (res.boot_settle_sysfs_violations > 0U) {
                fail("Sysfs mutation occurred during BOOT_SETTLE sanctuary");
            }
            if (!saw_tier_a_verified) fail("Expected Tier A SDE measured_fps to be verified");
            if (res.p99_active_ticks < 100U) fail("Expected Tier A p99 deadline control active (ticks=" + std::to_string(res.p99_active_ticks) + ")");
            if (res.oem_yielded_nodes < 1U) fail("Expected mi_thermald contended node(s) to reach permanent OEM yield");
            if (!saw_coarse_1c_batt) fail("Expected 1C integer battery sensor quantization to be detected on PARROTPRO replica");
            if (!res.chassis_fused_seen) fail("Expected zero-DC-bias ChassisTempFuser (1C battery + xo-therm) to engage");
            if (res.model_closed_loop_ticks < 200U) fail("Expected RLS thermal model to reach closed-loop active on 1C+xo-therm");
            {
                const fs::path edp_path = plant.state_dir() / "silicon_edp.map";
                std::ifstream eifs(edp_path);
                if (!eifs.is_open()) {
                    fail("Expected CFEDP2 silicon_edp.map to persist cleanly on shutdown");
                } else {
                    std::string hdr;
                    std::getline(eifs, hdr);
                    if (hdr.rfind("CFEDP2\t", 0) != 0) fail("silicon_edp.map missing CFEDP2 header");
                }
            }
            break;

        case ArchetypeId::MediaTekDimensity:
            if (!saw_tier_a_verified) fail("Expected Tier A mtk_fpsgo to be verified");
            if (res.p99_active_ticks < 100U) fail("Expected Tier A mtk_fpsgo p99 deadline control active");
            if (peak_starvation < 0.25) fail("Expected SchedstatSensor to detect runqueue starvation (peak=" + std::to_string(peak_starvation) + ")");
            if (starve_peak_uclamp_min < 450U) fail("Expected starvation rescue to elevate uclampMin >= 450 (got " + std::to_string(starve_peak_uclamp_min) + ")");
            if (res.oem_yielded_nodes < 1U) fail("Expected mtk_fpsgo contended node to reach permanent OEM yield");
            if (res.model_closed_loop_ticks < 200U) fail("Expected RLS thermal model to reach closed-loop active");
            if (!saw_uclamp_effective) fail("Expected CausalVerifier to prove thermal_uclamp_cap effective");
            if (!saw_semantic_bypassed) fail("Expected CausalVerifier to auto-bypass stubbed semantic_cpu_gpu_boost after >=5 trials");
            if (post_bypass_stub_writes > 0U) fail("Stubbed semantic_cpu_gpu_boost was not rolled back cleanly after bypass (" + std::to_string(post_bypass_stub_writes) + " ticks)");
            {
                const fs::path cfem_path = plant.state_dir() / "device_effectiveness.map";
                std::ifstream ifs(cfem_path);
                if (!ifs.is_open()) {
                    fail("Expected CFEM1 device_effectiveness.map to persist cleanly on shutdown");
                } else {
                    std::ostringstream ss;
                    ss << ifs.rdbuf();
                    const std::string content = ss.str();
                    if (content.rfind("CFEM1\t", 0) != 0) fail("device_effectiveness.map missing CFEM1 header");
                    if (content.find("thermal_uclamp_cap\teffective\t") == std::string::npos) {
                        fail("device_effectiveness.map missing thermal_uclamp_cap=effective");
                    }
                    if (content.find("semantic_cpu_gpu_boost\tbypassed\t") == std::string::npos) {
                        fail("device_effectiveness.map missing semantic_cpu_gpu_boost=bypassed");
                    }
                }
            }
            break;

        case ArchetypeId::SamsungExynosSdhms:
            if (!res.non_ltpo_quarantined) fail("Expected fixed-Hz 120 Hz panel to be quarantined fail_closed:fixed_hz_panel");
            if (verified_ticks_non_ltpo > 0U) fail("Non-LTPO fixed-Hz panel falsely became verified");
            if (fake_jank_ticks_non_ltpo > 0U) fail("Non-LTPO fixed-Hz panel emitted fake jank ratio on " + std::to_string(fake_jank_ticks_non_ltpo) + " tick(s)");
            if (fake_p99_ticks_non_ltpo > 0U) fail("Non-LTPO fixed-Hz panel emitted fake p99 on " + std::to_string(fake_p99_ticks_non_ltpo) + " tick(s)");
            if (peak_starvation < 0.25) fail("Expected SchedstatSensor fallback to catch game lag under quarantined panel");
            if (starve_peak_uclamp_min < 450U) fail("Expected SchedstatSensor to lift uclampMin >= 450 (got " + std::to_string(starve_peak_uclamp_min) + ")");
            if (res.oem_yielded_nodes < 1U) fail("Expected SDHMS contended node(s) to reach permanent OEM yield");
            break;

        case ArchetypeId::GoogleTensorLtpo:
            if (probation_verified_ticks > 0U || probation_non_null_jank > 0U) {
                fail("Tier-B LTPO VBlank failed to remain fail-closed during initial probation");
            }
            if (res.non_ltpo_quarantined) fail("Dynamic LTPO 1-120 Hz panel was falsely quarantined as fixed-Hz");
            if (!res.ltpo_promoted) fail("Dynamic LTPO 2 Hz idle probe failed to promote VBlank source to ProvenCommit");
            if (res.p99_active_ticks < 100U) fail("Expected promoted LTPO VBlank to enable p99 deadline control");
            if (ltpo_idle_p99_violations > 0U) {
                fail("Static LTPO 2 Hz idle falsely emitted p99 on " + std::to_string(ltpo_idle_p99_violations) + " tick(s)");
            }
            if (cadence60_max_jank < 0.0 || cadence60_max_jank >= 0.03) {
                fail("Locked 60 FPS cadence anchor on 120 Hz panel failed (max_jank=" + std::to_string(cadence60_max_jank) + ")");
            }
            if (stutter_peak_jank < 0.08) {
                fail("Micro-stutter oscillation not detected on promoted LTPO VBlank (peak_jank=" + std::to_string(stutter_peak_jank) + ")");
            }
            if (stutter_peak_uclamp_min <= smooth_peak_uclamp_min) {
                fail("Expected stutter rescue to lift uclampMin above smooth scroll baseline");
            }
            if (res.model_closed_loop_ticks < 200U) fail("Expected RLS thermal model to reach closed-loop active");
            break;

        case ArchetypeId::Budget4GbEmmcLegacy:
            if (saw_schedstat_supported) fail("Expected schedstatSupported=false on legacy kernel");
            if (!saw_vmstat_supported) fail("Expected legacy /proc/vmstat to be supported");
            if (!res.non_ltpo_quarantined) fail("Expected fixed 60 Hz non-LTPO panel to be quarantined fail-closed");
            if (verified_ticks_non_ltpo > 0U || fake_jank_ticks_non_ltpo > 0U || fake_p99_ticks_non_ltpo > 0U) {
                fail("Fixed 60 Hz non-LTPO panel emitted fake frame-pacing/p99 data");
            }
            if (regime_a_peak_thrashing < 0.45) {
                fail("Legacy /proc/vmstat Regime A refault thrashing not detected (peak=" + std::to_string(regime_a_peak_thrashing) + ")");
            }
            if (!regime_b_storm_detected) fail("Legacy /proc/vmstat Regime B direct-reclaim storm not detected");
            if (regime_b_max_ra_kb > 128U) fail("Direct-reclaim storm failed to clamp read_ahead_kb <= 128 (got " + std::to_string(regime_b_max_ra_kb) + ")");
            if (regime_b_max_preread > 0U) fail("Direct-reclaim storm failed to block prefetch");
            if (screen_off_ticks < 200U) fail("Expected clean ScreenOff transition on legacy budget device");
            if (!saw_coarse_1c_batt) fail("Expected pure 1C integer battery sensor quantization to be detected");
            if (res.chassis_fused_seen) fail("Pure 1C device without skin/xo-therm must NOT claim chassisFused=true");
            if (res.model_closed_loop_ticks < 150U) fail("Expected Rust pure-1C RLS fallback to activate closedLoopActive (ticks=" + std::to_string(res.model_closed_loop_ticks) + ")");
            break;

        case ArchetypeId::GamingBypassAmbientCooler:
            res.bypass_tested = phase_d_bypass_75_ok && phase_d_hw_suspend_1_ok &&
                                phase_d_uvlo_19_ok && phase_d_bypass_resumed_30_ok;
            if (res.p99_active_ticks < 200U) fail("Expected Tier A 144Hz gaming p99 deadline control active");
            if (!phase_a_closed_loop_ok || std::abs(phase_a_end_target_c - 38.0) > 0.2) {
                fail("Phase A (26C) failed to converge closed-loop target=38.0C (got " + std::to_string(phase_a_end_target_c) + ")");
            }
            if (phase_b_max_target_c <= 39.5 || phase_b_max_target_c > 41.51) {
                fail("Phase B (34C tropical) failed adaptive target lift into (39.5, 41.5] (got " + std::to_string(phase_b_max_target_c) + ")");
            }
            if (phase_c_final_target_c > 38.2) {
                fail("Phase C (16C Peltier cooler) failed to return target to 38.0C (got " + std::to_string(phase_c_final_target_c) + ")");
            }
            if ((phase_c_peak_budget_mw - res.phase_b_min_budget_mw) < 1500.0) {
                fail("Phase C Peltier cooler failed to unlock > +1500 mW power budget (delta=" +
                     std::to_string(phase_c_peak_budget_mw - res.phase_b_min_budget_mw) + " mW)");
            }
            if (phase_c_min_steady_scale < 0.92) {
                fail("Phase C Peltier cooler steady capacityScale dropped below 0.92 (got " + std::to_string(phase_c_min_steady_scale) + ")");
            }
            if (!phase_d_bypass_75_ok || !phase_d_hw_suspend_1_ok) {
                fail("Phase D failed to engage bypass charging (input_suspend=1) at 75% SOC");
            }
            if (!phase_d_uvlo_19_ok) {
                fail("Phase D UVLO floor failed to release bypass charging at 19% SOC");
            }
            if (!phase_d_bypass_resumed_30_ok) {
                fail("Phase D hysteresis failed to re-engage bypass charging at 30% SOC");
            }
            break;
    }

    res.passed = res.failures.empty();
    const auto t_end = std::chrono::steady_clock::now();
    res.wall_sec = std::chrono::duration<double>(t_end - t_start).count();
    return res;
}

void write_json_report(const std::string& path,
                       const std::vector<ArchetypeResult>& results,
                       double total_wall_sec,
                       bool all_passed) {
    std::error_code ec;
    const fs::path out_path(path);
    if (!out_path.parent_path().empty()) {
        fs::create_directories(out_path.parent_path(), ec);
    }
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) return;
    out << std::fixed << std::setprecision(3);
    out << "{\n"
        << "  \"suite\": \"CF-Sim Virtual SoC Farm\",\n"
        << "  \"passed\": " << (all_passed ? "true" : "false") << ",\n"
        << "  \"totalWallSec\": " << total_wall_sec << ",\n"
        << "  \"archetypes\": [\n";
    for (size_t i = 0U; i < results.size(); ++i) {
        const auto& r = results[i];
        out << "    {\n"
            << "      \"name\": \"" << r.name << "\",\n"
            << "      \"passed\": " << (r.passed ? "true" : "false") << ",\n"
            << "      \"wallSec\": " << r.wall_sec << ",\n"
            << "      \"virtualTicks\": " << r.virtual_ticks << ",\n"
            << "      \"peakChassisC\": " << r.peak_chassis_c << ",\n"
            << "      \"peakSocC\": " << r.peak_soc_c << ",\n"
            << "      \"maxEffectiveTargetC\": " << r.max_effective_target_c << ",\n"
            << "      \"finalEffectiveTargetC\": " << r.final_effective_target_c << ",\n"
            << "      \"maxReversalsPerMin\": " << r.max_reversals_per_min << ",\n"
            << "      \"slewViolations\": " << r.slew_violations << ",\n"
            << "      \"thermalGuardTicks\": " << r.thermal_guard_ticks << ",\n"
            << "      \"oemYieldedNodes\": " << r.oem_yielded_nodes << ",\n"
            << "      \"postYieldWriteWars\": " << r.post_yield_write_wars << ",\n"
            << "      \"modelClosedLoopTicks\": " << r.model_closed_loop_ticks << ",\n"
            << "      \"chassisFused\": " << (r.chassis_fused_seen ? "true" : "false") << ",\n"
            << "      \"causalEffectiveCount\": " << r.causal_effective_count << ",\n"
            << "      \"causalBypassedCount\": " << r.causal_bypassed_count << ",\n"
            << "      \"edpCalibrated\": " << (r.edp_calibrated ? "true" : "false") << ",\n"
            << "      \"edpKneeScale\": " << r.edp_knee_scale << ",\n"
            << "      \"edpPowerExponent\": " << r.edp_power_exponent << ",\n"
            << "      \"p99ActiveTicks\": " << r.p99_active_ticks << ",\n"
            << "      \"warmWattSavingsPct\": " << r.warm_watt_savings_pct << ",\n"
            << "      \"nonLtpoQuarantined\": " << (r.non_ltpo_quarantined ? "true" : "false") << ",\n"
            << "      \"ltpoPromoted\": " << (r.ltpo_promoted ? "true" : "false") << ",\n"
            << "      \"rollbackClean\": " << (r.rollback_clean ? "true" : "false") << ",\n"
            << "      \"failures\": [";
        for (size_t j = 0U; j < r.failures.size(); ++j) {
            if (j > 0U) out << ", ";
            out << "\"";
            for (char c : r.failures[j]) {
                if (c == '"' || c == '\\') out << '\\';
                out << c;
            }
            out << "\"";
        }
        out << "]\n    }" << (i + 1U < results.size() ? "," : "") << "\n";
    }
    out << "  ]\n}\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: test_cf_sim_farm <path/to/coreflowd_sim> [--report-json <path>] [--workers <1..6>] [--archetype <name>]\n";
        return 2;
    }

    const std::string bin_path = argv[1];
    std::string report_json_path;
    std::string filter_archetype;
    size_t workers = 4U;

    for (int i = 2; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (arg == "--report-json" && i + 1 < argc) {
            report_json_path = argv[++i];
        } else if (arg == "--archetype" && i + 1 < argc) {
            filter_archetype = argv[++i];
        } else if (arg == "--workers" && i + 1 < argc) {
            const int w = std::atoi(argv[++i]);
            workers = static_cast<size_t>(std::clamp(w, 1, 6));
        }
    }

    if (const char* env_report = std::getenv("CF_SIM_REPORT_JSON");
        report_json_path.empty() && env_report != nullptr && env_report[0] != '\0') {
        report_json_path = env_report;
    }

    auto archetypes = build_all_six_archetypes();
    if (!filter_archetype.empty()) {
        std::vector<ArchetypeConfig> filtered;
        for (auto& a : archetypes) {
            if (a.name == filter_archetype) filtered.push_back(std::move(a));
        }
        if (filtered.empty()) {
            std::cerr << "[CF-Sim] Unknown archetype: " << filter_archetype << "\n";
            return 2;
        }
        archetypes = std::move(filtered);
    }

    const auto suite_start = std::chrono::steady_clock::now();
    std::vector<ArchetypeResult> results(archetypes.size());
    std::atomic<size_t> next_idx{0U};

    const size_t thread_count = std::min(workers, archetypes.size());
    std::vector<std::thread> pool;
    pool.reserve(thread_count);
    for (size_t wi = 0U; wi < thread_count; ++wi) {
        pool.emplace_back([&]() {
            for (;;) {
                const size_t idx = next_idx.fetch_add(1U, std::memory_order_relaxed);
                if (idx >= archetypes.size()) break;
                results[idx] = run_archetype(bin_path, archetypes[idx]);
            }
        });
    }
    for (auto& th : pool) th.join();

    const double total_wall_sec =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - suite_start).count();

    bool all_passed = true;
    std::cout << "\n=== CoreFlow CF-Sim Virtual SoC Farm Scorecard ("
              << std::fixed << std::setprecision(2) << total_wall_sec << "s wall, "
              << thread_count << " workers) ===\n";
    std::cout << std::left << std::setw(30) << "Archetype"
              << std::setw(7) << "Status"
              << std::setw(8) << "Wall"
              << std::setw(9) << "PeakBat"
              << std::setw(9) << "MaxTgt"
              << std::setw(7) << "Osc/m"
              << std::setw(7) << "Guard"
              << std::setw(9) << "Yield/W"
              << std::setw(9) << "ClosedLp"
              << std::setw(8) << "Causal"
              << std::setw(12) << "EDP(k/a)"
              << std::setw(8) << "W-Save"
              << "CFJ4\n";
    for (const auto& r : results) {
        if (!r.passed) all_passed = false;
        char edp_buf[32];
        if (r.edp_calibrated) {
            std::snprintf(edp_buf, sizeof(edp_buf), "%.2f/%.2f", r.edp_knee_scale, r.edp_power_exponent);
        } else {
            std::snprintf(edp_buf, sizeof(edp_buf), "uncal");
        }
        char wsave_buf[16];
        std::snprintf(wsave_buf, sizeof(wsave_buf), "%.1f%%", r.warm_watt_savings_pct);
        std::cout << std::left << std::setw(30) << r.name
                  << std::setw(7) << (r.passed ? "PASS" : "FAIL")
                  << std::setw(8) << (std::to_string(r.wall_sec).substr(0, 4) + "s")
                  << std::setw(9) << (std::to_string(r.peak_chassis_c).substr(0, 4) + "C")
                  << std::setw(9) << (std::to_string(r.max_effective_target_c).substr(0, 4) + "C")
                  << std::setw(7) << r.max_reversals_per_min
                  << std::setw(7) << r.thermal_guard_ticks
                  << std::setw(9) << (std::to_string(r.oem_yielded_nodes) + "/" + std::to_string(r.post_yield_write_wars))
                  << std::setw(9) << (std::to_string(r.model_closed_loop_ticks) + (r.chassis_fused_seen ? "F" : ""))
                  << std::setw(8) << (std::to_string(r.causal_effective_count) + "/" + std::to_string(r.causal_bypassed_count))
                  << std::setw(12) << edp_buf
                  << std::setw(8) << wsave_buf
                  << (r.rollback_clean ? "CLEAN" : "DIRTY") << "\n";
        for (const auto& f : r.failures) {
            std::cerr << "  [FAIL][" << r.name << "] " << f << "\n";
        }
    }

    if (!report_json_path.empty()) {
        write_json_report(report_json_path, results, total_wall_sec, all_passed);
    }

    if (!all_passed) {
        std::cerr << "\n[CF-Sim] Virtual SoC Farm FAILED.\n";
        return 1;
    }
    std::cout << "\n[CF-Sim] All " << results.size() << " virtual SoC archetypes PASSED.\n";
    return 0;
}
