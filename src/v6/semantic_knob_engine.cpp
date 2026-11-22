/**
 * @file semantic_knob_engine.cpp
 * @brief CoreFlow v6.1 Dynamic Subsystem Scanner + Semantic Knob Classifier & Demand-Driven Actuator.
 * @license Apache-2.0
 */
#include "coreflow/v6/semantic_knob_engine.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <fcntl.h>
#include <filesystem>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow::v6 {
namespace fs = std::filesystem;

namespace {

std::string to_lower_copy(std::string_view s) {
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

std::string read_line_trimmed(const std::string& path) {
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return {};
    struct stat st{};
    if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        (void)::close(fd);
        return {};
    }
    char buf[256];
    const ssize_t n = ::read(fd, buf, sizeof(buf));
    (void)::close(fd);
    if (n <= 0) return {};
    std::string out(buf, static_cast<size_t>(n));
    const auto nl = out.find('\n');
    if (nl != std::string::npos) out.resize(nl);
    while (!out.empty() && (out.back() == '\r' || out.back() == ' ' || out.back() == '\t')) out.pop_back();
    size_t start = 0;
    while (start < out.size() && (out[start] == ' ' || out[start] == '\t')) ++start;
    return out.substr(start);
}

bool is_regular_writable(const fs::path& p) noexcept {
    struct stat st{};
    return ::lstat(p.c_str(), &st) == 0 && S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode) &&
           ::access(p.c_str(), R_OK | W_OK) == 0;
}

bool parse_i64_exact(std::string_view text, int64_t& out) noexcept {
    if (text.empty() || text.size() > 24U) return false;
    const auto r = std::from_chars(text.data(), text.data() + text.size(), out);
    return r.ec == std::errc{} && r.ptr == text.data() + text.size();
}

bool parse_bool_token(std::string_view raw, bool& out_val, bool& uses_yn) noexcept {
    if (raw == "0") { out_val = false; uses_yn = false; return true; }
    if (raw == "1") { out_val = true; uses_yn = false; return true; }
    if (raw == "N" || raw == "n") { out_val = false; uses_yn = true; return true; }
    if (raw == "Y" || raw == "y") { out_val = true; uses_yn = true; return true; }
    return false;
}

std::string format_bool_token(bool val, bool uses_yn, const std::string& baseline) {
    if (!uses_yn) return val ? "1" : "0";
    const bool upper = !baseline.empty() && (baseline[0] == 'Y' || baseline[0] == 'N');
    if (upper) return val ? "Y" : "N";
    return val ? "y" : "n";
}

bool parse_int_list(const std::string& raw, std::vector<int64_t>& out) {
    out.clear();
    std::istringstream ss(raw);
    std::string tok;
    while (ss >> tok && out.size() < 8U) {
        int64_t v = 0;
        if (!parse_i64_exact(tok, v)) return false;
        out.push_back(v);
    }
    return !out.empty();
}

std::string scale_migrate_values(const std::string& baseline, int64_t delta_pct, int64_t lo, int64_t hi) {
    std::vector<int64_t> vals;
    if (!parse_int_list(baseline, vals)) return {};
    std::string out;
    for (size_t i = 0; i < vals.size(); ++i) {
        if (i > 0U) out.push_back(' ');
        const int64_t adjusted = std::clamp<int64_t>(
            std::llround(static_cast<double>(vals[i]) * (1.0 + static_cast<double>(delta_pct) / 100.0)),
            lo, hi);
        out += std::to_string(adjusted);
    }
    return out;
}

uint64_t select_opp_by_fraction(const std::vector<uint64_t>& opps, double frac) noexcept {
    if (opps.empty()) return 0U;
    const double clamped = std::clamp(frac, 0.0, 1.0);
    const size_t idx = static_cast<size_t>(
        std::llround(static_cast<double>(opps.size() - 1U) * clamped));
    return opps[std::min(idx, opps.size() - 1U)];
}

} // namespace

const char* SemanticKnobEngine::family_name(KnobFamily family) noexcept {
    switch (family) {
        case KnobFamily::CpuBoost: return "cpu_boost";
        case KnobFamily::GpuLatency: return "gpu_latency";
        case KnobFamily::StorageLatency: return "storage_latency";
        case KnobFamily::MemoryCache: return "memory_cache";
        default: return "unknown";
    }
}

bool SemanticKnobEngine::is_forbidden_knob_name(std::string_view leaf) noexcept {
    const std::string lower = to_lower_copy(leaf);
    constexpr std::string_view kForbidden[] = {
        "voltage", "volt", "regulator", "thermal", "trip", "throttl",
        "suspend", "reset", "panic", "reboot", "fuse", "clock", "pll",
        "freq_limit", "scaling_max", "scaling_min", "force_clk", "force_rail",
        "force_bus", "sched_boost", "debug", "trace", "dump", "power_down", "offline",
        "hotplug", "kill", "uevent", "modalias", "dev", "subsystem",
    };
    for (const auto bad : kForbidden) {
        if (lower.find(bad) != std::string::npos) return true;
    }
    return false;
}

SemanticKnobEngine::SemanticKnobEngine(
    BaselineStore& baseline,
    OemCollaborationMap& oem_map) noexcept
    : baseline_(baseline), oem_map_(oem_map) {}

void SemanticKnobEngine::try_register_node(
    const std::string& path,
    const std::string& leaf,
    const std::vector<uint64_t>& cluster_opps,
    std::unordered_map<std::string, bool>& seen) noexcept {
    if (path.empty() || leaf.empty() || seen.contains(path)) return;
    if (is_forbidden_knob_name(leaf)) return;
    if (!is_regular_writable(path)) return;

    const std::string base = read_line_trimmed(path);
    if (base.empty()) return;

    const std::string lower = to_lower_copy(leaf);
    DiscoveredKnob knob{};
    knob.path = path;
    knob.leaf = leaf;
    knob.baseline_value = base;

    bool bval = false;
    bool uses_yn = false;
    int64_t ival = 0;

    // 1. CPU / WALT / EAS Boost & Latency Family
    if (lower == "boost") {
        // Silicon physics: per-cluster walt/boost pins cluster to hispeed even at low load.
        // Strictly limit to short transient LaunchBurst (1.2-2.4s) when OEM baseline is 0 —
        // NEVER touch Qualcomm's refcounted /proc/sys/walt/sched_boost or hold during Gaming.
        if (!parse_bool_token(base, bval, uses_yn) || bval) return;
        knob.family = KnobFamily::CpuBoost;
        knob.kind = KnobValueKind::BoolToggle;
        knob.uses_yn_bool = uses_yn;
        knob.active_burst_value = 1;
        knob.apply_in_launch_burst = true;
    } else if (lower == "pl") {
        // Qualcomm WALT Predictive Load (0/1) — workload-proportional, safe for active modes.
        if (!parse_bool_token(base, bval, uses_yn)) return;
        knob.family = KnobFamily::CpuBoost;
        knob.kind = KnobValueKind::BoolToggle;
        knob.uses_yn_bool = uses_yn;
        knob.active_burst_value = 1;
        knob.active_gaming_value = 1;
        knob.active_interactive_value = 1;
        knob.apply_in_launch_burst = true;
        knob.apply_in_gaming = true;
        knob.apply_in_interactive = true;
    } else if (lower == "sched_conservative_pl") {
        if (!parse_bool_token(base, bval, uses_yn)) return;
        knob.family = KnobFamily::CpuBoost;
        knob.kind = KnobValueKind::BoolToggle;
        knob.uses_yn_bool = uses_yn;
        knob.active_burst_value = 0;
        knob.active_gaming_value = 0;
        knob.apply_in_launch_burst = true;
        knob.apply_in_gaming = true;
    } else if (lower == "rtg_boost_freq" && !cluster_opps.empty()) {
        // Respect OEM disabled rtg_boost_freq == 0 (e.g. Big cluster intentionally unboosted).
        // Only apply gentle OEM-relative uplift during LaunchBurst/Gaming on OEM-enabled clusters.
        if (!parse_i64_exact(base, ival) || ival <= 0) return;
        knob.family = KnobFamily::CpuBoost;
        knob.kind = KnobValueKind::OppFreq;
        knob.baseline_int = ival;
        knob.opp_table_khz = cluster_opps;
        knob.apply_in_launch_burst = true;
        knob.apply_in_gaming = true;
    } else if (lower == "sched_upmigrate" || lower == "sched_downmigrate") {
        std::vector<int64_t> nums;
        if (!parse_int_list(base, nums)) return;
        for (int64_t v : nums) {
            if (v < 20 || v > 100) return;
        }
        knob.family = KnobFamily::CpuBoost;
        knob.kind = KnobValueKind::MigratePair;
        // Maintain WALT's strict sched_downmigrate <= sched_upmigrate kernel invariant:
        // downmigrate lowers slightly more than upmigrate, and rollback restores upmigrate first.
        const bool is_down = (lower == "sched_downmigrate");
        knob.active_burst_value = is_down ? -10 : -8;
        knob.active_gaming_value = is_down ? -8 : -6;
        knob.apply_in_launch_burst = true;
        knob.apply_in_gaming = true;
    } else if (lower == "sched_min_task_util_for_boost" ||
               lower == "sched_min_task_util_for_colocation") {
        if (!parse_i64_exact(base, ival) || ival <= 0 || ival > 1000) return;
        knob.family = KnobFamily::CpuBoost;
        knob.kind = KnobValueKind::RelativeInt;
        knob.baseline_int = ival;
        knob.active_burst_value = std::max<int64_t>(10, (ival * 85) / 100);
        knob.active_gaming_value = std::max<int64_t>(10, (ival * 88) / 100);
        knob.apply_in_launch_burst = true;
        knob.apply_in_gaming = true;
    } else if (lower == "context_burst_count") {
        if (!parse_i64_exact(base, ival) || ival < 0 || ival > 64) return;
        knob.family = KnobFamily::CpuBoost;
        knob.kind = KnobValueKind::RelativeInt;
        knob.baseline_int = ival;
        knob.active_burst_value = std::clamp<int64_t>(ival + 2, 2, 16);
        knob.active_gaming_value = std::clamp<int64_t>(ival + 2, 2, 16);
        knob.apply_in_launch_burst = true;
        knob.apply_in_gaming = true;
    }
    // 2. GPU / Adreno / MediaTek GED Latency Family
    else if (lower == "adrenoboost") {
        if (!parse_i64_exact(base, ival) || ival < 0 || ival > 3) return;
        knob.family = KnobFamily::GpuLatency;
        knob.kind = KnobValueKind::IntLevel;
        knob.baseline_int = ival;
        knob.active_burst_value = 1;
        knob.active_gaming_value = 2;
        knob.active_interactive_value = 1;
        knob.apply_in_launch_burst = true;
        knob.apply_in_gaming = true;
        knob.apply_in_interactive = true;
    } else if (lower == "bus_split") {
        if (!parse_bool_token(base, bval, uses_yn)) return;
        knob.family = KnobFamily::GpuLatency;
        knob.kind = KnobValueKind::BoolToggle;
        knob.uses_yn_bool = uses_yn;
        knob.active_gaming_value = 0; // Unified bus throughput during heavy gaming
        knob.apply_in_gaming = true;
    } else if (lower == "gx_game_mode" || lower == "gx_boost_on" || lower == "gpu_boost" ||
               lower == "ged_boost_enable" || lower == "boost_gpu_enable" || lower == "boost_extra") {
        if (!parse_bool_token(base, bval, uses_yn)) return;
        knob.family = KnobFamily::GpuLatency;
        knob.kind = KnobValueKind::BoolToggle;
        knob.uses_yn_bool = uses_yn;
        knob.active_burst_value = 1;
        knob.active_gaming_value = 1;
        knob.apply_in_launch_burst = true;
        knob.apply_in_gaming = true;
    } else if (lower == "gpu_boost_level") {
        // MediaTek GED gpu_boost_level (0..100 integer hint, OEM-relative gentle uplift in Gaming/Burst)
        if (!parse_i64_exact(base, ival) || ival < 0 || ival > 100) return;
        knob.family = KnobFamily::GpuLatency;
        knob.kind = KnobValueKind::RelativeInt;
        knob.baseline_int = ival;
        knob.active_burst_value = std::min<int64_t>(60, std::max<int64_t>(ival + 15, 25));
        knob.active_gaming_value = std::min<int64_t>(75, std::max<int64_t>(ival + 25, 40));
        knob.apply_in_launch_burst = true;
        knob.apply_in_gaming = true;
    } else if (lower == "boost_affinity" || lower == "fpsgo_enable") {
        // MediaTek FPSGO safe affinity hint (0/1); only enable in Gaming/Burst if OEM left at 0
        if (!parse_bool_token(base, bval, uses_yn) || bval) return;
        knob.family = KnobFamily::CpuBoost;
        knob.kind = KnobValueKind::BoolToggle;
        knob.uses_yn_bool = uses_yn;
        knob.active_burst_value = 1;
        knob.active_gaming_value = 1;
        knob.apply_in_launch_burst = true;
        knob.apply_in_gaming = true;
    }
    // 3. UFS / Block Queue Latency Family
    else if (lower == "inflight_low_latency") {
        // Zero idle power penalty: only affects in-flight UFS commands.
        if (!parse_bool_token(base, bval, uses_yn)) return;
        knob.family = KnobFamily::StorageLatency;
        knob.kind = KnobValueKind::BoolToggle;
        knob.uses_yn_bool = uses_yn;
        knob.active_burst_value = 1;
        knob.active_gaming_value = 1;
        knob.active_interactive_value = 1;
        knob.apply_in_launch_burst = true;
        knob.apply_in_gaming = true;
        knob.apply_in_interactive = true;
    } else if (lower == "iostats") {
        if (!parse_bool_token(base, bval, uses_yn)) return;
        knob.family = KnobFamily::StorageLatency;
        knob.kind = KnobValueKind::BoolToggle;
        knob.uses_yn_bool = uses_yn;
        knob.active_burst_value = 0;
        knob.active_gaming_value = 0;
        knob.apply_in_launch_burst = true;
        knob.apply_in_gaming = true;
    } else if (lower == "rq_affinity") {
        if (!parse_i64_exact(base, ival) || ival < 0 || ival > 2) return;
        knob.family = KnobFamily::StorageLatency;
        knob.kind = KnobValueKind::IntLevel;
        knob.baseline_int = ival;
        knob.active_burst_value = 2;
        knob.active_gaming_value = 2;
        knob.apply_in_launch_burst = true;
        knob.apply_in_gaming = true;
    } else if (lower == "clkgate_enable" || lower == "hibern8_on_idle_enable") {
        // UFS PHY physics: keeping link awake saves wake latency ONLY during short LaunchBurst
        // sequential reads; never hold disabled during sustained Gaming when flash is 99% idle.
        if (!parse_bool_token(base, bval, uses_yn)) return;
        knob.family = KnobFamily::StorageLatency;
        knob.kind = KnobValueKind::BoolToggle;
        knob.uses_yn_bool = uses_yn;
        knob.active_burst_value = 0;
        knob.apply_in_launch_burst = true;
    }
    // 4. Memory Cache Preservation Family
    else if (lower == "vfs_cache_pressure") {
        if (!parse_i64_exact(base, ival) || ival < 20 || ival > 300) return;
        knob.family = KnobFamily::MemoryCache;
        knob.kind = KnobValueKind::RelativeInt;
        knob.baseline_int = ival;
        knob.active_burst_value = std::max<int64_t>(40, (ival * 80) / 100);
        knob.active_gaming_value = std::max<int64_t>(40, (ival * 80) / 100);
        knob.active_interactive_value = std::max<int64_t>(45, (ival * 85) / 100);
        knob.apply_in_launch_burst = true;
        knob.apply_in_gaming = true;
        knob.apply_in_interactive = true;
    } else {
        return;
    }

    seen[path] = true;
    knobs_.push_back(std::move(knob));
}

void SemanticKnobEngine::discover(
    const v5::HardwareTopology& topo,
    const std::string& sysfs_root,
    const std::string& procfs_root) noexcept {
    rollback_all();
    knobs_.clear();
    stats_ = {};

    std::unordered_map<std::string, bool> seen;
    auto scan_dir = [&](const fs::path& dir, const std::vector<uint64_t>& opps) {
        std::error_code ec;
        if (!fs::is_directory(dir, ec) || ec) return;
        size_t scanned = 0U;
        for (fs::directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec), end;
             !ec && it != end && scanned++ < 256U; it.increment(ec)) {
            std::error_code iec;
            if (!it->is_regular_file(iec) || iec) continue;
            try_register_node(it->path().string(), it->path().filename().string(), opps, seen);
        }
    };

    // 1. CPU governor & scheduler directories (Qualcomm WALT, EAS, MediaTek FPSGO)
    for (const auto& cluster : topo.cpu_clusters) {
        if (cluster.policy_path.empty()) continue;
        const fs::path pdir(cluster.policy_path);
        if (!cluster.governor.empty()) {
            scan_dir(pdir / cluster.governor, cluster.opps_khz);
        }
        scan_dir(pdir, cluster.opps_khz);
    }
    scan_dir(fs::path(procfs_root) / "sys/walt", {});
    scan_dir(fs::path(procfs_root) / "sys/kernel", {});
    scan_dir(fs::path(sysfs_root) / "module/mtk_fpsgo/parameters", {});
    scan_dir(fs::path(sysfs_root) / "kernel/fpsgo/fbt", {});

    // 2. GPU device & MediaTek GED / Mali / Exynos parameters
    if (topo.gpu.available && !topo.gpu.device_path.empty()) {
        scan_dir(fs::path(topo.gpu.device_path), {});
    }
    scan_dir(fs::path(sysfs_root) / "module/ged/parameters", {});
    scan_dir(fs::path(sysfs_root) / "kernel/ged/hal", {});

    // 3. Physical storage queues & parent UFS host controller directories (Qualcomm/MediaTek/Exynos/Tensor)
    for (const auto& q : topo.storage_queues) {
        if (q.is_mapper || q.read_ahead_kb_path.empty()) continue;
        const fs::path qdir = fs::path(q.read_ahead_kb_path).parent_path();
        scan_dir(qdir, {});
        // Walk up to 5 ancestors to find UFS host controller latency knobs (e.g. ufshc/inflight_low_latency)
        fs::path anc = qdir.parent_path();
        for (int depth = 0; depth < 5 && !anc.empty() && anc != anc.root_path(); ++depth) {
            const std::string name = to_lower_copy(anc.filename().string());
            if (name.find("ufshc") != std::string::npos || name.find("ufs") != std::string::npos) {
                scan_dir(anc, {});
                break;
            }
            anc = anc.parent_path();
        }
    }

    // 4. VM cache tuning
    if (!topo.vm.vfs_cache_pressure_path.empty()) {
        try_register_node(topo.vm.vfs_cache_pressure_path, "vfs_cache_pressure", {}, seen);
    }

    // Sorted ascending: sched_downmigrate precedes sched_upmigrate during activation (lowering).
    std::sort(knobs_.begin(), knobs_.end(), [](const DiscoveredKnob& a, const DiscoveredKnob& b) {
        return a.path < b.path;
    });

    stats_.discovered_total = static_cast<uint32_t>(knobs_.size());
    for (const auto& k : knobs_) {
        switch (k.family) {
            case KnobFamily::CpuBoost: ++stats_.cpu_knobs; break;
            case KnobFamily::GpuLatency: ++stats_.gpu_knobs; break;
            case KnobFamily::StorageLatency: ++stats_.storage_knobs; break;
            case KnobFamily::MemoryCache: ++stats_.memory_knobs; break;
        }
    }
}

bool SemanticKnobEngine::write_knob(
    const DiscoveredKnob& knob,
    const std::string& target,
    uint64_t now_ms) noexcept {
    if (knob.path.empty() || target.empty()) return false;
    if (oem_map_.is_blocked(knob.path, now_ms)) return false;

    const std::string live = read_line_trimmed(knob.path);
    if (live.empty()) return false;

    if (!oem_map_.check_cooldown_expired(knob.path, live, now_ms)) {
        return false;
    }

    const auto prev = last_written_.find(knob.path);
    if (prev != last_written_.end() && live != prev->second) {
        (void)baseline_.rollback(knob.path);
        oem_map_.record_contention(knob.path, live, now_ms);
        last_written_.erase(knob.path);
        return false;
    }

    // If the kernel already holds the target value and we never mutated this node,
    // leave it completely untouched so stats stay honest and OEM changes never false-contend.
    if (live == target) {
        return true;
    }

    if (!baseline_.capture(knob.path)) return false;
    if (!baseline_.apply_mutation(knob.path, target)) return false;
    const std::string verify = read_line_trimmed(knob.path);
    if (verify != target) {
        (void)baseline_.rollback(knob.path);
        oem_map_.record_contention(knob.path, verify, now_ms);
        last_written_.erase(knob.path);
        return false;
    }
    last_written_[knob.path] = verify;
    return true;
}

void SemanticKnobEngine::rollback_knob(const std::string& path) noexcept {
    if (!last_written_.contains(path)) return;
    (void)baseline_.rollback(path);
    last_written_.erase(path);
}

void SemanticKnobEngine::rollback_all() noexcept {
    // Deterministic reverse-lexicographical rollback ensures sched_upmigrate restores UP
    // BEFORE sched_downmigrate restores UP (preserving WALT's down <= up invariant).
    std::vector<std::string> paths;
    paths.reserve(last_written_.size());
    for (const auto& [path, _] : last_written_) {
        paths.push_back(path);
    }
    std::sort(paths.begin(), paths.end(), std::greater<std::string>());
    for (const auto& path : paths) {
        (void)baseline_.rollback(path);
    }
    last_written_.clear();
    stats_.active_managed = 0U;
}

void SemanticKnobEngine::apply(
    v5::ContextMode mode,
    v5::PerformanceProfile profile,
    double capacity_scale,
    bool thermal_guard,
    bool enabled,
    double avail_ram_ratio,
    double mem_psi,
    uint64_t now_ms,
    bool passive_media_only,
    const SymbioticHints& hints) noexcept {
    const double scale = std::isfinite(capacity_scale) ? std::clamp(capacity_scale, 0.25, 1.0) : 1.0;

    // Release-on-Calm & Thermal Safety & Passive Video Playback:
    // Release ALL semantic boost/latency knobs cleanly to OEM baseline when calm, screen off,
    // passive hands-off video playback, excluded, thermally guarded, or PID thermal pacing active.
    const bool release_all =
        !enabled ||
        thermal_guard ||
        scale < 0.90 ||
        passive_media_only ||
        mode == v5::ContextMode::Excluded ||
        mode == v5::ContextMode::ScreenOff ||
        mode == v5::ContextMode::Efficiency;
    if (release_all) {
        if (!last_written_.empty()) rollback_all();
        return;
    }

    const bool is_burst = (mode == v5::ContextMode::LaunchBurst);
    const bool is_gaming = (mode == v5::ContextMode::Gaming);
    const bool is_interactive = (mode == v5::ContextMode::Interactive);
    const bool battery_profile = (profile == v5::PerformanceProfile::Battery);

    // Phase 1: Roll back any knobs that should no longer be active (reverse order preserves up >= down).
    std::vector<size_t> to_apply;
    to_apply.reserve(knobs_.size());
    for (size_t idx = knobs_.size(); idx > 0U; --idx) {
        const auto& knob = knobs_[idx - 1U];
        bool should_apply =
            (is_burst && knob.apply_in_launch_burst) ||
            (is_gaming && knob.apply_in_gaming) ||
            (is_interactive && knob.apply_in_interactive);

        // Symbiotic GPU/Memory co-pacing: allow GPU latency & bus supply knobs during GPU-bound Interactive
        if (!should_apply && is_interactive && hints.boost_gpu_memory_supply &&
            knob.family == KnobFamily::GpuLatency && knob.apply_in_gaming) {
            should_apply = true;
        }
        // Symbiotic Cluster Spread: allow WALT migration pair knobs to spread single-core saturation
        if (!should_apply && is_interactive && hints.encourage_cluster_spread &&
            knob.kind == KnobValueKind::MigratePair && knob.apply_in_gaming) {
            should_apply = true;
        }

        if (battery_profile && !is_burst &&
            (knob.family == KnobFamily::CpuBoost || knob.family == KnobFamily::GpuLatency)) {
            should_apply = false;
        }
        if (knob.family == KnobFamily::MemoryCache && (avail_ram_ratio < 0.30 || mem_psi >= 8.0)) {
            should_apply = false;
        }

        if (!should_apply) {
            rollback_knob(knob.path);
        } else {
            to_apply.push_back(idx - 1U);
        }
    }

    // Phase 2: Apply active knobs in ascending order (downmigrate lowers before upmigrate).
    std::reverse(to_apply.begin(), to_apply.end());
    for (size_t idx : to_apply) {
        const auto& knob = knobs_[idx];
        std::string target;
        switch (knob.kind) {
            case KnobValueKind::BoolToggle: {
                const bool gpu_mem_boost = hints.boost_gpu_memory_supply &&
                    (knob.family == KnobFamily::GpuLatency || knob.family == KnobFamily::MemoryCache);
                const int64_t v = is_burst ? knob.active_burst_value :
                                  (is_gaming || gpu_mem_boost) ? knob.active_gaming_value :
                                              knob.active_interactive_value;
                target = format_bool_token(v != 0, knob.uses_yn_bool, knob.baseline_value);
                break;
            }
            case KnobValueKind::IntLevel:
            case KnobValueKind::RelativeInt: {
                const bool gpu_mem_boost = hints.boost_gpu_memory_supply &&
                    (knob.family == KnobFamily::GpuLatency || knob.family == KnobFamily::MemoryCache);
                const int64_t v = is_burst ? knob.active_burst_value :
                                  (is_gaming || gpu_mem_boost) ? knob.active_gaming_value :
                                              knob.active_interactive_value;
                target = std::to_string(v);
                break;
            }
            case KnobValueKind::MigratePair: {
                const int64_t base_delta = is_burst ? knob.active_burst_value : knob.active_gaming_value;
                const int64_t delta = hints.encourage_cluster_spread ? (base_delta - 4) : base_delta;
                target = scale_migrate_values(knob.baseline_value, delta, 55, 98);
                break;
            }
            case KnobValueKind::OppFreq: {
                const double frac = is_burst ? 0.60 : 0.55;
                const uint64_t opp_cap = select_opp_by_fraction(knob.opp_table_khz, frac);
                const double rel_mult = is_burst ? 1.30 : 1.20;
                const uint64_t rel_cap = knob.baseline_int > 0
                    ? static_cast<uint64_t>(std::llround(static_cast<double>(knob.baseline_int) * rel_mult))
                    : 0U;
                const uint64_t ceiling = (rel_cap > 0U && opp_cap > 0U) ? std::min(opp_cap, rel_cap) : opp_cap;
                uint64_t chosen = 0U;
                for (uint64_t freq : knob.opp_table_khz) {
                    if (freq <= ceiling && static_cast<int64_t>(freq) > knob.baseline_int) {
                        chosen = freq;
                    }
                }
                if (chosen > 0U) {
                    target = std::to_string(chosen);
                } else {
                    rollback_knob(knob.path);
                    continue;
                }
                break;
            }
        }

        if (!target.empty()) {
            (void)write_knob(knob, target, now_ms);
        }
    }

    stats_.active_managed = static_cast<uint32_t>(last_written_.size());
}

} // namespace coreflow::v6
