/**
 * @file hardware_probe.cpp
 * @brief CoreFlow v5 Pillar 1: Single-pass capability-based hardware & kernel topology probe.
 * @license MIT
 */
#include "coreflow/v5/hardware_probe.hpp"
#include "coreflow/mount_topology.hpp"
#include "coreflow/types.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <fcntl.h>
#include <filesystem>
#include <set>
#include <sstream>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow::v5 {
namespace fs = std::filesystem;

namespace {

constexpr size_t kSmallReadLimit = 4096U;

std::string trim_copy(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1U);
}

std::string to_lower_copy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

std::string read_first_line(const fs::path& path) {
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return {};
    struct stat st{};
    if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        (void)::close(fd);
        return {};
    }
    char buf[512];
    std::string out;
    while (out.size() < kSmallReadLimit) {
        const ssize_t n = ::read(fd, buf, std::min(sizeof(buf), kSmallReadLimit - out.size()));
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        out.append(buf, static_cast<size_t>(n));
        if (out.find('\n') != std::string::npos) break;
    }
    (void)::close(fd);
    const auto nl = out.find('\n');
    if (nl != std::string::npos) out.resize(nl);
    return trim_copy(std::move(out));
}

std::string read_bounded(const fs::path& path, size_t max_bytes) {
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return {};
    struct stat st{};
    if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        (void)::close(fd);
        return {};
    }
    char buf[2048];
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

bool parse_u64(std::string_view text, uint64_t& out) noexcept {
    if (text.empty() || text.size() > 32U) return false;
    const auto r = std::from_chars(text.data(), text.data() + text.size(), out);
    return r.ec == std::errc{} && r.ptr == text.data() + text.size();
}

std::vector<uint64_t> parse_u64_list_sorted(std::string_view raw) {
    std::vector<uint64_t> values;
    std::istringstream ss{std::string(raw)};
    std::string tok;
    while (ss >> tok && values.size() < 256U) {
        uint64_t v = 0;
        if (parse_u64(tok, v) && v > 0U) values.push_back(v);
    }
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    return values;
}

bool is_regular_readable(const fs::path& p) noexcept {
    struct stat st{};
    return ::lstat(p.c_str(), &st) == 0 && S_ISREG(st.st_mode) && ::access(p.c_str(), R_OK) == 0;
}

bool is_regular_writable(const fs::path& p) noexcept {
    struct stat st{};
    return ::lstat(p.c_str(), &st) == 0 && S_ISREG(st.st_mode) &&
           ::access(p.c_str(), R_OK | W_OK) == 0;
}

fs::path canonical_under(const fs::path& input, const fs::path& root) {
    std::error_code ec;
    const auto c = fs::canonical(input, ec);
    if (ec) return {};
    const auto cr = fs::canonical(root, ec);
    if (ec) return {};
    const auto rel = c.lexically_relative(cr);
    if (rel.empty() || rel.native().rfind("..", 0) == 0) return {};
    return c;
}

std::vector<fs::path> child_dirs(const fs::path& parent, size_t limit = 256U) {
    std::vector<fs::path> out;
    std::error_code ec;
    for (fs::directory_iterator it(parent, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end && out.size() < limit; it.increment(ec)) {
        std::error_code iec;
        if (it->is_directory(iec) && !iec) out.push_back(it->path());
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<unsigned> parse_cpu_list(std::string_view raw) {
    std::vector<unsigned> cpus;
    size_t pos = 0;
    while (pos < raw.size()) {
        while (pos < raw.size() && (raw[pos] == ' ' || raw[pos] == '\t' || raw[pos] == ',')) ++pos;
        if (pos == raw.size()) break;
        const size_t end = raw.find(',', pos);
        std::string_view tok = raw.substr(pos, end == std::string_view::npos ? raw.size() - pos : end - pos);
        while (!tok.empty() && (tok.front() == ' ' || tok.front() == '\t')) tok.remove_prefix(1);
        while (!tok.empty() && (tok.back() == ' ' || tok.back() == '\t')) tok.remove_suffix(1);
        const size_t dash = tok.find('-');
        uint64_t a = 0, b = 0;
        if (dash == std::string_view::npos) {
            if (!parse_u64(tok, a)) return {};
            b = a;
        } else {
            if (!parse_u64(tok.substr(0, dash), a) || !parse_u64(tok.substr(dash + 1), b) || b < a) return {};
        }
        if (b > 511U) return {};
        for (uint64_t c = a; c <= b; ++c) cpus.push_back(static_cast<unsigned>(c));
        pos = end == std::string_view::npos ? raw.size() : end + 1;
    }
    std::sort(cpus.begin(), cpus.end());
    cpus.erase(std::unique(cpus.begin(), cpus.end()), cpus.end());
    return cpus;
}

std::optional<uint32_t> parse_refresh_from_text(std::string_view text) noexcept {
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] != '@') continue;
        size_t j = i + 1;
        while (j < text.size() && text[j] >= '0' && text[j] <= '9') ++j;
        uint64_t hz = 0;
        if (parse_u64(text.substr(i + 1, j - i - 1), hz) && hz >= 30U && hz <= 240U) {
            return static_cast<uint32_t>(hz);
        }
    }
    return std::nullopt;
}

fs::path find_governor_tunable(const fs::path& policy_dir, const std::string& governor, std::string_view leaf) {
    // Check direct governor subdir first (e.g. policy0/walt/hispeed_freq or policy0/schedutil/up_rate_limit_us)
    const fs::path gov_path = policy_dir / governor / leaf;
    if (is_regular_writable(gov_path)) return gov_path;
    for (const auto& sub : child_dirs(policy_dir, 16U)) {
        const fs::path cand = sub / leaf;
        if (is_regular_writable(cand)) return cand;
    }
    const fs::path direct = policy_dir / leaf;
    if (is_regular_writable(direct)) return direct;
    return {};
}

} // namespace

HardwareProbe::HardwareProbe(std::string sysfs_root, std::string procfs_root) noexcept
    : sysfs_root_(std::move(sysfs_root)), procfs_root_(std::move(procfs_root)) {}

const char* HardwareProbe::storage_type_name(StorageType type) noexcept {
    switch (type) {
        case StorageType::UFS: return "UFS";
        case StorageType::NVMe: return "NVMe";
        case StorageType::eMMC: return "eMMC";
        case StorageType::SolidState: return "SSD";
        default: return "Unknown";
    }
}

const char* HardwareProbe::cluster_role_name(ClusterRole role) noexcept {
    switch (role) {
        case ClusterRole::Little: return "little";
        case ClusterRole::Mid: return "mid";
        case ClusterRole::Big: return "big";
        default: return "unknown";
    }
}

HardwareTopology HardwareProbe::discover() const noexcept {
    HardwareTopology topo{};
    const fs::path sys(sysfs_root_);
    const fs::path proc(procfs_root_);

    // 1. Kernel version & SoC model
    const auto kver = read_first_line(proc / "sys/kernel/osrelease");
    if (!kver.empty()) topo.kernel_version = kver;

    auto soc = read_first_line(sys / "devices/soc0/machine");
    if (soc.empty()) soc = read_first_line(sys / "devices/soc0/soc_id");
    if (soc.empty()) soc = read_first_line(proc / "device-tree/model");
    if (soc.empty()) {
        const auto cpuinfo = read_bounded(proc / "cpuinfo", 16384U);
        std::istringstream ss(cpuinfo);
        std::string line;
        while (std::getline(ss, line)) {
            if (line.rfind("Hardware", 0) == 0 || line.rfind("model name", 0) == 0) {
                const auto colon = line.find(':');
                if (colon != std::string::npos) {
                    soc = trim_copy(line.substr(colon + 1));
                    if (!soc.empty()) break;
                }
            }
        }
    }
    if (!soc.empty()) topo.soc_model = soc;

    // 2. RAM & ZRAM (untruncated /sys/block scan + dynamic /proc/swaps discovery)
    {
        const auto mem = read_memory_snapshot();
        topo.total_ram_bytes = mem.total_bytes;

        std::set<std::string> seen_zram;
        for (const auto& blk : child_dirs(sys / "block", 512U)) {
            const std::string name = blk.filename().string();
            if (name.rfind("zram", 0) != 0) continue;
            uint64_t sz = 0;
            if (parse_u64(read_first_line(blk / "disksize"), sz) && sz > 0U) {
                seen_zram.insert(name);
                topo.zram_bytes += sz;
                topo.zram_active = true;
            }
        }
        // Dynamic /proc/swaps fallback (catches zram even when /sys/block differs)
        const auto swaps = read_bounded(proc / "swaps", 8192U);
        std::istringstream sw_ss(swaps);
        std::string sw_line;
        while (std::getline(sw_ss, sw_line)) {
            if (sw_line.find("zram") == std::string::npos) continue;
            std::istringstream row(sw_line);
            std::string dev_path, sw_type;
            uint64_t size_kb = 0;
            if (row >> dev_path >> sw_type >> size_kb && size_kb > 0U) {
                const auto slash = dev_path.find_last_of('/');
                const std::string dev_name = slash != std::string::npos ? dev_path.substr(slash + 1U) : dev_path;
                if (seen_zram.insert(dev_name).second) {
                    topo.zram_bytes += size_kb * 1024ULL;
                }
                topo.zram_active = true;
            }
        }
    }

    // 3. CPU Clusters & Real EAS Silicon Capacities
    {
        const fs::path cpufreq_root = sys / "devices/system/cpu/cpufreq";
        std::set<uint64_t> cap_levels;
        std::set<uint64_t> max_freqs;
        bool all_have_cap = true;
        for (const auto& pdir : child_dirs(cpufreq_root, 32U)) {
            const auto name = pdir.filename().string();
            if (name.rfind("policy", 0) != 0) continue;
            auto opps = parse_u64_list_sorted(read_first_line(pdir / "scaling_available_frequencies"));
            if (opps.size() < 2U) continue;

            CpuClusterInfo info{};
            info.policy_path = pdir.string();
            info.governor = read_first_line(pdir / "scaling_governor");
            info.opps_khz = std::move(opps);
            max_freqs.insert(info.opps_khz.back());

            auto cpus = parse_cpu_list(read_first_line(pdir / "related_cpus"));
            if (cpus.empty()) cpus = parse_cpu_list(read_first_line(pdir / "affected_cpus"));
            std::vector<uint64_t> caps;
            for (unsigned cpu : cpus) {
                uint64_t cap = 0;
                if (parse_u64(read_first_line(sys / "devices/system/cpu" /
                        ("cpu" + std::to_string(cpu)) / "cpu_capacity"), cap) &&
                    cap > 0U && cap <= 65536U) {
                    caps.push_back(cap);
                }
            }
            if (!caps.empty()) {
                std::sort(caps.begin(), caps.end());
                info.capacity = caps[caps.size() / 2U];
                cap_levels.insert(info.capacity);
            } else {
                all_have_cap = false;
            }

            if (info.governor == "walt" || info.governor == "schedutil" || info.governor == "interactive") {
                info.hispeed_freq_path = find_governor_tunable(pdir, info.governor, "hispeed_freq").string();
                info.hispeed_load_path = find_governor_tunable(pdir, info.governor, "hispeed_load").string();
                info.up_rate_limit_path = find_governor_tunable(pdir, info.governor, "up_rate_limit_us").string();
                info.down_rate_limit_path = find_governor_tunable(pdir, info.governor, "down_rate_limit_us").string();
                info.boost_path = find_governor_tunable(pdir, info.governor, "boost").string();
            }
            topo.cpu_clusters.push_back(std::move(info));
        }

        if (all_have_cap && cap_levels.size() >= 2U) {
            const uint64_t lo = *cap_levels.begin();
            const uint64_t hi = *cap_levels.rbegin();
            for (auto& c : topo.cpu_clusters) {
                if (c.capacity == lo) c.role = ClusterRole::Little;
                else if (c.capacity == hi) c.role = ClusterRole::Big;
                else c.role = ClusterRole::Mid;
            }
        } else if (max_freqs.size() >= 2U) {
            const uint64_t lo = *max_freqs.begin();
            const uint64_t hi = *max_freqs.rbegin();
            for (auto& c : topo.cpu_clusters) {
                if (c.capacity == 0U && hi > 0U) {
                    c.capacity = std::clamp<uint64_t>((c.opps_khz.back() * 1024ULL) / hi, 128ULL, 1024ULL);
                }
                if (c.opps_khz.back() == lo) c.role = ClusterRole::Little;
                else if (c.opps_khz.back() == hi) c.role = ClusterRole::Big;
                else c.role = ClusterRole::Mid;
            }
        } else if (topo.cpu_clusters.size() == 1U) {
            topo.cpu_clusters[0].role = ClusterRole::Big;
            if (topo.cpu_clusters[0].capacity == 0U) topo.cpu_clusters[0].capacity = 1024U;
        }

        // Normalize discovered EAS capacities onto Linux 0..1024 scale for self-calibrating actuators.
        uint64_t max_cap = 0U;
        for (const auto& c : topo.cpu_clusters) max_cap = std::max(max_cap, c.capacity);
        if (max_cap > 0U) {
            for (const auto& c : topo.cpu_clusters) {
                const uint32_t norm = static_cast<uint32_t>(
                    std::clamp<uint64_t>((c.capacity * 1024ULL) / max_cap, 128ULL, 1024ULL));
                if (c.role == ClusterRole::Little) {
                    topo.little_capacity = topo.little_capacity == 0U ? norm : std::min(topo.little_capacity, norm);
                } else if (c.role == ClusterRole::Mid) {
                    topo.mid_capacity = std::max(topo.mid_capacity, norm);
                } else if (c.role == ClusterRole::Big) {
                    topo.big_capacity = std::max(topo.big_capacity, norm);
                }
            }
            if (topo.little_capacity == 0U) topo.little_capacity = std::min<uint32_t>(topo.big_capacity / 2U, 448U);
            if (topo.mid_capacity == 0U) topo.mid_capacity = (topo.little_capacity + topo.big_capacity) / 2U;
        }
    }

    // 4. GPU Discovery (KGSL first, then generic devfreq Mali/Adreno/GPU)
    {
        for (const auto& entry : child_dirs(sys / "class/kgsl", 8U)) {
            const auto resolved = canonical_under(entry, sys);
            if (resolved.empty()) continue;
            const auto freqs = parse_u64_list_sorted(read_first_line(resolved / "gpu_available_frequencies"));
            uint64_t levels = 0;
            if (freqs.size() < 2U || !parse_u64(read_first_line(resolved / "num_pwrlevels"), levels) || levels < 2U) {
                continue;
            }
            topo.gpu.available = true;
            topo.gpu.is_kgsl = true;
            topo.gpu.device_path = resolved.string();
            const auto model = read_first_line(resolved / "gpu_model");
            topo.gpu.model = model.empty() ? entry.filename().string() : model;
            topo.gpu.kgsl_num_pwrlevels = levels;
            if (is_regular_writable(resolved / "max_pwrlevel") &&
                is_regular_writable(resolved / "min_pwrlevel")) {
                topo.gpu.kgsl_max_pwrlevel_path = (resolved / "max_pwrlevel").string();
                topo.gpu.kgsl_min_pwrlevel_path = (resolved / "min_pwrlevel").string();
            }
            if (is_regular_writable(resolved / "idle_timer")) {
                topo.gpu.idle_timer_path = (resolved / "idle_timer").string();
            }
            if (is_regular_readable(resolved / "gpu_busy_percentage")) {
                topo.gpu.busy_path = (resolved / "gpu_busy_percentage").string();
            }
            break;
        }
        if (!topo.gpu.available) {
            for (const auto& entry : child_dirs(sys / "class/devfreq", 32U)) {
                const auto resolved = canonical_under(entry, sys);
                if (resolved.empty()) continue;
                std::string label = read_first_line(resolved / "name");
                if (label.empty()) label = entry.filename().string();
                const std::string folded = to_lower_copy(label + " " + resolved.string());
                if (folded.find("gpu") == std::string::npos &&
                    folded.find("mali") == std::string::npos &&
                    folded.find("adreno") == std::string::npos &&
                    folded.find("kgsl") == std::string::npos) {
                    continue;
                }
                const auto opps = parse_u64_list_sorted(read_first_line(resolved / "available_frequencies"));
                if (opps.size() < 2U) continue;
                topo.gpu.available = true;
                topo.gpu.is_kgsl = false;
                topo.gpu.device_path = resolved.string();
                topo.gpu.model = label;
                topo.gpu.devfreq_opps_hz = opps;
                if (is_regular_writable(resolved / "max_freq")) {
                    topo.gpu.devfreq_max_freq_path = (resolved / "max_freq").string();
                }
                if (is_regular_writable(resolved / "idle_timer")) {
                    topo.gpu.idle_timer_path = (resolved / "idle_timer").string();
                }
                if (is_regular_readable(resolved / "load")) {
                    topo.gpu.busy_path = (resolved / "load").string();
                } else if (is_regular_readable(resolved / "gpu_busy_percentage")) {
                    topo.gpu.busy_path = (resolved / "gpu_busy_percentage").string();
                }
                break;
            }
        }
    }

    // 5. Dynamic Physical + Active Mapper Storage Discovery (UFS / NVMe / eMMC + mounted /data,/system dm-*)
    {
        // Discover block major:minor IDs backing active Android partitions dynamically from mountinfo.
        std::set<std::string> active_mount_maj_min;
        const auto mountinfo = read_bounded(proc / "self/mountinfo", 65536U);
        std::istringstream mss(mountinfo);
        std::string mline;
        while (std::getline(mss, mline)) {
            std::istringstream row(mline);
            std::string id, parent, maj_min, root, mpoint;
            if (!(row >> id >> parent >> maj_min >> root >> mpoint)) continue;
            if (mpoint == "/data" || mpoint == "/system" || mpoint == "/" ||
                mpoint == "/vendor" || mpoint == "/product" || mpoint == "/system_ext") {
                active_mount_maj_min.insert(maj_min);
            }
        }

        std::set<std::string> seen_queue_paths;
        for (const auto& entry : child_dirs(sys / "class/block", 512U)) {
            const auto resolved = canonical_under(entry, sys);
            if (resolved.empty()) continue;
            const std::string res_str = resolved.string();
            const auto name = entry.filename().string();
            if (name.rfind("loop", 0) == 0 || name.rfind("zram", 0) == 0 || name.rfind("ram", 0) == 0) {
                continue;
            }
            const bool is_dm = (name.rfind("dm-", 0) == 0);
            if (is_dm) {
                // Only manage dm-* queues that actively back /data or OS partitions discovered via mountinfo.
                const std::string dev_id = read_first_line(resolved / "dev");
                if (dev_id.empty() || !active_mount_maj_min.contains(dev_id)) continue;
            } else if (res_str.find("/devices/virtual/") != std::string::npos) {
                continue;
            }

            const fs::path qdir = resolved / "queue";
            if (read_first_line(qdir / "rotational") != "0") continue;
            const fs::path ra_path = qdir / "read_ahead_kb";
            if (!is_regular_writable(ra_path)) continue;
            if (!seen_queue_paths.insert(ra_path.string()).second) continue;

            StorageQueueInfo qi{};
            qi.device_name = name;
            qi.is_mapper = is_dm;
            const std::string folded = to_lower_copy(res_str + "/" + name);
            if (folded.find("ufshc") != std::string::npos || folded.find("ufs") != std::string::npos) {
                qi.type = StorageType::UFS;
            } else if (folded.find("nvme") != std::string::npos) {
                qi.type = StorageType::NVMe;
            } else if (folded.find("mmc") != std::string::npos) {
                qi.type = StorageType::eMMC;
            } else {
                qi.type = StorageType::SolidState;
            }
            qi.read_ahead_kb_path = ra_path.string();
            uint64_t val = 0U;
            if (parse_u64(read_first_line(ra_path), val) && val > 0U) qi.baseline_read_ahead_kb = val;
            if (parse_u64(read_first_line(qdir / "max_sectors_kb"), val) && val > 0U) qi.max_sectors_kb = val;
            if (parse_u64(read_first_line(qdir / "nr_requests"), val) && val > 0U) qi.nr_requests = val;
            if (parse_u64(read_first_line(qdir / "logical_block_size"), val) && val > 0U) qi.logical_block_size = val;
            if (is_regular_writable(qdir / "nr_requests")) {
                qi.nr_requests_path = (qdir / "nr_requests").string();
            }
            if (is_regular_writable(qdir / "scheduler")) {
                qi.scheduler_path = (qdir / "scheduler").string();
            }
            if (!is_dm && (topo.primary_storage == StorageType::Unknown ||
                (topo.primary_storage == StorageType::SolidState && qi.type != StorageType::SolidState))) {
                topo.primary_storage = qi.type;
            }
            topo.storage_queues.push_back(std::move(qi));
        }
        // Inherit discovered physical storage media type onto mounted dm-* queues.
        if (topo.primary_storage != StorageType::Unknown) {
            for (auto& qi : topo.storage_queues) {
                if (qi.is_mapper && qi.type == StorageType::SolidState) {
                    qi.type = topo.primary_storage;
                }
            }
        }
    }

    // 6. VM & MGLRU nodes
    {
        const fs::path vm_root = proc / "sys/vm";
        if (is_regular_writable(vm_root / "swappiness")) topo.vm.swappiness_path = (vm_root / "swappiness").string();
        if (is_regular_writable(vm_root / "page-cluster")) topo.vm.page_cluster_path = (vm_root / "page-cluster").string();
        if (is_regular_writable(vm_root / "vfs_cache_pressure")) topo.vm.vfs_cache_pressure_path = (vm_root / "vfs_cache_pressure").string();
        if (is_regular_writable(vm_root / "compaction_proactiveness")) topo.vm.compaction_proactiveness_path = (vm_root / "compaction_proactiveness").string();
        if (is_regular_writable(vm_root / "watermark_scale_factor")) topo.vm.watermark_scale_factor_path = (vm_root / "watermark_scale_factor").string();
        const fs::path mglru_en = sys / "kernel/mm/lru_gen/enabled";
        if (is_regular_writable(mglru_en)) topo.vm.mglru_enabled_path = mglru_en.string();
        const fs::path mglru = sys / "kernel/mm/lru_gen/min_ttl_ms";
        if (is_regular_writable(mglru)) topo.vm.mglru_min_ttl_path = mglru.string();
    }

    // 7. Backlights & Display Refresh Rate (fully dynamic scan across DRM & graphics nodes)
    {
        for (const auto& bl : child_dirs(sys / "class/backlight", 16U)) {
            const auto resolved = canonical_under(bl, sys);
            if (!resolved.empty() && is_regular_readable(resolved / "brightness")) {
                topo.backlight_dirs.push_back(resolved.string());
            }
        }
        uint32_t detected_hz = 0;
        for (const auto& root : std::array<fs::path, 2>{sys / "class/drm", sys / "class/graphics"}) {
            for (const auto& dev : child_dirs(root, 32U)) {
                const auto resolved = canonical_under(dev, sys);
                if (resolved.empty()) continue;
                for (const char* leaf : {"dynamic_fps", "refresh_rate", "fps", "measured_fps",
                                         "current_mode", "modes", "available_modes", "msm_fb_panel_info"}) {
                    const fs::path leaf_path = resolved / leaf;
                    const auto txt = read_bounded(leaf_path, 2048U);
                    if (txt.empty()) continue;
                    std::istringstream lss(txt);
                    std::string line;
                    while (std::getline(lss, line)) {
                        const std::string trimmed = trim_copy(line);
                        uint64_t direct_hz = 0;
                        if (parse_u64(trimmed, direct_hz) && direct_hz >= 30U && direct_hz <= 240U) {
                            detected_hz = std::max(detected_hz, static_cast<uint32_t>(direct_hz));
                            continue;
                        }
                        if (const auto parsed = parse_refresh_from_text(trimmed)) {
                            detected_hz = std::max(detected_hz, *parsed);
                            continue;
                        }
                        // Handle key=value formats such as "panel_max_fps=120" or "fps=120"
                        const auto eq = trimmed.find('=');
                        if (eq != std::string::npos) {
                            const std::string key = to_lower_copy(trimmed.substr(0, eq));
                            if (key.find("fps") != std::string::npos || key.find("refresh") != std::string::npos) {
                                uint64_t kv_hz = 0;
                                if (parse_u64(trim_copy(trimmed.substr(eq + 1U)), kv_hz) &&
                                    kv_hz >= 30U && kv_hz <= 240U) {
                                    detected_hz = std::max(detected_hz, static_cast<uint32_t>(kv_hz));
                                }
                            }
                        }
                    }
                }
            }
        }
        if (detected_hz != 0U) topo.refresh_hz = detected_hz;
    }

    // 8. Cgroup v2 topology (dynamic via MountTopologyResolver, never hardcoded vendor paths)
    {
        MountTopologyResolver resolver(procfs_root_);
        if (const auto fg = resolver.resolve_foreground_uclamp_group()) {
            topo.cgroups.top_app_dir = fg->absolute_path;
            const fs::path procs = fs::path(fg->absolute_path) / "cgroup.procs";
            if (is_regular_readable(procs)) topo.cgroups.top_app_procs_path = procs.string();
        }
        if (topo.cgroups.top_app_procs_path.empty()) {
            for (const auto& m : resolver.discover_cgroup_mounts()) {
                for (const char* sub : {"top-app", "foreground"}) {
                    const fs::path dir = fs::path(m.mount_point) / sub;
                    if (is_regular_readable(dir / "cgroup.procs")) {
                        topo.cgroups.top_app_dir = dir.string();
                        topo.cgroups.top_app_procs_path = (dir / "cgroup.procs").string();
                        break;
                    }
                }
                if (!topo.cgroups.top_app_procs_path.empty()) break;
            }
        }
        for (const auto& m : resolver.discover_cgroup_mounts()) {
            const fs::path bg = fs::path(m.mount_point) / "background";
            if (!fs::is_directory(bg)) continue;
            if (topo.cgroups.background_dir.empty()) topo.cgroups.background_dir = bg.string();
            if (topo.cgroups.background_io_weight_path.empty() && is_regular_writable(bg / "io.weight")) {
                topo.cgroups.background_io_weight_path = (bg / "io.weight").string();
            }
            if (topo.cgroups.background_uclamp_max_path.empty() && is_regular_writable(bg / "cpu.uclamp.max")) {
                topo.cgroups.background_uclamp_max_path = (bg / "cpu.uclamp.max").string();
                const auto cur = read_first_line(bg / "cpu.uclamp.max");
                // Android kernels commonly encode cpu.uclamp.max as "max" or percentage "0.00..100.00"
                topo.cgroups.uclamp_uses_percent =
                    cur == "max" || cur.find('.') != std::string::npos || cur == "100";
            }
        }
    }

    return topo;
}

bool HardwareProbe::is_screen_off(const HardwareTopology& topo) const noexcept {
    if (topo.backlight_dirs.empty()) return false;
    bool have_any = false;
    for (const auto& dir : topo.backlight_dirs) {
        uint64_t cur = 0, max_b = 0;
        if (!parse_u64(read_first_line(fs::path(dir) / "brightness"), cur)) continue;
        if (parse_u64(read_first_line(fs::path(dir) / "max_brightness"), max_b) && max_b == 0U) continue;
        have_any = true;
        if (cur != 0U) return false;
    }
    return have_any;
}

std::optional<double> HardwareProbe::read_gpu_busy_pct(const HardwareTopology& topo) const noexcept {
    if (!topo.gpu.available || topo.gpu.busy_path.empty()) return std::nullopt;
    const auto raw = read_first_line(topo.gpu.busy_path);
    if (raw.empty()) return std::nullopt;
    // KGSL format: " 42 %" or "42%". Devfreq load format: "42@600000000" or "42".
    size_t i = 0;
    while (i < raw.size() && (raw[i] == ' ' || raw[i] == '\t')) ++i;
    size_t j = i;
    while (j < raw.size() && ((raw[j] >= '0' && raw[j] <= '9') || raw[j] == '.')) ++j;
    if (j <= i) return std::nullopt;
    double v = 0.0;
    if (!parse_ascii_double(std::string_view(raw).substr(i, j - i), v)) return std::nullopt;
    if (v < 0.0 || v > 100.0) return std::nullopt;
    return v;
}

std::optional<double> HardwareProbe::read_psi_avg10(const std::string& resource) const noexcept {
    const fs::path path = fs::path(procfs_root_) / "pressure" / resource;
    const auto content = read_bounded(path, 512U);
    if (content.empty()) return std::nullopt;
    size_t pos = 0;
    while (pos < content.size()) {
        const size_t eol = content.find('\n', pos);
        const std::string_view line(content.data() + pos,
            eol == std::string::npos ? content.size() - pos : eol - pos);
        pos = eol == std::string::npos ? content.size() : eol + 1;
        if (line.rfind("some ", 0) != 0) continue;
        const size_t at = line.find("avg10=");
        if (at == std::string_view::npos) continue;
        const auto rest = line.substr(at + 6);
        const size_t tok_end = rest.find_first_of(" \t\r");
        double val = 0.0;
        if (parse_ascii_double(tok_end == std::string_view::npos ? rest : rest.substr(0, tok_end), val) &&
            val >= 0.0 && val <= 100.0) {
            return val;
        }
    }
    return std::nullopt;
}

MemorySnapshot HardwareProbe::read_memory_snapshot() const noexcept {
    MemorySnapshot snap{};
    const auto meminfo = read_bounded(fs::path(procfs_root_) / "meminfo", 8192U);
    if (meminfo.empty()) return snap;
    std::istringstream ss(meminfo);
    std::string line;
    int found = 0;
    while (std::getline(ss, line) && found < 4) {
        std::istringstream row(line);
        std::string key;
        uint64_t val_kb = 0;
        if (!(row >> key >> val_kb)) continue;
        if (key == "MemTotal:") {
            snap.total_bytes = val_kb * 1024ULL;
            ++found;
        } else if (key == "MemAvailable:") {
            snap.available_bytes = val_kb * 1024ULL;
            ++found;
        } else if (key == "SwapTotal:") {
            snap.swap_total_bytes = val_kb * 1024ULL;
            ++found;
        } else if (key == "SwapFree:") {
            snap.swap_free_bytes = val_kb * 1024ULL;
            ++found;
        }
    }
    snap.valid = (snap.total_bytes > 0U);
    if (snap.valid && snap.available_bytes == 0U) {
        snap.available_bytes = snap.total_bytes / 2U;
    }
    return snap;
}

} // namespace coreflow::v5
