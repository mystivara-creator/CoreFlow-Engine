# CFAE System Framework v3.1.0 — Mutation Safety & Coordinated Optimization Architecture

## 1. Architectural Principle

Zero device-specific hardcoding does not mean blind mutation. Ability to write to a sysfs/cgroup node proves only file access, not safety, semantic meaning, compatibility, or system benefit.

The primary unit of control in CFAE System Framework v3.1.0 is the **Coordinated Mutation Transaction**: one explicit optimization objective, a dependency-verified change plan within a bounded risk budget, domain ownership arbitration via `CentralMutationCoordinator`, per-write readback verification, settling-window + noise-floor validation, and ownership-safe rollback.

### Unified Optimization Lifecycle

```text
Device Discovery
  -> Capability & Semantic Classification (ReadOnly | Writable | WritableButUnsafe | UnknownSemantic | VerifiedConstraints | IncompleteMetadata)
  -> Trustworthy System Observation (Multi-Sensor Thermal Fusion, CPU/Memory/I/O PSI, Frequency Residency)
  -> Workload Detection (11-State Evidence-Based Classifier)
  -> Bottleneck Diagnosis (Correlated Multi-Metric Evidence)
  -> Optimization Objective Selection (HoldBaseline | InteractiveLatency | ThroughputBalance | MemoryRelief | IoLatencyRelief | ThermalStabilization | ThermalRecovery | EmergencyMitigation)
  -> Candidate Generation & Adaptive Step Sizing (Relative to OEM Factory Baseline)
  -> Context-Aware Anti-Repeat & UCB1 Scoring
  -> Safety, Dependency & CentralMutationCoordinator Admission Gate
  -> Single-Mutation-Per-Domain Transaction (CFJ4 Write-Ahead Journal + Readback Verification)
  -> Domain Settling Period
  -> Noise-Floor Outcome Verification
  -> Retain / Rollback / Inconclusive / OwnershipLost
  -> Crash-Safe Persistent Learning (CFTS2 with Capability Fingerprint Validation)
```

## 2. Six-Layer Coordinated Pipeline

1. **Universal Capability Classifier**: Classifies every discovered kernel node into `ReadOnly`, `Writable`, `WritableButUnsafe` (e.g., virtual block queues `zram*`/`loop*`/`dm-*`, governor mismatch, or inactive `zram`), `UnknownSemantic`, `VerifiedConstraints`, or `IncompleteMetadata`. Nodes with `UnknownSemantic`, `IncompleteMetadata`, or `WritableButUnsafe` are strictly excluded from mutation.
2. **Workload & Bottleneck Reasoning Engine**: Classifies runtime state into 11 workload classes (`Idle`, `GeneralInteraction`, `ScrollingAndAnimation`, `Gaming`, `HeavyMultitasking`, `CpuBound`, `GpuBound`, `MemoryPressure`, `StorageIoPressure`, `ThermalConstrained`, `BackgroundWorkload`) and correlates multi-metric PSI and thermal headroom/trend to select an explicit `OptimizationObjective`.
3. **Active Thermal Stabilization & Hysteresis Recovery**: Rather than merely halting when thermal headroom narrows (`ThermalConstrained`), the engine transitions to `OptimizationObjective::ThermalStabilization` and applies verified thermal-relief transactions (`thermal.schedutil_damped_pacing`, `thermal.io_burst_pacing`) that damp frequency ramp spikes and burst depth relative to OEM baselines. Emergency thermal thresholds (`>= 88.0` pressure or hardware emergency trip) trigger immediate `EmergencyMitigation` rollback. Exiting thermal stabilization transitions through a hysteresis-guarded `ThermalRecovery` window before re-enabling aggressive performance candidates.
4. **Central Mutation Coordinator (`CentralMutationCoordinator`)**: Serializes mutations across domains (`CpuPolicy`, `BlockIo`, `VmMemory`, `UclampCgroup`, `GpuDevfreq`, `PowerLongevity`, `ThermalCore`), enforces single-mutation-per-domain ownership, and arbitrates controller priorities (`EmergencySafety > ThermalStabilization > PressureGuard > WorkloadOptimization > BackgroundEfficiency`).
5. **Transaction Executor & `CFJ4` Journal (`BaselineStore`)**: Captures immutable OEM baselines with device/inode identity (`O_NOFOLLOW` + `fstat`), commits the write-ahead journal (`CFJ4`) with per-boot `boot_id` / `boot_epoch` validation prior to any write, applies steps in strict `RequiredAnchor -> RequiredDependent` order, and verifies every write via immediate readback.
6. **Noise-Floor Evaluator & Persistent Learning (`CFTS2`)**: Applies domain-specific settling ticks, evaluates post-settling utility delta ($\Delta U$) against the running noise floor, steps down `adaptive_scale` (`0.25..1.0`) on rollback, enforces context-aware anti-repeat cooldowns keyed by workload class and thermal bucket, and persists learning atomically in `CFTS2` format guarded by a topology/capability fingerprint.

## 3. Implemented Production Core (C++23 + Rust 2024 + Kotlin 2.1)

Component presence is not feature completion. See `docs/architecture/P1_FEATURE_ACCOUNTABILITY_REPORT.md`. ART/DEX2OAT/JIT/AOT, SurfaceFlinger control, ANGLE, OpenGL tuning, and Vulkan rendering acceleration are not implemented. Vulkan ICD probing and pass-through are observation/compatibility plumbing. GPU kernel mutation is narrowly limited to Mali `power_policy`; Qualcomm KGSL remains observation-only. Power HAL detection does not intercept or issue hints.

- `src/daemon/main.cpp`: Daemon lifecycle, singleton PID lock, signal hardening, PSI reads, and coordinated state application.
- `src/daemon/reactor.cpp`: Zero-polling Linux `epoll` + `timerfd` + `signalfd` event loop.
- `src/kernel/uclamp.cpp`: Semantic cgroup v2 discovery across `/proc/self/mountinfo`, capacity encoding detection (`0..1024` vs `%`), and bounded top-app UClamp writes.
- `src/kernel/thermal_monitor.cpp`: Type-labelled multi-sensor thermal discovery (`thermal_zone*` + secondary `hwmon*`), Android NDK thermal severity/headroom fusion, and degraded-sensor resilience.
- `src/kernel/baseline_store.cpp`: Write-ahead journal (`CFJ4`), object identity verification (`dev:ino` via `O_NOFOLLOW` + `fstat`), `boot_id` / `boot_epoch` validation, readback verification, and ownership-safe recovery.
- `rust/src/controller.rs`: Rust 2024 thermal/pressure state machine and slew-bounded PID capacity budget.
- `rust/src/bridge.rs`: Typed `cxx` FFI bridge between C++23 and Rust 2024.
- `src/kernel/frequency_controller.cpp`: CPUFreq and GPU devfreq ceiling actuation with OEM ownership verification.
- `src/kernel/autonomous_tuner.cpp`: Universal resource classifier, `CentralMutationCoordinator`, workload/bottleneck/objective reasoning engine, relative baseline scaling (`compute_relative_u64` with `adaptive_scale`), active `ThermalStabilization` & `ThermalRecovery`, settling + noise-floor outcome evaluator, and `CFTS2` persistent learning.
- `src/kernel/vulkan_controller.cpp` & `src/layer/vk_layer_coreflow.cpp`: Vulkan ICD probing and transparent pass-through dispatch only (`OBSERVATION_ONLY`); no SkiaVK, VRS, pipeline-cache, or rendering optimizer. Mali `power_policy` is a separate limited GPU sysfs mutation.
- `src/kernel/neural_predictor.cpp`: Hybrid ONNX Runtime C-API (`dlopen`) + Online Recursive Least Squares & 6×8×2 Micro-MLP 10-second horizon thermal/pressure predictor.
- `src/kernel/hardware_longevity.cpp`: Bounded battery step-charging ceiling reduction and cgroup v2 relative I/O weights.
- `src/kernel/power_bridge.cpp`: AIDL Power HAL presence discovery only (`OBSERVATION_ONLY`); Binder interception and hint submission are `NOT_IMPLEMENTED`.
- `android/src/main/kotlin/io/coreflow/`: Kotlin 2.1 Android Quick Settings TileService and thermal broadcast integration.

## 4. Twelve Non-Negotiable Safety Invariants

1. No write without a valid captured baseline (`CFJ4` with `dev:ino` verification).
2. No mutation without `VerifiedConstraints` classification, known semantics, unit, and range provenance.
3. No grouped mutation without an explainable shared goal and verified dependency edge (`RequiredAnchor` before `RequiredDependent`).
4. No two controllers or transactions may write conflicting domains concurrently (`CentralMutationCoordinator` lease arbitration).
5. Every write must be followed by immediate readback verification.
6. No rollback may overwrite an external OEM/Android modification (`OWNERSHIP_LOST` retires ownership cleanly).
7. No repeated retries of neutral/regressed candidates in the same workload/thermal context without cooldown expiry and adaptive step-size reduction.
8. No retention without a verified outcome above the noise floor and intact ownership.
9. No performance claims or capability spoofing from unmeasured or unsupported hardware features.
10. Validated Android/OEM thermal and power policies remain the supreme safety boundary.
11. Durable journal write must precede mutation, and crash recovery must be idempotent across reboots.
12. When safety, semantics, or ownership cannot be proven, the default action is no write.

