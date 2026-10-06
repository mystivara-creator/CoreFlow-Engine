# CoreFlow Autonomous Engine

### v1.0.0-A — Autonomous Foundation

CoreFlow Autonomous Engine is the Autonomous runtime layer of CoreFlow Engine for Android.

It is designed to observe runtime conditions, evaluate the current system state, make bounded adaptive decisions, apply supported CPU governor changes, verify the result, and restore a known baseline when required.

> **Release status:** Pre-release / non-production ready  
> **Version:** `v1.0.0-A`

---

## Overview

The `v1.0.0-A` release establishes the first frozen baseline of the CoreFlow Autonomous architecture.

The Autonomous layer focuses on:

- Runtime observation
- Runtime state evaluation
- Confidence-based decisions
- CPUFreq policy discovery
- Adaptive CPU governor selection
- Capability filtering
- Hysteresis
- Mutation verification
- Baseline restoration
- Runtime refresh
- Safe shutdown handling

The release intentionally freezes this Autonomous foundation before development continues into the Control Planner and SysFS execution layers.

---

## Architecture

```text
┌──────────────────────────────┐
│     Runtime Observation      │
│ memory / CPU / thermal /     │
│ charging / runtime signals   │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│      State Evaluation        │
│ IDLE / NORMAL / WARMING /    │
│ ELEVATED / PRESSURE /        │
│ THERMAL_GUARD                │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│     Adaptive Decision        │
│ workload + thermal pressure  │
│ + capabilities + hysteresis  │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│     Bounded Mutation         │
│ supported CPUFreq governor   │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│      Verification            │
│ write → read-back → verify   │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│   Restore / Rollback         │
│ baseline when required       │
└──────────────────────────────┘
```

---

## Adaptive Governor Selection

The production Adaptive path is not hardcoded to a single governor such as `schedutil`.

The engine first discovers the CPUFreq policies exposed by the device and evaluates only governors advertised by those policies.

Conceptually:

```text
CPUFreq Policy Discovery
        ↓
Available Governors
        ↓
Capability Filter
        ↓
Adaptive Evaluation
        ↓
Runtime-State Bias
        ↓
Hysteresis
        ↓
Selected Governor
        ↓
Apply
        ↓
Read-back Verification
```

Known governor candidates include:

- `powersave`
- `conservative`
- `schedutil`
- `walt`
- `performance`

A candidate is only eligible when the target CPUFreq policy reports it as available.

The Adaptive selector uses runtime conditions such as workload, thermal pressure, and current runtime state to choose a bounded candidate.

This is a bounded heuristic adaptive system; it does not claim that a selected governor is universally optimal for every Android device or workload.

---

## Runtime States

The Autonomous state machine currently works with:

```text
IDLE
NORMAL
WARMING
ELEVATED
PRESSURE
THERMAL_GUARD
```

State decisions are based on observed runtime signals and confidence.

Example runtime transition:

```text
THERMAL_GUARD
      ↓
    NORMAL
      ↓
   WARMING
      ↓
THERMAL_GUARD
```

The state machine is designed to react to changing runtime conditions rather than applying one permanent profile.

---

## Configuration

The validated runtime configuration is:

```ini
monitor_interval=5
min_confidence=0.70
mutation_mode=adaptive
allow_cpu_governor=yes
runtime_refresh=true
```

Runtime configuration:

```text
/data/adb/coreflow/config.ini
```

The Autonomous layer uses configuration and discovered capabilities as constraints rather than blindly applying predefined hardware assumptions.

---

## Verification

Mutations follow a basic verification flow:

```text
Discover
   ↓
Capture Baseline
   ↓
Check Capability
   ↓
Apply
   ↓
Read Back
   ↓
Verify
   ↓
Keep / Restore
```

The system should avoid treating a write operation as successful merely because the write call returned without an immediate error.

---

## Validation

The `v1.0.0-A` baseline was validated through host testing and Android device runtime testing.

### Host

```text
Deterministic tests: 3/3 passed
```

### Android ARM64

The release was built and packaged for the target Android ARM64 runtime.

### Device runtime

Observed during validation:

- `coreflowd` running successfully
- CPUFreq policy discovery working
- Multiple CPUFreq policies detected
- Adaptive governor transition observed
- `powersave → conservative` transition observed
- Repeated `ACTION VERIFIED` events
- Runtime confidence reaching `1.00`

Observed state transitions included:

```text
THERMAL_GUARD → NORMAL
NORMAL → WARMING
WARMING → THERMAL_GUARD
```

These results validate the tested device/runtime path. They do **not** imply that every Android device or kernel exposes identical CPUFreq interfaces or will behave identically.

---

## Release Status

### `v1.0.0-A`

This release is a **frozen Autonomous baseline** and is intentionally labeled as a **pre-release / non-production-ready release**.

Frozen does not mean:

- zero bugs
- universal Android compatibility
- guaranteed optimal performance
- guaranteed optimal battery life
- compatibility with every vendor kernel

Frozen means the Autonomous foundation has reached a defined architectural boundary and can now serve as the baseline for subsequent development.

---

## Future Architecture

The next phase is planned as:

```text
CoreFlow Autonomous
        ↓
Decision Layer
        ↓
Tunable Policy
        ↓
Control Planner
        ↓
SysFS Executor
        ↓
Read-back / Verification / Rollback
```

The SysFS layer should act as an executor and verifier.

It should not become a second decision-making layer.

---

## Versioning

CoreFlow follows this planned versioning direction:

```text
v1.0.x
    Maintenance / bug fixes

v1.1.x
    Feature releases

v2.x
    Architectural changes
```

`v1.0.0-A` represents the first frozen Autonomous baseline.

Future development should branch from this release rather than modifying the frozen release history directly.

---

## Repository Structure

The broader CoreFlow Engine repository is organized around the Autonomous implementation, supporting headers, tests, module integration, and build workflow.

The Autonomous layer is centered around:

```text
include/coreflow/
src/
tests/
module/
tools/
.github/workflows/
```

The main Autonomous implementation is:

```text
src/autonomous.cpp
```

---

## Safety Principles

CoreFlow operates close to Android/kernel-facing interfaces, so the Autonomous layer follows conservative principles:

1. Discover capabilities before mutation.
2. Capture a baseline before changing supported runtime controls.
3. Reject unsupported candidates.
4. Keep changes bounded.
5. Verify changes through read-back.
6. Restore the baseline when required.
7. Avoid making assumptions about unavailable hardware interfaces.
8. Fail closed when required runtime information is unavailable or invalid.

---

## Pre-release Notice

This project is provided for development, testing, and evaluation.

Kernel behavior, CPUFreq implementations, vendor modifications, thermal systems, permissions, and available governors can vary significantly between Android devices.

Use appropriate testing and recovery procedures when deploying privileged system software.

---

## License

MIT License.

---

## CoreFlow Autonomous

**`v1.0.0-A` — Frozen Autonomous Foundation**

```text
Observe
  ↓
Evaluate
  ↓
Decide
  ↓
Adapt
  ↓
Verify
  ↓
Restore
```
