# CoreFlow Engine

Native C++ Runtime Optimization Engine for Android

![Android](https://img.shields.io/badge/Android-14%2B-green.svg)
![Architecture](https://img.shields.io/badge/Architecture-ARM64-blue.svg)
![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)

CoreFlow Engine is a C++17-based Android runtime optimization engine designed to improve the efficiency of existing factory/OEM configurations through runtime observation, adaptive evaluation, controlled mutation, and verification.

Rather than replacing the factory configuration with a universal tuning profile, CoreFlow treats the existing device configuration as the baseline and attempts to optimize around the conditions actually observed on the running device.

> **Efficiency — Stable — Smart — Balance**

**Project Status:** Active development  
**Target:** Android 14+ / ARM64

---

## Overview

CoreFlow is designed around a runtime-aware approach instead of applying a single permanent performance configuration.

The engine observes system conditions, evaluates the current runtime state, determines whether optimization is justified, applies bounded changes when appropriate, verifies the result, and restores the previous state when an optimization produces a regression.

The fundamental architecture is:

```text
Factory / OEM Configuration
          ↓
   Capture actual state
          ↓
    Factory Baseline
          ↓
   CoreFlow Autonomous
          ↓
 Observe → Evaluate → Optimize
          ↓
      Verify
          ↓
   Keep / Restore Baseline
```

The goal is not to replace OEM tuning.

The goal is to provide an adaptive runtime layer that can improve efficiency while preserving the stability-oriented factory baseline.

---

# Features

## Dynamic Runtime State Machine

CoreFlow classifies device conditions based on runtime metrics such as CPU activity, system load, memory availability, thermal conditions, charging state, and other available telemetry.

| State | Purpose |
|---|---|
| Daily Efficiency | Normal usage with focus on system efficiency |
| App Launch / Burst | Handles short workload spikes such as application launches |
| Gaming Unleashed | Handles heavier CPU/GPU workloads |
| Ultra Deep Sleep | Conservative behavior during idle/screen-off conditions |
| Thermal Guardian | Reduces tuning aggressiveness during thermal conditions |

The state machine is designed to prevent unnecessary parameter rewrites when the runtime state has not meaningfully changed.

---

## Autonomous Runtime Control

CoreFlow Autonomous extends the runtime state system into an observation-driven control pipeline.

```text
Device Discovery
      ↓
Runtime Observation
      ↓
Signal / Trend Analysis
      ↓
Confidence Evaluation
      ↓
Decision Engine
      ↓
Adaptive Optimization
      ↓
Apply Mutation
      ↓
Verify
      ↓
Measure Outcome
      ↓
KEEP / HOLD / RESTORE
```

The Autonomous engine uses:

- runtime observation
- signal trends
- confidence evaluation
- capability detection
- bounded mutation
- hysteresis
- verification
- cooldown / hold periods
- baseline comparison
- rollback

When the available evidence is insufficient, the engine may choose not to modify the system.

---

## Runtime Tuning

CoreFlow can interact with available kernel interfaces through the Android filesystem.

Potential interfaces include:

- CPU frequency/governor interfaces
- CPU scheduler-related parameters
- Storage I/O parameters
- ZRAM / VM parameters
- GPU interfaces on devices exposing KGSL
- Thermal information
- Relevant Android system properties

Not all devices expose the same kernel nodes.

CoreFlow therefore uses a capability-based approach where parameters are only considered when the required interface is actually available and usable.

---

## Capability-Based Device Adaptation

```text
Node available
      ↓
Validate
      ↓
Capture current value
      ↓
Apply tuning
      ↓
Verify
```

If a node is unavailable:

```text
Node unavailable
      ↓
Skip parameter
      ↓
Continue with available capabilities
```

This allows the engine to adapt to differences between SoCs, vendors, kernel versions, Android versions, custom kernels, exposed sysfs/procfs nodes, and device configurations.

---

## Adaptive Optimization

The production Adaptive path is designed to select valid runtime candidates without depending on experimental trial mutation.

```text
CPUFreq Discovery
      ↓
Candidate Governors
      ↓
Capability Filter
      ↓
Adaptive Scoring
      ↓
Best Valid Candidate
      ↓
Hysteresis
      ↓
Apply + Verify
      ↓
Observe Result
      ↓
Update Experience
```

Candidate governors may include:

- `powersave`
- `conservative`
- `schedutil`
- `walt`
- `performance`

CoreFlow first discovers what the running kernel actually provides.

---

# Baseline Intelligence

Baseline Intelligence measures whether an optimization actually improves efficiency compared with the factory/runtime baseline.

> **Is the configuration after CoreFlow's mutation actually more efficient than the factory baseline?**

```text
Factory Runtime
      ↓
5 Baseline Samples
      ↓
Efficiency Baseline
      ↓
Verified Mutation
      ↓
5 Observation Samples
      ↓
Outcome Evaluation
      ↓
BENEFICIAL / NEUTRAL / REGRESSION
      ↓
KEEP / HOLD / RESTORE
```

A mutation is not considered successful merely because the kernel accepted the new value.

| Outcome | Action |
|---|---|
| BENEFICIAL | Keep the optimization and enter observation hold |
| NEUTRAL | Hold the current state and continue observation |
| REGRESSION | Reject the mutation and restore baseline |
| INCONCLUSIVE | Avoid making an unsupported decision |

The system uses observation windows and hold/cooldown behavior to reduce mutation churn.

### Baseline responsibilities

**MutationController baseline**

- safety
- original parameter snapshots
- restoration

**Baseline Intelligence**

- efficiency comparison
- outcome evaluation
- determining whether an optimization should be kept

---

# Thermal Handling

CoreFlow includes thermal protection and thermal-aware runtime evaluation.

When thermal conditions exceed defined boundaries, CoreFlow can enter a thermal protection state and reduce tuning aggressiveness. When conditions return to normal, the engine can transition back toward the appropriate runtime state.

Thermal thresholds are engine configuration values and should not be interpreted as a guarantee that a device will maintain a specific temperature.

---

# Predictive Thermal Intelligence

CoreFlow includes a predictive thermal layer using an ONNX-based thermal predictor.

```text
RuntimeSample
     ↓
ThermalPredictor
     ├── ONNX prediction
     └── Heuristic fallback
     ↓
Predicted Thermal State
     ↓
Thermal Guard / Decision Engine
     ↓
Runtime Action
```

The machine-learning model is a predictor, not the final safety authority. Thermal policy and safety decisions remain outside the model.

If the ONNX model is unavailable or cannot be executed, CoreFlow can fall back to the heuristic prediction path.

---

# Thermal Predictor — 14 Feature Contract

| # | Feature |
|---:|---|
| 0 | `cpu_utilization` |
| 1 | `load1` |
| 2 | `mem_available_ratio` |
| 3 | `thermal_current_c` |
| 4 | `thermal_delta_c` |
| 5 | `charging` |
| 6 | `battery_temperature_c` |
| 7 | `battery_current_a` |
| 8 | `battery_voltage_v` |
| 9 | `uptime_delta_s` |
| 10 | `thermal_trend` |
| 11 | `memory_trend` |
| 12 | `load_trend` |
| 13 | `runtime_confidence` |

Prediction configuration:

- Horizon: **3 ticks**
- Tick interval: **5 seconds**
- Prediction horizon: **15 seconds**
- Runtime history: **6 samples**

Trend encoding:

| Trend | Value |
|---|---:|
| Unknown | `0.0` |
| Falling | `-1.0` |
| Stable | `0.5` |
| Rising | `1.0` |

The feature contract is intended to remain synchronized between the Python trainer and native C++ runtime predictor.

---

# Runtime Signals

CoreFlow evaluates multiple runtime dimensions.

### Thermal

- Current thermal value
- Thermal delta
- Thermal trend
- Battery temperature

### CPU / Load

- CPU utilization
- Load average
- CPU runtime state

### Memory

- Available memory ratio
- Memory trend

### Charging / Battery

- Charging state
- Battery current
- Battery voltage
- Battery temperature
- Charging telemetry availability

### Runtime Confidence

Runtime signals contribute to an overall confidence value, allowing the engine to distinguish reliable telemetry from incomplete or uncertain runtime conditions.

---

# Native Architecture

CoreFlow is implemented primarily as a native C++ daemon.

The engine does not depend on shell commands for every runtime tuning operation. It uses native interfaces such as:

- `std::ifstream`
- `std::ofstream`
- Android/Bionic system property APIs
- filesystem interfaces
- native runtime logic

The architecture is designed to keep runtime overhead low, not to claim zero overhead.

---

# CoreFlow Architecture

```text
                 ┌────────────────────┐
                 │ Device Discovery   │
                 └─────────┬──────────┘
                           ↓
                 ┌────────────────────┐
                 │ Runtime Observer   │
                 └─────────┬──────────┘
                           ↓
                 ┌────────────────────┐
                 │ Signal / Trend     │
                 │ Analysis           │
                 └─────────┬──────────┘
                           ↓
          ┌────────────────┴────────────────┐
          ↓                                 ↓
┌────────────────────┐          ┌────────────────────┐
│ Thermal Predictor  │          │ Baseline           │
│ ONNX / Heuristic   │          │ Intelligence       │
└─────────┬──────────┘          └─────────┬──────────┘
          ↓                               ↓
          └───────────────┬───────────────┘
                          ↓
                 ┌────────────────────┐
                 │ Policy / Decision  │
                 │ Engine             │
                 └─────────┬──────────┘
                           ↓
                 ┌────────────────────┐
                 │ Mutation Controller│
                 └─────────┬──────────┘
                           ↓
                 ┌────────────────────┐
                 │ Verify / Restore   │
                 └────────────────────┘
```

---

# Configuration Philosophy

CoreFlow follows the original:

**Detect → Validate → Snapshot → Tune → Monitor → Restore**

The Autonomous architecture extends this into:

```text
Detect
  ↓
Validate
  ↓
Snapshot Factory State
  ↓
Observe
  ↓
Evaluate
  ↓
Optimize
  ↓
Verify
  ↓
Measure Outcome
  ↓
Keep / Hold / Restore
```

The engine should observe first, mutate only when justified, and restore the baseline after a measured regression.

---

# GPU Detection

On devices exposing the KGSL interface, CoreFlow can use GPU busy information to help determine GPU workload.

On devices without KGSL telemetry, the engine can rely on other available runtime signals.

GPU telemetry availability is device-dependent.

---

# Compatibility

| Component | Target |
|---|---|
| Architecture | ARM64 / AArch64 |
| Android | Android 14+ |
| Language | C++17 |
| Root Framework | Magisk / KernelSU / APatch |
| GPU Telemetry | KGSL if available |
| Thermal ML | ONNX Runtime + heuristic fallback |

Actual compatibility depends on kernel implementation, vendor implementation, exposed sysfs/procfs nodes, permissions, Android version, and device configuration.

---

# Installation

CoreFlow is distributed as a module for Android environments that support systemless modules.

1. Download the appropriate release.
2. Install the module using a compatible root manager.
3. Reboot the device if required by the release.
4. Check CoreFlow logs to verify that the daemon started successfully.
5. Verify runtime state transitions and optimization activity.

Release filenames may change between versions. Always use the artifact provided by the corresponding release.

---

# Runtime Verification

For Autonomous runtime logs:

```bash
su -c 'logcat -d -s CoreFlowAutonomous:I *:S'
```

Useful runtime events include:

- runtime state changes
- thermal predictions
- thermal guard transitions
- efficiency observations
- mutation application
- mutation verification
- adaptive decisions
- efficiency outcomes
- rollback / restoration

Examples:

```text
THERMAL_ML
PREDICTIVE_THERMAL_TRIGGER
EFFICIENCY_OBSERVATION_BEGIN
EFFICIENCY_OUTCOME
EFFICIENCY_REGRESSION
ACTION ROLLED_BACK
```

Log tags and exact messages may change during development.

---

# Building

## Requirements

- Android NDK
- CMake 3.22+
- C++17-compatible compiler
- ONNX Runtime Android package for Android builds

## Host Build

```bash
cmake -S . -B build
cmake --build build
```

## Android ONNX Runtime

Android builds require:

```text
COREFLOW_ONNX_ROOT/
├── headers/
│   └── onnxruntime_cxx_api.h
└── jni/
    └── arm64-v8a/
        └── libonnxruntime.so
```

The CI workflow prepares the Android ONNX Runtime dependency and assembles the production module.

---

# Project Structure

```text
CoreFlow-Engine/
├── include/
│   └── coreflow/
│       ├── autonomous.hpp
│       ├── config.hpp
│       ├── controller.hpp
│       ├── discovery.hpp
│       ├── mutation.hpp
│       ├── observer.hpp
│       ├── policy.hpp
│       ├── signal_state.hpp
│       ├── thermal_predictor.hpp
│       └── types.hpp
├── src/
│   ├── main.cpp
│   ├── autonomous.cpp
│   ├── baseline_intelligence.cpp
│   ├── config.cpp
│   ├── discovery.cpp
│   ├── mutation.cpp
│   ├── observer.cpp
│   ├── policy.cpp
│   ├── signal_state.cpp
│   └── thermal_predictor.cpp
├── tests/
│   ├── config_tests.cpp
│   ├── mutation_tests.cpp
│   ├── policy_tests.cpp
│   └── baseline_intelligence_tests.cpp
├── module/
├── third_party/
├── tools/
├── .github/
├── CMakeLists.txt
├── CHANGELOG.md
├── CONTRIBUTING.md
├── LICENSE
└── README.md
```

The structure may change as the Autonomous architecture continues to mature.

---

# Development Branches

## CoreFlow-Autonomous-Engine

The foundational Autonomous Engine branch.

Focus:

- runtime observation
- state management
- autonomous decision flow
- capability detection
- bounded mutation
- verification
- factory baseline preservation

## CoreFlow-Autonomous-Observation-v1

A maturation branch built on top of the Autonomous foundation.

Focus:

- deeper runtime observation
- Baseline Intelligence
- efficiency comparison
- outcome evaluation
- rollback validation
- adaptive refinement
- predictive thermal intelligence

The Observation branch is intended to improve the foundation rather than replace it.

---

# Design Goals

CoreFlow is developed around:

- Runtime-aware optimization
- Factory/OEM baseline preservation
- Device capability detection
- Evidence-based mutation
- Adaptive runtime behavior
- Thermal awareness
- Predictive thermal analysis
- Baseline comparison
- Verified mutations
- Automatic restoration after regression
- Graceful handling of missing kernel interfaces
- Minimal dependency on shell scripting
- State-based configuration
- Parameter snapshot and restore
- Modular C++ architecture
- Observable runtime behavior

CoreFlow does not aim to provide one universal configuration that is optimal for every Android device.

Instead, it attempts to determine what is appropriate for the device and runtime conditions actually observed.

---

# Development Direction

```text
Factory Baseline
      ↓
Autonomous Foundation
      ↓
Runtime Observation
      ↓
Baseline Intelligence
      ↓
Adaptive Optimization
      ↓
Predictive Thermal Intelligence
      ↓
Verified Runtime Control
```

The long-term objective is not to create the most aggressive tuning engine.

The objective is to create a runtime system that can determine:

- when optimization is justified
- when optimization is unnecessary
- when evidence is insufficient
- when an optimization should be kept
- when the factory baseline should be restored

---

# Limitations

CoreFlow interacts with low-level Android and kernel interfaces that can differ significantly between devices.

Differences may include:

- SoC
- vendor
- kernel version
- custom kernel
- Android version
- sysfs/procfs layout
- permission model
- available governors
- available telemetry
- device configuration

Because of this, tuning behavior and measured efficiency can differ across devices.

The absence of a kernel node does not necessarily mean that CoreFlow fails overall. Features dependent on unavailable interfaces can be skipped while other capabilities continue operating.

No universal tuning configuration can be guaranteed to be optimal for every device.

---

# Disclaimer

CoreFlow modifies system parameters on devices with root access.

Using inappropriate kernel configurations can cause:

- performance changes
- increased power consumption
- increased temperature
- system instability
- parameters not working as expected

Always understand the device and kernel environment before experimenting.

Keep appropriate backups and preserve the original system state whenever possible.

**Use at your own risk.**

---

# Contributing

Bug reports, improvements, testing results, and pull requests are welcome.

When reporting an issue, please include where possible:

- Device model
- SoC
- Android version
- Kernel version
- Root framework
- Relevant runtime logs
- Problematic parameters/nodes
- Runtime state
- Observed efficiency outcome

---

# License

CoreFlow Engine is released under the MIT License.

See [LICENSE](./LICENSE) for the complete license text.

---

# Author

**Mystivara**

GitHub: [@mystivara](https://github.com/mystivara-creator)

Project: CoreFlow Engine

---

> CoreFlow Engine is an experimental Android runtime optimization project focused on improving the efficiency of factory-tuned configurations through observation, evidence-based adaptation, predictive intelligence, verified control, and safe restoration.
