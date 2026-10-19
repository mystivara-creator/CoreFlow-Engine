#include "coreflow/kernel_roots.hpp"

#include <fstream>
#include <sstream>
#include <utility>

namespace coreflow {
namespace {

// /proc/mounts escapes space, tab, newline and backslash as three octal digits.
std::string decodeMountField(const std::string& raw) {
    std::string out;
    out.reserve(raw.size());
    for (std::size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '\\' && i + 3U < raw.size() &&
            raw[i + 1U] >= '0' && raw[i + 1U] <= '7' &&
            raw[i + 2U] >= '0' && raw[i + 2U] <= '7' &&
            raw[i + 3U] >= '0' && raw[i + 3U] <= '7') {
            const int value = (raw[i + 1U] - '0') * 64 + (raw[i + 2U] - '0') * 8 +
                              (raw[i + 3U] - '0');
            out.push_back(static_cast<char>(value));
            i += 3U;
            continue;
        }
        out.push_back(raw[i]);
    }
    return out;
}

} // namespace

KernelFsKind classifyFilesystem(const std::string& fstype) noexcept {
    if (fstype == "proc") return KernelFsKind::Procfs;
    if (fstype == "sysfs") return KernelFsKind::Sysfs;
    if (fstype == "cgroup" || fstype == "cgroup2") return KernelFsKind::Cgroup;
    return KernelFsKind::Other;
}

std::vector<KernelMount> parseMountTable(const std::string& text) {
    std::vector<KernelMount> out;
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line)) {
        std::istringstream fields(line);
        std::string device;
        std::string mountpoint;
        std::string fstype;
        if (!(fields >> device >> mountpoint >> fstype)) continue;
        KernelMount mount;
        mount.mountpoint = decodeMountField(mountpoint);
        if (mount.mountpoint.empty() || mount.mountpoint.front() != '/') continue;
        mount.fstype = fstype;
        mount.kind = classifyFilesystem(fstype);
        out.push_back(std::move(mount));
    }
    return out;
}

std::vector<KernelMount> readKernelMounts(const std::string& prefix) {
    std::ifstream file(prefix + "/proc/mounts");
    if (!file) return {};
    std::ostringstream all;
    all << file.rdbuf();
    return parseMountTable(all.str());
}


std::string kernelFilesystemPath(KernelFsKind kind, const std::string& relative) {
    // Mount topology is stable during normal operation. Cache the bootstrap
    // table so per-sample observers do not repeatedly parse /proc/mounts.
    static const std::vector<KernelMount> mounts = readKernelMounts("");

    // Accept only a relative path composed of ordinary path components.
    if (!relative.empty()) {
        if (relative.front() == '/' || relative.back() == '/') return {};
        std::istringstream parts(relative);
        std::string component;
        while (std::getline(parts, component, '/')) {
            if (component.empty() || component == "." || component == "..")
                return {};
        }
    }

    // Prefer conventional primary mountpoints when multiple instances of the
    // same filesystem type exist; otherwise use the first discovered mount.
    const char* preferred = nullptr;
    if (kind == KernelFsKind::Procfs) preferred = "/proc";
    else if (kind == KernelFsKind::Sysfs) preferred = "/sys";

    const KernelMount* selected = nullptr;
    for (const auto& mount : mounts) {
        if (mount.kind != kind) continue;
        if (!selected) selected = &mount;
        if (preferred && mount.mountpoint == preferred) {
            selected = &mount;
            break;
        }
    }

    if (!selected) return {};
    if (relative.empty()) return selected->mountpoint;
    return selected->mountpoint + "/" + relative;
}

bool hasFilesystemType(const std::vector<KernelMount>& mounts,
                       const std::string& fstype) noexcept {
    for (const auto& mount : mounts) {
        if (mount.fstype == fstype) return true;
    }
    return false;
}

} // namespace coreflow
