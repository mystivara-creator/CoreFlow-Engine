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

bool hasFilesystemType(const std::vector<KernelMount>& mounts,
                       const std::string& fstype) noexcept {
    for (const auto& mount : mounts) {
        if (mount.fstype == fstype) return true;
    }
    return false;
}

} // namespace coreflow
