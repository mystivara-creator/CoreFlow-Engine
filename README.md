# CoreFlow Autonomous Engine

### v1.9.2 — Ecosystem Intelligence & Control Foundation

CoreFlow Autonomous Engine is a native adaptive system-intelligence daemon for Android. v1.9 consolidates the pre-v2 architecture on top of the v1.4.1 Tier-2 safety foundation and the v1.5 Environment & Capability layer.

> **Package type:** Source release  
> **Version:** `v1.9.2` / `versionCode=1901`  
> **Mutation status:** New ecosystem resources remain observation/plan-only by default.  
> **Binary:** Produced by CI or `tools/build_android.sh`; production binaries are not shipped in the source package.

## Architecture

```text
Android / Kernel Environment
          │
          ▼
┌──────────────────────────────┐
│ Environment Discovery        │
│ Android / kernel / cgroup    │
│ CPUFreq / UClamp / CPUSet    │
│ Memory / I/O / GPU / thermal │
└──────────────┬───────────────┘
               ▼
┌──────────────────────────────┐
│ Capability Matrix             │
│ existence / read / write     │
│ runtime verification         │
└──────────────┬───────────────┘
               ▼
┌──────────────────────────────┐
│ Unified Resource State       │
│ observed → baseline →        │
│ desired → verified           │
└──────────────┬───────────────┘
               ▼
┌──────────────────────────────┐
│ Context Engine               │
│ workload / thermal / memory  │
│ power / confidence           │
└──────────────┬───────────────┘
               ▼
┌──────────────────────────────┐
│ Policy Engine                │
│ plan / constraint / risk     │
│ no final mutation authority  │
└──────────────┬───────────────┘
               ▼
┌──────────────────────────────┐
│ Actuator Manager             │
│ common discovery/verification│
│ CPUFreq adapter today        │
└──────────────┬───────────────┘
               ▼
┌──────────────────────────────┐
│ v1.4.1 Safety Foundation     │
│ journal / permit / rollback  │
│ read-back / safety hold      │
└──────────────────────────────┘
```

## Ecosystem coverage

The common resource vocabulary covers:

- CPUFreq and governors
- UClamp
- CPU sets / cgroups
- scheduler controls
- memory / VM tunables
- I/O queue controls
- GPU / devfreq
- thermal resources
- charging / power
- Android runtime and device environment

Discovery is capability-driven. A writable path is **not** treated as permission to mutate it.

## v1.9 foundation components

### Unified Resource State Model

`ResourceStateModel` tracks:

`observed → baseline → desired → verified`

and records blocking constraints. It can revoke mutation eligibility, but it cannot manufacture the final `MutationPermit`.

### Context Engine

`ContextEngine` classifies runtime conditions such as idle, interactive, CPU-bound, memory-bound, sustained and thermal-limited states. It produces context eligibility only; it does not write kernel state.

### Policy Engine

`PolicyEngine` converts context plus verified resource state into a plan/candidate representation. v1.9 remains conservative: the engine does not automatically activate new ecosystem mutations.

### Actuator Manager

`ActuatorManager` provides a common registration, discovery and capability boundary for future actuators. CPUFreq remains the only existing mutation adapter and remains behind the established safety controller.

### Outcome model

`DecisionOutcome` and `OutcomeClass` provide the pre-v2 representation needed to distinguish neutral, safety-blocked, failed and rolled-back operations before learning is expanded in v2.

## Safety contract

- Default configuration is observation-only.
- New ecosystem resources are not mutation-ready merely because they are writable.
- Policy never owns the final actuator permit.
- `MutationController` remains the authority for CPUFreq mutation.
- Durable journal precedes kernel-control writes.
- Writes require verification and restore/rollback paths.
- Safety hold and recovery gates can force observe-only operation.

## Build

```bash
cmake -S . -B build -G Ninja -DCORE_FLOW_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure --no-tests=error
```

Android production builds use the pinned CI toolchain and ONNX Runtime package.
