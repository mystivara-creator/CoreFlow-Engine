# CoreFlow Modern v2 architecture (implementation contract)

This file describes what the native daemon actually implements. It is not a promise of measured performance.

## Runtime

- `src/daemon/main.cpp`: one root-owned process with a PID lock, signalfd shutdown, a bounded timerfd thermal heartbeat and an epoll reactor.
- `src/daemon/reactor.cpp`: Linux epoll for registered file descriptors; timerfd wakeups still occur at a configured interval. The daemon does **not** claim zero CPU or zero wakeups.
- `src/kernel/device_profiler.cpp`: enumerates CPU frequency policies and reads type-labelled thermal sensors. Missing sensor data is reported as unavailable, not fabricated.
- `src/kernel/uclamp.cpp`: applies interactive or thermal utilization clamp settings only when a supported cgroup interface exists. The configured interactive top-app floor is honored. A failed write stops the operation.
- `src/kernel/cgroup_v2.cpp`: detects a unified cgroup hierarchy, but automatic task migration, vendor cpuset override and direct display override are **not** activated by the daemon.
- `src/kernel/render_booster.cpp`: placeholder frame callback API; no SurfaceFlinger frame acquisition or timed render boost is active. Display vendor refresh nodes are never written.
- `src/kernel/psi_monitor.cpp` and `src/kernel/memory_guardian.cpp`: capability helpers; no automatic pressure-triggered memory mutation is currently wired into the daemon loop.

## Safety and lifecycle

Before modifying a supported kernel node, `BaselineStore` stores the current value in a private, fsync-backed write-ahead journal at `/data/adb/coreflow/baseline.journal`. After each attempted write, it checks the read-back. On failure or normal termination it attempts to restore all tracked values. On the **next daemon start in the same boot**, it replays unfinished recovery before allowing new writes. Reboot-created journal entries from an earlier kernel boot are not replayed, because the vendor kernel may have installed new defaults.

This is **not atomic kernel rollback**. A fatal signal handler cannot safely take mutexes or write C++ streams. It terminates immediately; restoration requires a follow-up daemon restart. Crashes in the kernel, storage failures, driver side effects and concurrent vendor writers cannot be guaranteed recoverable. The snapshots reflect values observed when CoreFlow first modified each node, not an immutable OEM factory image.

## Control protocol

`coreflowd --status` prints a JSON object containing `running`, `pid`, `uclampMin`, `uclampMax`, `socTempCelsius` (nullable), `cgroupV2` and `recoveryPending`.
`coreflowd --stop` sends SIGTERM to the active daemon and checks the recovery journal.
`coreflowd --rollback-baseline` uses the same verified stop/recovery path. The command does not claim to rewind arbitrary edits from other modules.

`module/service.sh` starts one daemon instance after Android boot; operator markers `/data/adb/coreflow/{DISABLE,SAFE_MODE}` prevent new native mutations. `module/post-fs-data.sh` applies the boot-attempt guard. Configuration lives in `/data/adb/coreflow/coreflow.conf` and is applied on daemon restart.

## WebUI

React components and animations remain in `webui/`. In a normal browser the data source is an explicitly labelled simulator. With KernelSU's native shell bridge, the UI reads `--status`, tail-reads the daemon log and calls the stop/recovery commands. Native frame FPS, jank, foreground app and PSI telemetry are **not implemented** and must display unavailable rather than synthetic numbers.

## Verification

CMake host tests exercise baseline restoration after an abrupt child-process exit, failed write / capture fallback and malformed configuration bounds. CI runs host tests, ASan+UBSan, TypeScript, shell syntax and Android ARM64 packaging. A successful CI run proves build/test invariants, **not** compatibility with any particular vendor kernel.
