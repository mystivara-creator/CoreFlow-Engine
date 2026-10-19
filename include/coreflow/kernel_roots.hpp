#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace coreflow {

// Kernel filesystems that can expose tunables. Classification is made from the
// mounted filesystem type alone. No parameter name or vendor path is involved,
// so the set of searched roots is whatever the kernel actually mounted.
enum class KernelFsKind : std::uint8_t {
    Procfs = 0,  // sysctl tree lives under <mount>/sys
    Sysfs,       // device and driver attribute tree
    Cgroup,      // cgroup v1 controller mounts and cgroup v2 hierarchy
    Other,       // debugfs, tracefs, pstore, bpf, configfs, ... (never walked)
};

struct KernelMount {
    std::string mountpoint;
    std::string fstype;
    KernelFsKind kind{KernelFsKind::Other};
};

KernelFsKind classifyFilesystem(const std::string& fstype) noexcept;

// Parses text in /proc/mounts format. Octal escapes such as \040 in the
// mountpoint are decoded. Lines without an absolute mountpoint are ignored.
std::vector<KernelMount> parseMountTable(const std::string& text);

// Reads <prefix>/proc/mounts. An unreadable table yields an empty result, so
// discovery fails closed instead of falling back to guessed paths.
std::vector<KernelMount> readKernelMounts(const std::string& prefix);

// Resolve a path relative to the currently mounted procfs/sysfs root.
// Empty result means the filesystem root could not be discovered.
std::string kernelFilesystemPath(KernelFsKind kind, const std::string& relative = {});

bool hasFilesystemType(const std::vector<KernelMount>& mounts,
                       const std::string& fstype) noexcept;

} // namespace coreflow
