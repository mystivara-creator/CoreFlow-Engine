#include "coreflow/discovery/classification.hpp"

#include <algorithm>
#include <cctype>
#include <vector>
#include <sys/stat.h>

namespace coreflow {
namespace {

std::vector<std::string> pathParts(const std::string& path) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start < path.size()) {
        while (start < path.size() && path[start] == '/') ++start;
        if (start == path.size()) break;
        const auto slash = path.find('/', start);
        const auto end = slash == std::string::npos ? path.size() : slash;
        parts.emplace_back(path.substr(start, end - start));
        start = end;
    }
    return parts;
}

bool startsWith(std::string_view text, std::string_view prefix) noexcept {
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

} // namespace

bool isProtectedKernelControl(const std::string& path) {
    std::string lower = path;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    // These surfaces can trigger power loss, safety-policy bypass, hardware
    // stress or irreversible device actions. Discover them as protected rather
    // than considering a writable bit or a numeric companion range sufficient.
    static const char* const protected_fragments[] = {
        "/thermal/", "/class/thermal/", "/power_supply/", "/class/power_supply/",
        "/regulator/", "/watchdog/", "/reset/", "/reset_reason/", "/debug/",
        "/tracing/", "/firmware/", "/security/", "/bcl/", "/battery/", "/charger/",
        "/crash/", "/panic/", "/lockup/", "/kexec/", "/reboot/", "/cooling_device/"
    };
    for (const auto* fragment : protected_fragments) {
        if (lower.find(fragment) != std::string::npos) return true;
    }
    const auto slash = lower.find_last_of('/');
    const auto name = lower.substr(slash == std::string::npos ? 0 : slash + 1);
    static const char* const protected_attributes[] = {
        "voltage", "current_limit", "current_now", "charge", "trip_point",
        "emergency", "watchdog", "reset", "reboot", "panic", "crash",
        "lockup", "battery", "charger", "regulator", "cur_state", "target_state"
    };
    for (const auto* fragment : protected_attributes) {
        if (name.find(fragment) != std::string::npos) return true;
    }
    return false;
}

std::string classifyKernelDomain(const std::string& path, std::string_view active_governor) {
    if (isProtectedKernelControl(path)) return std::string(kDomainProtected);

    constexpr std::string_view proc_root = "/proc/sys/";
    constexpr std::string_view sys_root = "/sys/";
    if (startsWith(path, proc_root)) {
        const auto relative = path.substr(proc_root.size());
        const auto slash = relative.rfind('/');
        if (slash == std::string::npos) return "proc_sys";
        std::string directory = relative.substr(0, slash);
        std::replace(directory.begin(), directory.end(), '/', '.');
        return "proc_sys." + directory;
    }
    if (!startsWith(path, sys_root)) return {};

    const auto parts = pathParts(path);
    const auto size = parts.size();
    for (std::size_t i = 0; i < size; ++i) {
        // /sys/block/<dev>/queue/<attr>
        if (parts[i] == "block" && i + 2 < size && parts[i + 2] == "queue") {
            if (i + 4 == size) return std::string(kDomainBlockQueue);
            // /sys/block/<dev>/queue/iosched/<attr>: exists only for the active elevator.
            if (i + 5 == size && parts[i + 3] == "iosched") return std::string(kDomainBlockIosched);
        }
        if (parts[i] == "cpufreq" && i + 1 < size && startsWith(parts[i + 1], "policy")) {
            if (i + 3 == size) return std::string(kDomainCpuFreq);
            // Governor tunables: only the governor the policy currently runs.
            if (i + 4 == size && !active_governor.empty() && parts[i + 2] == active_governor)
                return std::string(kDomainCpuGovernor);
        }
        if (parts[i] == "devfreq" && i + 2 == size - 1) return std::string(kDomainDevfreq);
    }
    // General sysfs domain: resources with kernel-published bounds or a matching
    // catalogue rule may participate; everything else stays observe-only.
    return std::string(kDomainGeneric);
}

bool hasWritePermissionBits(mode_t mode) noexcept {
    return (mode & (S_IWUSR | S_IWGRP | S_IWOTH)) != 0;
}

bool isPrunedDiscoverySubtree(const std::string& path) {
    static const char* const pruned[] = {
        "/sys/kernel/debug", "/sys/kernel/tracing", "/sys/firmware", "/sys/fs/pstore"
    };
    for (const auto* prefix : pruned) {
        const std::string_view view(prefix);
        if (path == view) return true;
    }
    return false;
}

} // namespace coreflow
