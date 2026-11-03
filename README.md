<p align="center">
  <img src="docs/assets/coreflow-banner.svg" width="960" alt="CoreFlow Engine Modern v2">
</p>

# CoreFlow Engine Modern v2.0

CoreFlow is a compact native C++20 stability-first Android optimizer. The target is not maximum benchmark output. The target is stable responsiveness across daily use, multitasking, and sustained workloads without a large overlapping decision architecture.

## Design rule: discover broadly, mutate narrowly

CoreFlow does not keep device-specific sysfs paths or model lists. At runtime it:

- enumerates mounted cgroup and cgroup2 filesystems from /proc/self/mountinfo;
- finds the semantic Android/Linux top-app group with cpu.uclamp.min and cpu.uclamp.max;
- verifies the pair is readable and writable;
- detects whether the kernel exposes percentage or 0..1024 capacity encoding;
- discovers trusted CPU/SoC thermal zones by type labels;
- reads standard Linux PSI from /proc/pressure/cpu, memory, and io.

Discovery never grants permission to mutate arbitrary nodes. The production actuator is intentionally limited to the verified top-app UClamp pair.

## One small stability state machine

- Balanced: configured responsive baseline.
- Sustained Load: sustained CPU pressure with thermal headroom; moderately raises the top-app floor.
- Pressure Guard: memory or I/O contention is elevated; backs off artificial boosting while keeping the ceiling open.
- Thermal Guard: temperature reaches the configured trip point; gently lowers the top-app ceiling.
- Emergency thermal exit: restores tracked values and stops CoreFlow so the OEM thermal stack has full control.

The engine never raises an OEM top-app maximum observed at startup. If another component changes an owned clamp, CoreFlow releases that value and stops; it can be restarted after the device policy settles.

There is no package-name game list, round-robin mutation, vendor display override, arbitrary cgroup migration, or background/foreground clamp rewriting.

## Safety

Every native write is preceded by a private fsync-backed write-ahead journal and followed by read-back verification. Clean stop restores tracked pre-CoreFlow values. An abrupt same-boot process death is recovered before the next daemon instance can mutate anything. The journal also records the intended value, so recovery leaves externally changed nodes under OEM control.

This is best-effort per-node recovery, not an atomic whole-device transaction.

## Runtime files

Config: /data/adb/coreflow/coreflow.conf  
Journal: /data/adb/coreflow/baseline.journal  
Daemon log: /data/adb/coreflow/logs/coreflowd.log  
Emergency disable: /data/adb/coreflow/DISABLE

Commands:

    su -c '/data/adb/modules/coreflow_engine/system/bin/coreflowd --status'
    su -c '/data/adb/modules/coreflow_engine/system/bin/coreflowd --stop'
    su -c '/data/adb/modules/coreflow_engine/system/bin/coreflowd --rollback-baseline'

See docs/ARCHITECTURE.md, docs/TUNING.md, and docs/DEVICE_TEST.md.

CI validates host tests, ASan/UBSan, WebUI build, shell syntax, and Android ARM64 packaging. Green CI is required before device testing, but is not proof of device compatibility.

License: MIT.
