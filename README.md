# CoreFlow Autonomous v1.8.0

**Native Adaptive System-Intelligence Daemon for Android**

CoreFlow is a privileged, native C++ daemon designed to observe Android runtime conditions and make bounded, reversible system adjustments when the device state justifies them.

> **Discover → Baseline → Observe → Evaluate → Mutate → Verify → Restore**

This repository is the **production source tree**. The compiled ARM64 daemon is intentionally not checked into source control. GitHub Actions builds the real ELF and assembles the flashable module.

## What v1.8.0 provides

### Runtime intelligence

CoreFlow continuously samples:

- thermal zones and thermal trends
- battery/charging telemetry
- memory availability and memory trend
- system load and CPU utilization
- CPU policy topology and governor capability
- block-device read-ahead capability
- VM swappiness capability
- root cgroup uClamp capability
- cpuset capability

The decision engine combines these observations with hysteresis and a confidence score. No single sensor is allowed to trigger an uncontrolled write.

### Controlled mutation

The release includes a bounded mutation layer with:

1. baseline capture
2. capability validation
3. confidence gating
4. bounded write
5. read-back verification
6. rollback on failure
7. baseline restoration during shutdown and rediscovery

The currently enabled production mutation is **CPU governor selection** during `THERMAL_GUARD`/`PRESSURE`, and only when `schedutil` is explicitly advertised and the governor node is writable. The baseline governor is restored when the protected state ends.

I/O, VM, uClamp, cpusets, charging, graphics and ART are currently inventory/telemetry surfaces. Their mutation adapters must be proven against real device/kernel semantics before being enabled.

## Runtime rediscovery

`SIGUSR1` requests a safe environment refresh. The signal handler only records a flag; discovery runs on the normal daemon thread.

```sh
su -c 'kill -USR1 $(pidof coreflowd)'
```

The refresh flow is:

```text
SIGUSR1
  ↓
restore active baseline
  ↓
rediscover environment
  ↓
capture new baseline
  ↓
resume observation
```

## Configuration

Runtime configuration is stored at:

```text
/data/adb/coreflow/config.ini
```

Default:

```ini
monitor_interval=5
min_confidence=0.70
mutation_mode=adaptive
allow_cpu_governor=true
runtime_refresh=true
```

`mutation_mode=disabled` forces observation-only operation without uninstalling the module.

## Fail-closed rules

CoreFlow does not treat writability as proof of safety. A control must be understood before mutation. Unknown or ambiguous interfaces are skipped.

The charging layer deliberately keeps `semantics_validated=false` until a platform-specific contract proves what the exposed control means.

## Production build

Requirements are encoded in `.github/workflows/android-ndk.yml`:

- Android API 34
- arm64-v8a
- NDK 27.2.12479018
- C++17
- Release build
- PIE
- RELRO/now linker hardening
- stack protector
- section garbage collection
- `-Werror`

GitHub Actions also runs deterministic host-side unit tests for policy, configuration and mutation before the Android build.

## Local verification

The host environment can validate the non-Android components:

```sh
cmake -S . -B build/host -G Ninja -DBUILD_TESTING=ON
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

The Android ELF requires the NDK toolchain specified by the CI workflow.

## Module layout

The source tree contains module scripts and configuration, but intentionally does **not** contain a fake/placeholder `coreflowd` ELF. The CI pipeline supplies the compiled binary and then packages:

```text
module.prop
customize.sh
post-fs-data.sh
service.sh
uninstall.sh
sepolicy.rule
system/bin/coreflowd
system/etc/coreflow/default.conf
```

## Safety boundary

The daemon is designed for rooted Android devices and may run with privileged access. This project therefore avoids blanket SELinux permissions, unrestricted writable-node traversal, voltage manipulation, blind frequency writes, and unverified charging controls.

## Project status

v1.8.0 is a **production foundation** for adaptive observation plus bounded CPU-governor mutation. It is not a claim that every Android vendor exposes identical kernel controls.

The architecture is intentionally adapter-based so additional subsystems can be introduced without weakening the safety model.

## License

MIT
