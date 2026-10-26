# ADR 0005: Render, app launch, and frame pacing domains

- Status: Partially implemented. Frame telemetry and the interactivity boost actuators
  (GPU minimum frequency, top-app uclamp, read-ahead) are wired into the autonomous
  loop behind the interactivity_mode and global arm gates. The launch detector and
  render collector are now called by the loop when a discovered foreground cgroup
  path is available. Their process-count/PSI signal is corroborative; ForegroundTracker
  remains the PID-change signal. Runtime behavior and target-device impact still need
  Android ARM64 validation. Boost target paths are selected from discovered writable
  capabilities, not fixed vendor paths.
- Date: 2026-10-18

## Context

The goals are lighter rendering load on the GPU and driver stack, faster
foreground app switching on UFS 4.x storage, and fewer frame-time spikes
(micro-stutter). The engine already has the pipeline these need: discovery of
kernel tunables, a vendor-neutral telemetry contract (`performance_telemetry`), a
frame analyzer (`frame_performance`), PSI parsing, journaled and reversible
mutation. What was missing was the producers for the signals and an honest
account of what the daemon can and cannot control.

## What the daemon can and cannot control

- The Vulkan driver, shader compiler, pipeline cache, and the app's own render
  work are outside the daemon's reach. It does not "accelerate Vulkan". It can
  change how the work that the driver submits is scheduled and clocked.
- GPU work is controlled through the kernel's devfreq and vendor GPU nodes.
  Their names differ by vendor, so they are found by discovery, never written
  into source. Admission follows the existing rule (recognised subsystem or vendor
  declaration).
- The foreground app is not visible to a root daemon through an API. The daemon
  can see which processes joined the foreground scheduling group (top-app cpuset)
  and when storage pressure rises. It cannot see the activity name without a
  framework hook.
- Frame timing is available from SurfaceFlinger's diagnostic output
  (`dumpsys SurfaceFlinger --latency`). That output is not a stable API, so the
  parser fails closed on any shape it does not know.

## Decision

1. **Frame interval producer (implemented).** `frame_latency` parses the
   documented shape of `--latency` output into consecutive present intervals.
   It bounds input size and row count, rejects non-monotonic and implausible rows,
   and counts pending frames instead of guessing them. Its output feeds
   `FramePerformanceAnalyzer::recordFrame`, which is already in place.
2. **Launch window detector (implemented).** `launch_detector` is a pure state
   machine. A new process in the foreground group opens a window. Continued I/O
   pressure extends it, up to a hard cap. Its thresholds are heuristics and must be
   measured on device before any actuator depends on them.
3. **GPU busy producer (present, not universally integrated).** `GpuBusySource` probes
   candidate files and validates numeric load readings. A reviewed discovery-to-sensor
   mapping for arbitrary GPU interfaces is still required before it can be claimed as
   a universally available signal; the current loop does not treat arbitrary GPU nodes
   as a trusted, universal busy metric.
4. **Interactivity actuators (wired, outcome validation pending).** The source tree
   connects discovered uclamp, read-ahead, and GPU-floor candidates through
   `InteractivityBoostController`, a dedicated journal and restore paths. These are
   opt-in through `interactivity_mode=adaptive` and global Adaptive/armed authority.
   The frame/launch observations are corroborative; code presence alone does not
   establish that each boost reduces jank on a device. The outcome-specific causal
   trigger and before/after device measurements remain validation gaps.
5. **Measurement before claims.** No actuator is enabled by default. Each one needs
   an observed before and after on the target device (frame deadline miss rate,
   launch-to-first-frame time), recorded in device validation, not inferred from
   the code.

## Consequences

- No vendor node name appears in the new code. The producers depend only on
  documented formats and on discovery's output.
- Frame timing from dumpsys is best effort. On a build where its format differs,
  the producer reports `valid=false` and the engine behaves as it does today.
- The decision to actuate on frame misses depends on the frame analyzer's bounds
  (`kMaxFrameIntervalNs`), which were set for steady rendering. Those bounds need
  review for variable-refresh displays.
- Frame telemetry runs one bounded child process per poll (no shell, 500 ms timeout,
  256 KiB output cap). The poll rate is capped at 5 Hz. The cap is a starting point and
  must be measured on device.
- **Status correction (v2.4.0):** the launch detector is called from the live loop,
  and `render_collector.cpp` provides the bounded `readLaunchObservation` helper
  used to count foreground-group PIDs and sample I/O PSI. Frame intervals are read
  independently by `FrameTelemetrySource`. The filename `render_collector` does not
  mean that it directly reads rendered frames. Both producer inputs are corroborative;
  neither by itself proves a tuning outcome.
