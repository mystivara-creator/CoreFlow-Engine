# CFAE System Framework v3.0.R — C++23 + Rust 2024 + Kotlin Equilibrium & Mutation Safety Blueprint

## 1. Runtime Architecture & Language Implementation Stack

- **ISO C++23 Core (`src/daemon/`, `src/kernel/`, `include/coreflow/`)**: Handles semantic sysfs/cgroup v2 discovery, Linux `epoll`/`timerfd`/`signalfd` event loop, `O_NOFOLLOW` + `fstat` object identity checks, durable write-ahead journal (`CFJ3`), readback verification, and Android service capability probing.
- **Rust 2024 Decision Engine (`rust/src/`)**: Owns the memory-safe thermal PID controller, hysteresis state machine, PSI pressure arbitration, invalid-sample rejection, and slew-bounded capacity budget via `cxx` FFI.
- **Kotlin 2.1 Android Integration (`android/src/main/kotlin/io/coreflow/`)**: Provides native Android 15 (API 35) Quick Settings TileService (`CoreFlowTileService`) and thermal broadcast integration (`CoreFlowThermalReceiver`).
- **Material Design 3 WebUI (`webui/`, `module/webroot/`)**: Lightweight offline dashboard synchronized between `dist/` and `module/webroot/`.

## 2. Coordinated Mutation Transactions (5-Layer Pipeline)

Rather than rigid single-parameter-only rules or blind bulk writes, the primary unit of control is the **Mutation Transaction**: one explicit optimization goal, a dependency-verified change plan within a bounded risk budget, per-write readback verification, and ownership-safe rollback.

1. **Reasoning Engine**: Combines workload observations, CPU/Memory/I/O PSI, thermal headroom, and power state into an inspectable *Optimization Intent*.
2. **Dependency Analyzer**: Evaluates parameter relationships with explicit provenance (`Required-before`, `Requires`, `Coupled-with`, `Conflicts-with`, `Unknown`). Unknown relationships are never grouped.
3. **Mutation Planner**: Builds a transaction plan with seven risk dimensions (`Semantic uncertainty`, `Platform uncertainty`, `Interaction risk`, `Reversibility`, `System impact`, `Observability`, `Ownership confidence`) and classifies risk (`Low`, `Medium`, `High`, `Forbidden`).
4. **Safety Admission Gate & Transaction Coordinator**: Verifies 10 pre-write invariants, serializes conflicting transactions, writes the durable journal before the first write, applies changes in dependency order, and verifies every write via readback (`PROPOSED → ADMISSION_CHECK → PREPARED → APPLYING → READBACK_VERIFY → OBSERVING → COMMITTED → RETAINED`).
5. **Outcome Evaluator & Learning**: Evaluates impact across Stability, Responsiveness, Pressure, Thermal, Power, Consistency, and Compatibility (`IMPROVED`, `NEUTRAL`, `REGRESSED`, `INCONCLUSIVE`, `ABORTED`, `OWNERSHIP_LOST`, `RECOVERY_REQUIRED`), retaining beneficial settings while preventing unproductive repeat trials.

## 3. Cross-Device Thermal & Power Policy

Raw CPU/GPU junction, battery, and skin temperatures are distinct diagnostics and never share an arbitrary universal cutoff in native mode. Linux thermal-zone values are converted from the documented millidegree ABI.

Production decisions prioritize the Android NDK thermal severity API alongside recognized compute, battery, and skin zones evaluated against their read-only OEM passive/hot/critical trip points. Missing OEM thermal policy retains the stock baseline and reports unavailable.

## 4. Predictive Controller & Slew Bounding

Rust consumes a dimensionless thermal-policy pressure index. Its PID controller includes a bounded integral and pressure velocity, bounding output to `0.4–1.0` with a maximum slew of 2% per elapsed second. CPU/GPU ceilings are quantized to exposed operating points while honoring existing OEM maximums.

## 5. Ownership Guard & Idempotent Recovery

For every mutation, `BaselineStore` records the baseline value, written value, last verified actual value, object identity, and ownership state. Rollback restores baseline values only if object identity matches and no external Android/OEM process has taken over (`OWNERSHIP_LOST` retires ownership without overwriting external changes). Crash recovery on startup is strictly idempotent.

## 6. Hardware Introspection & Preservation

- **CPU/GPU Ceilings**: Discovered from `cpufreq` and `devfreq` frequency tables. Missing tables disable that domain.
- **Battery Step-Charging**: Uses standard `constant_charge_current_max`, evaluates battery temperature exclusively while charging, and never raises an OEM current ceiling.
- **Storage I/O Weighting**: Uses documented cgroup v2 `io.weight` (`default N` format) for relative scheduling priority without modifying global VM writeback ratios.
