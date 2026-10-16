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
        std::ifstream file(path);
        if (!file) return false;
        std::getline(file, out);
        while (!out.empty() && (out.back() == '\r' || out.back() == ' ' || out.back() == '\t'))
            out.pop_back();
        return !out.empty();
    } catch (...) {
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

std::vector<std::string> listDir(const std::string& dir) {
    std::vector<std::string> out;
    std::unique_ptr<DIR, int (*)(DIR*)> d(::opendir(dir.c_str()), ::closedir);
    if (!d) return out;
    while (const dirent* e = ::readdir(d.get())) {
        const std::string name = e->d_name;
        if (name == "." || name == "..") continue;
        out.push_back(name);
    }
    std::sort(out.begin(), out.end());
    return out;
}

// Directory names that are kernel device-model plumbing rather than tunables.
// These describe the device model, not a parameter, so they are not a tuning list.
bool acceptAllDirs(std::string_view) { return true; }

bool acceptSysfsDir(std::string_view name) {
    return name != "stats" && name != "power" && name != "subsystem" && name != "device" &&
           name != "driver" && name != "holders" && name != "slaves" && name != "firmware_node";
}

bool acceptCgroupGroup(std::string_view name) {
    return !startsWith(name, "cgroup.") && !startsWith(name, "uid_") && !startsWith(name, "pid_");
}

// A node is vendor-declared when the vendor's own init/power configuration
// writes it. This is the only way an unrecognised subsystem is admitted.
bool vendorDeclares(const VendorKnowledge& vendor, const std::string& path) noexcept {
    return vendor.find(VendorKnowledge::canonicalPath(path)) != nullptr ||
           vendor.find(path) != nullptr;
}

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
    if (startsWith(p, "/proc/sys/vm/")) return ResourceDomain::Memory;
    if (startsWith(p, "/proc/sys/walt/")) return ResourceDomain::Scheduler;
    if (startsWith(p, "/proc/sys/kernel/"))
        return startsWith(leaf, "sched") ? ResourceDomain::Scheduler : ResourceDomain::Vendor;
    if (startsWith(p, "/sys/devices/system/cpu/cpufreq/policy") ||
        (startsWith(p, "/sys/devices/system/cpu/cpu") && containsText(p, "/cpufreq/"))) {
        // policyN/<node> is cpufreq; policyN/<governor>/<node> is a governor
        // (scheduler-side) tunable.
        const std::string_view rest = p.substr(p.find("/cpufreq/") + 9U);
        const auto first = rest.find('/');
        const std::string_view after = first == std::string_view::npos ? rest : rest.substr(first + 1U);
        return after.find('/') == std::string_view::npos ? ResourceDomain::CpuFreq
                                                         : ResourceDomain::Scheduler;
    }
    if (startsWith(p, "/sys/devices/system/cpu/") && containsText(p, "/core_ctl/"))
        return ResourceDomain::Scheduler;
    if (startsWith(p, "/sys/block/")) return ResourceDomain::Io;
    if (startsWith(p, "/sys/class/devfreq/") || startsWith(p, "/sys/class/kgsl/") ||
        startsWith(p, "/sys/class/misc/mali") || startsWith(p, "/sys/kernel/gpu/") ||
        containsText(p, "kgsl") || containsText(p, "mali") || containsText(p, "/devfreq/"))
        return ResourceDomain::Gpu;
    if (startsWith(p, "/dev/cpuctl/") || startsWith(p, "/dev/stune/") ||
        (startsWith(p, "/sys/fs/cgroup/") && startsWith(leaf, "cpu.")))
        return ResourceDomain::UClamp;
    if (startsWith(p, "/dev/cpuset/") ||
        (startsWith(p, "/sys/fs/cgroup/") && startsWith(leaf, "cpuset.")))
        return ResourceDomain::CpuSet;
    return ResourceDomain::Vendor;
}

std::string TunableDiscovery::nameFor(std::string_view raw) const {
    const std::string_view p = strip(raw);
    if (startsWith(p, "/proc/sys/")) {
        std::string name(p.substr(10));
        std::replace(name.begin(), name.end(), '/', '.');
        return name;  // vm.swappiness, kernel.sched_x, walt.y
    }
    if (startsWith(p, "/sys/block/")) {
        const std::string_view rest = p.substr(11);
        const auto slash = rest.find('/');
        if (slash != std::string_view::npos) {
            std::string tail(rest.substr(slash + 1U));
            if (startsWith(tail, "queue/")) tail.erase(0, 6);
            std::replace(tail.begin(), tail.end(), '/', '.');
            return std::string(rest.substr(0, slash)) + ":" + tail;  // sda:read_ahead_kb
        }
    }
    return std::string(p);
}

bool TunableDiscovery::addTunable(EnvironmentCapabilityMatrix& matrix, const std::string& path,
                                  const VendorKnowledge& vendor,
                                  TunableDiscoveryStats& stats) const noexcept {
    try {
        if (matrix.resources.size() >= kMaxTunables) return false;
        ++stats.walked;
        if (isFatalSurface(path)) {
            ++stats.fatal_skipped;
            return false;
        }
        if (!kernelWritable(path)) return false;

        const ResourceDomain domain = domainFor(path);
        // Admission: a subsystem the kernel exposes, or a node the vendor declares.
        // Anything else that is writable is recorded as unclaimed and not admitted.
        if (domain == ResourceDomain::Vendor && !vendorDeclares(vendor, path)) {
            ++stats.unclaimed;
            return false;
        }

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
        std::size_t visits = 0;

        while (!queue.empty()) {
            const std::string dir = queue.front().first;
            const std::size_t depth = queue.front().second;
            queue.pop_front();

            for (const auto& name : listDir(dir)) {
                if (++visits > kMaxVisits) {
                    stats.truncated = true;
                    return;
                }
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
            }
        }
    } catch (...) {
    }
}

TunableDiscoveryStats TunableDiscovery::discover(EnvironmentCapabilityMatrix& matrix,
                                                 const VendorKnowledge& vendor) const noexcept {
    TunableDiscoveryStats stats;
    try {
        // The kernel mount table is the inventory of tunable filesystems. Nothing
        // here names a directory: a root exists only if the kernel mounted it.
        for (const KernelMount& mount : readKernelMounts(prefix_)) {
            switch (mount.kind) {
                case KernelFsKind::Procfs:
                    walkTree(matrix, vendor, mount.mountpoint + "/sys", kSysctlDepth,
                             acceptAllDirs, stats);
                    break;
                case KernelFsKind::Sysfs:
                    walkTree(matrix, vendor, mount.mountpoint, kSysfsDepth, acceptSysfsDir, stats);
                    break;
                case KernelFsKind::Cgroup:
                    walkTree(matrix, vendor, mount.mountpoint, kCgroupDepth, acceptCgroupGroup,
                             stats);
                    break;
                case KernelFsKind::Other:
                    break;
            }
        }

        // Every node the vendor itself configures, wherever it lives. These are
        // declared by the device's own configuration, so they are admitted here.
        for (const auto& knob : vendor.knobs()) {
            if (addTunable(matrix, knob.path, vendor, stats)) ++stats.vendor_paths;
        }

        enrich(matrix, vendor);
    } catch (...) {
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
            for (const auto& sibling : listDir(dir)) {
                if (!containsText(sibling, "available") || sibling == leaf) continue;
                const bool match_governor = governor_like && (containsText(sibling, "governor") ||
                                                              containsText(sibling, "polic"));
                const bool match_scale = scale_like && (containsText(sibling, "freq") ||
                                                        containsText(sibling, "clk"));
                if (!match_governor && !match_scale) continue;
                std::string list;
                {
                    std::ifstream file(dir + "/" + sibling);
                    std::ostringstream all;
                    all << file.rdbuf();
                    list = all.str();
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
