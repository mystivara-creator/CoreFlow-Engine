/**
 * @file context_engine.cpp
 * @brief CoreFlow v5 Pillar 2: 5-Mode Context Classifier, Foreground App & Critical Thread Resolver.
 * @license Apache-2.0
 */
#include "coreflow/v5/context_engine.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <fcntl.h>
#include <filesystem>
#include <sched.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow::v5 {
namespace fs = std::filesystem;

namespace {

std::string to_lower_copy(std::string_view s) {
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

std::string read_small_file(const fs::path& path, size_t limit = 512U) {
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return {};
    char buf[512];
    std::string out;
    while (out.size() < limit) {
        const ssize_t n = ::read(fd, buf, std::min(sizeof(buf), limit - out.size()));
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        out.append(buf, static_cast<size_t>(n));
    }
    (void)::close(fd);
    return out;
}

struct ParsedCmdline {
    std::string name;
    bool has_path_slash{false};
    bool has_service_suffix{false};
};

ParsedCmdline read_cmdline_parsed(const fs::path& proc_dir) {
    ParsedCmdline out{};
    std::string raw = read_small_file(proc_dir / "cmdline", 256U);
    if (raw.empty()) return out;
    const auto nul = raw.find('\0');
    if (nul != std::string::npos) raw.resize(nul);
    while (!raw.empty() && (raw.back() == '\n' || raw.back() == '\r' || raw.back() == ' ')) {
        raw.pop_back();
    }
    if (raw.find('/') != std::string::npos) {
        out.has_path_slash = true;
        const auto slash = raw.find_last_of('/');
        raw = raw.substr(slash + 1U);
    }
    const auto colon = raw.find(':');
    if (colon != std::string::npos && colon > 0U) {
        out.has_service_suffix = true;
        raw.resize(colon);
    }
    out.name = std::move(raw);
    return out;
}

bool parse_i32_signed(std::string_view text, int32_t& out) noexcept {
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) text.remove_prefix(1);
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r' || text.back() == ' ')) text.remove_suffix(1);
    if (text.empty() || text.size() > 12U) return false;
    const auto r = std::from_chars(text.data(), text.data() + text.size(), out);
    return r.ec == std::errc{} && r.ptr == text.data() + text.size();
}

bool parse_i32(std::string_view text, int32_t& out) noexcept {
    return parse_i32_signed(text, out) && out > 0;
}

std::optional<int32_t> read_oom_score_adj(const fs::path& proc_dir) noexcept {
    const std::string raw = read_small_file(proc_dir / "oom_score_adj", 32U);
    if (raw.empty()) return std::nullopt;
    int32_t adj = 0;
    if (!parse_i32_signed(raw, adj)) return std::nullopt;
    return adj;
}

bool is_android_user_app_package(std::string_view pkg, bool has_path_slash) noexcept {
    // Real Android app processes have package names without path separators (e.g. com.example.app).
    if (has_path_slash || pkg.size() < 3U || pkg == "coreflowd" || pkg == "surfaceflinger" ||
        pkg == "system_server" || pkg == "com.android.systemui") {
        return false;
    }
    return pkg.find('.') != std::string_view::npos;
}

bool has_app_uid_or_test_proc(const fs::path& proc_dir) noexcept {
    struct stat st{};
    if (::stat(proc_dir.c_str(), &st) != 0) return false;
    // On Android, user apps have uid >= 10000. On Linux host unit tests, allow any regular test fixture.
#ifdef __ANDROID__
    return st.st_uid >= 10000U;
#else
    return true;
#endif
}

bool is_cfs_thread(int32_t tid, const std::string& procfs_root) noexcept {
    if (procfs_root != "/proc") return true;
    const int policy = ::sched_getscheduler(static_cast<pid_t>(tid));
    if (policy < 0) return true; // Let sched_setattr decide if inaccessible
    const int base_policy = policy & ~SCHED_RESET_ON_FORK;
    // Never clamp down RT (SCHED_FIFO / SCHED_RR) or SCHED_DEADLINE threads.
    return base_policy == SCHED_OTHER || base_policy == SCHED_BATCH || base_policy == SCHED_IDLE;
}

uint64_t steady_now_ms() noexcept {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

} // namespace

ContextEngine::ContextEngine(std::string procfs_root) noexcept
    : procfs_root_(std::move(procfs_root)) {}

void ContextEngine::set_profile(PerformanceProfile profile) noexcept {
    profile_ = profile;
}

void ContextEngine::set_app_rules(
    const std::vector<std::string>& gaming_apps,
    const std::vector<std::string>& excluded_apps,
    const std::vector<std::string>& smoothness_apps,
    const std::vector<std::string>& battery_apps) {
    gaming_apps_.clear();
    excluded_apps_.clear();
    smoothness_apps_.clear();
    battery_apps_.clear();
    for (const auto& pkg : gaming_apps) {
        if (!pkg.empty()) gaming_apps_.insert(to_lower_copy(pkg));
    }
    for (const auto& pkg : excluded_apps) {
        if (!pkg.empty()) excluded_apps_.insert(to_lower_copy(pkg));
    }
    for (const auto& pkg : smoothness_apps) {
        if (!pkg.empty()) smoothness_apps_.insert(to_lower_copy(pkg));
    }
    for (const auto& pkg : battery_apps) {
        if (!pkg.empty()) battery_apps_.insert(to_lower_copy(pkg));
    }
}

void ContextEngine::notify_touch(uint64_t now_ms) noexcept {
    if (now_ms >= last_touch_ms_) last_touch_ms_ = now_ms;
}

PerformanceProfile ContextEngine::parse_profile(std::string_view text) noexcept {
    const std::string lower = to_lower_copy(text);
    if (lower == "smoothness" || lower == "smooth") return PerformanceProfile::Smoothness;
    if (lower == "battery" || lower == "efficiency" || lower == "powersave") return PerformanceProfile::Battery;
    if (lower == "gaming" || lower == "performance") return PerformanceProfile::Gaming;
    return PerformanceProfile::Balanced;
}

const char* ContextEngine::profile_name(PerformanceProfile profile) noexcept {
    switch (profile) {
        case PerformanceProfile::Smoothness: return "smoothness";
        case PerformanceProfile::Battery: return "battery";
        case PerformanceProfile::Gaming: return "gaming";
        default: return "balanced";
    }
}

const char* ContextEngine::mode_name(ContextMode mode) noexcept {
    switch (mode) {
        case ContextMode::LaunchBurst: return "LAUNCH_BURST";
        case ContextMode::Interactive: return "INTERACTIVE";
        case ContextMode::Gaming: return "GAMING";
        case ContextMode::ScreenOff: return "SCREEN_OFF";
        case ContextMode::Excluded: return "EXCLUDED";
        default: return "EFFICIENCY";
    }
}

bool ContextEngine::is_game_engine_thread_name(std::string_view comm) noexcept {
    if (comm.empty()) return false;
    return comm == "UnityMain" ||
           comm.rfind("UnityGfxDevice", 0) == 0 ||
           comm == "GameThread" ||
           comm == "RHIThread" ||
           comm == "VulkanThread" ||
           comm == "RenderThread 0" ||
           comm == "Godot" ||
           comm == "RenderServer" ||
           comm == "EmuThread" ||
           comm == "CPUThread" ||
           comm == "VideoThread" ||
           comm == "SDLThread" ||
           comm.rfind("GLThread", 0) == 0;
}

bool ContextEngine::is_helper_pipeline_thread_name(std::string_view comm) noexcept {
    if (comm.empty()) return false;
    return comm.rfind("hwuiTask", 0) == 0 ||
           comm == "CrRendererMain" ||
           comm == "Compositor" ||
           comm == "VizCompositorTh" ||
           comm == "SkiaOutput" ||
           comm == "mali-cmar-backe" ||
           comm == "adreno-worker";
}

bool ContextEngine::is_critical_render_thread_name(std::string_view comm) noexcept {
    if (comm.empty()) return false;
    if (is_game_engine_thread_name(comm)) return true;
    // Android UI / HWUI / Compositor / Chromium WebView & GPU driver worker threads
    return comm == "RenderThread" ||
           comm == "surfaceflinger" ||
           comm == "RenderEngine" ||
           comm == "Binder:sf" ||
           comm.rfind("RenderThread", 0) == 0 ||
           is_helper_pipeline_thread_name(comm);
}

bool ContextEngine::is_media_pipeline_thread_name(std::string_view comm) noexcept {
    if (comm.empty()) return false;
    return comm == "MediaCodec_loop" ||
           comm == "CodecLooper" ||
           comm == "NuPlayerDriver" ||
           comm == "AudioTrack" ||
           comm == "OMXCallbackDisp" ||
           comm.rfind("ExoPlayer:Playb", 0) == 0 ||
           comm.rfind("ExoPlayer:Load", 0) == 0 ||
           comm.rfind("c2.qti.", 0) == 0 ||
           comm.rfind("c2.mtk.", 0) == 0 ||
           comm.rfind("c2.android.", 0) == 0;
}

bool ContextEngine::is_camera_package(std::string_view pkg) noexcept {
    if (pkg.empty()) return false;
    const std::string lower = to_lower_copy(pkg);
    return lower == "com.android.camera" ||
           lower == "com.miui.camera" ||
           lower == "com.google.android.googlecamera" ||
           lower == "org.codeaurora.snapcam" ||
           lower == "com.sec.android.app.camera" ||
           lower == "com.oplus.camera" ||
           lower == "com.oneplus.camera" ||
           lower == "net.sourceforge.opencamera" ||
           lower.find(".camera") != std::string::npos ||
           lower.find("gcam") != std::string::npos;
}

bool ContextEngine::is_background_system_service(std::string_view pkg) noexcept {
    if (pkg.empty()) return true;
    const std::string lower = to_lower_copy(pkg);
    auto starts_with = [&](std::string_view prefix) {
        return lower.size() >= prefix.size() && lower.compare(0, prefix.size(), prefix) == 0;
    };
    auto ends_with = [&](std::string_view suffix) {
        return lower.size() >= suffix.size() &&
               lower.compare(lower.size() - suffix.size(), suffix.size(), suffix) == 0;
    };
    if (ends_with(".service") || ends_with(".provider") || ends_with(".overlay") ||
        ends_with(".providers") || ends_with(".daemon") || ends_with(".verifier") ||
        ends_with(".agent") || ends_with(".stub")) {
        return true;
    }
    if (starts_with("com.android.providers.") ||
        starts_with("com.qualcomm.") ||
        starts_with("com.qti.") ||
        starts_with("com.mediatek.")) {
        return true;
    }
    return lower == "com.xiaomi.xmsf" ||
           lower == "com.xiaomi.joyose" ||
           lower == "com.xiaomi.metoknlp" ||
           lower == "com.xiaomi.mi_connect_service" ||
           lower == "com.xiaomi.finddevice" ||
           lower == "com.xiaomi.location.fused" ||
           lower == "com.xiaomi.account" ||
           lower == "com.xiaomi.barrage" ||
           lower == "com.xiaomi.simactivate.service" ||
           lower == "com.xiaomi.aiasst.service" ||
           lower == "com.miui.daemon" ||
           lower == "com.miui.analytics" ||
           lower == "com.miui.powerkeeper" ||
           lower == "com.miui.wmsvc" ||
           lower == "com.miui.securitycore" ||
           lower == "com.miui.guardprovider" ||
           lower == "com.miui.hybrid" ||
           lower == "com.miui.cloudservice" ||
           lower == "com.miui.micloudsync" ||
           lower == "com.miui.notification" ||
           lower == "com.miui.msa.global" ||
           lower == "com.miui.systemadsolution" ||
           lower == "com.miui.miwallpaper" ||
           lower == "com.miui.aod" ||
           lower == "com.miui.mishare.connectivity" ||
           lower == "com.xiaomi.glgm" ||
           lower == "com.xiaomi.mirror" ||
           lower == "com.android.externalstorage" ||
           lower == "com.android.se" ||
           lower == "com.android.keychain" ||
           lower == "com.android.shell" ||
           lower == "com.android.smspush" ||
           lower == "com.android.ims" ||
           lower == "com.android.ons" ||
           lower == "com.android.stk" ||
           lower == "com.android.server.telecom" ||
           lower == "com.android.cellbroadcastreceiver" ||
           lower == "com.android.dynsystem" ||
           lower == "com.android.localtransport" ||
           lower == "com.android.systemui" ||
           lower == "com.android.bluetooth" ||
           lower == "com.android.nfc" ||
           lower == "com.android.phone" ||
           lower == "com.android.permissioncontroller" ||
           lower == "com.android.networkstack" ||
           lower == "com.miui.securityadd" ||
           lower == "com.miui.cit" ||
           lower == "com.xiaomi.otrpbroker" ||
           lower == "com.xiaomi.bluetooth" ||
           lower == "com.google.android.gms" ||
           lower == "com.google.android.gsf" ||
           lower == "com.google.android.ext.services" ||
           lower == "com.google.android.verifier" ||
           lower == "com.google.android.adservices.api" ||
           lower == "com.google.android.as" ||
           lower == "com.google.android.as.oss" ||
           lower == "com.google.android.modulemetadata";
}

ContextDecision ContextEngine::evaluate(const ContextSignalInput& input) noexcept {
    ContextDecision out{};
    out.foreground_pid = input.foreground_pid;
    out.foreground_package = input.foreground_package;

    const std::string pkg_lower = to_lower_copy(input.foreground_package);
    out.app_excluded = !pkg_lower.empty() && excluded_apps_.contains(pkg_lower);
    out.app_forced_gaming = !pkg_lower.empty() && gaming_apps_.contains(pkg_lower);
    const bool is_cam = !out.app_excluded &&
        (input.is_camera_app || is_camera_package(input.foreground_package));
    out.camera_boost_active = is_cam && !input.screen_off;
    out.media_pipeline_active = !out.app_excluded && !input.screen_off && input.has_media_thread;

    PerformanceProfile eff_profile = profile_;
    if (out.app_forced_gaming) {
        eff_profile = PerformanceProfile::Gaming;
    } else if (!pkg_lower.empty() && smoothness_apps_.contains(pkg_lower)) {
        eff_profile = PerformanceProfile::Smoothness;
    } else if (!pkg_lower.empty() && battery_apps_.contains(pkg_lower)) {
        eff_profile = PerformanceProfile::Battery;
    }
    out.effective_profile = eff_profile;

    // Detect foreground app switch -> trigger LaunchBurst (except for excluded apps or screen-off)
    const bool fg_changed =
        !input.foreground_package.empty() &&
        (input.foreground_package != last_fg_package_ ||
         (input.foreground_pid > 0 && input.foreground_pid != last_fg_pid_));
    if (fg_changed) {
        last_fg_pid_ = input.foreground_pid;
        last_fg_package_ = input.foreground_package;
        gaming_latched_ = false;
        gpu_high_streak_ = 0;
        gpu_low_streak_ = 0;
        last_gaming_eval_ms_ = 0;
        if (!input.screen_off && !out.app_excluded) {
            const uint64_t burst_ms = is_cam ? 2400ULL :
                eff_profile == PerformanceProfile::Battery ? 1200ULL :
                eff_profile == PerformanceProfile::Smoothness ? 2000ULL : 1600ULL;
            launch_burst_until_ms_ = input.now_ms + burst_ms;
            out.newly_launched_app = true;
        }
    }

    if (input.screen_off) {
        gaming_latched_ = false;
        gpu_high_streak_ = 0;
        gpu_low_streak_ = 0;
        last_gaming_eval_ms_ = 0;
        out.mode = ContextMode::ScreenOff;
        return out;
    }

    if (out.app_excluded) {
        gaming_latched_ = false;
        out.mode = ContextMode::Excluded;
        return out;
    }

    // Hysteretic Gaming detection (rate-limited to >= 800 ms per sample so 200 ms fast-path
    // evaluations sharing one cached GPU sample cannot falsely latch Gaming in 600 ms):
    //  - Balanced/Smoothness/Battery: GPU >= 48% (or >= 35% + CPU >= 55%) for 3 ticks; exit < 25% for 5 ticks.
    //  - Gaming profile: lower entry threshold (GPU >= 32% or CPU >= 42%) for 2 ticks; exit GPU < 20% && CPU < 22% for 5 ticks
    //    so Gaming profile stays steady instead of flapping EFFICIENCY <-> GAMING every second.
    //  - Fallback (non-Qualcomm / CPU-bound emulators without GPU busy node):
    //    active game-engine thread (UnityMain/GameThread/EmuThread/etc.) + CPU >= 50% for 3 ticks.
    const bool streak_tick_ready =
        (last_gaming_eval_ms_ == 0U) ||
        (input.now_ms >= last_gaming_eval_ms_ + 800ULL) ||
        (input.now_ms < last_gaming_eval_ms_);
    if (streak_tick_ready) {
        const bool gaming_prof = (eff_profile == PerformanceProfile::Gaming);
        const uint32_t enter_ticks = gaming_prof ? 2U : 3U;
        if (input.gpu_busy_pct.has_value()) {
            last_gaming_eval_ms_ = input.now_ms;
            const double gpu = *input.gpu_busy_pct;
            const bool high_load = gaming_prof
                ? (gpu >= 32.0 || input.cpu_util_pct >= 42.0)
                : (gpu >= 48.0 || (gpu >= 35.0 && input.cpu_util_pct >= 55.0));
            const bool low_load = gaming_prof
                ? (gpu < 20.0 && input.cpu_util_pct < 22.0)
                : (gpu < 25.0);
            if (high_load) {
                ++gpu_high_streak_;
                gpu_low_streak_ = 0;
                if (gpu_high_streak_ >= enter_ticks) gaming_latched_ = true;
            } else if (low_load) {
                ++gpu_low_streak_;
                gpu_high_streak_ = 0;
                if (gpu_low_streak_ >= 5U) gaming_latched_ = false;
            }
        } else if (input.has_game_thread || gaming_prof) {
            last_gaming_eval_ms_ = input.now_ms;
            const double enter_cpu = gaming_prof ? 42.0 : 50.0;
            const double exit_cpu = gaming_prof ? 22.0 : 25.0;
            if (input.cpu_util_pct >= enter_cpu) {
                ++gpu_high_streak_;
                gpu_low_streak_ = 0;
                if (gpu_high_streak_ >= enter_ticks) gaming_latched_ = true;
            } else if (input.cpu_util_pct < exit_cpu) {
                ++gpu_low_streak_;
                gpu_high_streak_ = 0;
                if (gpu_low_streak_ >= 5U) gaming_latched_ = false;
            }
        }
    }

    const uint64_t touch_hold_ms =
        eff_profile == PerformanceProfile::Smoothness ? 550ULL :
        eff_profile == PerformanceProfile::Battery ? 250ULL :
        eff_profile == PerformanceProfile::Gaming ? 450ULL : 400ULL;

    if (launch_burst_until_ms_ > input.now_ms) {
        out.launch_burst_remaining_ms = launch_burst_until_ms_ - input.now_ms;
    }
    if (last_touch_ms_ > 0U && input.now_ms >= last_touch_ms_ &&
        input.now_ms - last_touch_ms_ < touch_hold_ms) {
        out.touch_hold_remaining_ms = touch_hold_ms - (input.now_ms - last_touch_ms_);
    }

    if (out.launch_burst_remaining_ms > 0U) {
        out.mode = ContextMode::LaunchBurst;
    } else if (out.app_forced_gaming || gaming_latched_) {
        out.mode = ContextMode::Gaming;
    } else if (out.touch_hold_remaining_ms > 0U || input.cpu_util_pct >= 55.0 ||
               out.camera_boost_active || out.media_pipeline_active) {
        out.mode = ContextMode::Interactive;
    } else {
        out.mode = ContextMode::Efficiency;
    }
    return out;
}

ForegroundAppInfo ContextEngine::resolve_foreground_app(
    const std::string& top_app_procs_path) const noexcept {
    ForegroundAppInfo result{};
    if (top_app_procs_path.empty()) return result;

    const std::string content = read_small_file(top_app_procs_path, 16384U);
    if (content.empty()) return result;

    auto process_has_render_thread = [&](int32_t pid) -> bool {
        if (pid == cached_fg_pid_ && !cached_fg_tids_.empty()) {
            // Fast path: already verified this PID owns a render thread (> 1 critical TID)
            if (cached_fg_tids_.size() > 1U) return true;
        }
        const fs::path task_dir = fs::path(procfs_root_) / std::to_string(pid) / "task";
        std::error_code tec;
        size_t scanned = 0U;
        for (fs::directory_iterator tit(task_dir, fs::directory_options::skip_permission_denied, tec), tend;
             !tec && tit != tend && scanned++ < 192U; tit.increment(tec)) {
            std::string comm = read_small_file(tit->path() / "comm", 64U);
            while (!comm.empty() && (comm.back() == '\n' || comm.back() == '\r')) comm.pop_back();
            if (is_critical_render_thread_name(comm)) return true;
        }
        return false;
    };

    const int32_t self_pid = static_cast<int32_t>(::getpid());
    std::istringstream ss(content);
    std::string line;
    int32_t best_pid = 0;
    std::string best_pkg;
    int best_rank = -1;

    while (std::getline(ss, line)) {
        int32_t pid = 0;
        if (!parse_i32(line, pid) || pid == self_pid) continue;
        const fs::path pdir = fs::path(procfs_root_) / std::to_string(pid);
        if (!has_app_uid_or_test_proc(pdir)) continue;

        const auto cmd = read_cmdline_parsed(pdir);
        if (cmd.name.empty() || !is_android_user_app_package(cmd.name, cmd.has_path_slash)) continue;

        const std::string pkg_lower = to_lower_copy(cmd.name);
        const bool user_configured =
            gaming_apps_.contains(pkg_lower) || smoothness_apps_.contains(pkg_lower) ||
            battery_apps_.contains(pkg_lower) || excluded_apps_.contains(pkg_lower);
        const bool is_bg_service =
            !user_configured && (cmd.has_service_suffix || is_background_system_service(pkg_lower));

        // Android ActivityManager invariant:
        //   adj == 0    -> FOREGROUND_APP_ADJ (visible foreground app OR bound BIND_ABOVE_CLIENT service)
        //   1..100      -> VISIBLE_APP_ADJ (split-screen / PiP / overlay)
        //   >= 200      -> PERCEPTIBLE_APP_ADJ (IME keyboard, background service)
        //   < 0         -> PERSISTENT_PROC_ADJ (system_server, zygote, vendor daemons)
        // Real visible UI apps own a RenderThread/GLThread/UnityMain/GameThread, whereas bound
        // services (e.g. com.xiaomi.xmsf, com.milink.service, com.android.externalstorage) do not.
        const auto adj = read_oom_score_adj(pdir);
        int rank = 0;
        if (adj.has_value()) {
            if (*adj < 0 || *adj >= 200) continue;
            if (*adj == 0) {
                const bool has_rt = !cmd.has_service_suffix && process_has_render_thread(pid);
                if (is_bg_service && !has_rt) continue;
                rank = has_rt ? 6 : (cmd.has_service_suffix ? 3 : 4);
            } else {
                if (is_bg_service) continue;
                rank = cmd.has_service_suffix ? 1 : 2;
            }
        } else {
            const bool has_rt = !cmd.has_service_suffix && process_has_render_thread(pid);
            if (is_bg_service && !has_rt) continue;
            rank = has_rt ? 6 : (cmd.has_service_suffix ? 3 : 4);
        }

        // Tie-breaker: keep currently tracked foreground PID sticky so sibling processes
        // in top-app do not flap LaunchBurst.
        const bool stick_with_current = (best_pid == cached_fg_pid_ && cached_fg_pid_ > 0);
        if (rank > best_rank || (rank == best_rank && (!stick_with_current || pid == cached_fg_pid_))) {
            best_rank = rank;
            best_pid = pid;
            best_pkg = cmd.name;
        }
    }

    if (best_pid <= 0) return result;
    result.pid = best_pid;
    result.package_name = std::move(best_pkg);
    result.is_camera_app = is_camera_package(result.package_name);
    result.has_render_thread = (best_rank == 6);

    const uint64_t now = steady_now_ms();
    if (best_pid == cached_fg_pid_ && now >= cached_fg_tids_ms_ &&
        now - cached_fg_tids_ms_ < 3000ULL && !cached_fg_tids_.empty()) {
        result.critical_tids = cached_fg_tids_;
        result.helper_tids = cached_fg_helper_tids_;
        result.has_game_thread = cached_fg_has_game_thread_;
        result.has_media_thread = cached_fg_has_media_thread_;
        result.engine_class = cached_fg_engine_class_;
        result.has_render_thread = result.has_render_thread || (cached_fg_tids_.size() > 1U);
        return result;
    }

    if (is_cfs_thread(best_pid, procfs_root_)) {
        result.critical_tids.push_back(best_pid); // Main UI thread always has TID == PID
    }

    const fs::path task_dir = fs::path(procfs_root_) / std::to_string(best_pid) / "task";
    std::error_code ec;
    for (fs::directory_iterator it(task_dir, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end && (result.critical_tids.size() + result.helper_tids.size()) < 20U;
         it.increment(ec)) {
        int32_t tid = 0;
        if (!parse_i32(it->path().filename().string(), tid) || tid == best_pid) continue;
        std::string comm = read_small_file(it->path() / "comm", 64U);
        while (!comm.empty() && (comm.back() == '\n' || comm.back() == '\r')) comm.pop_back();
        if (is_game_engine_thread_name(comm)) {
            result.has_game_thread = true;
        }
        if (is_media_pipeline_thread_name(comm)) {
            result.has_media_thread = true;
            if (is_cfs_thread(tid, procfs_root_) && result.helper_tids.size() < 8U) {
                result.helper_tids.push_back(tid);
            }
            continue;
        }
        if (is_helper_pipeline_thread_name(comm)) {
            result.has_render_thread = true;
            if (is_cfs_thread(tid, procfs_root_) && result.helper_tids.size() < 8U) {
                result.helper_tids.push_back(tid);
            }
            continue;
        }
        if (is_critical_render_thread_name(comm)) {
            result.has_render_thread = true;
            if (is_cfs_thread(tid, procfs_root_) && result.critical_tids.size() < 12U) {
                result.critical_tids.push_back(tid);
            }
        }
    }

    // Bounded one-shot runtime engine library signature check (<= 48 KB read, cached per PID)
    // detects Unreal/Unity/Godot binaries even before dedicated render/RHI threads spin up.
    if (!result.has_game_thread) {
        const fs::path maps_path = fs::path(procfs_root_) / std::to_string(best_pid) / "maps";
        const std::string maps_head = read_small_file(maps_path, 49152U);
        if (!maps_head.empty() &&
            (maps_head.find("libUE4.so") != std::string::npos ||
             maps_head.find("libUnreal.so") != std::string::npos ||
             maps_head.find("libunity.so") != std::string::npos ||
             maps_head.find("libil2cpp.so") != std::string::npos ||
             maps_head.find("libgodot") != std::string::npos)) {
            result.has_game_thread = true;
        }
    }
    result.engine_class = result.has_game_thread
        ? AppEngineClass::HeavyGame3D
        : AppEngineClass::StandardUi;

    cached_fg_pid_ = best_pid;
    cached_fg_tids_ms_ = now;
    cached_fg_tids_ = result.critical_tids;
    cached_fg_helper_tids_ = result.helper_tids;
    cached_fg_has_game_thread_ = result.has_game_thread;
    cached_fg_has_media_thread_ = result.has_media_thread;
    cached_fg_engine_class_ = result.engine_class;
    return result;
}

std::vector<int32_t> ContextEngine::resolve_surfaceflinger_tids() noexcept {
    const uint64_t now = steady_now_ms();
    // Refresh SurfaceFlinger TIDs at most every 30 seconds if cached PID is still alive.
    if (cached_sf_pid_ > 0 && now >= cached_sf_checked_ms_ && now - cached_sf_checked_ms_ < 30000ULL) {
        const fs::path sf_dir = fs::path(procfs_root_) / std::to_string(cached_sf_pid_);
        if (fs::is_directory(sf_dir)) return cached_sf_tids_;
    }

    cached_sf_checked_ms_ = now;
    cached_sf_pid_ = 0;
    cached_sf_tids_.clear();

    std::error_code ec;
    for (fs::directory_iterator it(procfs_root_, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end; it.increment(ec)) {
        int32_t pid = 0;
        if (!parse_i32(it->path().filename().string(), pid)) continue;
        const auto cmd = read_cmdline_parsed(it->path());
        if (cmd.name != "surfaceflinger") continue;
        cached_sf_pid_ = pid;
        if (is_cfs_thread(pid, procfs_root_)) {
            cached_sf_tids_.push_back(pid);
        }
        const fs::path task_dir = it->path() / "task";
        std::error_code tec;
        for (fs::directory_iterator tit(task_dir, fs::directory_options::skip_permission_denied, tec), tend;
             !tec && tit != tend && cached_sf_tids_.size() < 8U; tit.increment(tec)) {
            int32_t tid = 0;
            if (!parse_i32(tit->path().filename().string(), tid) || tid == pid) continue;
            std::string comm = read_small_file(tit->path() / "comm", 64U);
            while (!comm.empty() && (comm.back() == '\n' || comm.back() == '\r')) comm.pop_back();
            if ((comm == "RenderEngine" || comm == "surfaceflinger" || comm.rfind("app", 0) == 0) &&
                is_cfs_thread(tid, procfs_root_)) {
                cached_sf_tids_.push_back(tid);
            }
        }
        break;
    }
    return cached_sf_tids_;
}

std::vector<int32_t> ContextEngine::resolve_camera_hal_tids() noexcept {
    const uint64_t now = steady_now_ms();
    if (!cached_cam_tids_.empty() && now >= cached_cam_checked_ms_ &&
        now - cached_cam_checked_ms_ < 15000ULL) {
        return cached_cam_tids_;
    }

    cached_cam_checked_ms_ = now;
    cached_cam_tids_.clear();

    std::error_code ec;
    for (fs::directory_iterator it(procfs_root_, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end && cached_cam_tids_.size() < 12U; it.increment(ec)) {
        int32_t pid = 0;
        if (!parse_i32(it->path().filename().string(), pid)) continue;
        const auto cmd = read_cmdline_parsed(it->path());
        const std::string lower = to_lower_copy(cmd.name);
        const bool is_cam_daemon =
            lower == "cameraserver" ||
            lower.find("camera.provider") != std::string::npos ||
            lower.find("camerahalserver") != std::string::npos ||
            lower.find("miuicamera") != std::string::npos;
        if (!is_cam_daemon) continue;

        if (is_cfs_thread(pid, procfs_root_)) {
            cached_cam_tids_.push_back(pid);
        }
        const fs::path task_dir = it->path() / "task";
        std::error_code tec;
        size_t scanned = 0U;
        for (fs::directory_iterator tit(task_dir, fs::directory_options::skip_permission_denied, tec), tend;
             !tec && tit != tend && cached_cam_tids_.size() < 12U && scanned++ < 64U; tit.increment(tec)) {
            int32_t tid = 0;
            if (!parse_i32(tit->path().filename().string(), tid) || tid == pid) continue;
            std::string comm = read_small_file(tit->path() / "comm", 64U);
            while (!comm.empty() && (comm.back() == '\n' || comm.back() == '\r')) comm.pop_back();
            if ((comm.rfind("Binder:", 0) == 0 || comm.rfind("HwBinder:", 0) == 0 ||
                 comm.find("Cam") != std::string::npos || comm.find("ISP") != std::string::npos ||
                 comm.find("HDR") != std::string::npos) &&
                is_cfs_thread(tid, procfs_root_)) {
                cached_cam_tids_.push_back(tid);
            }
        }
    }
    return cached_cam_tids_;
}

} // namespace coreflow::v5
