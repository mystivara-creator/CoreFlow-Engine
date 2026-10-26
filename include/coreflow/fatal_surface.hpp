#pragma once

#include <string_view>

namespace coreflow {

// FatalSurfaceGuard is the ONLY hard restriction on autonomous resource writes.
//
// CoreFlow is capability-driven: every node that discovery finds readable and
// writable, and that the kernel can verify by read-back, is a mutation
// candidate. Nothing is admitted by name. Instead, this guard denies the small
// set of surface classes whose mutation is fatal on every Android device:
//
//   - one-shot actions / triggers (drop_caches, sysrq-trigger, bind/unbind,
//     uevent, reset, delete, rescan, cgroup.kill, memory.reclaim, ...)
//   - crash / panic / watchdog / lockup detectors
//   - kernel security and integrity (SELinux, kptr, bpf, modules, kexec, ...)
//   - thermal protection (zones, trip points, cooling devices, *thermal*)
//   - charging, battery, regulators, clocks, power state / suspend
//   - CPU / memory hotplug ("online"/"offline")
//   - hard memory / process limits that cause OOM, hangs or fork failure
//   - storage integrity (write_cache, fua, discard)
//   - raw device nodes, debugfs, tracefs, firmware and bus control
//
// The guard is a pure function of the path (and, for values, of the factory
// and requested value). It never consults a per-parameter allow-list.
enum class FatalClass : unsigned char {
    None = 0,
    ActionTrigger,
    CrashControl,
    Security,
    ThermalProtection,
    PowerDelivery,
    Hotplug,
    HardLimit,
    StorageIntegrity,
    RawInterface,
    NotKernelTunable,
};

// Classify a path. FatalClass::None means the path is a kernel tunable that is
// not fatal to change.
FatalClass classifyFatalSurface(std::string_view path) noexcept;

inline bool isFatalSurface(std::string_view path) noexcept {
    return classifyFatalSurface(path) != FatalClass::None;
}

// Value-level invariants that hold on every device:
//   - value is non-empty, single-line, printable, at most 256 bytes
//   - a node whose factory value is a non-negative integer never receives a
//     negative integer (-1 means "unlimited/disabled" for many kernel knobs)
bool isFatalValue(std::string_view factory, std::string_view requested) noexcept;

const char* fatalClassName(FatalClass value) noexcept;

} // namespace coreflow
