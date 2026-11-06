# CFAE System Framework v3.1.0 — C++23 + Rust 2024 + Kotlin Equilibrium & Mutation Safety Blueprint

## 1. Runtime Architecture & Language Implementation Stack

- **ISO C++23 Core (`src/daemon/`, `src/kernel/`, `include/coreflow/`)**: Handles semantic sysfs/cgroup v2 discovery, `ResourcePermissionClass` classification (`ReadOnly`, `Writable`, `WritableButUnsafe`, `UnknownSemantic`, `VerifiedConstraints`, `IncompleteMetadata`), `CentralMutationCoordinator` domain arbitration, Linux `epoll`/`timerfd`/`signalfd` event loop, `O_NOFOLLOW` + `fstat` (`dev:ino`) object identity checks, durable write-ahead journal (`CFJ4`), persistent bandit state (`CFTS2`), readback verification, and Android service capability probing.
- **Rust 2024 Decision Engine (`rust/src/`)**: Owns the memory-safe thermal PID controller, hysteresis state machine, PSI pressure arbitration, invalid-sample rejection, and slew-bounded capacity budget via `cxx` FFI.
- **Kotlin 2.1 Android Integration (`android/src/main/kotlin/io/coreflow/`)**: Provides native Android 15 (API 35) Quick Settings TileService (`CoreFlowTileService`) and thermal broadcast integration (`CoreFlowThermalReceiver`).
- **Material Design 3 WebUI (`webui/`, `module/webroot/`)**: Lightweight offline dashboard synchronized between `dist/` and `module/webroot/`.

## 2. Coordinated Mutation Transactions (6-Layer Pipeline)

Rather than rigid single-parameter-only rules or blind bulk writes, the primary unit of control is the **Coordinated Mutation Transaction**: one explicit optimization objective, a dependency-verified change plan within a bounded risk budget, domain ownership arbitration via `CentralMutationCoordinator`, per-write readback verification, settling-window + noise-floor validation, and ownership-safe rollback.

1. **Universal Capability Classifier**: Classifies every discovered kernel node into `ReadOnly`, `Writable`, `WritableButUnsafe`, `UnknownSemantic`, `VerifiedConstraints`, or `IncompleteMetadata`. Unknown or unsafe nodes are never mutated.
2. **Workload & Bottleneck Reasoning Engine**: Combines 11-state workload classification, CPU/Memory/I/O PSI, thermal headroom, and temperature trend into an inspectable `OptimizationObjective`.
3. **Active Thermal Stabilization & Hysteresis Recovery**: Switches objective to `ThermalStabilization` (`thermal.schedutil_damped_pacing`, `thermal.io_burst_pacing`) under non-emergency thermal load, and transitions through hysteresis-guarded `ThermalRecovery` when cooling.
4. **Central Mutation Coordinator & Dependency Analyzer**: Enforces single-mutation-per-domain leases, priority preemption (`EmergencySafety > ThermalStabilization > PressureGuard > WorkloadOptimization > BackgroundEfficiency`), and `RequiredAnchor -> RequiredDependent` ordering.
5. **Safety Admission Gate & `CFJ4` Journal**: Verifies pre-write invariants, commits the `CFJ4` write-ahead journal with `boot_id`/`boot_epoch` and `dev:ino` identity prior to the first write, applies changes in dependency order, and verifies every write via readback.
6. **Outcome Evaluator & `CFTS2` Persistent Learning**: Evaluates post-settling impact against the domain noise floor (`IMPROVED`, `NEUTRAL`, `REGRESSED`, `INCONCLUSIVE`, `ABORTED`, `OWNERSHIP_LOST`, `RECOVERY_REQUIRED`, `REJECTED`), steps down `adaptive_scale` on rollback, enforces context-aware anti-repeat cooldowns, and persists learning atomically in `CFTS2` format guarded by a capability fingerprint.

## 3. Cross-Device Thermal & Power Policy

Raw CPU/GPU junction, battery, and skin temperatures are distinct diagnostics and never share an arbitrary universal cutoff in native mode. Linux thermal-zone values are converted from the documented millidegree ABI.

Production decisions prioritize the Android NDK thermal severity and headroom API alongside recognized compute, battery, and skin zones evaluated against their read-only OEM passive/hot/critical trip points and secondary `hwmon` sensors. Missing OEM thermal policy retains the stock baseline and reports unavailable.

## 4. Predictive Controller & Slew Bounding

Rust consumes a dimensionless thermal-policy pressure index. Its PID controller includes a bounded integral and pressure velocity, bounding output to `0.4–1.0` with a maximum slew of 2% per elapsed second. CPU/GPU ceilings are quantized to exposed operating points while honoring existing OEM maximums.

## 5. Ownership Guard & Idempotent Recovery

For every mutation, `BaselineStore` records the baseline value, written value, last verified actual value, object identity (`dev:ino`), and ownership state. Rollback restores baseline values only if object identity matches and no external Android/OEM process has taken over (`OWNERSHIP_LOST` retires ownership without overwriting external changes). Crash recovery on startup is strictly idempotent.

## 6. Hardware Introspection & Preservation

- **CPU/GPU Ceilings**: Discovered from `cpufreq` and `devfreq` frequency tables. Missing tables disable that domain.
- **Battery Step-Charging**: Uses standard `constant_charge_current_max`, evaluates battery temperature exclusively while charging, and never raises an OEM current ceiling.
- **Storage I/O Weighting & Relative Queue Scaling**: Uses documented cgroup v2 `io.weight` (`default N` format) and scales block queue tunables (`read_ahead_kb`, `nr_requests`, `iosched/*`) relative to captured OEM baselines.
- **Truthful Vulkan Pass-Through**: `libVkLayer_coreflow.so` reports only physical GPU ICD capabilities (`safe_hardware`) without spoofing API versions or unsupported features.

