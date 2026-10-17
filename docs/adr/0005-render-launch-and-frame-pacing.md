# ADR 0005: Render, app launch, and frame pacing domains

- Status: Proposed (producers implemented; actuators not yet)
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
3. **GPU load producer (not yet).** Read the GPU busy or load node that discovery
   finds in the GPU class. Map it to `Signal::GpuBusyRatio`. Node format must be
   validated per SoC family before use.
4. **Actuators (not yet), in order of risk:**
   a. Launch window only: during a window, raise the foreground cpu floor through
      existing uclamp discovery and restore it when the window closes. Reversible
      and bounded by the discovered options.
   b. Storage read-ahead during a window, restored afterwards. Uses the existing
      block-queue tunables. Reverted by the journal if the window ends unclean.
   c. GPU frequency floor, only inside a window that the frame analyzer reports as
      missing deadlines, and capped at the discovered maximum. Lowering the GPU
      maximum to save power is a separate, later decision, because it can cause
      stutter.
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
- The new producers are not yet wired into the autonomous loop. Wiring is the next
  step, and it is gated on the device-validation plan above.
