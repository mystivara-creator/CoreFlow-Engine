# P2-B Platform Context & P2-C Workload Intelligence

## Implemented P2-C scope

This revision adds a read-only procfs/PSI fallback sampler and a deterministic, backend-neutral workload decision tree. It does not claim a production eBPF program is loaded or attached. The passive BPF probe leaves syscall permission, ring-buffer map creation, SELinux load authorization and verifier/attachment status unknown. Until a verified BPF object and tracepoint attachment lifecycle exist, backend selection remains on sysfs/procfs.

## Unified feature contract

WorkloadFeatureSnapshot is the sole classifier input. Backend provenance is metadata only; the classifier does not branch on source. Every feature is guarded by valid_features. Missing GPU busy, frame timeline, wakeup, run-queue and latency metrics remain invalid rather than being synthesized as zero.

The fallback samples:
- PSI some avg10 for CPU, memory and I/O pressure.
- /proc/diskstats sector counters, converted to bytes using Linux 512-byte sectors. The first sample establishes a baseline; only subsequent monotonic deltas are valid.
- It does not fabricate GPU busy, frame deadlines, run-queue wait, wakeup rate or I/O latency where no reliable portable source exists.

Counter reset, missing files and malformed windows leave affected features invalid. Fallback emits feature windows directly; it does not invent synthetic scheduler events.

## Classifier and temporal controls

The decision tree is deterministic and package-name agnostic:
- Jank-sensitive requires explicit frame-deadline miss percentage plus an independent CPU, I/O or GPU contention signal.
- Memory pressure requires valid memory PSI.
- I/O-heavy requires valid I/O byte delta and I/O PSI.
- GPU-bound requires GPU busy plus CPU context.
- CPU-bound requires CPU PSI and, when available, run-queue evidence.
- Gaming requires GPU busy plus explicit frame-deadline telemetry; package identity is never a classifier feature.

The classifier rejects malformed, future, older-than-three-second, duplicate and out-of-order windows. A candidate must persist for two windows. Non-initial transitions require a three-second dwell from the last committed transition. Confidence and evidence masks accompany classifications. The classifier has no actuator or MutationCoordinator dependency.

## HyperOS 4-inspired KPI and P2-D contract

The feature schema carries frame deadline count, miss count and miss percentage, GPU busy, CPU/memory/I/O PSI, I/O byte deltas and I/O latency p95. The fallback currently supplies PSI and I/O byte deltas only. FrameTimeline/GPU telemetry requires a separately validated platform source. The 0.08% frame-drop target cannot be inferred from PSI or scheduler events.

Memory PSI is evidence for P2-D, not a reclaim command. Proactive cleanup must additionally consume reclaimable-memory estimates, foreground/working-set protection, pressure duration, reclaim cost and post-action verification. No reclaim or other kernel mutation is performed here.

## eBPF load gate and remaining acceptance

A readable BTF/tracefs path or bpffs mount is not proof that BPF_PROG_LOAD, tracepoint attachment or ring-buffer delivery is permitted. event_pipeline_ready remains false until syscall, ring-buffer, SELinux, loader verification and required tracepoints are positively established. The active source now attempts a bounded raw-tracepoint loader only after passive prerequisites pass; any failed syscall, verifier, attachment or mmap step cleans up acquired resources and leaves fallback selected.

A raw-syscall loader creates a bounded BPF ring-buffer map, loads two GPL raw-tracepoint programs (sched_switch and sched_wakeup) with verifier logging, attaches both tracepoints, maps consumer/producer pages, drains fixed-size events, and closes attachments/programs/maps plus unmaps pages on every failure and stop path. BPF map creation is the syscall capability probe; EACCES/EPERM is reported as Restricted. SELinux load permission is considered established only after BPF_PROG_LOAD succeeds; loader verification is true only after both attachments and ring-buffer mappings succeed. Any failure leaves the backend inactive so the caller can use procfs fallback. The loader is not auto-started by the daemon and still requires device-side permission/overhead validation. The current event program records timestamp, PID/TID and CPU for scheduler switch/wakeup events. The pipeline derives a valid wakeup count per sampled window; PSI and disk counters are still obtained through procfs. It does not provide GPU busy, FrameTimeline, block latency or a verified dropped-event counter. Therefore it is an operational eBPF scheduler-event backend, not a complete all-domain workload telemetry implementation.

## Replay and host validation

tests/test_platform_context.cpp exercises synthetic procfs/PSI and deterministic replay windows: PSI parsing, diskstats delta units, invalid-feature preservation, source-agnostic classification, stale/future/duplicate rejection, exact freshness boundary, jank threshold boundary, two-window confirmation, transient candidate reset, dwell-gated workload transition and safe BPF prerequisite failure/stop. These are host fixtures, not physical-device telemetry or KPI results.

## Physical-device diagnostic entry point

The Android daemon binary exposes `coreflowd --workload-probe` as an explicit, read-only two-window diagnostic. It attempts the capability-gated BPF source, falls back to procfs/PSI when any prerequisite or load step is rejected, samples two one-second windows while polling and draining the ring-buffer every 10 ms, runs the deterministic classifier, emits JSON feature validity/backend/permission/classification fields plus drained-event count and batch-saturation state, and detaches all BPF resources before exit. The 10 ms cadence prevents the previous one-drain-per-second behavior from allowing scheduler events to accumulate behind a bounded 2048-record consumer batch. `eventDrainSaturated` is a consumer batch-capacity indicator set when a drain fills all 2048 slots; it is not a kernel-side dropped-event counter, and a separate producer-loss counter remains future work. This command is not run automatically at daemon startup and does not apply tuning mutations. A device batch test can therefore verify the actual kernel's syscall/SELinux/verifier/attachment behavior without enabling the workload pipeline as a daemon policy actuator.


## GPU utilization and FrameTimeline adapters

The feature pipeline adds a capability-discovered GPU utilization reader. It prefers KGSL `gpubusy` first and treats its two values as an interval snapshot: each distinct read computes `busy / total * 100` directly, so interval rollover or decreasing values are valid. A `(0, 0)` snapshot reports zero utilization for power-collapse; nonzero busy with a zero denominator, `busy > total`, signed error sentinels, malformed pairs, and duplicate consecutive snapshots are rejected (duplicates are not counted twice). Generic devfreq `busy_time`/`total_time` remains a separate monotonic cumulative-counter source sampled using `Δbusy / Δtotal * 100`; reset, zero total delta, or impossible deltas leave that feature invalid. It then checks other bounded semantic sysfs candidates (KGSL percentage/devfreq, DRM, Mali-style utilization, and named generic devfreq devices). Direct percentage sources must be finite and within 0..100. Generic devfreq `busy_time`/`total_time` is accepted only from a GPU-named devfreq node and converted using monotonic counter deltas. The OEM `gpu_busy_percentage` node is a final fallback only, because some kernels expose a readable but permanently-zero compatibility value. GPU reads are rate-limited to 100 ms; unavailable or invalid values do not set `FeatureGpuBusy`.

FrameTimeline is collected only by the explicit `--workload-probe` diagnostic. The probe starts a bounded 1-second Perfetto system trace for `android.surfaceflinger.frametimeline` during its second feature window, launches `/system/bin/perfetto` directly (no shell), reads a capped trace file, and parses the protobuf `Trace.packet -> TracePacket.frame_timeline_event -> actual_display_frame_start` path. It counts actual DisplayFrames only, not per-app SurfaceFrames, to avoid counting multiple app surfaces against one display presentation. A miss is counted for late or dropped presentation, a false on-time-finish signal, or SurfaceFlinger CPU/GPU deadline/dropped jank bits. The percentage is `misses / actual_display_frames * 100`.

The Perfetto adapter is fail-closed: missing binary, unavailable data source, denied trace-output path, malformed trace, empty actual-display-frame stream, child failure, timeout or oversized trace leaves `FeatureFrameDeadline` invalid and the JSON field null. FrameTimeline requires Android 12+ and may omit SurfaceView-related work on the currently documented implementation. The `frameDeadlineMissPct` diagnostic is a system DisplayFrame late/drop/deadline ratio; it is not a claim that every application SurfaceFrame or every OEM's frame-drop KPI is covered. This trace capture is diagnostic-only and is not enabled as a permanent daemon trace session.

GPU busy percentage is interval utilization from the discovered driver source. It is not a per-frame GPU render duration and is not substituted for FrameTimeline. Missing GPU counters remain invalid. Both metrics must be validated against device-side evidence before any workload policy uses them for actuation.
