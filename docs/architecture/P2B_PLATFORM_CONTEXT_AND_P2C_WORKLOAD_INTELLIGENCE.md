# P2-B Platform Context & P2-C Workload Intelligence

## Scope

This branch establishes the platform identity contract, execution-tier policy selection, and event-driven workload telemetry interfaces. It does not yet load an eBPF program, attach tracepoints, mutate kernel controls, or claim HyperOS 4 KPI attainment.

## P2-B — Platform & Capability Context

`PlatformContextDetector` reads Android system properties through an injectable `AndroidPropertySource`. Production Android builds use the native system-property API; host tests use an in-memory fake. No shelling out to `getprop`, parsing arbitrary property files, or writing properties is involved.

Tier 1 is deliberately gated on all of the following:
- Manufacturer/vendor evidence identifies Xiaomi.
- Product brand independently corroborates Xiaomi, Redmi, or POCO.
- The explicit `ro.mi.os.version.name` marker identifies HyperOS.
- Parsed HyperOS major generation is exactly 2 or 3.

MIUI-only devices, ambiguous Xiaomi identity, HyperOS versions outside the supported 2/3 set, and all non-Xiaomi platforms use `UniversalFallback`. Vendor identity is descriptive; it is not mutation authority. Every controller still requires CDDP capability evidence, ownership, baseline, constraints, and verified readback.

`CoreFlowContext` owns the immutable `PlatformContextSnapshot` for its lifecycle generation. The read-only `--status` JSON exposes vendor, OS family, OS major, execution tier, policy family, confidence, and Tier 1 eligibility. This is the policy selector boundary for Tier 1 vs Tier 2. It must not be used to bypass OEM service ownership or capability discovery.

## P2-C — Event-driven telemetry contract

The workload header defines bounded fixed-width event records, a source interface, feature validity bits, and a classifier output. The classifier is telemetry-only: it has no coordinator, sysfs write, actuator, or mutation API.

Preferred data path:

`BPF capability discovery -> BPF tracepoints/ring buffer -> event aggregation -> feature windows -> confidence + hysteresis -> WorkloadClassification -> policy evaluation -> MutationCoordinator`

Fallback path:

`PSI + sysfs/procfs counters -> same feature windows -> same classifier -> policy evaluation`

The fallback does not fabricate unavailable signals as zero. Each feature carries a validity bit and source identity. Workload class names (Gaming, I/O-heavy, Jank-sensitive, CPU-bound, GPU-bound) are labels, not conclusions from package names alone. Classification requires multiple independent signals, minimum valid-feature coverage, confidence thresholds, dwell/hysteresis, and stale-event rejection.

The initial read-only BPF probe inventories BTF, tracefs, scheduler/block tracepoint IDs, and verifies the bpffs filesystem magic rather than treating a directory as a mount. It deliberately leaves syscall permission, ring-buffer map support, SELinux load permission, and loader verification unknown until the dedicated loader stage establishes them. The eBPF source is not selected unless all required capabilities are verified; this foundation therefore defaults to sysfs/procfs fallback.

## HyperOS 4-inspired KPI contract

These are target benchmarks, not guaranteed outcomes or pass/fail thresholds until their measurement methodology and device comparability are independently validated:
- Frame pacing: 0.08% normal average dropped frames; 0.15% after 9 hours.
- Camera cold launch: 808.8 ms.
- Thirty-app hot-launch aggregate: approximately 36.8 s, representing an 8.7% reduction.
- Memory preloading/cleanup: workload-triggered policy, only after pressure evidence and reclaim safety are established.

Tier 1 may evaluate the HyperOS 4-inspired policy family on supported HyperOS 2/3 devices. Tier 2 uses CoreFlow Base Engine. Neither tier is allowed to bypass capability checks, emergency arbitration, OEM ownership, settling windows, or rollback.

## Next P2-C implementation gates

1. Implement read-only BPF capability probe and tracepoint inventory.
2. Add a loader abstraction with bounded map/ring-buffer memory budgets and explicit SELinux/permission outcomes.
3. Add a no-BPF sysfs/PSI sampler with identical feature schema.
4. Add event loss, timestamp monotonicity, stale-window, and source-switch tests.
5. Implement workload features and classifier with replay fixtures, confidence/hysteresis and overhead budgets.
6. Integrate classifications as policy evidence only; mutation remains exclusively through the existing coordinator and transaction pipeline.
