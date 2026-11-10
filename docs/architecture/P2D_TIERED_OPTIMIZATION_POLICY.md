# P2-D — Tiered Optimization Policy Router

## Scope

This branch establishes header-level contracts only. No policy is activated by
the daemon and no new sysfs/procfs mutation is performed by these interfaces.

## Data path

`WorkloadTelemetryPipeline` -> `WorkloadClassifier` -> `PolicyEvidence` ->
`TieredPolicyRouter` -> `PolicyDecision` -> mutation admission ->
domain controller -> readback -> settling -> outcome evaluation.

A decision is not an authorization to write. Every mutation remains subject to
CDDP capability discovery, semantic validation, baseline/journal availability,
CentralMutationCoordinator ownership, readback verification, thermal safety,
and rollback.

## Policy contracts

- `CoreFlowBase` is the universal conservative policy.
- `HyperOS4Inspired` may tune thresholds and budgets for a corroborated
  Xiaomi HyperOS execution tier.
- Both policies consume the same immutable evidence and return the same
  `PolicyDecision` contract.
- HyperOS specialization cannot bypass safety, ownership, or recovery gates.
- Stale telemetry, unknown workload, saturated event drains, or invalid required
  features force Tier 0 / ObserveOnly.
- Tier 3 thermal recovery preempts performance objectives.

## Frequency target selection

`FrequencyTargetPlanner` selects only from kernel-exposed OPPs. An OPP is
eligible only when the capability snapshot confirms it is exposed, within the
independently validated hardware limit, and permitted by active constraints.
The currently observed OEM `scaling_max_freq` is not treated as the hardware
ceiling. Selecting above that observed setting is permitted only if the
independent hardware and active safety constraints explicitly allow it.
Thermal admission and capability fingerprint must be valid.

## Memory working-set warmup

`MemoryWarmupController` is an advisory page-cache optimization based on
validated file-backed working-set ranges. The implementation will use
`posix_fadvise(fd, offset, length, POSIX_FADV_WILLNEED)` and will enforce a
hard byte budget plus fresh memory/I/O/thermal admission. It must use safe file
opening (no symlink following), regular-file validation, overflow-safe range
checks, and cancellation on pressure or foreground-workload change.

WILLNEED is advisory: it does not guarantee residency, pin pages, or prevent
kernel reclaim. It must not be represented as application launch or RAM locking.

## Implementation sequence

1. Implement pure policy evaluation and deterministic tier/hysteresis tests.
2. Implement frequency capability enumeration and validated OPP selection.
3. Implement bounded WILLNEED with injectable file/advisory operations for host
   tests.
4. Integrate both controllers through existing mutation ownership and baseline
   recovery paths; no direct write path is allowed.
5. Add Android device tests for readback, thermal preemption, rollback, and
   workload transitions before enabling autonomous actuation.

## Physical telemetry status (Redmi Note 15 5G)

The supplied samples show eBPF drain batches of 7,348 and 9,523 events without
saturation, CPU PSI 18.27% with CPU-bound classification confidence 0.74, and
FrameTimeline available in one capture but unavailable in another. Therefore
FrameTimeline remains opportunistic. KGSL source discovery selects
`kgsl/gpubusy`, but zero valid delta samples means GPU utilization is unknown,
not zero. A longer sampling window / minimum inter-sample interval is required
before GPU evidence can influence policy.
