#pragma once

#include <string>
#include <string_view>
#include <sys/types.h>

namespace coreflow {

// Semantic domain vocabulary shared by discovery, workload scoring, outcome
// verification and the strategy catalogue. Domains describe the kernel layout
// that owns a control, never a device-specific absolute path.
//
//   proc_sys.*            /proc/sys/<dir>/<attr>
//   sysfs.block.queue     /sys/block/<dev>/queue/<attr>
//   sysfs.block.iosched   /sys/block/<dev>/queue/iosched/<attr>   (active elevator only)
//   sysfs.cpu.cpufreq     /sys/devices/system/cpu/cpufreq/policyN/<attr>
//   sysfs.cpu.governor    .../cpufreq/policyN/<active-governor>/<attr>
//   sysfs.devfreq         .../devfreq/<dev>/<attr>
//   sysfs.protected       thermal, battery, regulator, watchdog, debug, ...
//   sysfs.generic        everything else (inventoried, never eligible by itself)
inline constexpr std::string_view kDomainBlockQueue = "sysfs.block.queue";
inline constexpr std::string_view kDomainBlockIosched = "sysfs.block.iosched";
inline constexpr std::string_view kDomainCpuFreq = "sysfs.cpu.cpufreq";
inline constexpr std::string_view kDomainCpuGovernor = "sysfs.cpu.governor";
inline constexpr std::string_view kDomainDevfreq = "sysfs.devfreq";
inline constexpr std::string_view kDomainProtected = "sysfs.protected";
inline constexpr std::string_view kDomainGeneric = "sysfs.generic";

// True for surfaces whose mutation can cause power loss, bypass a safety policy,
// or perform irreversible hardware actions. These are never optimization targets.
[[nodiscard]] bool isProtectedKernelControl(const std::string& path);

// Classifies a discovered regular file. `active_governor` is the governor name
// published by the owning cpufreq policy (empty when unknown); a governor
// directory is only treated as tunable when it is the active one, so stats/ and
// other policy subdirectories are never mistaken for governors.
[[nodiscard]] std::string classifyKernelDomain(const std::string& path,
                                               std::string_view active_governor);

// Owner/group/other write bit. Root bypasses DAC, so access(W_OK) alone cannot
// distinguish writable sysfs attributes from read-only ones.
[[nodiscard]] bool hasWritePermissionBits(mode_t mode) noexcept;

// Directories whose subtrees are never useful for tuning and are expensive to
// walk. Pruning them keeps periodic inventory bounded on real devices.
[[nodiscard]] bool isPrunedDiscoverySubtree(const std::string& path);

} // namespace coreflow
