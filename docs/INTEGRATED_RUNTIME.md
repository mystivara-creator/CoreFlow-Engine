# Integrated CoreFlow runtime

This change is based on main at `41b582c4c5050e96a9d5b3c8289b219adf9b90d2`.
It connects diagnosis, hardware-condition evidence, control continuity, rendering
outcomes, existing local experience, and an optional application ADPF interface.
It is not evidence of measured device performance gains or additional hardware capabilities.

## Implemented behavior

- Runtime diagnosis identifies dominant measured thermal/memory/I/O/CPU pressure.
  GPU render pressure requires both fresh rendering disturbance and high GPU load.
  Utilization by itself is not treated as proof of a bottleneck. Competing pressure
  signals reduce confidence. Non-finite or missing signals do not manufacture evidence.
- The decision agent demotes unrelated resource proposals only with confident,
  unambiguous diagnosis. It never increases authority, changes grants, invents a
  kernel setting, or overrides a stabilizing candidate. Existing candidate derivation,
  experience blending, ONNX thermal inference, permits and journal remain in control.
- A read-only UFS descriptor snapshot is captured at discovery/rediscovery, using
  ABI-defined sysfs aliases under the discovered mount. Enumeration is capped;
  aliases are deduplicated. Reads are deliberately outside the tick loop: descriptor
  queries may involve the storage driver. Unsupported/eMMC devices report unavailable.
  Codes are not exact health percentages. A caution demotes exploratory I/O proposals;
  it is not proof that a tuning value changes NAND write amplification or lifespan.
- CPU/resource/boost owned settings are checked before attributing telemetry to an
  outcome. A changed value invalidates the measurement, uses the established recovery
  path, and latches tuning off until explicit rediscovery. An unreadable setting
  triggers recovery/retry without blaming an external writer. The identity of the
  writer cannot be inferred; a mismatch may also represent another root tool or
  autonomous kernel behavior. Existing recovery writes can restore a baseline;
  this is not a transparent ownership handoff to Power HAL.
- Live SurfaceFlinger history is interpreted according to AOSP column order:
  desired-present, actual-present, ready. Presentation intervals are deduplicated
  across polls and pending fences are excluded. Static history supplies no new
  evidence. This is a cadence estimate, not input latency, application deadline
  intent, or a complete Perfetto FrameTimeline classification. An intentional
  lower frame rate may look like missed display cadence; on-device validation is
  required. A global --latency dump may provide only the refresh header on some
  ROMs; those devices remain unavailable instead of receiving fabricated frames.
- Outcome comparison includes fresh rendering evidence from both windows. A clear
  increase in missed presentation cadence vetoes retention even if CPU/thermal metrics
  improved. Changing refresh period during either comparison yields inconclusive.
  Existing rollback handles these outcomes; unavailable rendering is not scored as zero.
- Status JSON and native WebUI tables show diagnosis, control continuity, local
  experience count, rendering availability, and UFS health. Older snapshots remain
  readable. Status fields describe evidence and backend scope, not authorization.

## Platform integration: exact scope

| Path | Included | Required for broader integration |
|---|---|---|
| Linux CPU/GPU/memory/I/O | Existing discovered actuators plus diagnosis/outcome coordination | Device exposes an admitted writable interface and valid domain grant |
| AI | Existing thermal ONNX and measured local experience remain active; new diagnosis constrains candidate ranking | Calibrated new models and device data before claiming learned performance prediction |
| Power HAL | Existing vendor-profile knowledge plus external-change detection | ROM/platform adapter using generated matching AIDL/HIDL interfaces, service access and SELinux policy; not raw guessed Binder transactions |
| ADPF | `integration/android/performance_session.hpp`, a C++20 app-side RAII wrapper | A cooperating application reports genuine target and actual work durations for its own threads |
| ROM animation features | No implementation claimed | Launcher/SystemUI/framework code and compatible hardware; cannot be supplied by a daemon alias |

The daemon does not boost itself through ADPF and pretend this benefits other apps.
No Power HAL connection success is inferred from a JSON profile. No thermal trip,
firmware power limit, voltage, fsync, or SELinux protection is disabled.

## App-side ADPF API

Use `add_subdirectory(integration/android)` in an Android NDK application targeting
API 33+ and link `coreflow_application_hints`. Then include `performance_session.hpp`.

`PerformanceSession::start(tids, target_ns)` creates a session for caller-owned
threads. Invalid/unavailable sessions return false. Failed replacement preserves
an existing session. `report(actual_ns)` and `setTarget(target_ns)` return native
error codes; they never fake success. `close()` is idempotent. Every operation on
the session is serialized; all callers must finish before object destruction.
Use monotonic timing and real application work durations, not polling intervals
or synthetic load. The wrapper is not linked into coreflowd.

## Validation

New tests follow the repository's standalone CTest executable convention; no new
test framework dependency is introduced. They exercise missing/conflicting/NaN
signals, storage code parsing and bounded discovery, control mismatches, repeated
and pending frame history, rendering regressions, refresh changes, and ADPF
lifecycle/error propagation/concurrent calls against a fake native backend.
The fake backend verifies wrapper semantics, not Android service availability.

Run the existing CMake build/CTest and tools/test_webui_status.sh. Android-only
sources also require an actual NDK build and device validation. A host syntax
check using Android declarations is not an NDK/link or runtime verification.

Before deployment compare stock and CoreFlow on the same ROM/workload, including
warm sustained use. Measure frame-time distributions, app reloads, stalls, power,
and daemon overhead. Use Perfetto offline for attribution; do not leave a heavy
trace recorder running as a substitute for a low-overhead daemon sensor.

## Primary references

- https://docs.kernel.org/scheduler/sched-util-clamp.html
- https://docs.kernel.org/admin-guide/pm/cpufreq.html
- https://source.android.com/docs/core/perf/lmkd
- https://perfetto.dev/docs/data-sources/frametimeline
- https://developer.android.com/ndk/reference/group/a-performance-hint
- https://android.googlesource.com/platform/hardware/interfaces/+/master/power/aidl/android/hardware/power/IPower.aidl
- https://android.googlesource.com/platform/frameworks/native/+/8c9cd3c537f7d4781862e2d95b32247fa5bb3d66/services/surfaceflinger/FrameTracker.cpp
- https://android.googlesource.com/kernel/common/+/dd831ac8221e691e9e918585b1003c7071df0379/Documentation/ABI/testing/sysfs-driver-ufs
