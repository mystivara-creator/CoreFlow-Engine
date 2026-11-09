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

A readable BTF/tracefs path or bpffs mount is not proof that BPF_PROG_LOAD, tracepoint attachment or ring-buffer delivery is permitted. event_pipeline_ready remains false until syscall, ring-buffer, SELinux, loader verification and required tracepoints are positively established. The current code intentionally does not attempt to load an unverified object; fallback remains selected.

A raw-syscall loader creates a bounded BPF ring-buffer map, loads two GPL raw-tracepoint programs (sched_switch and sched_wakeup) with verifier logging, attaches both tracepoints, maps consumer/producer pages, drains fixed-size events, and closes attachments/programs/maps plus unmaps pages on every failure and stop path. BPF map creation is the syscall capability probe; EACCES/EPERM is reported as Restricted. SELinux load permission is considered established only after BPF_PROG_LOAD succeeds; loader verification is true only after both attachments and ring-buffer mappings succeed. Any failure leaves the backend inactive so the caller can use procfs fallback. The loader is not auto-started by the daemon and still requires device-side permission/overhead validation. The current event program records timestamp, PID/TID and CPU for scheduler switch/wakeup events. The pipeline derives a valid wakeup count per sampled window; PSI and disk counters are still obtained through procfs. It does not provide GPU busy, FrameTimeline, block latency or a verified dropped-event counter. Therefore it is an operational eBPF scheduler-event backend, not a complete all-domain workload telemetry implementation.

## Replay and host validation

tests/test_platform_context.cpp exercises synthetic procfs/PSI and deterministic replay windows: PSI parsing, diskstats delta units, invalid-feature preservation, source-agnostic classification, stale/future/duplicate rejection, exact freshness boundary, jank threshold boundary, two-window confirmation, transient candidate reset, dwell-gated workload transition and safe BPF prerequisite failure/stop. These are host fixtures, not physical-device telemetry or KPI results.
