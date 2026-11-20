/**
 * @file context_engine.cpp
 * @brief CoreFlow v5 Pillar 2: 5-Mode Context Classifier, Foreground App & Critical Thread Resolver.
 * @license MIT
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
    // Never clamp down RT (SCHED_FIFO / SCHED_RR) threads like surfaceflinger main thread.
    return base_policy != SCHED_FIFO && base_policy != SCHED_RR;
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
    const std::vector<std::string>& excluded_apps) {
    gaming_apps_.clear();
    excluded_apps_.clear();
    for (const auto& pkg : gaming_apps) {
        if (!pkg.empty()) gaming_apps_.insert(to_lower_copy(pkg));
    }
    for (const auto& pkg : excluded_apps) {
        if (!pkg.empty()) excluded_apps_.insert(to_lower_copy(pkg));
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

bool ContextEngine::is_critical_render_thread_name(std::string_view comm) noexcept {
    if (comm.empty()) return false;
    // Android UI / HWUI / Compositor & major mobile game engine main/render threads
    return comm == "RenderThread" ||
           comm == "surfaceflinger" ||
           comm == "RenderEngine" ||
           comm == "Binder:sf" ||
           comm.rfind("hwuiTask", 0) == 0 ||
           comm.rfind("RenderThread", 0) == 0 ||
           comm == "UnityMain" ||
           comm.rfind("UnityGfxDevice", 0) == 0 ||
           comm == "GameThread" ||
           comm == "RHIThread" ||
           comm == "RenderThread 0" ||
           comm == "GLThread" ||
           comm.rfind("GLThread", 0) == 0 ||
           comm == "VulkanThread" ||
           comm == "mali-cmar-backe" ||
           comm == "adreno-worker";
}

ContextDecision ContextEngine::evaluate(const ContextSignalInput& input) noexcept {
    ContextDecision out{};
    out.foreground_pid = input.foreground_pid;
    out.foreground_package = input.foreground_package;

    const std::string pkg_lower = to_lower_copy(input.foreground_package);
    out.app_excluded = !pkg_lower.empty() && excluded_apps_.contains(pkg_lower);
    out.app_forced_gaming = !pkg_lower.empty() && gaming_apps_.contains(pkg_lower);

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
        if (!input.screen_off && !out.app_excluded) {
            const uint64_t burst_ms = profile_ == PerformanceProfile::Battery ? 1200ULL :
                profile_ == PerformanceProfile::Smoothness ? 2000ULL : 1600ULL;
            launch_burst_until_ms_ = input.now_ms + burst_ms;
            out.newly_launched_app = true;
        }
    }

    if (input.screen_off) {
        gaming_latched_ = false;
        gpu_high_streak_ = 0;
        gpu_low_streak_ = 0;
        out.mode = ContextMode::ScreenOff;
        return out;
    }

    if (out.app_excluded) {
        gaming_latched_ = false;
        out.mode = ContextMode::Excluded;
        return out;
    }

    // Hysteretic GPU-driven Gaming detection (3 ticks >= 48% to enter, 5 ticks < 25% to exit)
    if (input.gpu_busy_pct.has_value()) {
        const double gpu = *input.gpu_busy_pct;
        if (gpu >= 48.0 || (gpu >= 35.0 && input.cpu_util_pct >= 55.0)) {
            ++gpu_high_streak_;
            gpu_low_streak_ = 0;
            if (gpu_high_streak_ >= 3U) gaming_latched_ = true;
        } else if (gpu < 25.0) {
            ++gpu_low_streak_;
            gpu_high_streak_ = 0;
            if (gpu_low_streak_ >= 5U) gaming_latched_ = false;
        }
    }

    const uint64_t touch_hold_ms =
        profile_ == PerformanceProfile::Smoothness ? 550ULL :
        profile_ == PerformanceProfile::Battery ? 250ULL :
        profile_ == PerformanceProfile::Gaming ? 450ULL : 400ULL;

    if (launch_burst_until_ms_ > input.now_ms) {
        out.launch_burst_remaining_ms = launch_burst_until_ms_ - input.now_ms;
    }
    if (last_touch_ms_ > 0U && input.now_ms >= last_touch_ms_ &&
        input.now_ms - last_touch_ms_ < touch_hold_ms) {
        out.touch_hold_remaining_ms = touch_hold_ms - (input.now_ms - last_touch_ms_);
    }

    if (out.launch_burst_remaining_ms > 0U) {
        out.mode = ContextMode::LaunchBurst;
    } else if (out.app_forced_gaming || gaming_latched_ ||
               (profile_ == PerformanceProfile::Gaming && input.cpu_util_pct >= 30.0)) {
        out.mode = ContextMode::Gaming;
    } else if (out.touch_hold_remaining_ms > 0U || input.cpu_util_pct >= 55.0) {
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

        // Android ActivityManager invariant:
        //   adj == 0    -> FOREGROUND_APP_ADJ (true active visible foreground app)
        //   1..100      -> VISIBLE_APP_ADJ (split-screen / PiP / overlay)
        //   >= 200      -> PERCEPTIBLE_APP_ADJ (IME keyboard like com.preff.kb.xm, foreground service)
        //   < 0         -> PERSISTENT_PROC_ADJ (system_server, zygote, vendor QCC daemons)
        const auto adj = read_oom_score_adj(pdir);
        int rank = 0;
        if (adj.has_value()) {
            if (*adj < 0 || *adj >= 200) continue;
            if (*adj == 0) {
                rank = cmd.has_service_suffix ? 3 : 4;
            } else {
                rank = cmd.has_service_suffix ? 1 : 2;
            }
        } else {
            // Host unit-test fixtures without oom_score_adj default to foreground rank.
            rank = cmd.has_service_suffix ? 3 : 4;
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

    const uint64_t now = steady_now_ms();
    if (best_pid == cached_fg_pid_ && now >= cached_fg_tids_ms_ &&
        now - cached_fg_tids_ms_ < 3000ULL && !cached_fg_tids_.empty()) {
        result.critical_tids = cached_fg_tids_;
        return result;
    }

    if (is_cfs_thread(best_pid, procfs_root_)) {
        result.critical_tids.push_back(best_pid); // Main UI thread always has TID == PID
    }

    const fs::path task_dir = fs::path(procfs_root_) / std::to_string(best_pid) / "task";
    std::error_code ec;
    for (fs::directory_iterator it(task_dir, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end && result.critical_tids.size() < 16U; it.increment(ec)) {
        int32_t tid = 0;
        if (!parse_i32(it->path().filename().string(), tid) || tid == best_pid) continue;
        std::string comm = read_small_file(it->path() / "comm", 64U);
        while (!comm.empty() && (comm.back() == '\n' || comm.back() == '\r')) comm.pop_back();
        if (is_critical_render_thread_name(comm) && is_cfs_thread(tid, procfs_root_)) {
            result.critical_tids.push_back(tid);
        }
    }
    cached_fg_pid_ = best_pid;
    cached_fg_tids_ms_ = now;
    cached_fg_tids_ = result.critical_tids;
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

} // namespace coreflow::v5
