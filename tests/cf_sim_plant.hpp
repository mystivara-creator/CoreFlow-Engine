/**
 * @file cf_sim_plant.hpp
 * @brief CoreFlow Engine CF-Sim Virtual SoC Farm — Coupled Multi-Domain Physics Plant
 *        and 6 Cross-OEM Android Device Archetypes.
 *
 * Simulates real kernel sysfs/procfs topology, 2-node RC chassis+die thermal physics,
 * convex C*V^2*f silicon power scaling, adversarial OEM daemons (mi_thermald, mtk_fpsgo,
 * SDHMS/GOS), Tier-A/Tier-B/Non-LTPO/LTPO frame-pacing clocks, schedstat runqueue
 * starvation, legacy & modern /proc/vmstat thrashing, and bypass charging + UVLO.
 *
 * Compiled with -fno-exceptions -fno-rtti for Host Release, ASan+UBSan, and TSan CI gates.
 * @license Apache-2.0
 */
#pragma once

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cf_sim {

namespace fs = std::filesystem;

enum class ArchetypeId : uint8_t {
    SnapdragonHyperOs = 0,
    MediaTekDimensity = 1,
    SamsungExynosSdhms = 2,
    GoogleTensorLtpo = 3,
    Budget4GbEmmcLegacy = 4,
    GamingBypassAmbientCooler = 5,
};

enum class PanelClockKind : uint8_t {
    TierASdeMeasured = 0,   // Qualcomm sde-crtc-0/measured_fps (real commit rate)
    TierAMtkFpsgo = 1,      // MediaTek /proc/mtk_fpsgo/fps (real commit rate)
    TierBFixedPanelFps = 2, // Samsung panel/panel/fps locked at target_hz (non-LTPO)
    TierBLtpoVblank = 3,    // Google Tensor /proc/interrupts VBlank (dynamic 1..120 Hz)
    TierBFixedVblank = 4,   // Legacy 60 Hz non-LTPO /proc/interrupts VBlank
};

enum class GpuKind : uint8_t {
    Kgsl = 0,
    MtkGed = 1,
    Devfreq = 2,
};

struct OemContenderSpec {
    std::string rel_path;         // Relative to sysfs root
    std::string val_a;
    std::string val_b;
    uint32_t period_sec{15};
    uint32_t start_sec{60};
    uint32_t end_sec{1600};
    bool toggle{false};
};

struct WorkloadStep {
    bool screen_off{false};
    int32_t fg_pid{2001};
    std::string fg_pkg{"com.android.launcher3"};
    bool is_game{false};
    bool send_touch{false};
    double cpu_util_pct{10.0};
    double gpu_busy_pct{2.0};
    double render_exec_ratio{0.05};
    double run_delay_ms_per_sec{0.5};
    double cpu_psi{0.5};
    double mem_psi{0.2};
    double io_psi{0.2};
    double avail_ram_ratio{0.55};
    double demand_power_w{1.2};
    double bg_power_w{0.4};
    double actual_fps{0.0};       // Real compositor commit rate (used by Tier A & LTPO)
    double refault_per_sec{0.0};
    double direct_reclaim_per_sec{0.0};
    double pswpin_per_sec{0.0};
    // Environment overrides
    double ambient_c{26.0};
    double r_theta_c_per_w{5.5};
    double c_chassis_j_per_c{180.0 / 5.5};
    bool charging{false};
    double charging_heat_w{0.0};
    int battery_soc_pct{75};
    bool trigger_reload_bypass_on{false};
};

struct ArchetypeConfig {
    ArchetypeId id{ArchetypeId::SnapdragonHyperOs};
    std::string name;
    std::string soc_model;
    std::string kernel_release;
    std::string rom_version;
    uint32_t refresh_hz{120};
    uint32_t total_ram_mb{12288};
    uint32_t zram_mb{4096};
    bool is_emmc{false};
    bool has_psi{true};
    bool has_mglru{true};
    bool has_schedstat{true};
    bool legacy_vmstat{false};
    bool has_bypass_node{false};
    bool is_two_cluster{false};
    bool use_walt{false};
    GpuKind gpu_kind{GpuKind::Kgsl};
    std::string gpu_model;
    PanelClockKind panel_kind{PanelClockKind::TierASdeMeasured};
    double initial_uptime_sec{300.0};
    double initial_chassis_c{31.0};
    double initial_die_c{35.0};
    double r_die_c_per_w{4.0};
    double tau_die_s{8.0};
    double battery_quantum_c{0.1};
    bool has_skin_sensor{true};
    bool has_xo_therm{false};
    double silicon_power_alpha{1.8}; // True convex C*V^2*f exponent per virtual SoC
    double peak_dyn_power_w{4.0};    // Reference peak dynamic power (W) at scale = 1.0
    uint32_t total_ticks{1800};
    uint32_t little_cap{384};
    uint32_t mid_cap{768};
    uint32_t big_cap{1024};
    std::vector<OemContenderSpec> oem_contenders;
};

struct TickSample {
    uint64_t now_ms{0};
    std::string mode;
    std::string autonomous_regime;
    bool boot_settle_active{false};
    double boot_ramp_progress{1.0};
    double capacity_scale{1.0};
    bool thermal_guard{false};
    bool thermal_emergency{false};
    double soc_temp{-1.0};
    double battery_temp{-1.0};
    double thermal_pressure{0.0};
    double thermal_velocity_cpm{0.0};
    double effective_target_c{38.0};
    double power_budget_mw{-1.0};
    bool model_valid{false};
    uint32_t model_confidence{0};
    bool model_closed_loop{false};
    bool chassis_fused{false};
    bool battery_coarse_1c{false};
    double model_ambient_c{-1.0};
    double model_r_theta{-1.0};
    uint32_t eas_top_app_cap_pct{100};
    uint32_t eas_capped_groups{0};
    uint32_t uclamp_min{0};
    uint32_t uclamp_max{1024};
    uint32_t boosted_tids{0};
    bool frame_pacing_available{false};
    bool frame_pacing_verified{false};
    std::string frame_pacing_source;
    double measured_fps{0.0};
    double jank_ratio{-1.0};
    double frame_budget_ms{16.6667};
    double p99_frame_time_ms{0.0};
    double p99_deadline_util{-1.0};
    bool p99_available{false};
    bool schedstat_supported{false};
    double runqueue_delay_ms{0.0};
    double starvation_pressure{-1.0};
    bool vmstat_supported{false};
    double refault_file_per_sec{0.0};
    double direct_reclaim_per_sec{0.0};
    double thrashing_pressure{0.0};
    bool direct_reclaim_storm{false};
    uint32_t memory_health_pct{100};
    uint64_t read_ahead_kb{128};
    bool background_isolated{false};
    uint64_t preread_files{0};
    bool bypass_supported{false};
    bool bypass_active{false};
    std::string bypass_reason;
    bool charging_damped{false};
    uint32_t contended_nodes{0};
    uint32_t oem_yielded_nodes{0};
    std::vector<std::string> oem_yielded_paths;
    uint32_t managed_envelope_nodes{0};
    uint32_t semantic_knobs_active{0};
    uint32_t baseline_nodes{0};
    uint32_t journal_entries{0};
    uint32_t locked_nodes{0};
    uint32_t quarantined_nodes{0};
    uint32_t causal_effective_count{0};
    uint32_t causal_bypassed_count{0};
    std::vector<std::string> causal_effective_domains;
    std::vector<std::string> causal_bypassed_domains;
    bool edp_calibrated{false};
    uint32_t edp_confidence{0};
    double edp_knee_scale{0.78};
    double edp_power_exponent{1.8};
    double edp_effective_exponent{1.8};
    uint32_t edp_samples{0};
    uint32_t edp_flush_count{0};
};

// Lightweight zero-exception filesystem helpers (inode-preserving for sysfs actuators)
inline bool write_file_in_place(const fs::path& path, std::string_view content) noexcept {
    std::error_code ec;
    if (!path.parent_path().empty()) {
        fs::create_directories(path.parent_path(), ec);
        if (ec) return false;
    }
    struct stat st{};
    if (::lstat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & S_IWUSR) == 0) {
        (void)::chmod(path.c_str(), 0644);
    }
    const int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW, 0644);
    if (fd < 0) return false;
    size_t off = 0U;
    while (off < content.size()) {
        const ssize_t n = ::write(fd, content.data() + off, content.size() - off);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) {
            (void)::close(fd);
            return false;
        }
        off += static_cast<size_t>(n);
    }
    (void)::close(fd);
    return true;
}

inline bool write_sensor_with_mtime(const fs::path& path, std::string_view content, uint64_t virtual_ms) noexcept {
    if (!write_file_in_place(path, content)) return false;
    struct timespec ts[2]{};
    const time_t sec = static_cast<time_t>(1'700'000'000ULL + (virtual_ms / 1000ULL));
    const long nsec = static_cast<long>((virtual_ms % 1000ULL) * 1'000'000ULL);
    ts[0].tv_sec = sec;
    ts[0].tv_nsec = nsec;
    ts[1].tv_sec = sec;
    ts[1].tv_nsec = nsec;
    (void)::utimensat(AT_FDCWD, path.c_str(), ts, 0);
    return true;
}

inline std::string read_file_trimmed(const fs::path& path) {
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return {};
    char buf[256];
    ssize_t n = 0;
    do { n = ::read(fd, buf, sizeof(buf)); } while (n < 0 && errno == EINTR);
    (void)::close(fd);
    if (n <= 0) return {};
    std::string out(buf, static_cast<size_t>(n));
    const auto nl = out.find('\n');
    if (nl != std::string::npos) out.resize(nl);
    while (!out.empty() && (out.back() == '\r' || out.back() == ' ' || out.back() == '\t')) out.pop_back();
    size_t start = 0U;
    while (start < out.size() && (out[start] == ' ' || out[start] == '\t')) ++start;
    return out.substr(start);
}

inline double parse_double_or(std::string_view s, double fallback) noexcept {
    if (s.empty()) return fallback;
    std::string tmp(s);
    char* end = nullptr;
    const double v = std::strtod(tmp.c_str(), &end);
    if (end == tmp.c_str() || !std::isfinite(v)) return fallback;
    return v;
}

class VirtualSocPlant {
public:
    explicit VirtualSocPlant(ArchetypeConfig cfg) : cfg_(std::move(cfg)) {
        const char* base_dir = (::access("/dev/shm", W_OK | X_OK) == 0) ? "/dev/shm" : "/tmp";
        root_ = fs::path(base_dir) / ("coreflow-cfsim-" + std::to_string(::getpid()) + "-" + cfg_.name);
        sys_ = root_ / "sys";
        proc_ = root_ / "proc";
        state_ = root_ / "state";
        config_path_ = root_ / "config" / "coreflow.conf";
        t_chassis_c_ = cfg_.initial_chassis_c;
        t_die_c_ = cfg_.initial_die_c;
        uptime_sec_ = cfg_.initial_uptime_sec;
    }

    ~VirtualSocPlant() { cleanup(); }

    VirtualSocPlant(const VirtualSocPlant&) = delete;
    VirtualSocPlant& operator=(const VirtualSocPlant&) = delete;

    void cleanup() noexcept {
        if (!root_.empty()) {
            std::error_code ec;
            fs::remove_all(root_, ec);
            root_.clear();
        }
    }

    [[nodiscard]] const ArchetypeConfig& config() const noexcept { return cfg_; }
    [[nodiscard]] const fs::path& root() const noexcept { return root_; }
    [[nodiscard]] const fs::path& sys_root() const noexcept { return sys_; }
    [[nodiscard]] const fs::path& proc_root() const noexcept { return proc_; }
    [[nodiscard]] const fs::path& state_dir() const noexcept { return state_; }
    [[nodiscard]] const fs::path& config_path() const noexcept { return config_path_; }
    [[nodiscard]] double chassis_temp_c() const noexcept { return t_chassis_c_; }
    [[nodiscard]] double die_temp_c() const noexcept { return t_die_c_; }
    [[nodiscard]] double last_power_w() const noexcept { return last_power_w_; }
    [[nodiscard]] double last_unthrottled_power_w() const noexcept { return last_unthrottled_power_w_; }
    void set_last_uclamp_max(uint32_t uclamp_max) noexcept {
        last_uclamp_max_ = (uclamp_max >= 128U && uclamp_max <= 1024U) ? uclamp_max : 1024U;
    }
    [[nodiscard]] const std::unordered_map<std::string, std::string>& factory_baselines() const noexcept {
        return factory_baselines_;
    }

    bool initialize() {
        std::error_code ec;
        fs::remove_all(root_, ec);
        ec.clear();
        fs::create_directories(sys_, ec);
        fs::create_directories(proc_, ec);
        fs::create_directories(state_, ec);
        fs::create_directories(config_path_.parent_path(), ec);
        if (ec) return false;

        // 1. Default isolated daemon config
        write_config_file(false);

        // 2. Kernel release, SoC identity, /proc/version, /proc/uptime
        write_file_in_place(proc_ / "sys/kernel/osrelease", cfg_.kernel_release + "\n");
        write_file_in_place(proc_ / "version", "Linux version " + cfg_.kernel_release + " (" + cfg_.rom_version + ")\n");
        write_file_in_place(sys_ / "devices/soc0/machine", cfg_.soc_model + "\n");
        write_file_in_place(sys_ / "devices/soc0/family", cfg_.soc_model + "\n");
        write_uptime();

        // 3. CPU Topology & EAS Capacities + Governor Tunables
        setup_cpu_topology();

        // 4. GPU + Memory Bus Devfreq
        setup_gpu_and_bus();

        // 5. Storage (UFS or eMMC)
        setup_storage();

        // 6. VM, ZRAM, MGLRU (optional), PSI (optional), /proc/vmstat
        setup_memory_and_vm();

        // 7. Cgroup v2 hierarchy & /proc/self/mountinfo
        setup_cgroups();

        // 8. Display & Frame-Pacing nodes
        setup_display();

        // 9. Thermal zones & Power Supply (Battery + optional Bypass Charging)
        setup_thermals_and_battery();

        // 10. Processes: SurfaceFlinger + initial foreground apps
        setup_processes();

        // Write initial tick=0 telemetry snapshot
        const WorkloadStep init_step = workload_at(0U);
        apply_workload_and_sensors(init_step, 1000ULL, 0U);
        return true;
    }

    bool write_config_file(bool bypass_on_gaming) const {
        std::ostringstream conf;
        conf << "[general]\n"
             << "version = 6.1.0-R\n"
             << "profile = autonomous\n"
             << "inspection_mode = false\n\n"
             << "[features]\n"
             << "enable_frame_smoothness = true\n"
             << "enable_storage_burst = true\n"
             << "enable_memory_guard = true\n"
             << "enable_thermal_pacing = true\n"
             << "enable_idle_compaction = true\n"
             << "enable_predictive_preload = true\n"
             << "enable_camera_boost = true\n\n"
             << "[power]\n"
             << "bypass_charging = false\n"
             << "bypass_on_gaming = " << (bypass_on_gaming ? "true" : "false") << "\n"
             << "charge_limit_pct = 0\n\n"
             << "[thermal_guardian]\n"
             << "temp_monitor_interval_ms = 1000\n"
             << "thermal_trip_celsius = 44.0\n"
             << "thermal_recover_celsius = 40.0\n"
             << "thermal_emergency_margin_celsius = 5.0\n";
        return write_file_in_place(config_path_, conf.str());
    }

    // Returns deterministic workload parameters for virtual second `t_sec` (0..total_ticks-1)
    [[nodiscard]] WorkloadStep workload_at(uint32_t t_sec) const {
        WorkloadStep w{};
        w.ambient_c = 26.0;
        w.r_theta_c_per_w = 5.5;
        w.c_chassis_j_per_c = 180.0 / 5.5; // tau = 180s

        switch (cfg_.id) {
            case ArchetypeId::SnapdragonHyperOs: {
                if (t_sec < 45U) {
                    // Cold boot storm -> stays in BOOT_SETTLE (0 sysfs mutations!)
                    w.fg_pid = 2001;
                    w.fg_pkg = "com.miui.home";
                    w.cpu_util_pct = 74.0;
                    w.gpu_busy_pct = 12.0;
                    w.io_psi = 18.0;
                    w.cpu_psi = 15.0;
                    w.demand_power_w = 2.6;
                    w.actual_fps = 60.0;
                } else if (t_sec < 95U) {
                    // Post-boot quiescence -> exits BOOT_SETTLE & completes 30s soft-start ramp
                    w.fg_pid = 2001;
                    w.fg_pkg = "com.miui.home";
                    w.cpu_util_pct = 10.0;
                    w.gpu_busy_pct = 2.0;
                    w.io_psi = 0.5;
                    w.demand_power_w = 1.0;
                    w.actual_fps = 0.0;
                } else if (t_sec < 120U) {
                    // Interactive touch scroll in Chrome
                    w.fg_pid = 2002;
                    w.fg_pkg = "com.android.chrome";
                    w.send_touch = (t_sec % 3U == 0U);
                    w.cpu_util_pct = 34.0;
                    w.gpu_busy_pct = 24.0;
                    w.render_exec_ratio = 0.35;
                    w.demand_power_w = 2.2;
                    w.actual_fps = 120.0;
                } else if (t_sec < 1500U) {
                    // Bursty 3D gaming (combat 4.0W / exploration 1.9W every 90s for RLS excitation;
                    // unthrottled steady-state ~44.2C would trip THERMAL_GUARD, CoreFlow steers smoothly <= 41.5C)
                    const bool combat = ((t_sec - 120U) / 90U) % 2U == 0U;
                    w.fg_pid = 2003;
                    w.fg_pkg = "com.miHoYo.GenshinImpact";
                    w.is_game = true;
                    w.cpu_util_pct = combat ? 72.0 : 48.0;
                    w.gpu_busy_pct = combat ? 78.0 : 52.0;
                    w.render_exec_ratio = combat ? 0.68 : 0.44;
                    w.run_delay_ms_per_sec = combat ? 3.0 : 1.2;
                    w.cpu_psi = combat ? 6.0 : 2.0;
                    w.demand_power_w = combat ? 3.9 : 1.9;
                    w.bg_power_w = 0.35;
                    w.actual_fps = 60.0;
                } else {
                    // Cooldown on launcher
                    w.fg_pid = 2001;
                    w.fg_pkg = "com.miui.home";
                    w.cpu_util_pct = 8.0;
                    w.gpu_busy_pct = 1.0;
                    w.demand_power_w = 0.9;
                    w.actual_fps = 0.0;
                }
                break;
            }
            case ArchetypeId::MediaTekDimensity: {
                w.r_theta_c_per_w = 5.4;
                w.c_chassis_j_per_c = 175.0 / 5.4;
                if (t_sec < 25U) {
                    // App launch burst + touch scroll
                    w.fg_pid = 2002;
                    w.fg_pkg = "com.android.chrome";
                    w.send_touch = (t_sec % 3U == 0U);
                    w.cpu_util_pct = 42.0;
                    w.gpu_busy_pct = 28.0;
                    w.render_exec_ratio = 0.40;
                    w.demand_power_w = 2.4;
                    w.actual_fps = 120.0;
                } else if (t_sec < 1500U) {
                    // Natural combat/exploration waves (90s each, 8 cycles) with matched load delta <= 16%
                    // and measurable CPU contention headroom:
                    //  - thermal_uclamp_cap genuinely cuts convex power (P ~ s^1.8) -> VerifiedEffective
                    //  - stubbed ged_boost_enable / gpu_boost_level yields 0% contention lift -> BypassedIneffective
                    const bool combat = ((t_sec - 25U) / 90U) % 2U == 0U;
                    const bool starve_window = (t_sec >= 140U && t_sec < 220U);
                    w.fg_pid = 2003;
                    w.fg_pkg = "com.tencent.ig";
                    w.is_game = true;
                    w.cpu_util_pct = combat ? 72.0 : 58.0;
                    w.gpu_busy_pct = combat ? 74.0 : 60.0;
                    w.render_exec_ratio = combat ? 0.65 : 0.48;
                    w.run_delay_ms_per_sec = starve_window ? 240.0 : (combat ? 22.0 : 18.0);
                    w.cpu_psi = starve_window ? 16.0 : (combat ? 10.0 : 9.0);
                    w.demand_power_w = combat ? 4.1 : 1.45;
                    w.bg_power_w = 0.30;
                    w.actual_fps = starve_window ? 96.0 : 120.0;
                } else {
                    w.fg_pid = 2001;
                    w.fg_pkg = "com.android.launcher3";
                    w.cpu_util_pct = 8.0;
                    w.gpu_busy_pct = 1.0;
                    w.demand_power_w = 0.9;
                    w.actual_fps = 0.0;
                }
                break;
            }
            case ArchetypeId::SamsungExynosSdhms: {
                w.r_theta_c_per_w = 5.6;
                w.c_chassis_j_per_c = 185.0 / 5.6;
                if (t_sec < 60U) {
                    // Static idle on non-LTPO fixed 120 Hz panel -> triggers 3-strike quarantine
                    w.fg_pid = 2001;
                    w.fg_pkg = "com.sec.android.app.launcher";
                    w.cpu_util_pct = 6.0;
                    w.gpu_busy_pct = 1.0;
                    w.render_exec_ratio = 0.01;
                    w.demand_power_w = 0.9;
                    w.actual_fps = 120.0; // panel/panel/fps stays pinned at 120 Hz
                } else if (t_sec < 1500U) {
                    // Game genuinely lagging at 25 FPS (high schedstat run_delay) while fixed-Hz panel lies "120"
                    const bool heavy = ((t_sec - 60U) / 90U) % 2U == 0U;
                    const bool lag_window = (t_sec >= 120U && t_sec < 240U);
                    w.fg_pid = 2003;
                    w.fg_pkg = "com.epicgames.fortnite";
                    w.is_game = true;
                    w.cpu_util_pct = heavy ? 78.0 : 52.0;
                    w.gpu_busy_pct = heavy ? 75.0 : 50.0;
                    w.render_exec_ratio = heavy ? 0.72 : 0.45;
                    w.run_delay_ms_per_sec = lag_window ? 260.0 : (heavy ? 24.0 : 4.0);
                    w.cpu_psi = lag_window ? 18.0 : 6.0;
                    w.demand_power_w = heavy ? 3.8 : 1.8;
                    w.bg_power_w = 0.35;
                    w.actual_fps = 120.0; // Fixed-Hz panel stays stuck at 120!
                } else {
                    w.fg_pid = 2001;
                    w.fg_pkg = "com.sec.android.app.launcher";
                    w.cpu_util_pct = 8.0;
                    w.gpu_busy_pct = 1.0;
                    w.demand_power_w = 0.9;
                    w.actual_fps = 120.0;
                }
                break;
            }
            case ArchetypeId::GoogleTensorLtpo: {
                w.r_theta_c_per_w = 5.5;
                w.c_chassis_j_per_c = 180.0 / 5.5;
                if (t_sec < 15U) {
                    // Initial scroll during Tier-B probation (must remain fail-closed!)
                    w.fg_pid = 2002;
                    w.fg_pkg = "com.google.android.apps.photos";
                    w.send_touch = (t_sec % 2U == 0U);
                    w.cpu_util_pct = 36.0;
                    w.gpu_busy_pct = 28.0;
                    w.render_exec_ratio = 0.35;
                    w.demand_power_w = 2.0;
                    w.actual_fps = 120.0;
                } else if (t_sec < 45U) {
                    // Static idle: LTPO VRR drops VBlank IRQ rate to 2 Hz -> promotes to ProvenCommit!
                    w.fg_pid = 2001;
                    w.fg_pkg = "com.google.android.apps.nexuslauncher";
                    w.cpu_util_pct = 6.0;
                    w.gpu_busy_pct = 1.0;
                    w.render_exec_ratio = 0.01;
                    w.demand_power_w = 0.9;
                    w.actual_fps = 2.0;
                } else if (t_sec < 105U) {
                    // Smooth 120 Hz scroll
                    w.fg_pid = 2002;
                    w.fg_pkg = "com.google.android.apps.photos";
                    w.send_touch = (t_sec % 2U == 0U);
                    w.cpu_util_pct = 32.0;
                    w.gpu_busy_pct = 26.0;
                    w.render_exec_ratio = 0.32;
                    w.demand_power_w = 2.1;
                    w.actual_fps = 120.0;
                } else if (t_sec < 185U) {
                    // Locked 60 FPS game on 120 Hz panel -> tests cadence anchor (0% false jank)
                    w.fg_pid = 2003;
                    w.fg_pkg = "com.supercell.brawlstars";
                    w.is_game = true;
                    w.cpu_util_pct = 48.0;
                    w.gpu_busy_pct = 55.0;
                    w.render_exec_ratio = 0.45;
                    w.demand_power_w = 2.8;
                    w.actual_fps = 60.0;
                } else if (t_sec < 265U) {
                    // Stutter oscillation (alternating 56 / 34 / 52 / 32 FPS) -> tests jank detection & uclamp rescue
                    static constexpr double kStutterPattern[4] = {56.0, 34.0, 52.0, 32.0};
                    w.fg_pid = 2003;
                    w.fg_pkg = "com.supercell.brawlstars";
                    w.is_game = true;
                    w.cpu_util_pct = 66.0;
                    w.gpu_busy_pct = 68.0;
                    w.render_exec_ratio = 0.65;
                    w.demand_power_w = 3.5;
                    w.actual_fps = kStutterPattern[t_sec % 4U];
                } else if (t_sec < 1500U) {
                    // Sustained bursty gaming (LTPO 90/120 Hz) for RLS convergence
                    const bool combat = ((t_sec - 265U) / 90U) % 2U == 0U;
                    w.fg_pid = 2003;
                    w.fg_pkg = "com.supercell.brawlstars";
                    w.is_game = true;
                    w.cpu_util_pct = combat ? 70.0 : 46.0;
                    w.gpu_busy_pct = combat ? 72.0 : 48.0;
                    w.render_exec_ratio = combat ? 0.62 : 0.42;
                    w.demand_power_w = combat ? 4.0 : 1.9;
                    w.bg_power_w = 0.30;
                    w.actual_fps = combat ? 120.0 : 90.0;
                } else {
                    w.fg_pid = 2001;
                    w.fg_pkg = "com.google.android.apps.nexuslauncher";
                    w.cpu_util_pct = 7.0;
                    w.gpu_busy_pct = 1.0;
                    w.demand_power_w = 0.9;
                    w.actual_fps = 2.0;
                }
                break;
            }
            case ArchetypeId::Budget4GbEmmcLegacy: {
                w.r_theta_c_per_w = 6.2;
                w.c_chassis_j_per_c = 180.0 / 6.2;
                if (t_sec < 35U) {
                    // Static idle on fixed 60 Hz non-LTPO panel -> fail-closed quarantine
                    w.fg_pid = 2001;
                    w.fg_pkg = "com.android.launcher3";
                    w.cpu_util_pct = 8.0;
                    w.gpu_busy_pct = 1.0;
                    w.render_exec_ratio = 0.01;
                    w.avail_ram_ratio = 0.40;
                    w.demand_power_w = 0.85;
                    w.actual_fps = 60.0;
                } else if (t_sec < 90U) {
                    // App launch (normal RAM)
                    w.fg_pid = 2002;
                    w.fg_pkg = "com.whatsapp";
                    w.send_touch = (t_sec == 36U || t_sec == 42U);
                    w.cpu_util_pct = 45.0;
                    w.gpu_busy_pct = 22.0;
                    w.render_exec_ratio = 0.42;
                    w.avail_ram_ratio = 0.36;
                    w.demand_power_w = 2.0;
                    w.actual_fps = 60.0;
                } else if (t_sec < 210U) {
                    // Regime A: Working-set file refault thrashing (no direct reclaim yet)
                    w.fg_pid = 2002;
                    w.fg_pkg = "com.whatsapp";
                    w.send_touch = (t_sec % 4U == 0U);
                    w.cpu_util_pct = 48.0;
                    w.gpu_busy_pct = 20.0;
                    w.render_exec_ratio = 0.45;
                    w.avail_ram_ratio = 0.26;
                    w.refault_per_sec = 1500.0;
                    w.direct_reclaim_per_sec = 4.0;
                    w.pswpin_per_sec = 10.0;
                    w.demand_power_w = 2.1;
                    w.actual_fps = 60.0;
                } else if (t_sec < 360U) {
                    // Regime B: Direct reclaim storm + tight 14% RAM + app switch attempt
                    w.fg_pid = (t_sec >= 250U) ? 2003 : 2002;
                    w.fg_pkg = (t_sec >= 250U) ? "com.instagram.android" : "com.whatsapp";
                    w.send_touch = (t_sec % 3U == 0U);
                    w.cpu_util_pct = 62.0;
                    w.gpu_busy_pct = 30.0;
                    w.render_exec_ratio = 0.58;
                    w.avail_ram_ratio = 0.14;
                    w.refault_per_sec = 900.0;
                    w.direct_reclaim_per_sec = 160.0;
                    w.pswpin_per_sec = 65.0;
                    w.demand_power_w = 2.4;
                    w.actual_fps = 60.0;
                } else if (t_sec < 1500U) {
                    // Mixed budget workload for thermal convergence
                    const bool active = ((t_sec - 360U) / 90U) % 2U == 0U;
                    w.fg_pid = 2003;
                    w.fg_pkg = "com.instagram.android";
                    w.send_touch = active && (t_sec % 3U == 0U);
                    w.cpu_util_pct = active ? 60.0 : 28.0;
                    w.gpu_busy_pct = active ? 45.0 : 12.0;
                    w.render_exec_ratio = active ? 0.50 : 0.22;
                    w.avail_ram_ratio = 0.34;
                    w.demand_power_w = active ? 3.2 : 1.4;
                    w.actual_fps = 60.0;
                } else {
                    // ScreenOff anti-aging / calm
                    w.screen_off = true;
                    w.fg_pid = 2001;
                    w.fg_pkg = "com.android.launcher3";
                    w.cpu_util_pct = 4.0;
                    w.gpu_busy_pct = 0.0;
                    w.avail_ram_ratio = 0.44;
                    w.demand_power_w = 0.45;
                    w.actual_fps = 0.0;
                }
                break;
            }
            case ArchetypeId::GamingBypassAmbientCooler: {
                w.fg_pid = 2003;
                w.fg_pkg = "com.activision.callofduty.shooter";
                w.is_game = true;
                w.actual_fps = 120.0;
                if (t_sec < 1080U) {
                    // Phase A (0..18m): Normal room Ta=26C, R=5.5, tau=180s -> RLS exceeds conf>=45, target=38.0C
                    const bool burst = (t_sec / 60U) % 2U == 0U;
                    w.ambient_c = 26.0;
                    w.r_theta_c_per_w = 5.5;
                    w.c_chassis_j_per_c = 180.0 / 5.5;
                    w.cpu_util_pct = burst ? 64.0 : 34.0;
                    w.gpu_busy_pct = burst ? 70.0 : 36.0;
                    w.render_exec_ratio = burst ? 0.60 : 0.30;
                    w.demand_power_w = burst ? 3.8 : 1.0;
                    w.bg_power_w = 0.20;
                    w.battery_soc_pct = 78;
                } else if (t_sec < 1860U) {
                    // Phase B (18..31m): Tropical ambient jump Ta=34.5C, R=5.5 -> 90s burst/lull breaks Ta/R collinearity
                    // so RLS identifies Ta~34.5C cleanly and lifts adaptive target into (39.5, 41.5]
                    const bool burst = ((t_sec - 1080U) / 90U) % 2U == 0U;
                    w.ambient_c = 34.5;
                    w.r_theta_c_per_w = 5.5;
                    w.c_chassis_j_per_c = 180.0 / 5.5;
                    w.cpu_util_pct = burst ? 54.0 : 18.0;
                    w.gpu_busy_pct = burst ? 58.0 : 14.0;
                    w.render_exec_ratio = burst ? 0.52 : 0.16;
                    w.demand_power_w = burst ? 2.05 : 0.60;
                    w.bg_power_w = burst ? 0.10 : 0.05;
                    w.battery_soc_pct = 76;
                } else if (t_sec < 2460U) {
                    // Phase C (31..41m): Sudden Peltier cooler attach Ta=16C, R=3.2 -> target returns 38.0C, P_budget surges
                    const bool burst = ((t_sec - 1860U) / 60U) % 2U == 0U;
                    w.ambient_c = 16.0;
                    w.r_theta_c_per_w = 3.2;
                    w.c_chassis_j_per_c = 112.0 / 3.2; // tau = 112s
                    w.cpu_util_pct = burst ? 78.0 : 54.0;
                    w.gpu_busy_pct = burst ? 84.0 : 58.0;
                    w.render_exec_ratio = burst ? 0.72 : 0.50;
                    w.demand_power_w = burst ? 5.6 : 2.8;
                    w.bg_power_w = 0.25;
                    w.battery_soc_pct = 75;
                } else {
                    // Phase D (41..45m): Plugged in charging + bypass_on_gaming=true + UVLO <=20% + >=25% resume
                    w.ambient_c = 22.0;
                    w.r_theta_c_per_w = 4.2;
                    w.c_chassis_j_per_c = 140.0 / 4.2;
                    w.charging = true;
                    w.charging_heat_w = 3.2; // Eliminated when coreflowd engages input_suspend=1
                    w.cpu_util_pct = 70.0;
                    w.gpu_busy_pct = 76.0;
                    w.render_exec_ratio = 0.65;
                    w.demand_power_w = 4.0;
                    w.bg_power_w = 0.25;
                    w.trigger_reload_bypass_on = (t_sec == 2460U);
                    if (t_sec < 2540U) {
                        w.battery_soc_pct = 75;
                    } else if (t_sec < 2620U) {
                        w.battery_soc_pct = 19; // Triggers hard UVLO floor release (<= 20%)
                    } else {
                        w.battery_soc_pct = 30; // Above 25% resume hysteresis -> cleanly re-engages bypass
                    }
                }
                break;
            }
        }
        return w;
    }

    // Advance plant state by `dt_sec` using actual sysfs actuator caps currently written by coreflowd_sim,
    // inject scheduled OEM adversarial overwrites, and publish updated virtual sysfs/procfs telemetry.
    void step_before_daemon(uint32_t t_sec, uint64_t now_ms, double dt_sec) {
        const WorkloadStep w = workload_at(t_sec);

        // 1. Read actual caps written by coreflowd_sim to close the physical feedback loop
        const double top_app_cap_frac = read_uclamp_max_fraction(sys_ / "fs/cgroup/top-app/cpu.uclamp.max", 1.0);
        const double bg_cap_frac = read_uclamp_max_fraction(sys_ / "fs/cgroup/background/cpu.uclamp.max", 1.0);
        const double gpu_cap_frac = read_gpu_cap_fraction();
        const double uclamp_max_frac = static_cast<double>(last_uclamp_max_) / 1024.0;
        const double eff_top_cap = std::min(top_app_cap_frac, uclamp_max_frac);

        // Per-SoC convex C*V^2*f silicon power scaling: P_dyn = (P_demand - P_base) * s_eff^alpha_soc
        const double cpu_eff = std::clamp(0.30 + 0.70 * eff_top_cap, 0.35, 1.0);
        const double gpu_eff = std::clamp(0.40 + 0.60 * gpu_cap_frac, 0.45, 1.0);
        const double s_eff = w.is_game ? (0.55 * cpu_eff + 0.45 * gpu_eff) : (0.75 * cpu_eff + 0.25 * gpu_eff);
        const double alpha = std::clamp(cfg_.silicon_power_alpha, 1.35, 2.80);
        const double p_base = w.screen_off ? 0.35 : 0.65;
        const double p_bg = w.bg_power_w * (0.45 + 0.55 * bg_cap_frac);
        const double raw_dyn = std::max(0.0, w.demand_power_w - p_base);
        const double p_dyn = raw_dyn * std::pow(s_eff, alpha);
        const double p_sys = std::clamp(p_base + p_dyn + p_bg, 0.30, 14.0);
        last_power_w_ = p_sys;
        last_unthrottled_power_w_ = std::clamp(p_base + raw_dyn + w.bg_power_w, 0.30, 14.0);
        const double raw_freq_ratio = std::clamp(
            std::pow(std::max(0.02, raw_dyn / std::max(cfg_.peak_dyn_power_w, 1.0)), 1.0 / alpha),
            0.26, 1.0);
        last_freq_ratio_ = std::clamp(raw_freq_ratio * s_eff, 0.25, 1.0);

        // Check whether coreflowd_sim engaged hardware bypass charging (input_suspend == "1")
        bool bypass_hw_on = false;
        if (cfg_.has_bypass_node) {
            bypass_hw_on = (read_file_trimmed(sys_ / "class/power_supply/battery/input_suspend") == "1");
        }
        const double p_chg = (w.charging && !bypass_hw_on) ? w.charging_heat_w : 0.0;

        // 2. Integrate 2-node lumped RC thermal ODE (Chassis + Silicon Die)
        if (t_sec > 0U) {
            const double r_theta = std::max(1.0, w.r_theta_c_per_w);
            const double c_chassis = std::max(30.0, w.c_chassis_j_per_c);
            const double dt_chassis = (dt_sec / c_chassis) *
                (p_sys + p_chg - (t_chassis_c_ - w.ambient_c) / r_theta);
            t_chassis_c_ = std::clamp(t_chassis_c_ + dt_chassis, 10.0, 56.0);

            const double die_target = t_chassis_c_ + p_sys * cfg_.r_die_c_per_w;
            const double alpha_die = 1.0 - std::exp(-dt_sec / std::max(1.0, cfg_.tau_die_s));
            t_die_c_ = std::clamp(t_die_c_ + alpha_die * (die_target - t_die_c_), 15.0, 92.0);
            uptime_sec_ += dt_sec;
        }

        // 3. Inject scheduled OEM adversarial daemon overwrites (e.g. mi_thermald, mtk_fpsgo, SDHMS)
        for (auto& oem : cfg_.oem_contenders) {
            if (t_sec >= oem.start_sec && t_sec <= oem.end_sec &&
                oem.period_sec > 0U && ((t_sec - oem.start_sec) % oem.period_sec == 0U)) {
                oem.toggle = !oem.toggle;
                const std::string& val = oem.toggle ? oem.val_a : oem.val_b;
                const fs::path target = sys_ / oem.rel_path;
                (void)write_file_in_place(target, val + "\n");
                oem_live_values_[target.string()] = val;
            }
        }

        // 4. Write all updated kernel sensors and /proc telemetry
        apply_workload_and_sensors(w, now_ms, t_sec);
    }

    // Record snapshot of OEM-contended nodes right before a daemon STEP to detect post-yield writes.
    [[nodiscard]] std::unordered_map<std::string, std::string> snapshot_contended_nodes() const {
        std::unordered_map<std::string, std::string> snap;
        for (const auto& oem : cfg_.oem_contenders) {
            const fs::path p = sys_ / oem.rel_path;
            snap[p.string()] = read_file_trimmed(p);
        }
        return snap;
    }

    [[nodiscard]] const std::unordered_map<std::string, std::string>& oem_live_values() const noexcept {
        return oem_live_values_;
    }

    void note_uclamp_min_feedback(uint32_t uclamp_min) noexcept {
        last_uclamp_min_ = uclamp_min;
    }

private:
    void record_factory(const fs::path& p, const std::string& val) {
        (void)write_file_in_place(p, val + "\n");
        factory_baselines_[p.string()] = val;
    }

    void write_uptime() const {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.2f %.2f\n", uptime_sec_, uptime_sec_ * 0.75);
        (void)write_file_in_place(proc_ / "uptime", buf);
    }

    void setup_cpu_topology() {
        const std::string gov = cfg_.use_walt ? "walt" : "schedutil";
        const std::string avail_govs = cfg_.use_walt ? "walt schedutil performance" : "schedutil performance";
        if (cfg_.is_two_cluster) {
            write_file_in_place(sys_ / "devices/system/cpu/possible", "0-7\n");
            write_file_in_place(sys_ / "devices/system/cpu/present", "0-7\n");
            write_file_in_place(sys_ / "devices/system/cpu/online", "0-7\n");
            for (int c = 0; c < 8; ++c) {
                const uint32_t cap = (c < 6) ? cfg_.little_cap : cfg_.big_cap;
                write_file_in_place(sys_ / "devices/system/cpu" / ("cpu" + std::to_string(c)) / "cpu_capacity",
                                    std::to_string(cap) + "\n");
            }
            create_policy("policy0", "0-5", "400000 700000 1000000 1300000 1600000 1800000", "1000000", gov, avail_govs);
            create_policy("policy6", "6-7", "600000 1000000 1400000 1800000 2000000", "1400000", gov, avail_govs);
        } else {
            write_file_in_place(sys_ / "devices/system/cpu/possible", "0-7\n");
            write_file_in_place(sys_ / "devices/system/cpu/present", "0-7\n");
            write_file_in_place(sys_ / "devices/system/cpu/online", "0-7\n");
            for (int c = 0; c < 8; ++c) {
                const uint32_t cap = (c < 4) ? cfg_.little_cap : (c < 7 ? cfg_.mid_cap : cfg_.big_cap);
                write_file_in_place(sys_ / "devices/system/cpu" / ("cpu" + std::to_string(c)) / "cpu_capacity",
                                    std::to_string(cap) + "\n");
            }
            create_policy("policy0", "0-3", "300000 600000 900000 1200000 1500000 1800000", "1200000", gov, avail_govs);
            create_policy("policy4", "4-6", "500000 900000 1300000 1700000 2100000 2600000", "1700000", gov, avail_govs);
            create_policy("policy7", "7", "700000 1200000 1700000 2200000 2700000 3200000", "1700000", gov, avail_govs);
        }
        if (cfg_.use_walt) {
            record_factory(proc_ / "sys/walt/sched_upmigrate", "85 95");
            record_factory(proc_ / "sys/walt/sched_downmigrate", "75 85");
        }
    }

    void create_policy(const std::string& name, const std::string& cpus,
                       const std::string& opps, const std::string& cur_khz,
                       const std::string& gov, const std::string& avail_govs) {
        const fs::path p = sys_ / "devices/system/cpu/cpufreq" / name;
        write_file_in_place(p / "related_cpus", cpus + "\n");
        write_file_in_place(p / "affected_cpus", cpus + "\n");
        write_file_in_place(p / "scaling_available_frequencies", opps + "\n");
        write_file_in_place(p / "scaling_governor", gov + "\n");
        write_file_in_place(p / "scaling_available_governors", avail_govs + "\n");
        write_file_in_place(p / "scaling_cur_freq", cur_khz + "\n");
        const fs::path g = p / gov;
        record_factory(g / "hispeed_freq", cur_khz);
        record_factory(g / "hispeed_load", "90");
        if (cfg_.use_walt) {
            record_factory(g / "up_rate_limit_us", "0");
            record_factory(g / "down_rate_limit_us", "0");
            record_factory(g / "pl", "0");
            record_factory(g / "boost", "0");
        } else {
            record_factory(g / "up_rate_limit_us", "1000");
            record_factory(g / "down_rate_limit_us", "10000");
        }
    }

    void setup_gpu_and_bus() {
        if (cfg_.gpu_kind == GpuKind::Kgsl) {
            const fs::path g = sys_ / "class/kgsl/kgsl-3d0";
            write_file_in_place(g / "gpu_model", cfg_.gpu_model + "\n");
            write_file_in_place(g / "gpu_available_frequencies",
                                "220000000 320000000 420000000 520000000 620000000 720000000 810000000 900000000\n");
            write_file_in_place(g / "num_pwrlevels", "8\n");
            record_factory(g / "max_pwrlevel", "0");
            record_factory(g / "min_pwrlevel", "7");
            record_factory(g / "idle_timer", "80");
            record_factory(g / "adrenoboost", "0");
            record_factory(g / "bus_split", "1");
            write_file_in_place(g / "gpu_busy_percentage", " 5 %\n");
        } else if (cfg_.gpu_kind == GpuKind::MtkGed) {
            write_file_in_place(sys_ / "kernel/ged/hal/gpu_utilization", "5 0 0\n");
            write_file_in_place(sys_ / "kernel/gpu/gpu_model", cfg_.gpu_model + "\n");
            write_file_in_place(sys_ / "kernel/gpu/gpu_freq_table",
                                "380000 550000 720000 900000 1100000 1300000\n");
            record_factory(sys_ / "kernel/gpu/gpu_max_clock", "1300000");
            record_factory(sys_ / "module/ged/parameters/ged_boost_enable", "0");
            record_factory(sys_ / "module/ged/parameters/gpu_boost_level", "0");
        } else {
            const fs::path g = sys_ / "class/devfreq/mali.0";
            write_file_in_place(g / "name", cfg_.gpu_model + "\n");
            write_file_in_place(g / "available_frequencies",
                                "250000000 450000000 650000000 850000000 1050000000\n");
            record_factory(g / "max_freq", "1050000000");
            write_file_in_place(g / "min_freq", "250000000\n");
            write_file_in_place(g / "cur_freq", "450000000\n");
            write_file_in_place(g / "load", "5@450000000\n");
        }

        // Memory bus devfreq (DDR) on non-legacy devices
        if (!cfg_.is_two_cluster) {
            const fs::path bus = sys_ / "class/devfreq/soc-ddr";
            write_file_in_place(bus / "name", "soc-ddr\n");
            write_file_in_place(bus / "available_frequencies",
                                "547000 1017000 1555000 2092000 2736000 3196000\n");
            record_factory(bus / "min_freq", "547000");
            write_file_in_place(bus / "max_freq", "3196000\n");
        }
    }

    void setup_storage() {
        std::error_code ec;
        fs::create_directories(sys_ / "class/block", ec);
        if (cfg_.is_emmc) {
            const fs::path dev = sys_ / "devices/platform/13540000.mmc/mmc_host/mmc0/block/mmcblk0";
            const fs::path q = dev / "queue";
            write_file_in_place(q / "rotational", "0\n");
            record_factory(q / "read_ahead_kb", "128");
            record_factory(q / "nr_requests", "64");
            write_file_in_place(q / "max_sectors_kb", "512\n");
            write_file_in_place(q / "logical_block_size", "512\n");
            write_file_in_place(q / "scheduler", "[mq-deadline] none\n");
            fs::create_symlink("../../devices/platform/13540000.mmc/mmc_host/mmc0/block/mmcblk0",
                               sys_ / "class/block/mmcblk0", ec);
        } else {
            const fs::path ufshc = sys_ / "devices/platform/1d84000.ufshc";
            record_factory(ufshc / "inflight_low_latency", "0");
            const fs::path dev = ufshc / "host0/target0:0:0/0:0:0:0/block/sda";
            const fs::path q = dev / "queue";
            write_file_in_place(q / "rotational", "0\n");
            record_factory(q / "read_ahead_kb", "128");
            record_factory(q / "nr_requests", "64");
            write_file_in_place(q / "max_sectors_kb", "1024\n");
            write_file_in_place(q / "logical_block_size", "4096\n");
            write_file_in_place(q / "scheduler", "[mq-deadline] kyber none\n");
            fs::create_symlink("../../devices/platform/1d84000.ufshc/host0/target0:0:0/0:0:0:0/block/sda",
                               sys_ / "class/block/sda", ec);
        }
    }

    void setup_memory_and_vm() {
        const uint64_t zram_bytes = static_cast<uint64_t>(cfg_.zram_mb) * 1024ULL * 1024ULL;
        write_file_in_place(sys_ / "block/zram0/disksize", std::to_string(zram_bytes) + "\n");
        write_file_in_place(proc_ / "swaps",
                            "Filename\tType\tSize\tUsed\tPriority\n"
                            "/dev/block/zram0\tpartition\t" + std::to_string(cfg_.zram_mb * 1024U) + "\t0\t-2\n");

        record_factory(proc_ / "sys/vm/swappiness", "60");
        record_factory(proc_ / "sys/vm/page-cluster", "0");
        record_factory(proc_ / "sys/vm/vfs_cache_pressure", "100");
        record_factory(proc_ / "sys/vm/watermark_scale_factor", "125");
        record_factory(proc_ / "sys/vm/compaction_proactiveness", "20");
        write_file_in_place(proc_ / "sys/vm/compact_memory", "0\n");

        if (cfg_.has_mglru) {
            write_file_in_place(sys_ / "kernel/mm/lru_gen/enabled", "0x0007\n");
            record_factory(sys_ / "kernel/mm/lru_gen/min_ttl_ms", "0");
        }
        write_vmstat();
    }

    void setup_cgroups() {
        const fs::path cg = sys_ / "fs/cgroup";
        write_file_in_place(proc_ / "self/mountinfo",
                            "31 20 0:28 / " + cg.string() + " rw - cgroup2 cgroup rw\n");
        write_file_in_place(cg / "cgroup.controllers", "cpu cpuset memory io\n");
        for (const char* grp : {"top-app", "foreground", "system-background", "background"}) {
            const fs::path d = cg / grp;
            write_file_in_place(d / "cpu.uclamp.min", "0.00\n");
            record_factory(d / "cpu.uclamp.max", "max");
            record_factory(d / "io.weight", "default 100");
            write_file_in_place(d / "cgroup.procs", "");
            write_file_in_place(d / "cgroup.threads", "");
        }
    }

    void setup_display() {
        const fs::path conn = sys_ / "class/drm/card0-DSI-1";
        write_file_in_place(conn / "status", "connected\n");
        write_file_in_place(conn / "dpms", "On\n");
        write_file_in_place(conn / "modes", "1080x2400@" + std::to_string(cfg_.refresh_hz) + "\n");
        write_file_in_place(sys_ / "class/backlight/panel0/brightness", "180\n");
        write_file_in_place(sys_ / "class/backlight/panel0/max_brightness", "255\n");
        write_file_in_place(sys_ / "class/backlight/panel0/bl_power", "0\n");

        switch (cfg_.panel_kind) {
            case PanelClockKind::TierASdeMeasured:
                write_file_in_place(sys_ / "class/drm/card0/sde-crtc-0/measured_fps",
                                    "fps: " + std::to_string(cfg_.refresh_hz) + ".0 duration: 1000.0 frame_count: " +
                                    std::to_string(cfg_.refresh_hz) + "\n");
                break;
            case PanelClockKind::TierAMtkFpsgo:
                write_file_in_place(proc_ / "mtk_fpsgo/fps", std::to_string(cfg_.refresh_hz) + "\n");
                break;
            case PanelClockKind::TierBFixedPanelFps:
                write_file_in_place(sys_ / "class/panel/panel/fps", std::to_string(cfg_.refresh_hz) + "\n");
                write_vblank_interrupts();
                break;
            case PanelClockKind::TierBLtpoVblank:
            case PanelClockKind::TierBFixedVblank:
                write_vblank_interrupts();
                break;
        }
    }

    void setup_thermals_and_battery() {
        // Thermal zones with realistic OEM passive trips (high enough so normal Cool-Glide at 38-41.5C
        // never triggers false THERMAL_GUARD while genuine overheating >= 46C battery / 88C die still trips).
        auto make_tz = [&](int idx, const std::string& type, int passive_mc, int hyst_mc, int crit_mc) {
            const fs::path tz = sys_ / "class/thermal" / ("thermal_zone" + std::to_string(idx));
            write_file_in_place(tz / "type", type + "\n");
            write_file_in_place(tz / "temp", "32000\n");
            if (passive_mc > 0) {
                write_file_in_place(tz / "trip_point_0_type", "passive\n");
                write_file_in_place(tz / "trip_point_0_temp", std::to_string(passive_mc) + "\n");
                write_file_in_place(tz / "trip_point_0_hyst", std::to_string(hyst_mc) + "\n");
            }
            if (crit_mc > 0) {
                write_file_in_place(tz / "trip_point_1_type", "critical\n");
                write_file_in_place(tz / "trip_point_1_temp", std::to_string(crit_mc) + "\n");
            }
        };
        make_tz(0, "cpu-0-0-usr", 88000, 5000, 115000);
        make_tz(1, "gpuss-0-usr", 86000, 5000, 112000);
        make_tz(2, "battery", 46000, 3000, 58000);
        if (cfg_.has_skin_sensor) {
            make_tz(3, "skin-msm-therm-usr", 46000, 3000, 55000);
        }
        if (cfg_.has_xo_therm) {
            make_tz(4, "xo-therm", 65000, 5000, 85000);
        }

        const fs::path bat = sys_ / "class/power_supply/battery";
        write_file_in_place(bat / "type", "Battery\n");
        write_file_in_place(bat / "status", "Discharging\n");
        write_file_in_place(bat / "temp", "310\n");
        write_file_in_place(bat / "capacity", "75\n");
        write_file_in_place(bat / "voltage_now", "4000000\n");
        write_file_in_place(bat / "current_now", "300000\n");
        record_factory(bat / "constant_charge_current_max", "5000000");
        if (cfg_.has_bypass_node) {
            record_factory(bat / "input_suspend", "0");
        }
    }

    void setup_processes() {
        // SurfaceFlinger (PID 500, RenderEngine TID 501)
        create_proc(500, "/system/bin/surfaceflinger", -900,
                    {{500, "surfaceflinger"}, {501, "RenderEngine"}});
        // Launcher (PID 2001)
        const std::string launcher_pkg =
            cfg_.id == ArchetypeId::SnapdragonHyperOs ? "com.miui.home" :
            cfg_.id == ArchetypeId::SamsungExynosSdhms ? "com.sec.android.app.launcher" :
            cfg_.id == ArchetypeId::GoogleTensorLtpo ? "com.google.android.apps.nexuslauncher" :
            "com.android.launcher3";
        create_proc(2001, launcher_pkg, 0, {{2001, "launcher"}, {2011, "RenderThread"}});

        // Interactive UI app (PID 2002)
        const std::string ui_pkg =
            cfg_.id == ArchetypeId::GoogleTensorLtpo ? "com.google.android.apps.photos" :
            cfg_.id == ArchetypeId::Budget4GbEmmcLegacy ? "com.whatsapp" :
            "com.android.chrome";
        create_proc(2002, ui_pkg, 0, {{2002, "main"}, {2021, "RenderThread"}, {2022, "hwuiTask0"}});

        // Heavy Game / Secondary App (PID 2003)
        if (cfg_.id == ArchetypeId::Budget4GbEmmcLegacy) {
            create_proc(2003, "com.instagram.android", 0, {{2003, "main"}, {2031, "RenderThread"}});
        } else {
            const std::string game_pkg =
                cfg_.id == ArchetypeId::SnapdragonHyperOs ? "com.miHoYo.GenshinImpact" :
                cfg_.id == ArchetypeId::MediaTekDimensity ? "com.tencent.ig" :
                cfg_.id == ArchetypeId::SamsungExynosSdhms ? "com.epicgames.fortnite" :
                cfg_.id == ArchetypeId::GoogleTensorLtpo ? "com.supercell.brawlstars" :
                "com.activision.callofduty.shooter";
            const std::string engine_tid_name =
                cfg_.id == ArchetypeId::SnapdragonHyperOs ? "UnityMain" :
                cfg_.id == ArchetypeId::MediaTekDimensity ? "GameThread" :
                "GameThread";
            create_proc(2003, game_pkg, 0,
                        {{2003, "main"}, {2031, "RenderThread"}, {2032, engine_tid_name}, {2033, "VulkanThread"}});
        }
    }

    void create_proc(int32_t pid, const std::string& cmdline, int32_t oom_adj,
                     const std::vector<std::pair<int32_t, std::string>>& tasks) {
        const fs::path pdir = proc_ / std::to_string(pid);
        std::string cmd = cmdline;
        cmd.push_back('\0');
        write_file_in_place(pdir / "cmdline", cmd);
        write_file_in_place(pdir / "oom_score_adj", std::to_string(oom_adj) + "\n");
        for (const auto& [tid, comm] : tasks) {
            const fs::path tdir = pdir / "task" / std::to_string(tid);
            write_file_in_place(tdir / "comm", comm + "\n");
            write_tid_stat(pdir, tid, comm, 0U);
            if (cfg_.has_schedstat) {
                write_file_in_place(tdir / "schedstat", "0 0 1\n");
                write_file_in_place(proc_ / std::to_string(tid) / "schedstat", "0 0 1\n");
            }
        }
    }

    void write_tid_stat(const fs::path& pdir, int32_t tid, const std::string& comm, uint64_t ticks) const {
        char buf[256];
        std::snprintf(buf, sizeof(buf),
                      "%d (%s) R 1 1 1 0 -1 0 0 0 0 0 %llu 0 0 0 20 0 1 0 100 0 0\n",
                      tid, comm.c_str(), static_cast<unsigned long long>(ticks));
        (void)write_file_in_place(pdir / "task" / std::to_string(tid) / "stat", buf);
    }

    void write_vblank_interrupts() const {
        const char* irq_label =
            cfg_.panel_kind == PanelClockKind::TierBFixedPanelFps ? "145: " :
            cfg_.panel_kind == PanelClockKind::TierBLtpoVblank ? "182: " : "92: ";
        const char* irq_desc =
            cfg_.panel_kind == PanelClockKind::TierBFixedPanelFps ? "GICv3 145 Edge exynos-drm-vblank" :
            cfg_.panel_kind == PanelClockKind::TierBLtpoVblank ? "GICv3 182 Edge dpu-vblank" :
            "GICv3 92 Edge mdss_dsi_vblank";
        std::ostringstream ss;
        ss << "           CPU0       CPU1\n"
           << irq_label << vblank_irqs_ << " 0 " << irq_desc << "\n";
        (void)write_file_in_place(proc_ / "interrupts", ss.str());
    }

    void write_vmstat() const {
        std::ostringstream ss;
        if (cfg_.legacy_vmstat) {
            ss << "workingset_refault " << vm_refault_file_ << "\n"
               << "pgsteal_direct_normal " << vm_direct_reclaim_ << "\n"
               << "pgsteal_direct_movable 0\n"
               << "pswpin " << vm_pswpin_ << "\n";
        } else {
            ss << "workingset_refault_file " << vm_refault_file_ << "\n"
               << "workingset_refault_anon 0\n"
               << "pgsteal_direct " << vm_direct_reclaim_ << "\n"
               << "pswpin " << vm_pswpin_ << "\n";
        }
        (void)write_file_in_place(proc_ / "vmstat", ss.str());
    }

    double read_uclamp_max_fraction(const fs::path& p, double fallback) const {
        const std::string raw = read_file_trimmed(p);
        if (raw.empty() || raw == "max") return 1.0;
        const double v = parse_double_or(raw, -1.0);
        if (v < 0.0) return fallback;
        return (v > 100.5) ? std::clamp(v / 1024.0, 0.2, 1.0) : std::clamp(v / 100.0, 0.2, 1.0);
    }

    double read_gpu_cap_fraction() const {
        if (cfg_.gpu_kind == GpuKind::Kgsl) {
            const double lvl = parse_double_or(read_file_trimmed(sys_ / "class/kgsl/kgsl-3d0/max_pwrlevel"), 0.0);
            return std::clamp(1.0 - (lvl / 7.0) * 0.70, 0.30, 1.0);
        }
        if (cfg_.gpu_kind == GpuKind::MtkGed) {
            const double clk = parse_double_or(read_file_trimmed(sys_ / "kernel/gpu/gpu_max_clock"), 1300000.0);
            return std::clamp(clk / 1300000.0, 0.30, 1.0);
        }
        const double freq = parse_double_or(read_file_trimmed(sys_ / "class/devfreq/mali.0/max_freq"), 1050000000.0);
        return std::clamp(freq / 1050000000.0, 0.30, 1.0);
    }

    void apply_workload_and_sensors(const WorkloadStep& w, uint64_t now_ms, uint32_t t_sec) {
        write_uptime();

        // Display power (screen on/off)
        write_file_in_place(sys_ / "class/drm/card0-DSI-1/dpms", w.screen_off ? "Off\n" : "On\n");
        write_file_in_place(sys_ / "class/backlight/panel0/brightness", w.screen_off ? "0\n" : "180\n");

        // Foreground app membership in top-app/cgroup.procs
        write_file_in_place(sys_ / "fs/cgroup/top-app/cgroup.procs",
                            w.screen_off ? "" : (std::to_string(w.fg_pid) + "\n"));

        // /proc/stat monotonic advance (1000 ticks per second)
        const uint64_t busy_ticks = static_cast<uint64_t>(std::llround(std::clamp(w.cpu_util_pct, 0.0, 100.0) * 10.0));
        const uint64_t idle_ticks = 1000ULL - busy_ticks;
        cpu_user_ticks_ += (busy_ticks * 7ULL) / 10ULL;
        cpu_sys_ticks_ += busy_ticks - (busy_ticks * 7ULL) / 10ULL;
        cpu_idle_ticks_ += idle_ticks;
        {
            char buf[160];
            std::snprintf(buf, sizeof(buf), "cpu %llu 0 %llu %llu 0 0 0 0 0 0\n",
                          static_cast<unsigned long long>(cpu_user_ticks_),
                          static_cast<unsigned long long>(cpu_sys_ticks_),
                          static_cast<unsigned long long>(cpu_idle_ticks_));
            (void)write_file_in_place(proc_ / "stat", buf);
        }

        // Foreground thread stat + schedstat (run_delay improves when coreflowd lifts uclamp_min)
        const double rescue_factor = (last_uclamp_min_ >= 300U) ? 0.45 : 1.0;
        const uint64_t exec_ticks = static_cast<uint64_t>(std::llround(std::clamp(w.render_exec_ratio, 0.0, 1.0) * 100.0));
        const uint64_t exec_ns = static_cast<uint64_t>(std::llround(std::clamp(w.render_exec_ratio, 0.0, 1.0) * 1'000'000'000.0));
        const uint64_t delay_ns = static_cast<uint64_t>(std::llround(
            std::max(0.0, w.run_delay_ms_per_sec * rescue_factor) * 1'000'000.0));
        if (!w.screen_off && w.fg_pid > 0) {
            tid_cpu_ticks_[w.fg_pid] += exec_ticks;
            const fs::path pdir = proc_ / std::to_string(w.fg_pid);
            const int32_t rt_tid = (w.fg_pid == 2001) ? 2011 : (w.fg_pid == 2002 ? 2021 : 2031);
            tid_cpu_ticks_[rt_tid] += exec_ticks;
            write_tid_stat(pdir, w.fg_pid, "main", tid_cpu_ticks_[w.fg_pid]);
            write_tid_stat(pdir, rt_tid, "RenderThread", tid_cpu_ticks_[rt_tid]);
            if (cfg_.has_schedstat) {
                tid_exec_ns_[rt_tid] += exec_ns;
                tid_delay_ns_[rt_tid] += delay_ns;
                const uint64_t wakeups = (w.run_delay_ms_per_sec >= 80.0)
                    ? 35ULL
                    : std::max<uint64_t>(10U, static_cast<uint64_t>(std::llround(std::max(12.0, w.actual_fps))));
                tid_pcount_[rt_tid] += wakeups;
                char sbuf[128];
                std::snprintf(sbuf, sizeof(sbuf), "%llu %llu %llu\n",
                              static_cast<unsigned long long>(tid_exec_ns_[rt_tid]),
                              static_cast<unsigned long long>(tid_delay_ns_[rt_tid]),
                              static_cast<unsigned long long>(tid_pcount_[rt_tid]));
                (void)write_file_in_place(pdir / "task" / std::to_string(rt_tid) / "schedstat", sbuf);
                (void)write_file_in_place(proc_ / std::to_string(rt_tid) / "schedstat", sbuf);
                // SurfaceFlinger schedstat
                tid_exec_ns_[501] += exec_ns / 3U;
                tid_delay_ns_[501] += delay_ns / 2U;
                tid_pcount_[501] += 60U;
                std::snprintf(sbuf, sizeof(sbuf), "%llu %llu %llu\n",
                              static_cast<unsigned long long>(tid_exec_ns_[501]),
                              static_cast<unsigned long long>(tid_delay_ns_[501]),
                              static_cast<unsigned long long>(tid_pcount_[501]));
                (void)write_file_in_place(proc_ / "500/task/501/schedstat", sbuf);
                (void)write_file_in_place(proc_ / "501/schedstat", sbuf);
            }
        }

        // GPU busy sensor
        const int gpu_int = static_cast<int>(std::llround(std::clamp(w.gpu_busy_pct, 0.0, 100.0)));
        if (cfg_.gpu_kind == GpuKind::Kgsl) {
            write_file_in_place(sys_ / "class/kgsl/kgsl-3d0/gpu_busy_percentage",
                                " " + std::to_string(gpu_int) + " %\n");
        } else if (cfg_.gpu_kind == GpuKind::MtkGed) {
            write_file_in_place(sys_ / "kernel/ged/hal/gpu_utilization",
                                std::to_string(gpu_int) + " 0 0\n");
        } else {
            write_file_in_place(sys_ / "class/devfreq/mali.0/load",
                                std::to_string(gpu_int) + "@650000000\n");
        }

        // PSI (absent on legacy kernel)
        if (cfg_.has_psi) {
            auto put_psi = [&](const char* name, double avg10) {
                char buf[96];
                std::snprintf(buf, sizeof(buf), "some avg10=%.2f avg60=%.2f avg300=%.2f total=%u\n",
                              avg10, avg10 * 0.7, avg10 * 0.5, t_sec * 100U + 10U);
                (void)write_file_in_place(proc_ / "pressure" / name, buf);
            };
            put_psi("cpu", w.cpu_psi);
            put_psi("memory", w.mem_psi);
            put_psi("io", w.io_psi);
        }

        // /proc/meminfo & /proc/vmstat
        {
            const uint64_t total_kb = static_cast<uint64_t>(cfg_.total_ram_mb) * 1024ULL;
            const uint64_t avail_kb = static_cast<uint64_t>(std::llround(
                static_cast<double>(total_kb) * std::clamp(w.avail_ram_ratio, 0.05, 0.90)));
            const uint64_t swap_total_kb = static_cast<uint64_t>(cfg_.zram_mb) * 1024ULL;
            const uint64_t swap_free_kb = (swap_total_kb * 3ULL) / 4ULL;
            std::ostringstream ms;
            ms << "MemTotal: " << total_kb << " kB\n"
               << "MemAvailable: " << avail_kb << " kB\n"
               << "SwapTotal: " << swap_total_kb << " kB\n"
               << "SwapFree: " << swap_free_kb << " kB\n";
            (void)write_file_in_place(proc_ / "meminfo", ms.str());
        }
        vm_refault_file_ += static_cast<uint64_t>(std::llround(std::max(0.0, w.refault_per_sec)));
        vm_direct_reclaim_ += static_cast<uint64_t>(std::llround(std::max(0.0, w.direct_reclaim_per_sec)));
        vm_pswpin_ += static_cast<uint64_t>(std::llround(std::max(0.0, w.pswpin_per_sec)));
        write_vmstat();

        // Frame-pacing sensor update
        if (!w.screen_off) {
            switch (cfg_.panel_kind) {
                case PanelClockKind::TierASdeMeasured: {
                    char fbuf[96];
                    const uint64_t fc = static_cast<uint64_t>(std::llround(std::max(0.0, w.actual_fps)));
                    std::snprintf(fbuf, sizeof(fbuf), "fps: %.1f duration: 1000.0 frame_count: %llu\n",
                                  w.actual_fps, static_cast<unsigned long long>(fc));
                    (void)write_file_in_place(sys_ / "class/drm/card0/sde-crtc-0/measured_fps", fbuf);
                    break;
                }
                case PanelClockKind::TierAMtkFpsgo: {
                    char fbuf[32];
                    std::snprintf(fbuf, sizeof(fbuf), "%.1f\n", w.actual_fps);
                    (void)write_file_in_place(proc_ / "mtk_fpsgo/fps", fbuf);
                    break;
                }
                case PanelClockKind::TierBFixedPanelFps:
                    write_file_in_place(sys_ / "class/panel/panel/fps", std::to_string(cfg_.refresh_hz) + "\n");
                    vblank_irqs_ += cfg_.refresh_hz;
                    write_vblank_interrupts();
                    break;
                case PanelClockKind::TierBLtpoVblank:
                    vblank_irqs_ += static_cast<uint64_t>(std::llround(std::max(1.0, w.actual_fps)));
                    write_vblank_interrupts();
                    break;
                case PanelClockKind::TierBFixedVblank:
                    vblank_irqs_ += cfg_.refresh_hz;
                    write_vblank_interrupts();
                    break;
            }
        }

        // Thermal sensors: battery quantized to cfg_.battery_quantum_c (0.1C or 1.0C integer steps
        // replicating PARROTPRO thermal_zone52), optional skin sensor, and optional 0.001C xo-therm.
        const double batt_q = std::max(0.1, cfg_.battery_quantum_c);
        const double batt_quantized_c = std::round(t_chassis_c_ / batt_q) * batt_q;
        const int batt_mc = static_cast<int>(std::llround(batt_quantized_c * 1000.0));
        const int batt_deci = static_cast<int>(std::llround(batt_quantized_c * 10.0));
        const int die_deci = static_cast<int>(std::llround(t_die_c_ * 10.0));
        const int gpu_deci = static_cast<int>(std::llround((t_die_c_ - 2.0) * 10.0));
        const int skin_deci = static_cast<int>(std::llround((t_chassis_c_ - 0.3) * 10.0));
        write_sensor_with_mtime(sys_ / "class/thermal/thermal_zone0/temp", std::to_string(die_deci * 100) + "\n", now_ms);
        write_sensor_with_mtime(sys_ / "class/thermal/thermal_zone1/temp", std::to_string(gpu_deci * 100) + "\n", now_ms);
        write_sensor_with_mtime(sys_ / "class/thermal/thermal_zone2/temp", std::to_string(batt_mc) + "\n", now_ms);
        if (cfg_.has_skin_sensor) {
            write_sensor_with_mtime(sys_ / "class/thermal/thermal_zone3/temp", std::to_string(skin_deci * 100) + "\n", now_ms);
        }
        if (cfg_.has_xo_therm) {
            // Millidegree board/XO thermistor with realistic +2.8C static offset & mild die coupling
            const double xo_c = t_chassis_c_ + 2.8 + 0.08 * std::max(0.0, t_die_c_ - t_chassis_c_);
            const int xo_mc = static_cast<int>(std::llround(xo_c * 1000.0));
            write_sensor_with_mtime(sys_ / "class/thermal/thermal_zone4/temp", std::to_string(xo_mc) + "\n", now_ms);
        }

        // Power supply / battery telemetry (voltage = 4.0V => current_uA = P_sys * 250000)
        const fs::path bat = sys_ / "class/power_supply/battery";
        write_sensor_with_mtime(bat / "temp", std::to_string(batt_deci) + "\n", now_ms);
        write_file_in_place(bat / "status", w.charging ? "Charging\n" : "Discharging\n");
        write_file_in_place(bat / "capacity", std::to_string(std::clamp(w.battery_soc_pct, 1, 100)) + "\n");
        const uint64_t current_ua = static_cast<uint64_t>(std::llround(last_power_w_ * 250000.0));
        write_file_in_place(bat / "current_now", std::to_string(current_ua) + "\n");

        // Dynamic CPU cluster scaling_cur_freq reflecting actual silicon load & thermal/EDP caps
        const auto write_cur_freq = [&](const char* policy, uint64_t max_khz) {
            const uint64_t khz = static_cast<uint64_t>(std::llround(
                static_cast<double>(max_khz) * std::clamp(last_freq_ratio_, 0.25, 1.0)));
            write_file_in_place(
                sys_ / "devices/system/cpu/cpufreq" / policy / "scaling_cur_freq",
                std::to_string(khz) + "\n");
        };
        if (cfg_.is_two_cluster) {
            write_cur_freq("policy0", 1800000ULL);
            write_cur_freq("policy6", 2000000ULL);
        } else {
            write_cur_freq("policy0", 1800000ULL);
            write_cur_freq("policy4", 2600000ULL);
            write_cur_freq("policy7", 3200000ULL);
        }
    }

    ArchetypeConfig cfg_;
    fs::path root_;
    fs::path sys_;
    fs::path proc_;
    fs::path state_;
    fs::path config_path_;
    double t_chassis_c_{31.0};
    double t_die_c_{35.0};
    double last_power_w_{1.5};
    double last_unthrottled_power_w_{1.5};
    double last_freq_ratio_{0.55};
    double uptime_sec_{300.0};
    uint64_t cpu_user_ticks_{10000};
    uint64_t cpu_sys_ticks_{2000};
    uint64_t cpu_idle_ticks_{50000};
    uint64_t vblank_irqs_{10000};
    uint64_t vm_refault_file_{500};
    uint64_t vm_direct_reclaim_{0};
    uint64_t vm_pswpin_{0};
    uint32_t last_uclamp_min_{0};
    uint32_t last_uclamp_max_{1024};
    std::unordered_map<int32_t, uint64_t> tid_cpu_ticks_;
    std::unordered_map<int32_t, uint64_t> tid_exec_ns_;
    std::unordered_map<int32_t, uint64_t> tid_delay_ns_;
    std::unordered_map<int32_t, uint64_t> tid_pcount_;
    std::unordered_map<std::string, std::string> factory_baselines_;
    std::unordered_map<std::string, std::string> oem_live_values_;
};

// Factory for the 6 mandatory cross-OEM Virtual SoC Farm archetypes
inline std::vector<ArchetypeConfig> build_all_six_archetypes() {
    std::vector<ArchetypeConfig> list;
    list.reserve(6U);

    // 1. Snapdragon + WALT + KGSL + HyperOS (mi_thermald fighting back + cold boot sanctuary)
    {
        ArchetypeConfig a{};
        a.id = ArchetypeId::SnapdragonHyperOs;
        a.name = "snapdragon_hyperos";
        a.soc_model = "Qualcomm SM8650 (Snapdragon 8 Gen 3)";
        a.kernel_release = "6.1.75-android14-hyperos";
        a.rom_version = "OS2.0.104.0.VNCMIXM";
        a.refresh_hz = 120;
        a.total_ram_mb = 12288;
        a.zram_mb = 4096;
        a.use_walt = true;
        a.gpu_kind = GpuKind::Kgsl;
        a.gpu_model = "Adreno 750";
        a.panel_kind = PanelClockKind::TierASdeMeasured;
        a.initial_uptime_sec = 15.0; // Cold boot -> triggers BOOT_SETTLE sanctuary!
        a.initial_chassis_c = 30.0;
        a.initial_die_c = 36.0;
        // Replicates real PARROTPRO hardware: 1°C integer battery + 0.001°C millidegree xo-therm
        a.battery_quantum_c = 1.0;
        a.has_skin_sensor = false;
        a.has_xo_therm = true;
        a.silicon_power_alpha = 2.15;
        a.peak_dyn_power_w = 4.0;
        a.total_ticks = 1800;
        a.oem_contenders = {
            {"devices/system/cpu/cpufreq/policy7/walt/hispeed_freq", "1900000", "2200000", 15U, 150U, 1480U, false},
            {"class/kgsl/kgsl-3d0/max_pwrlevel", "1", "2", 15U, 240U, 1480U, false},
        };
        list.push_back(std::move(a));
    }

    // 2. MediaTek Dimensity + Mali/Immortalis + GED/FPSGO + mtk_fpsgo contention
    {
        ArchetypeConfig a{};
        a.id = ArchetypeId::MediaTekDimensity;
        a.name = "mediatek_dimensity";
        a.soc_model = "MediaTek Dimensity 9300 (MT6989)";
        a.kernel_release = "6.1.68-android14-mtk";
        a.rom_version = "ColorOS-14.1-MT6989";
        a.refresh_hz = 120;
        a.total_ram_mb = 12288;
        a.zram_mb = 4096;
        a.little_cap = 400;
        a.mid_cap = 780;
        a.big_cap = 1024;
        a.use_walt = false;
        a.gpu_kind = GpuKind::MtkGed;
        a.gpu_model = "Mali-G720 Immortalis MC12";
        a.panel_kind = PanelClockKind::TierAMtkFpsgo;
        a.initial_uptime_sec = 300.0;
        a.silicon_power_alpha = 2.05;
        a.peak_dyn_power_w = 4.0;
        a.total_ticks = 1800;
        a.oem_contenders = {
            {"devices/system/cpu/cpufreq/policy7/schedutil/up_rate_limit_us", "500", "2000", 15U, 60U, 1480U, false},
        };
        list.push_back(std::move(a));
    }

    // 3. Samsung Exynos + Aggressive SDHMS/GOS + Non-LTPO Fixed-Hz 120 Hz Panel
    {
        ArchetypeConfig a{};
        a.id = ArchetypeId::SamsungExynosSdhms;
        a.name = "samsung_exynos_sdhms";
        a.soc_model = "Samsung Exynos 2400 (S5E9945)";
        a.kernel_release = "6.1.62-android14-exynos";
        a.rom_version = "OneUI-6.1-S926BXXU3";
        a.refresh_hz = 120;
        a.total_ram_mb = 8192;
        a.zram_mb = 4096;
        a.little_cap = 360;
        a.mid_cap = 720;
        a.big_cap = 1024;
        a.use_walt = false;
        a.gpu_kind = GpuKind::Devfreq;
        a.gpu_model = "Xclipse 940";
        a.panel_kind = PanelClockKind::TierBFixedPanelFps;
        a.initial_uptime_sec = 300.0;
        a.silicon_power_alpha = 2.40;
        a.peak_dyn_power_w = 4.0;
        a.total_ticks = 1800;
        a.oem_contenders = {
            {"devices/system/cpu/cpufreq/policy7/schedutil/hispeed_freq", "1700000", "2100000", 10U, 90U, 1480U, false},
            {"fs/cgroup/top-app/cpu.uclamp.max", "75.00", "82.00", 10U, 200U, 1480U, false},
        };
        list.push_back(std::move(a));
    }

    // 4. Google Tensor + Dynamic LTPO 1-120 Hz VRR (VBlank IRQ Tier B Promotion)
    {
        ArchetypeConfig a{};
        a.id = ArchetypeId::GoogleTensorLtpo;
        a.name = "google_tensor_ltpo";
        a.soc_model = "Google Tensor G3 (Zuma)";
        a.kernel_release = "6.1.84-android15-gki";
        a.rom_version = "AP3A.241005.015";
        a.refresh_hz = 120;
        a.total_ram_mb = 12288;
        a.zram_mb = 4096;
        a.little_cap = 350;
        a.mid_cap = 700;
        a.big_cap = 1024;
        a.use_walt = false;
        a.gpu_kind = GpuKind::Devfreq;
        a.gpu_model = "Mali-G715";
        a.panel_kind = PanelClockKind::TierBLtpoVblank;
        a.initial_uptime_sec = 300.0;
        a.silicon_power_alpha = 2.20;
        a.peak_dyn_power_w = 4.0;
        a.total_ticks = 1800;
        list.push_back(std::move(a));
    }

    // 5. Budget 4 GB RAM + eMMC + Legacy Kernel (NO PSI, NO MGLRU, NO schedstat, Fixed 60 Hz non-LTPO)
    {
        ArchetypeConfig a{};
        a.id = ArchetypeId::Budget4GbEmmcLegacy;
        a.name = "budget_4gb_emmc_legacy";
        a.soc_model = "Helio G85 / Legacy ARM64";
        a.kernel_release = "4.19.191-legacy-emmc";
        a.rom_version = "AOSP-13-Budget4GB";
        a.refresh_hz = 60;
        a.total_ram_mb = 4096;
        a.zram_mb = 2048;
        a.is_emmc = true;
        a.has_psi = false;
        a.has_mglru = false;
        a.has_schedstat = false;
        a.legacy_vmstat = true;
        a.is_two_cluster = true;
        a.little_cap = 420;
        a.big_cap = 1024;
        a.gpu_kind = GpuKind::Devfreq;
        a.gpu_model = "Mali-G52 MC2";
        a.panel_kind = PanelClockKind::TierBFixedVblank;
        a.initial_uptime_sec = 300.0;
        // Pure 1°C integer battery sensor with NO skin / xo-therm auxiliary sensor
        a.battery_quantum_c = 1.0;
        a.has_skin_sensor = false;
        a.has_xo_therm = false;
        a.silicon_power_alpha = 1.60;
        a.peak_dyn_power_w = 3.0;
        a.total_ticks = 1800;
        list.push_back(std::move(a));
    }

    // 6. Flagship Gaming + Bypass Charging + Tropical Ambient Shift + Sudden Peltier Cooler
    {
        ArchetypeConfig a{};
        a.id = ArchetypeId::GamingBypassAmbientCooler;
        a.name = "gaming_bypass_ambient_cooler";
        a.soc_model = "Qualcomm SM8650 Gaming Edition";
        a.kernel_release = "6.1.75-android14-gaming";
        a.rom_version = "GamingOS-14.2";
        a.refresh_hz = 144;
        a.total_ram_mb = 16384;
        a.zram_mb = 4096;
        a.use_walt = true;
        a.has_bypass_node = true;
        a.gpu_kind = GpuKind::Kgsl;
        a.gpu_model = "Adreno 750";
        a.panel_kind = PanelClockKind::TierASdeMeasured;
        a.initial_uptime_sec = 300.0;
        a.initial_chassis_c = 30.0;
        a.initial_die_c = 36.0;
        a.silicon_power_alpha = 2.10;
        a.peak_dyn_power_w = 5.2;
        a.total_ticks = 2700;
        list.push_back(std::move(a));
    }

    return list;
}

} // namespace cf_sim
