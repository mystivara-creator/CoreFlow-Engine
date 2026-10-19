#include "coreflow/tunable_discovery.hpp"

#include "coreflow/consultation.hpp"
#include "coreflow/fatal_surface.hpp"
#include "coreflow/kernel_roots.hpp"

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <dirent.h>
#include <fstream>
#include <memory>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace coreflow {
namespace {

bool startsWith(std::string_view v, std::string_view p) noexcept {
    return v.size() >= p.size() && v.compare(0, p.size(), p) == 0;
}

bool containsText(std::string_view v, std::string_view n) noexcept {
    return v.find(n) != std::string_view::npos;
}

std::string_view leafOf(std::string_view path) noexcept {
    const auto slash = path.rfind('/');
    return slash == std::string_view::npos ? path : path.substr(slash + 1U);
}

std::string_view dirOf(std::string_view path) noexcept {
    const auto slash = path.rfind('/');
    return slash == std::string_view::npos ? std::string_view{} : path.substr(0, slash);
}

bool readFirstLine(const std::string& path, std::string& out) noexcept {
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file) return false;
        out.clear();
        out.reserve(TunableDiscovery::kMaxFirstLineBytes);
        char ch = '\0';
        while (file.get(ch)) {
            if (ch == '\n') break;
            // Kernel text attributes are text, not arbitrary byte streams. A
            // NUL or an overlong value is incomplete evidence and must not be
            // silently truncated into a different actuator value.
            if (ch == '\0' || out.size() >= TunableDiscovery::kMaxFirstLineBytes) {
                out.clear();
                return false;
            }
            out.push_back(ch);
        }
        if (file.bad()) {
            out.clear();
            return false;
        }
        while (!out.empty() && (out.back() == '\r' || out.back() == ' ' || out.back() == '\t'))
            out.pop_back();
        return !out.empty();
    } catch (...) {
        out.clear();
        return false;
    }
}

std::vector<std::string> words(const std::string& text) {
    std::vector<std::string> out;
    std::istringstream in(text);
    std::string w;
    while (in >> w) out.push_back(w);
    return out;
}

std::vector<std::string> listDir(const std::string& dir, std::size_t max_entries,
                                 bool& truncated) {
    std::vector<std::string> out;
    out.reserve(std::min(max_entries, TunableDiscovery::kMaxDirectoryEntries));
    std::unique_ptr<DIR, int (*)(DIR*)> d(::opendir(dir.c_str()), ::closedir);
    if (!d) return out;
    while (const dirent* e = ::readdir(d.get())) {
        const std::string_view name(e->d_name);
        if (name == "." || name == "..") continue;
        if (out.size() >= max_entries) {
            truncated = true;
            break;
        }
        out.emplace_back(name);
    }
    std::sort(out.begin(), out.end());
    return out;
}

// Discovery traverses every directory on the mounted kernel filesystem. Symlinks,
// non-regular files, nested filesystem boundaries, and resource/work budgets are
// handled by walkTree; names alone never cause a subtree to be skipped.
bool acceptAllDirs(std::string_view) { return true; }


} // namespace

bool TunableDiscovery::kernelWritable(const std::string& path) noexcept {
    struct stat st {};
    // Match walkTree's no-symlink rule for vendor-declared paths as well.
    // Following a leaf symlink could otherwise misclassify a non-kernel file
    // as a writable tunable.
    if (::lstat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) return false;
    // Root passes access(W_OK) everywhere (CAP_DAC_OVERRIDE). The kernel marks
    // nodes that actually accept writes with a write permission bit.
    if ((st.st_mode & 0222) == 0) return false;
    return ::access(path.c_str(), R_OK | W_OK) == 0;
}

std::string_view TunableDiscovery::strip(std::string_view path) const noexcept {
    if (!prefix_.empty() && startsWith(path, prefix_)) return path.substr(prefix_.size());
    return path;
}

ResourceDomain TunableDiscovery::domainFor(std::string_view p) const noexcept {
    p = strip(p);
    const std::string_view leaf = leafOf(p);

    // Classify by resource topology and attribute semantics rather than requiring
    // a particular mountpoint. Paths may be rooted under alternate procfs/sysfs
    // mounts or fixture prefixes.
    const auto hasSegment = [p](std::string_view segment) noexcept {
        return containsText(p, segment);
    };
    if (hasSegment("/sys/vm/") || hasSegment("/vm/") ||
        leaf == "swappiness" || leaf == "overcommit_memory" ||
        leaf == "dirty_ratio" || leaf == "dirty_background_ratio")
        return ResourceDomain::Memory;
    if (hasSegment("/walt/") || hasSegment("/core_ctl/"))
        return ResourceDomain::Scheduler;
    if (hasSegment("/kernel/") && startsWith(leaf, "sched"))
        return ResourceDomain::Scheduler;

    if (hasSegment("/cpufreq/")) {
        // A direct attribute under policyN or cpuN/cpufreq is a frequency
        // control; deeper governor-specific attributes are scheduler controls.
        const auto marker = p.find("/cpufreq/");
        const std::string_view rest = p.substr(marker + 9U);
        const auto slash = rest.find('/');
        const std::string_view after = slash == std::string_view::npos
            ? rest : rest.substr(slash + 1U);
        return after.find('/') == std::string_view::npos
            ? ResourceDomain::CpuFreq : ResourceDomain::Scheduler;
    }
    if (hasSegment("/block/") && hasSegment("/queue/"))
        return ResourceDomain::Io;
    if (hasSegment("/devfreq/") || hasSegment("/kgsl/") ||
        hasSegment("/mali/") || containsText(leaf, "gpu") ||
        containsText(leaf, "gpuclk"))
        return ResourceDomain::Gpu;
    if (hasSegment("/cpuctl/") || hasSegment("/stune/") ||
        (hasSegment("/cgroup/") && startsWith(leaf, "cpu.")))
        return ResourceDomain::UClamp;
    if (hasSegment("/cpuset/") ||
        (hasSegment("/cgroup/") && startsWith(leaf, "cpuset.")))
        return ResourceDomain::CpuSet;
    return ResourceDomain::Vendor;
}

std::string TunableDiscovery::nameFor(std::string_view raw) const {
    const std::string_view p = strip(raw);
    // Identify the sysctl subtree structurally so paths under an
    // alternate fixture or mount prefix retain stable resource names.
    const auto sysctl = p.find("/proc/sys/");
    if (sysctl != std::string_view::npos) {
        std::string name(p.substr(sysctl + 10U));
        std::replace(name.begin(), name.end(), '/', '.');
        return name;  // vm.swappiness, kernel.sched_x, walt.y
    }
    // Produce stable per-device identities from either a /sys/block alias or
    // the real /sys/devices/.../block/<device>/queue/... hierarchy. The device
    // name is discovered from the path; no particular block name is assumed.
    std::string_view rest;
    const auto block = p.find("/block/");
    if (block != std::string_view::npos && containsText(p, "/queue/"))
        rest = p.substr(block + 7U);
    if (!rest.empty()) {
        const auto slash = rest.find('/');
        if (slash != std::string_view::npos) {
            const std::string_view device = rest.substr(0, slash);
            std::string tail(rest.substr(slash + 1U));
            if (startsWith(tail, "queue/")) tail.erase(0, 6);
            std::replace(tail.begin(), tail.end(), '/', '.');
            return std::string(device) + ":" + tail;
        }
    }
    return std::string(p);
}

bool TunableDiscovery::addTunable(EnvironmentCapabilityMatrix& matrix, const std::string& path,
                                  const VendorKnowledge& vendor,
                                  TunableDiscoveryStats& stats) const noexcept {
    try {
        if (matrix.resources.size() >= kMaxTunables) {
            stats.truncated = true;
            return false;
        }
        ++stats.walked;
        if (isFatalSurface(path)) {
            ++stats.fatal_skipped;
            return false;
        }
        if (!kernelWritable(path)) return false;

        const ResourceDomain domain = domainFor(path);
        // Unknown writable kernel surfaces are not discarded merely because
        // their subsystem or name is absent from a catalog. They enter the
        // generic Vendor domain and still pass the fatal-surface and value
        // validation path before any mutation can occur.
        const bool unclaimed = domain == ResourceDomain::Vendor &&
                               vendor.find(VendorKnowledge::canonicalPath(path)) == nullptr &&
                               vendor.find(path) == nullptr;
        if (unclaimed) ++stats.unclaimed;

        std::string value;
        if (!readFirstLine(path, value)) return false;  // write-only or empty

        for (const auto& existing : matrix.resources) {
            if (existing.path == path) return false;
        }

        ResourceCapability cap;
        cap.domain = domain;
        cap.name = nameFor(path);
        cap.path = path;
        cap.exists = true;
        cap.readable = true;
        cap.writable = true;
        cap.permission_granted = true;
        cap.runtime_verified = true;
        cap.policy_authorized =
            classifySurface(cap.domain, cap.name, cap.path) == SurfaceClass::Admissible;
        cap.mutation_ready = cap.policy_authorized;
        matrix.resources.push_back(std::move(cap));
        ++stats.added;
        return true;
    } catch (...) {
        return false;
    }
}

// Breadth-first walk. Shallow nodes are visited before deep ones, so the visit
// budget can only truncate the deepest levels, and it reports that explicitly.
// Symlinks are never followed, and a subtree on another filesystem (a nested
// mount such as debugfs) is never entered.
void TunableDiscovery::walkTree(EnvironmentCapabilityMatrix& matrix, const VendorKnowledge& vendor,
                                const std::string& root, std::size_t max_depth,
                                bool (*accept_dir)(std::string_view),
                                TunableDiscoveryStats& stats) const noexcept {
    try {
        struct stat root_st {};
        if (::lstat(root.c_str(), &root_st) != 0 || !S_ISDIR(root_st.st_mode)) return;

        std::deque<std::pair<std::string, std::size_t>> queue;
        queue.emplace_back(root, 0U);

        while (!queue.empty()) {
            // One budget is shared by every mount walk in this discovery run.
            if (stats.visited_entries >= kMaxVisits ||
                matrix.resources.size() >= kMaxTunables) {
                stats.truncated = true;
                return;
            }
            const std::string dir = queue.front().first;
            const std::size_t depth = queue.front().second;
            queue.pop_front();

            const std::size_t remaining = kMaxVisits - stats.visited_entries;
            const std::size_t dir_budget = std::min(remaining, TunableDiscovery::kMaxDirectoryEntries);
            bool dir_truncated = false;
            const auto entries = listDir(dir, dir_budget, dir_truncated);
            for (const auto& name : entries) {
                ++stats.visited_entries;
                const std::string full = dir + "/" + name;
                struct stat st {};
                if (::lstat(full.c_str(), &st) != 0) continue;
                if (S_ISLNK(st.st_mode)) continue;
                if (S_ISDIR(st.st_mode)) {
                    if (depth + 1U <= max_depth && st.st_dev == root_st.st_dev &&
                        accept_dir(name)) {
                        queue.emplace_back(full, depth + 1U);
                    }
                    continue;
                }
                if (!S_ISREG(st.st_mode)) continue;
                (void)addTunable(matrix, full, vendor, stats);
                if (matrix.resources.size() >= kMaxTunables) {
                    stats.truncated = true;
                    return;
                }
            }
            if (dir_truncated) {
                stats.truncated = true;
                return;
            }
        }
    } catch (...) {
        stats.truncated = true;
    }
}

TunableDiscoveryStats TunableDiscovery::discover(EnvironmentCapabilityMatrix& matrix,
                                                 const VendorKnowledge& vendor) const noexcept {
    TunableDiscoveryStats stats;
    try {
        if (matrix.resources.size() >= kMaxTunables) {
            stats.truncated = true;
            return stats;
        }
        // The kernel mount table is the inventory of tunable filesystems. Nothing
        // here names a directory: a root exists only if the kernel mounted it.
        for (const KernelMount& mount : readKernelMounts(prefix_)) {
            if (stats.truncated) break;
            switch (mount.kind) {
                case KernelFsKind::Procfs:
                    walkTree(matrix, vendor, mount.mountpoint + "/sys", kSysctlDepth,
                             acceptAllDirs, stats);
                    break;
                case KernelFsKind::Sysfs:
                    walkTree(matrix, vendor, mount.mountpoint, kSysfsDepth, acceptAllDirs, stats);
                    break;
                case KernelFsKind::Cgroup:
                    walkTree(matrix, vendor, mount.mountpoint, kCgroupDepth, acceptAllDirs,
                             stats);
                    break;
                case KernelFsKind::Other:
                    break;
            }
        }

        // Every node the vendor itself configures, wherever it lives. These are
        // declared by the device's own configuration, so they are admitted here.
        for (const auto& knob : vendor.knobs()) {
            if (stats.visited_entries >= kMaxVisits ||
                matrix.resources.size() >= kMaxTunables) {
                stats.truncated = true;
                break;
            }
            ++stats.visited_entries;
            if (addTunable(matrix, knob.path, vendor, stats)) ++stats.vendor_paths;
            if (stats.truncated) break;
        }

        enrich(matrix, vendor);
    } catch (...) {
        stats.truncated = true;
    }
    return stats;
}

void TunableDiscovery::enrich(EnvironmentCapabilityMatrix& matrix,
                              const VendorKnowledge& vendor) const noexcept {
    for (auto& cap : matrix.resources) {
        try {
            if (!cap.mutation_ready || cap.path.empty() || cap.path.front() != '/') continue;

            // Vendor values: look up by canonical path first, then as written.
            if (cap.vendor_values.empty()) {
                const VendorKnob* knob = vendor.find(VendorKnowledge::canonicalPath(cap.path));
                if (knob == nullptr) knob = vendor.find(cap.path);
                if (knob != nullptr) cap.vendor_values = knob->values;
            }

            if (!cap.options.empty()) continue;
            std::string live;
            if (!readFirstLine(cap.path, live)) continue;

            // 1) Selector line: "a [b] c" advertises every accepted token.
            const auto tokens = words(live);
            bool bracket = false;
            for (const auto& t : tokens) bracket = bracket || (t.size() >= 3U && t.front() == '[' && t.back() == ']');
            if (bracket && tokens.size() > 1U) {
                for (auto t : tokens) {
                    if (t.size() >= 3U && t.front() == '[' && t.back() == ']') t = t.substr(1, t.size() - 2U);
                    cap.options.push_back(t);
                }
                continue;
            }

            // 2) Sibling available_* list published by the same driver.
            const std::string dir(dirOf(cap.path));
            const std::string leaf(leafOf(cap.path));
            const bool governor_like = containsText(leaf, "governor") || containsText(leaf, "policy");
            // Level indices (e.g. *pwrlevel) are not frequencies: only nodes
            // whose own name is a frequency/clock pair with a frequency list.
            const bool scale_like = containsText(leaf, "freq") || containsText(leaf, "clk");
            if (!governor_like && !scale_like) continue;
            bool siblings_truncated = false;
            const auto siblings = listDir(dir, TunableDiscovery::kMaxDirectoryEntries, siblings_truncated);
            if (siblings_truncated) continue;  // incomplete option inventories are not evidence
            for (const auto& sibling : siblings) {
                if (!containsText(sibling, "available") || sibling == leaf) continue;
                const bool match_governor = governor_like && (containsText(sibling, "governor") ||
                                                              containsText(sibling, "polic"));
                const bool match_scale = scale_like && (containsText(sibling, "freq") ||
                                                        containsText(sibling, "clk"));
                if (!match_governor && !match_scale) continue;
                std::string list(TunableDiscovery::kMaxOptionListBytes + 1U, '\0');
                {
                    std::ifstream file(dir + "/" + sibling);
                    if (!file) continue;
                    file.read(list.data(), static_cast<std::streamsize>(list.size()));
                    const std::streamsize count = file.gcount();
                    if (count < 0 || static_cast<std::size_t>(count) > TunableDiscovery::kMaxOptionListBytes) {
                        continue;  // oversized options cannot be trusted as a complete list
                    }
                    list.resize(static_cast<std::size_t>(count));
                }
                auto opts = words(list);
                if (opts.empty()) continue;
                if (match_scale) {
                    std::vector<long long> nums;
                    for (const auto& o : opts) {
                        char* end = nullptr;
                        const long long v = std::strtoll(o.c_str(), &end, 10);
                        if (end != o.c_str() && *end == '\0') nums.push_back(v);
                    }
                    if (nums.size() < 2U) continue;
                    std::sort(nums.begin(), nums.end());
                    nums.erase(std::unique(nums.begin(), nums.end()), nums.end());
                    cap.options.clear();
                    for (const auto v : nums) cap.options.push_back(std::to_string(v));
                    cap.ordered_scale = true;
                } else {
                    cap.options = opts;
                }
                break;
            }
        } catch (...) {
        }
    }
}

} // namespace coreflow
