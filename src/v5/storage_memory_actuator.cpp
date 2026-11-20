/**
 * @file storage_memory_actuator.cpp
 * @brief CoreFlow v5 Pillar 3B: Dynamic UFS/eMMC Burst, Background QoS, MGLRU Anti-Stutter, and Smart Pre-Read.
 * @license MIT
 */
#include "coreflow/v5/storage_memory_actuator.hpp"

#include <algorithm>
#include <cerrno>
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
    StorageType type, ContextMode mode, PerformanceProfile profile) noexcept {
    const bool burst = (mode == ContextMode::LaunchBurst);
    switch (type) {
        case StorageType::UFS:
        case StorageType::NVMe:
            if (burst) {
                return profile == PerformanceProfile::Battery ? 256U : 512U;
            }
            // Steady-state UFS/NVMe excels at 128 KB (low random-read latency & zero read amplification)
            return 128U;
        case StorageType::eMMC:
            if (burst) return 384U;
            return 256U;
        default:
            return burst ? 256U : 128U;
    }
}

bool StorageMemoryActuator::write_managed_node(
    const std::string& path, const std::string& target) noexcept {
    if (path.empty() || target.empty() || contended_paths_.contains(path)) return false;

    const auto prev = last_written_.find(path);
    const std::string live = read_first_line_raw(path);
    if (live.empty()) return false;

    // If we previously wrote this node and an external OEM daemon changed it,
    // step back gracefully instead of fighting the vendor daemon.
    if (prev != last_written_.end() && live != prev->second) {
        contended_paths_.insert(path);
        stats_.contended_nodes = static_cast<uint32_t>(contended_paths_.size());
        return false;
    }
    if (live == target) {
        last_written_[path] = target;
        return true;
    }

    if (!baseline_.capture_durable(path)) return false;
    if (!baseline_.apply_mutation(path, target)) return false;
    const std::string verify = read_first_line_raw(path);
    if (verify != target) {
        contended_paths_.insert(path);
        stats_.contended_nodes = static_cast<uint32_t>(contended_paths_.size());
        return false;
    }
    last_written_[path] = target;
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
    uint64_t now_ms) noexcept {
    // 1. Dynamic UFS/eMMC Burst Readahead + Background Isolation
    if (enable_storage_burst && ctx.mode != ContextMode::Excluded) {
        stats_.burst_active = (ctx.mode == ContextMode::LaunchBurst);
        uint64_t primary_ra = 0U;
        for (const auto& q : topo.storage_queues) {
            const uint64_t target_ra = compute_read_ahead_kb(q.type, ctx.mode, profile);
            if (write_managed_node(q.read_ahead_kb_path, std::to_string(target_ra))) {
                if (primary_ra == 0U) primary_ra = target_ra;
            }
        }
        if (primary_ra > 0U) stats_.active_read_ahead_kb = primary_ra;

        const bool isolate_bg = (ctx.mode == ContextMode::LaunchBurst ||
                                 ctx.mode == ContextMode::Interactive ||
                                 ctx.mode == ContextMode::Gaming);
        if (isolate_bg) {
            bool any_bg = false;
            if (!topo.cgroups.background_io_weight_path.empty()) {
                const std::string weight = ctx.mode == ContextMode::LaunchBurst ? "default 15" : "default 30";
                any_bg = write_managed_node(topo.cgroups.background_io_weight_path, weight) || any_bg;
            }
            if (!topo.cgroups.background_uclamp_max_path.empty()) {
                const std::string bg_cap = topo.cgroups.uclamp_uses_percent
                    ? (ctx.mode == ContextMode::Gaming ? "25.00" : "35.00")
                    : (ctx.mode == ContextMode::Gaming ? "256" : "358");
                any_bg = write_managed_node(topo.cgroups.background_uclamp_max_path, bg_cap) || any_bg;
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

    // 2. Anti-Stutter Memory Guard (MGLRU + ZRAM page-cluster + swappiness + anti-compaction stall)
    if (enable_memory_guard && ctx.mode != ContextMode::Excluded) {
        const bool ram_ge_6gb = topo.total_ram_bytes >= 6ULL * 1024ULL * 1024ULL * 1024ULL;
        if (!topo.vm.mglru_min_ttl_path.empty()) {
            const char* ttl =
                ctx.mode == ContextMode::ScreenOff ? "0" :
                ctx.mode == ContextMode::Efficiency ? "500" : "1000";
            (void)write_managed_node(topo.vm.mglru_min_ttl_path, ttl);
        }
        if (topo.zram_active && !topo.vm.page_cluster_path.empty()) {
            (void)write_managed_node(topo.vm.page_cluster_path, "0");
        }
        if (!topo.vm.swappiness_path.empty()) {
            const char* swap = (ram_ge_6gb && topo.zram_active) ? "60" :
                               ram_ge_6gb ? "70" : "80";
            (void)write_managed_node(topo.vm.swappiness_path, swap);
        }
        if (!topo.vm.compaction_proactiveness_path.empty()) {
            // Pause proactive compaction stalls during LaunchBurst & Gaming; allow gently when idle.
            const char* comp = (ctx.mode == ContextMode::LaunchBurst || ctx.mode == ContextMode::Gaming)
                ? "0" : "20";
            (void)write_managed_node(topo.vm.compaction_proactiveness_path, comp);
        }

        // 3. Smart asynchronous UFS/eMMC page-cache pre-read on newly launched foreground app
        if (ctx.newly_launched_app && ctx.foreground_pid > 0 && !ctx.foreground_package.empty()) {
            (void)prefetch_app_assets(ctx.foreground_pid, ctx.foreground_package,
                                      topo.total_ram_bytes, now_ms);
        }
    } else {
        rollback_managed_node(topo.vm.mglru_min_ttl_path);
        rollback_managed_node(topo.vm.page_cluster_path);
        rollback_managed_node(topo.vm.swappiness_path);
        rollback_managed_node(topo.vm.compaction_proactiveness_path);
    }
}

uint64_t StorageMemoryActuator::prefetch_app_assets(
    int32_t pid,
    const std::string& package_name,
    uint64_t total_ram_bytes,
    uint64_t now_ms,
    bool allow_any_path_for_test) noexcept {
    if (pid <= 0 || package_name.empty()) return 0U;

    // Enforce 30-second per-package cooldown so rapid task-switching never spams fadvise.
    const auto prev = last_preread_ms_.find(package_name);
    if (prev != last_preread_ms_.end() && now_ms >= prev->second && now_ms - prev->second < 30000ULL) {
        return 0U;
    }
    last_preread_ms_[package_name] = now_ms;
    if (last_preread_ms_.size() > 128U) last_preread_ms_.clear();

    const uint64_t ram_gb = total_ram_bytes / (1024ULL * 1024ULL * 1024ULL);
    const uint64_t byte_budget = ram_gb >= 6ULL ? (32ULL * 1024ULL * 1024ULL)
                                                : (16ULL * 1024ULL * 1024ULL);
    constexpr size_t kMaxFilesPerLaunch = 16U;
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
