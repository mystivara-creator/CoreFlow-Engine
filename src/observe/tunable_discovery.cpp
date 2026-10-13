#include "coreflow/tunable_discovery.hpp"

#include "coreflow/consultation.hpp"
#include "coreflow/fatal_surface.hpp"

#include <algorithm>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <memory>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_set>

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

bool isDirectory(const std::string& path) noexcept {
    struct stat st {};
    return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool acceptNone(std::string_view) { return false; }
bool acceptAll(std::string_view name) { return name != "stats" && name != "power" && name != "subsystem" && name != "device"; }
bool acceptCgroupGroup(std::string_view name) { return !startsWith(name, "cgroup.") && name != "uid_0" && !startsWith(name, "uid_") && !startsWith(name, "pid_"); }

} // namespace

bool TunableDiscovery::kernelWritable(const std::string& path) noexcept {
    struct stat st {};
    if (::stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) return false;
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
                                  TunableDiscoveryStats& stats) const noexcept {
    try {
        if (matrix.resources.size() >= kMaxTunables) return false;
        ++stats.walked;
        if (isFatalSurface(path)) {
            ++stats.fatal_skipped;
            return false;
        }
        if (!kernelWritable(path)) return false;
        std::string value;
        if (!readFirstLine(path, value)) return false;  // write-only or empty

        for (const auto& existing : matrix.resources) {
            if (existing.path == path) return false;
        }

        ResourceCapability cap;
        cap.domain = domainFor(path);
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

void TunableDiscovery::walk(EnvironmentCapabilityMatrix& matrix, const std::string& dir,
                            std::size_t depth, TunableDiscoveryStats& stats,
                            bool (*accept_dir)(std::string_view)) const noexcept {
    try {
        for (const auto& name : listDir(dir)) {
            const std::string full = dir + "/" + name;
            if (isDirectory(full)) {
                if (depth > 0U && accept_dir(name)) walk(matrix, full, depth - 1U, stats, accept_dir);
                continue;
            }
            (void)addTunable(matrix, full, stats);
        }
    } catch (...) {
    }
}

TunableDiscoveryStats TunableDiscovery::discover(EnvironmentCapabilityMatrix& matrix,
                                                 const VendorKnowledge& vendor) const noexcept {
    TunableDiscoveryStats stats;
    try {
        const std::string& r = prefix_;

        walk(matrix, r + "/proc/sys/vm", 0, stats, acceptNone);
        walk(matrix, r + "/proc/sys/kernel", 0, stats, acceptNone);
        walk(matrix, r + "/proc/sys/walt", 2, stats, acceptAll);

        // cpufreq policies: policy nodes + governor tunable subdirectories.
        const std::string cpufreq = r + "/sys/devices/system/cpu/cpufreq";
        for (const auto& name : listDir(cpufreq)) {
            if (startsWith(name, "policy")) walk(matrix, cpufreq + "/" + name, 1, stats, acceptAll);
        }
        // Per-CPU core_ctl (vendor hotplug governor; its "enable"/limits are
        // tunables, while cpuN/online stays fatal).
        const std::string cpus = r + "/sys/devices/system/cpu";
        for (const auto& name : listDir(cpus)) {
            if (startsWith(name, "cpu") && name.size() > 3U &&
                name.find_first_not_of("0123456789", 3) == std::string::npos) {
                walk(matrix, cpus + "/" + name + "/core_ctl", 0, stats, acceptNone);
            }
        }

        // Block queues (+ I/O scheduler tunables). Loop/ram devices are noise.
        const std::string block = r + "/sys/block";
        for (const auto& name : listDir(block)) {
            if (startsWith(name, "loop") || startsWith(name, "ram")) continue;
            walk(matrix, block + "/" + name + "/queue", 1, stats, acceptAll);
        }

        // GPU / bus clock domains.
        const std::string devfreq = r + "/sys/class/devfreq";
        for (const auto& name : listDir(devfreq)) walk(matrix, devfreq + "/" + name, 0, stats, acceptNone);
        const std::string kgsl = r + "/sys/class/kgsl";
        for (const auto& name : listDir(kgsl)) walk(matrix, kgsl + "/" + name, 1, stats, acceptAll);
        walk(matrix, r + "/sys/class/misc/mali0/device", 0, stats, acceptNone);
        walk(matrix, r + "/sys/kernel/gpu", 0, stats, acceptNone);

        // cpu / uclamp / schedtune / cpuset cgroups (root + one group level).
        walk(matrix, r + "/dev/cpuctl", 1, stats, acceptCgroupGroup);
        walk(matrix, r + "/dev/stune", 1, stats, acceptCgroupGroup);
        walk(matrix, r + "/dev/cpuset", 1, stats, acceptCgroupGroup);
        walk(matrix, r + "/sys/fs/cgroup", 1, stats, acceptCgroupGroup);

        // Every node the vendor itself configures, wherever it lives.
        for (const auto& knob : vendor.knobs()) {
            if (addTunable(matrix, knob.path, stats)) ++stats.vendor_paths;
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
