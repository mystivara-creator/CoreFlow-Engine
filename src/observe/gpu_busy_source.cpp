#include "coreflow/gpu_busy_source.hpp"

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <array>
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include "coreflow/kernel_roots.hpp"

namespace coreflow::observe {

void GpuBusySource::discoverDefaultRoots() noexcept {
    try {
    roots_.clear();

    // Resolve the sysfs mount from the live mount table instead of assuming
    // that sysfs is mounted at /sys. Search the canonical device model rather
    // than enumerating vendor-specific class aliases (KGSL, Mali, etc.).
    const auto mounts = readKernelMounts("");
    std::vector<std::string> sysfs_roots;
    for (const auto& mount : mounts) {
        if (mount.kind == KernelFsKind::Sysfs) sysfs_roots.push_back(mount.mountpoint);
    }

    constexpr std::size_t kMaxDirectories = 12000U;
    constexpr std::size_t kMaxDepth = 24U;
    std::size_t visited = 0U;
    const auto containsGpuIdentity = [](const std::string& path) {
        std::string normalized(path);
        std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                       [](unsigned char ch) {
                           return static_cast<char>(std::tolower(ch));
                       });
        const auto contains = [&normalized](const char* token) {
            return normalized.find(token) != std::string::npos;
        };
        // Bandwidth/DDR/bus devfreq devices can also expose `load`; their
        // ratio is not a GPU busy signal. Do not use generic "devfreq" as
        // identity because it matches every devfreq device.
        if (contains("ddr") || contains("bw") || contains("bus") ||
            contains("mem")) {
            return false;
        }
        // Universal Linux & Android GPU architectures across all vendors:
        // Qualcomm (kgsl, adreno), ARM (mali, valhall, bifrost, immortalis),
        // Imagination (pvr, powervr, rogue), Samsung Exynos / AMD (xclipse, sgpu, rdna),
        // NXP / VeriSilicon (galcore, etnaviv), Intel (i915, xe), Broadcom (v3d, vc4).
        return contains("gpu") || contains("kgsl") || contains("adreno") ||
               contains("mali") || contains("valhall") || contains("bifrost") ||
               contains("immortalis") || contains("pvr") || contains("powervr") ||
               contains("xclipse") || contains("sgpu") || contains("rdna") ||
               contains("galcore") || contains("etnaviv") || contains("i915") ||
               contains("xe") || contains("v3d") || contains("vc4");
    };

    const auto walk = [&](const auto& self, const std::string& path,
                          std::size_t depth) -> void {
        if (depth > kMaxDepth || visited >= kMaxDirectories) return;
        ++visited;

        // A load node is accepted only under a path that carries a GPU/devfreq
        // identity. Reading remains in poll(), and no mutation is performed.
        if (containsGpuIdentity(path)) {
            const std::string load_path = path + "/load";
            if (::access(load_path.c_str(), R_OK) == 0) roots_.push_back(path);
        }

        DIR* dir = ::opendir(path.c_str());
        if (!dir) return;
        while (visited < kMaxDirectories) {
            dirent* entry = ::readdir(dir);
            if (!entry) break;
            const std::string name(entry->d_name);
            if (name == "." || name == "..") continue;
            const std::string child = path + "/" + name;
            struct stat info {};
            if (::lstat(child.c_str(), &info) != 0 || !S_ISDIR(info.st_mode)) continue;
            self(self, child, depth + 1U);
        }
        ::closedir(dir);
    };

    for (const auto& root : sysfs_roots) {
        // /devices is the canonical device model in sysfs. The mountpoint is
        // discovered; no absolute /sys/devices or vendor-specific path is used.
        const std::string devices = root + "/devices";
        walk(walk, devices, 0U);
    }

    std::sort(roots_.begin(), roots_.end());
    roots_.erase(std::unique(roots_.begin(), roots_.end()), roots_.end());
    active_root_.clear();
    } catch (...) {
        roots_.clear();
        active_root_.clear();
    }
}

bool GpuBusySource::poll(double& ratio_out) noexcept {
    try {
    const auto readRootLoad = [&](const std::string& root) -> bool {
        const std::string path = root + "/load";
        const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
        if (fd < 0) return false;
        std::array<char, 64> buffer{};
        const ssize_t n = ::read(fd, buffer.data(), buffer.size() - 1U);
        ::close(fd);
        if (n <= 0) return false;
        const double percent = parseDevfreqLoadPercent(
            std::string_view(buffer.data(), static_cast<std::size_t>(n)));
        if (percent < 0.0) return false;
        ratio_out = percent / 100.0;
        active_root_ = root;
        return true;
    };

    if (!active_root_.empty() &&
        std::find(roots_.begin(), roots_.end(), active_root_) != roots_.end() &&
        readRootLoad(active_root_)) {
        return true;
    }

    for (const auto& root : roots_) {
        if (root == active_root_) continue;
        if (readRootLoad(root)) return true;
    }
    active_root_.clear();
    return false;
    } catch (...) {
        return false;
    }
}

} // namespace coreflow::observe
