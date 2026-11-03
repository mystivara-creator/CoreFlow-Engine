<p align="center">
  <img src="docs/assets/coreflow-banner.svg" width="960" alt="CoreFlow Engine Modern v2">
</p>

# CoreFlow Engine Modern v2.0

A compact native **C++20 Android performance daemon** using Linux `epoll`, a periodic `timerfd`, supported EAS/UClamp interfaces and a React single-page dashboard. Designed for Android ARM64 and KernelSU/Magisk-style installation. Android support depends on the actual kernel and cgroup layout.

[Build and checks](https://github.com/mystivara-creator/CoreFlow-Engine/actions/workflows/build.yml) · [Source architecture](docs/ARCHITECTURE.md) · [Tuning guide](docs/TUNING.md) · [Security](SECURITY.md)

## Implemented

- **Native event reactor:** `epoll_wait` handles registered events and a bounded timerfd thermal heartbeat. No busy loop, but this does **not** mean zero idle wakeups or zero CPU use.
- **UClamp adaptation:** reads a compatible top-app cgroup interface, then selects interactive or thermal state with hysteresis. Settings are captured, journaled, written and read-back verified.
- **Crash recovery:** a private, fsync-backed write-ahead journal records pre-mutation values. A daemon restart within the same kernel boot restores unfinished changes before new mutations are allowed. The supervised service starts a single instance.
- **Native controls:** `coreflowd --status`, `--stop` and `--rollback-baseline`. A successful stop reports only verified journal recovery.
- **Android module:** boot-attempt guard, explicit disable/safe-mode markers, persistent config, bounded logs and offline bundled WebUI.
- **Dashboard:** modern responsive React/Tailwind layout, dark/light appearance, real native UClamp/thermal/status via KernelSU shell bridge, and separately identified browser-only demonstration mode.
- **CI:** deterministic host baseline/recovery and config tests, ASan/UBSan, shell syntax checks, TypeScript/Vite bundling and ARM64 NDK packaging.

## Disabled until platform validation

No automatic vendor thermal-unlock, arbitrary task migration, display-node refresh override, fabricated 120 FPS measurement, foreground application detection, memory-pressure retuning or permanent touch bursts. Those areas must be validated per kernel/ROM and must not be represented as currently operational. FPS/jank and foreground metrics are **unavailable** in native mode.

## Install and diagnostics

Use the latest **successful** [CI workflow](https://github.com/mystivara-creator/CoreFlow-Engine/actions/workflows/build.yml) to inspect the build artifact. Do not treat green CI as evidence that an untested device is compatible. Before installing, remove conflicting previous CoreFlow versions and keep a usable recovery path.

Configuration: `/data/adb/coreflow/coreflow.conf` (persistent across module upgrades).

```sh
su -c '/data/adb/modules/coreflow_engine/system/bin/coreflowd --status'
su -c '/data/adb/modules/coreflow_engine/system/bin/coreflowd --stop'
su -c '/data/adb/modules/coreflow_engine/system/bin/coreflowd --rollback-baseline'
```

For emergency suspension, create `/data/adb/coreflow/DISABLE` as root. The daemon checks this marker at startup and on its next timer event. Crash recovery remains recorded in `/data/adb/coreflow/baseline.journal`; **do not delete that journal to bypass a recovery error**.

Daemon logs: `/data/adb/coreflow/logs/coreflowd.log`. Boot guard: `/data/adb/coreflow/logs/boot_guard.log`.

## Project layout

```
src/daemon/                Native entry point and event reactor
src/kernel/                UClamp, baseline journal, device and capability helpers
include/coreflow/          Native C++20 interfaces
webui/                    Modern React dashboard and native bridge
module/                   KernelSU/Magisk boot scripts, config and built WebUI
config/                   Default tuning file
tests/                    Native host validation
docs/                     Architecture and tuning documentation
.github/workflows/        Host, sanitizer and Android ARM64 CI
```

## Engineering limits

Rollback reverts **the values observed before CoreFlow's own writes**, not guaranteed immutable OEM factory settings. It is a verified, best-effort, per-node recovery process, not an atomic kernel transaction. Kernel crashes, filesystem corruption, interactions with other root modules and vendor driver side effects may be unrecoverable. Device acceptance needs stock readings, thermal logging, stress testing, normal shutdown/reboot recovery and explicit rollback verification.

License: Apache-2.0.
