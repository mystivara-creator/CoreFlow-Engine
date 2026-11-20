/**
 * @file storage_memory_actuator.cpp
 * @brief CoreFlow v5 Pillar 3B: Zero-Fixed-Value Dynamic UFS/eMMC Burst, Background QoS, MGLRU Anti-Stutter, and Smart Pre-Read.
 * @license MIT
 */
#include "coreflow/v5/storage_memory_actuator.hpp"
#include "coreflow/types.hpp"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sstream>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow::v5 {

namespace {

std::string read_first_line_raw(const std::string& path) {
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return {};
    char buf[256];
    std::string out;
    const ssize_t n = ::read(fd, buf, sizeof(buf));
    (void)::close(fd);
    if (n <= 0) return {};
    out.assign(buf, static_cast<size_t>(n));
    const auto nl = out.find('\n');
    if (nl != std::string::npos) out.resize(nl);
    while (!out.empty() && (out.back() == '\r' || out.back() == ' ' || out.back() == '\t')) out.pop_back();
    size_t start = 0;
    while (start < out.size() && (out[start] == ' ' || out[start] == '\t')) ++start;
    return out.substr(start);
}

std::string read_maps_bounded(const std::string& path, size_t max_bytes = 128U * 1024U) {
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return {};
    char buf[4096];
    std::string out;
    while (out.size() < max_bytes) {
        const ssize_t n = ::read(fd, buf, std::min(sizeof(buf), max_bytes - out.size()));
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        out.append(buf, static_cast<size_t>(n));
    }
    (void)::close(fd);
    return out;
}

bool ends_with(std::string_view text, std::string_view suffix) noexcept {
    return text.size() >= suffix.size() &&
           text.substr(text.size() - suffix.size()) == suffix;
}

bool is_prefetchable_asset_path(std::string_view path, bool allow_any_root) noexcept {
    if (path.empty() || path.front() != '/') return false;
    if (path.find("..") != std::string_view::npos) return false;
    const bool valid_ext = ends_with(path, ".apk") || ends_with(path, ".odex") ||
                           ends_with(path, ".vdex") || ends_with(path, ".art") ||
                           ends_with(path, ".so");
    if (!valid_ext) return false;
    if (allow_any_root) return true;
    return path.rfind("/data/app/", 0) == 0 ||
           path.rfind("/system/", 0) == 0 ||
           path.rfind("/product/", 0) == 0 ||
           path.rfind("/system_ext/", 0) == 0 ||
           path.rfind("/vendor/", 0) == 0;
}

bool parse_hex_or_dec_mask(const std::string& raw, uint64_t& out) noexcept {
    if (raw.empty() || raw.size() > 24U) return false;
    char* end = nullptr;
    errno = 0;
    const unsigned long long v = std::strtoull(raw.c_str(), &end, 0);
    if (errno != 0 || end == raw.c_str() || *end != '\0') return false;
    out = static_cast<uint64_t>(v);
    return true;
}

bool values_equivalent(const std::string& live,
                       const std::string& target,
                       double numeric_tolerance,
                       bool verify_nonzero_mask) noexcept {
    if (live == target) return true;
    if (verify_nonzero_mask) {
        uint64_t mask = 0U;
        if (parse_hex_or_dec_mask(live, mask)) return mask != 0U;
        return live == "y" || live == "Y" || live == "1";
    }
    if (numeric_tolerance > 0.0) {
        double a = 0.0, b = 0.0;
        if (parse_ascii_double(live, a) && parse_ascii_double(target, b)) {
            return std::abs(a - b) <= numeric_tolerance;
        }
    }
    return false;
}

uint64_t round_kb_step(double raw_kb, uint64_t lo, uint64_t hi) noexcept {
    const double clamped = std::clamp(raw_kb, static_cast<double>(lo), static_cast<double>(hi));
    const uint64_t stepped = static_cast<uint64_t>(std::llround(clamped / 32.0)) * 32ULL;
    return std::clamp(stepped, lo, hi);
}

std::string format_quantized_uclamp_pct(double pct) {
    // Linux cgroup v2 cpu.uclamp.max quantizes percentages onto 1/1024 increments.
    const double clamped = std::clamp(pct, 5.0, 100.0);
    const double bucket = std::round(clamped * 10.24);
    const double exact_pct = (bucket * 100.0) / 1024.0;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f", exact_pct);
    return std::string(buf);
}

} // namespace

StorageMemoryActuator::StorageMemoryActuator(
    BaselineStore& baseline, std::string procfs_root) noexcept
    : baseline_(baseline), procfs_root_(std::move(procfs_root)) {}

void StorageMemoryActuator::reset_contention() noexcept {
    contended_paths_.clear();
    last_written_.clear();
    stats_.contended_nodes = 0U;
}

uint64_t StorageMemoryActuator::compute_read_ahead_kb(
    const StorageQueueInfo& q,
    ContextMode mode,
    PerformanceProfile profile,
    double io_psi) noexcept {
    // Derive bounds strictly from each queue's discovered hardware geometry.
    const uint64_t base = q.baseline_read_ahead_kb > 0U ? q.baseline_read_ahead_kb : 128U;
    const uint64_t media_cap = (q.type == StorageType::eMMC) ? 512U : 1024U;
    const uint64_t hw_ceiling = q.max_sectors_kb > 0U
        ? std::clamp(q.max_sectors_kb, std::max<uint64_t>(base, 128U), media_cap)
        : std::max<uint64_t>(base, q.type == StorageType::eMMC ? 384U : 512U);

    // Deeper hardware queues (e.g. UFS nr_requests >= 64..128) sustain wider sequential bursts.
    const double depth_factor = q.nr_requests > 0U
        ? std::clamp(static_cast<double>(q.nr_requests) / 64.0, 0.75, 1.50)
        : (q.type == StorageType::eMMC ? 0.85 : 1.00);
    const double psi_scale = 1.0 + std::clamp(io_psi / 50.0, 0.0, 0.60);
    const double profile_scale =
        profile == PerformanceProfile::Battery ? 0.70 :
        profile == PerformanceProfile::Smoothness ? 1.15 :
        profile == PerformanceProfile::Gaming ? 1.15 : 1.00;

    if (mode == ContextMode::LaunchBurst) {
        // Sequential APK / OAT / asset loading burst scaled to hardware queue ceiling.
        const double burst_base = std::max(static_cast<double>(base) * 2.0,
                                           static_cast<double>(hw_ceiling) * 0.65);
        return round_kb_step(burst_base * depth_factor * profile_scale * psi_scale,
                             std::max<uint64_t>(128U, base), hw_ceiling);
    }

    if (mode == ContextMode::Gaming) {
        // Stay lean when GPU/CPU is steady; widen smoothly when io_psi signals level streaming.
        const double game_base = (q.type == StorageType::eMMC ? 192.0 : 128.0) * psi_scale;
        return round_kb_step(game_base * profile_scale, 64U, std::min<uint64_t>(hw_ceiling, 512U));
    }

    // Steady-state Interactive / Efficiency / ScreenOff: keep random-read latency crisp.
    // On fast UFS/NVMe, excessive read-ahead causes read amplification; on eMMC, keep a bit more.
    const double steady_anchor = (q.type == StorageType::eMMC)
        ? std::clamp(static_cast<double>(base), 128.0, 256.0)
        : std::clamp(static_cast<double>(base) * 0.5, 64.0, 128.0);
    const uint64_t steady_hi = std::min<uint64_t>(hw_ceiling, q.type == StorageType::eMMC ? 384U : 256U);
    return round_kb_step(steady_anchor * psi_scale, 64U, steady_hi);
}

uint64_t StorageMemoryActuator::compute_read_ahead_kb(
    StorageType type, ContextMode mode, PerformanceProfile profile) noexcept {
    StorageQueueInfo synth{};
    synth.type = type;
    synth.baseline_read_ahead_kb = (type == StorageType::eMMC) ? 256U : 128U;
    synth.max_sectors_kb = (type == StorageType::eMMC) ? 384U : 512U;
    synth.nr_requests = (type == StorageType::eMMC) ? 32U : 64U;
    return compute_read_ahead_kb(synth, mode, profile, 0.0);
}

bool StorageMemoryActuator::write_managed_node(
    const std::string& path,
    const std::string& target,
    double numeric_tolerance,
    bool verify_nonzero_mask,
    double deadband) noexcept {
    if (path.empty() || target.empty() || contended_paths_.contains(path)) return false;

    const auto prev = last_written_.find(path);
    const std::string live = read_first_line_raw(path);
    if (live.empty()) return false;

    // If we previously wrote this node and an external OEM daemon changed it outside tolerance,
    // step back gracefully instead of fighting the vendor daemon.
    if (prev != last_written_.end() &&
        !values_equivalent(live, prev->second, std::max(numeric_tolerance, 0.6), verify_nonzero_mask)) {
        contended_paths_.insert(path);
        stats_.contended_nodes = static_cast<uint32_t>(contended_paths_.size());
        return false;
    }
    if (values_equivalent(live, target, numeric_tolerance, verify_nonzero_mask)) {
        last_written_[path] = live;
        return true;
    }
    // Suppress minor PSI/RAM jitter writes so CFJ4 journal is not fsynced for negligible deltas.
    if (deadband > 0.0 && prev != last_written_.end()) {
        double live_num = 0.0, tgt_num = 0.0;
        if (parse_ascii_double(live, live_num) && parse_ascii_double(target, tgt_num) &&
            std::abs(live_num - tgt_num) <= deadband) {
            return true;
        }
    }

    // BaselineStore::apply_mutation already performs durable write-ahead journaling before writing.
    if (!baseline_.capture(path)) return false;
    if (!baseline_.apply_mutation(path, target)) return false;
    const std::string verify = read_first_line_raw(path);
    if (!values_equivalent(verify, target, std::max(numeric_tolerance, 0.6), verify_nonzero_mask)) {
        contended_paths_.insert(path);
        stats_.contended_nodes = static_cast<uint32_t>(contended_paths_.size());
        return false;
    }
    // Store the kernel's actual canonical readback (e.g. quantized "34.96" or "0x0003")
    // so subsequent ticks never misclassify quantization as OEM contention.
    last_written_[path] = verify;
    return true;
}

void StorageMemoryActuator::rollback_managed_node(const std::string& path) noexcept {
    if (path.empty() || !last_written_.contains(path)) return;
    (void)baseline_.rollback(path);
    last_written_.erase(path);
}

void StorageMemoryActuator::apply(
    const HardwareTopology& topo,
    const ContextDecision& ctx,
    PerformanceProfile profile,
    bool enable_storage_burst,
    bool enable_memory_guard,
    uint64_t now_ms,
    const StorageMemoryTelemetry& telemetry) noexcept {
    const double io_psi = std::isfinite(telemetry.io_psi) ? std::clamp(telemetry.io_psi, 0.0, 100.0) : 0.0;
    const double mem_psi = std::isfinite(telemetry.mem_psi) ? std::clamp(telemetry.mem_psi, 0.0, 100.0) : 0.0;
    const double cpu_psi = std::isfinite(telemetry.cpu_psi) ? std::clamp(telemetry.cpu_psi, 0.0, 100.0) : 0.0;

    // 1. Dynamic UFS/eMMC/dm-* Burst Readahead + Adaptive Background Isolation
    if (enable_storage_burst && ctx.mode != ContextMode::Excluded) {
        stats_.burst_active = (ctx.mode == ContextMode::LaunchBurst);
        uint64_t primary_ra = 0U;
        for (const auto& q : topo.storage_queues) {
            const uint64_t target_ra = compute_read_ahead_kb(q, ctx.mode, profile, io_psi);
            if (write_managed_node(q.read_ahead_kb_path, std::to_string(target_ra), 0.0, false, 32.0)) {
                if (primary_ra == 0U || !q.is_mapper) primary_ra = target_ra;
            }
        }
        if (primary_ra > 0U) stats_.active_read_ahead_kb = primary_ra;

        const bool isolate_bg = (ctx.mode == ContextMode::LaunchBurst ||
                                 ctx.mode == ContextMode::Interactive ||
                                 ctx.mode == ContextMode::Gaming);
        if (isolate_bg) {
            bool any_bg = false;
            if (!topo.cgroups.background_io_weight_path.empty()) {
                // Scale background I/O weight inversely with live io_psi & foreground urgency.
                const double base_w = (ctx.mode == ContextMode::LaunchBurst) ? 18.0 :
                                      (ctx.mode == ContextMode::Gaming) ? 24.0 : 35.0;
                const uint32_t weight_val = static_cast<uint32_t>(
                    std::clamp(std::lround(base_w * (1.0 - std::clamp(io_psi / 120.0, 0.0, 0.5))), 10L, 50L));
                any_bg = write_managed_node(topo.cgroups.background_io_weight_path,
                                            "default " + std::to_string(weight_val)) || any_bg;
            }
            if (!topo.cgroups.background_uclamp_max_path.empty()) {
                // Scale background CPU ceiling inversely with live cpu_psi & gaming/burst urgency.
                const double base_pct = (ctx.mode == ContextMode::Gaming) ? 25.0 :
                                        (ctx.mode == ContextMode::LaunchBurst) ? 31.25 : 37.50;
                const double adapted_pct = std::clamp(base_pct - (cpu_psi * 0.15), 15.625, 50.0);
                if (topo.cgroups.uclamp_uses_percent) {
                    const std::string bg_cap = format_quantized_uclamp_pct(adapted_pct);
                    any_bg = write_managed_node(topo.cgroups.background_uclamp_max_path, bg_cap, 0.6, false, 2.5) || any_bg;
                } else {
                    const uint32_t raw_cap = static_cast<uint32_t>(
                        std::clamp(std::lround(adapted_pct * 10.24), 160L, 512L));
                    any_bg = write_managed_node(topo.cgroups.background_uclamp_max_path,
                                                std::to_string(raw_cap), 4.0, false, 24.0) || any_bg;
                }
            }
            stats_.background_isolated = any_bg;
        } else {
            // Screen-off / Efficiency: let background syncs and Android maintenance run cleanly.
            rollback_managed_node(topo.cgroups.background_io_weight_path);
            rollback_managed_node(topo.cgroups.background_uclamp_max_path);
            stats_.background_isolated = false;
        }
    } else {
        stats_.burst_active = false;
        stats_.background_isolated = false;
        for (const auto& q : topo.storage_queues) rollback_managed_node(q.read_ahead_kb_path);
        rollback_managed_node(topo.cgroups.background_io_weight_path);
        rollback_managed_node(topo.cgroups.background_uclamp_max_path);
    }

    // 2. Self-Calibrating Anti-Stutter Memory Guard (MGLRU enable + dynamic TTL + ZRAM + live MemAvailable)
    if (enable_memory_guard && ctx.mode != ContextMode::Excluded) {
        const double avail_ratio = telemetry.memory.valid ? telemetry.memory.available_ratio() : 0.45;
        const double mem_psi_norm = std::clamp(mem_psi / 100.0, 0.0, 1.0);
        const double zram_ratio = (topo.total_ram_bytes > 0U && topo.zram_active)
            ? std::clamp(static_cast<double>(topo.zram_bytes) / static_cast<double>(topo.total_ram_bytes), 0.0, 1.5)
            : 0.0;

        // Enable Linux Multi-Gen LRU if kernel exposes it disabled (e.g. HyperOS 0x0000).
        if (!topo.vm.mglru_enabled_path.empty()) {
            uint64_t live_mask = 0U;
            const std::string cur = read_first_line_raw(topo.vm.mglru_enabled_path);
            if (!parse_hex_or_dec_mask(cur, live_mask) || live_mask == 0U) {
                (void)write_managed_node(topo.vm.mglru_enabled_path, "y", 0.0, true);
            }
        }

        if (!topo.vm.mglru_min_ttl_path.empty()) {
            uint32_t ttl_ms = 0U;
            if (ctx.mode != ContextMode::ScreenOff) {
                const double urgency = (ctx.mode == ContextMode::Gaming || ctx.mode == ContextMode::LaunchBurst)
                    ? 1.25 : (ctx.mode == ContextMode::Interactive ? 1.00 : 0.60);
                // Protect hot UI/game pages longer when RAM is plentiful; shrink smoothly under memory pressure.
                const double raw_ttl = 1400.0 * std::clamp(avail_ratio * 2.0, 0.25, 1.0) *
                                       (1.0 - mem_psi_norm * 0.65) * urgency;
                ttl_ms = static_cast<uint32_t>(
                    std::clamp(std::lround(raw_ttl / 50.0) * 50L, 150L, 1500L));
            }
            const double ttl_deadband = (ctx.mode == ContextMode::ScreenOff) ? 0.0 : 100.0;
            (void)write_managed_node(topo.vm.mglru_min_ttl_path, std::to_string(ttl_ms), 0.0, false, ttl_deadband);
        }

        if (topo.zram_active && !topo.vm.page_cluster_path.empty()) {
            (void)write_managed_node(topo.vm.page_cluster_path, "0");
        }

        if (!topo.vm.swappiness_path.empty()) {
            // Continuous swappiness derived from live RAM headroom, ZRAM ratio, and mem_psi:
            // Abundant RAM -> keep hot pages uncompressed; tight RAM with ZRAM -> reclaim proactively before stutter.
            const double base_swap = topo.zram_active ? (45.0 + 25.0 * std::min(zram_ratio, 1.0)) : 35.0;
            const double scarcity = std::clamp((0.40 - avail_ratio) / 0.30, 0.0, 1.0);
            const double adapted_swap = base_swap + scarcity * (topo.zram_active ? 35.0 : 25.0) +
                                        mem_psi_norm * 15.0;
            const uint32_t swap_val = static_cast<uint32_t>(
                std::clamp(std::lround(adapted_swap / 5.0) * 5L, 30L, topo.zram_active ? 100L : 80L));
            (void)write_managed_node(topo.vm.swappiness_path, std::to_string(swap_val), 0.0, false, 5.0);
        }

        if (!topo.vm.compaction_proactiveness_path.empty()) {
            // Pause proactive compaction stalls during latency-critical LaunchBurst & Gaming;
            // allow gentle background compaction scaled to available memory when calm.
            uint32_t comp = 0U;
            if (ctx.mode != ContextMode::LaunchBurst && ctx.mode != ContextMode::Gaming) {
                const double raw_comp = 20.0 * std::clamp(avail_ratio * 2.0, 0.25, 1.0) * (1.0 - mem_psi_norm * 0.5);
                comp = static_cast<uint32_t>(std::clamp(std::lround(raw_comp / 5.0) * 5L, 5L, 25L));
            }
            const double comp_deadband = (comp == 0U) ? 0.0 : 5.0;
            (void)write_managed_node(topo.vm.compaction_proactiveness_path, std::to_string(comp), 0.0, false, comp_deadband);
        }

        // 3. Smart asynchronous UFS/eMMC page-cache pre-read on newly launched foreground app
        //    Self-gated: skips prefetch under memory pressure so it never triggers kswapd churn.
        if (ctx.newly_launched_app && ctx.foreground_pid > 0 && !ctx.foreground_package.empty() &&
            mem_psi < 10.0 && avail_ratio >= 0.15) {
            const uint64_t budget_basis = telemetry.memory.valid
                ? telemetry.memory.available_bytes
                : topo.total_ram_bytes;
            (void)prefetch_app_assets(ctx.foreground_pid, ctx.foreground_package,
                                      budget_basis, now_ms);
        }
    } else {
        rollback_managed_node(topo.vm.mglru_enabled_path);
        rollback_managed_node(topo.vm.mglru_min_ttl_path);
        rollback_managed_node(topo.vm.page_cluster_path);
        rollback_managed_node(topo.vm.swappiness_path);
        rollback_managed_node(topo.vm.compaction_proactiveness_path);
    }
}

uint64_t StorageMemoryActuator::prefetch_app_assets(
    int32_t pid,
    const std::string& package_name,
    uint64_t available_or_total_ram_bytes,
    uint64_t now_ms,
    bool allow_any_path_for_test) noexcept {
    if (pid <= 0 || package_name.empty()) return 0U;

    // Enforce 60-second per-package cooldown so rapid task-switching never spams fadvise.
    const auto prev = last_preread_ms_.find(package_name);
    if (prev != last_preread_ms_.end() && now_ms >= prev->second && now_ms - prev->second < 60000ULL) {
        return 0U;
    }
    last_preread_ms_[package_name] = now_ms;
    if (last_preread_ms_.size() > 128U) last_preread_ms_.clear();

    // Scale byte budget dynamically (~0.75% of available RAM, bounded 4..32 MiB).
    const uint64_t dynamic_budget = available_or_total_ram_bytes / 128ULL;
    const uint64_t byte_budget = std::clamp<uint64_t>(
        dynamic_budget, 4ULL * 1024ULL * 1024ULL, 32ULL * 1024ULL * 1024ULL);
    constexpr size_t kMaxFilesPerLaunch = 12U;
    constexpr uint64_t kMaxBytesPerFile = 8ULL * 1024ULL * 1024ULL;

    const std::string maps = read_maps_bounded(
        procfs_root_ + "/" + std::to_string(pid) + "/maps");
    if (maps.empty()) return 0U;

    std::unordered_set<std::string> seen;
    uint64_t total_advised = 0U;
    size_t files_advised = 0U;
    std::istringstream ss(maps);
    std::string line;
    while (std::getline(ss, line) && files_advised < kMaxFilesPerLaunch && total_advised < byte_budget) {
        const auto slash = line.find('/');
        if (slash == std::string::npos) continue;
        std::string file_path = line.substr(slash);
        while (!file_path.empty() && (file_path.back() == '\r' || file_path.back() == '\n' || file_path.back() == ' ')) {
            file_path.pop_back();
        }
        if (!is_prefetchable_asset_path(file_path, allow_any_path_for_test)) continue;
        if (!seen.insert(file_path).second) continue;

        const int fd = ::open(file_path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
        if (fd < 0) continue;
        struct stat st{};
        if (::fstat(fd, &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0) {
            const uint64_t remaining = byte_budget - total_advised;
            const uint64_t len = std::min<uint64_t>({static_cast<uint64_t>(st.st_size), kMaxBytesPerFile, remaining});
            if (len > 0U && ::posix_fadvise(fd, 0, static_cast<off_t>(len), POSIX_FADV_WILLNEED) == 0) {
                total_advised += len;
                ++files_advised;
                ++stats_.preread_files_total;
                stats_.preread_bytes_total += len;
            }
        }
        (void)::close(fd);
    }
    return total_advised;
}

} // namespace coreflow::v5
