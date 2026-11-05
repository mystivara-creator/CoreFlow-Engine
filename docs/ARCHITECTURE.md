# CFAE System Framework v3.1.0 — Mutation Safety & Coordinated Optimization Architecture

## 1. Architectural Principle

Zero device-specific hardcoding does not mean blind mutation. Ability to write to a sysfs/cgroup node proves only file access, not safety, semantic meaning, compatibility, or system benefit.

The primary unit of control in CFAE System Framework v3.1.0 is the **Mutation Transaction**: one explicit optimization goal, a dependency-verified change plan within a bounded risk budget, per-write readback verification, and ownership-safe rollback.

### Logical Architecture Flow

```text
Sensor & Capability Discovery
            |
            v
     State Normalization
            |
            v
      Reasoning Engine
            |
            v
    Dependency Analyzer
            |
            v
      Mutation Planner
            |
            v
     Safety Admission Gate
            |
            v
   Transaction Coordinator
            |
      +-----+------+
      |            |
      v            v
  Apply Order   Ownership Guard
      |            |
      +-----+------+
            |
            v
      Readback Verify
            |
            v
     Outcome Evaluator
            |
       +----+----+
       |         |
       v         v
    Retain    Rollback
       |
       v
  Outcome Learning
```

## 2. Five-Layer Safety Pipeline

1. **Reasoning Engine**: Combines workload observations, CPU/Memory/I/O PSI pressure, thermal status, and power state to emit an inspectable *Optimization Intent*.
2. **Dependency Analyzer**: Maps parameter relationships with explicit provenance (`Required-before`, `Requires`, `Coupled-with`, `Conflicts-with`, `Unknown`). Unknown relationships are strictly isolated and never grouped.
3. **Mutation Planner**: Constructs a deterministic transaction plan specifying baseline values, target values, range provenance, execution order, seven-dimensional risk assessment (`Low`, `Medium`, `High`, `Forbidden`), and abort conditions.
4. **Safety Admission Gate & Transaction Coordinator**: Enforces 10 pre-write admission checks, serializes conflicting transactions, writes the durable journal prior to the first write, applies mutations in dependency order, and verifies every write via readback.
5. **Outcome Evaluator & Learning**: Evaluates impact across Stability, Responsiveness, Pressure, Thermal, Power, Consistency, and Compatibility (`IMPROVED`, `NEUTRAL`, `REGRESSED`, `INCONCLUSIVE`, `ABORTED`, `OWNERSHIP_LOST`, `RECOVERY_REQUIRED`), retaining beneficial configurations while preventing unproductive repeat trials.

## 3. Active Production Core (C++23 + Rust 2024 + Kotlin 2.1)

- `src/daemon/main.cpp`: Daemon lifecycle, singleton PID lock, signal hardening, PSI reads, and coordinated state application.
- `src/daemon/reactor.cpp`: Zero-polling Linux `epoll` + `timerfd` + `signalfd` event loop.
- `src/kernel/uclamp.cpp`: Semantic cgroup v2 discovery across `/proc/self/mountinfo`, capacity encoding detection (`0..1024` vs `%`), and bounded top-app UClamp writes.
- `src/kernel/thermal_monitor.cpp`: Type-labelled multi-sensor thermal discovery and Android NDK thermal severity fusion.
- `src/kernel/baseline_store.cpp`: Write-ahead journal (`CFJ3`), object identity verification (`O_NOFOLLOW` + `fstat`), readback verification, and ownership-safe recovery.
- `rust/src/controller.rs`: Rust 2024 thermal/pressure state machine and slew-bounded PID capacity budget.
- `rust/src/bridge.rs`: Typed `cxx` FFI bridge between C++23 and Rust 2024.
- `src/kernel/frequency_controller.cpp`: CPUFreq and GPU devfreq ceiling actuation with OEM ownership verification.
- `src/kernel/autonomous_tuner.cpp`: Need-driven multi-domain block I/O scheduler (`mq-deadline`, `kyber`, `bfq`, `none`), `iosched/*` & queue tunable (`read_expire`, `writes_starved`, `fifo_batch`, `read_ahead_kb`, `nr_requests`, `rq_affinity`), and `/proc/sys/vm/*` evaluator with UCB1 Contextual Bandit selection and empirical Before/After utility ($\Delta U$) validation.
- `src/kernel/neural_predictor.cpp`: Hybrid ONNX Runtime C-API (`dlopen`) + Online Recursive Least Squares & 6×8×2 Micro-MLP 10-second horizon thermal/pressure predictor.
- `src/kernel/hardware_longevity.cpp`: Bounded battery step-charging ceiling reduction and cgroup v2 relative I/O weights.
- `src/kernel/power_bridge.cpp`: Nonblocking AIDL Power HAL capability discovery.
- `android/src/main/kotlin/io/coreflow/`: Kotlin 2.1 Android Quick Settings TileService and thermal broadcast integration.

## 4. Twelve Non-Negotiable Safety Invariants

1. No write without a valid captured baseline.
2. No mutation without known semantics, unit, and range provenance.
3. No grouped mutation without an explainable shared goal and verified dependency edge.
4. No two transactions may write conflicting resources concurrently.
5. Every write must be followed by immediate readback verification.
6. No rollback may overwrite an external OEM/Android modification (`OWNERSHIP_LOST` retires ownership cleanly).
7. No repeated retries of neutral/failed candidates without new evidence or material condition change.
8. No retention without a verified outcome and intact ownership.
9. No performance claims from unmeasured or unavailable metrics (such as synthetic FPS or jank).
10. Validated Android/OEM thermal and power policies remain the supreme safety boundary.
11. Durable journal write must precede mutation, and crash recovery must be idempotent.
12. When safety or ownership cannot be proven, the default action is no write.
