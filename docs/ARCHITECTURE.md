# CoreFlow Modern v2 architecture

## Principle

Zero device-specific hardcoding does not mean blind mutation.

Stable Linux/Android ABI names such as cpu.uclamp.min, PSI fields, and thermal-zone type are semantic contracts. Device-specific mount paths, CPU counts, cluster layouts, and vendor knobs are discovered at runtime or ignored.

Flow:

    Discover capability
          |
    Read thermal + PSI
          |
    Select one stability state
          |
    Apply one bounded top-app UClamp pair
          |
    Read-back verify
          |
    Keep state until inputs materially change
          |
    Rollback on stop/failure/emergency

There is no Observe -> Understand -> Balance -> Adapt pipeline.

## Active production core

- src/daemon/main.cpp: lifecycle, PID lock, signal handling, PSI reads, state application.
- src/daemon/reactor.cpp: epoll plus timerfd event loop.
- src/kernel/uclamp.cpp: semantic cgroup discovery, encoding detection, bounded top-app UClamp writes.
- src/kernel/thermal_monitor.cpp: type-labelled thermal discovery and hottest-zone reading.
- src/kernel/baseline_store.cpp: write-ahead journal, read-back verification, recovery.
- include/coreflow/stability_policy.hpp: pure deterministic four-state decision function.
- include/coreflow/config.hpp: small bounded runtime policy config.

Generic cgroup, render, memory, and PSI mutation placeholders are not part of the production daemon.

## UClamp discovery

The daemon does not assume /sys/fs/cgroup/top-app or /dev/cpuctl/top-app.

It parses /proc/self/mountinfo for cgroup and cgroup2 mounts, performs a bounded search for a top-app directory, and requires both cpu.uclamp.min and cpu.uclamp.max. The pair must be readable and writable.

Existing values determine whether the interface uses percentage semantics or a 0..1024 capacity representation. Internal policy always uses 0..1024 and encodes only at the kernel boundary.

## Stability policy

Four states only:

1. Balanced.
2. SustainedLoad.
3. PressureGuard.
4. ThermalGuard.

Memory/I/O pressure outranks performance boosting. Thermal guard outranks all pressure states. Severe heat triggers rollback and daemon stop.

Games are not identified by package name. A game becomes a sustained workload because the system reports sustained CPU pressure while thermal headroom remains.

## Recovery

Before a supported write, BaselineStore persists the observed pre-CoreFlow value to /data/adb/coreflow/baseline.journal. The write is then read back. Any failure starts rollback.

Fatal signal handlers do not run C++ rollback code. They terminate safely; the next daemon start in the same boot recovers the journal before new writes are allowed.

## WebUI

Native mode displays measured daemon/UClamp/thermal status and daemon logs. Browser-only simulation stays explicitly labelled preview. FPS, jank, and foreground-app metrics are not claimed as native telemetry until a validated source exists.
