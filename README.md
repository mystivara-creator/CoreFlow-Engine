# CoreFlow Autonomous Engine

### v1.9.2 — Audited Final · Ecosystem Intelligence & Control Foundation

CoreFlow Autonomous Engine is a native C++17 adaptive system-intelligence daemon for Android.

The v1.9.2 source package consolidates environment/capability discovery, unified resource state, context classification, policy planning, actuator boundaries, outcome representation and the existing mutation-safety foundation.

> **Package type:** Source release  
> **Version:** `v1.9.2`  
> **versionCode:** `1902`  
> **ABI:** `arm64-v8a`  
> **Android minimum API:** `34`  
> **Build SDK:** `36`  
> **NDK:** `30.0.16248370`  
> **ONNX Runtime Android:** `1.24.3`

## Release position

v1.9.2 is an **audited final source package**. It is not a pre-built production module.

The package contains the source, tests, module scaffolding, thermal model, CI workflow and validation tooling required to produce the Android ARM64 artifact.

Production binaries and the ONNX Runtime shared library are intentionally produced during the build process rather than shipped in the source package.

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
│ Charging / power / runtime   │
└──────────────┬───────────────┘
               ▼
┌──────────────────────────────┐
│ Capability Matrix             │
│ existence / read / write     │
│ permission / runtime verify  │
│ mutation readiness           │
└──────────────┬───────────────┘
               ▼
┌──────────────────────────────┐
│ Unified Resource State       │
│ observed → baseline →        │
│ desired → verified           │
│ constraints / eligibility    │
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
│ plan / candidate / risk     │
│ no final mutation authority  │
└──────────────┬───────────────┘
               ▼
┌──────────────────────────────┐
│ Actuator Manager             │
│ registration / discovery     │
│ CPUFreq adapter boundary     │
└──────────────┬───────────────┘
               ▼
┌──────────────────────────────┐
│ Mutation Safety Foundation   │
│ permit / journal / verify    │
│ rollback / safety hold       │
└──────────────────────────────┘
```

## v1.9 foundation components

### Environment & Capability Intelligence

The discovery layer inventories Android/kernel resources and records capability information such as existence, readability, writability, permission state, runtime verification and mutation readiness.

A writable path is **not** treated as permission to mutate it.

The resource vocabulary covers:

- CPUFreq and governors
- UClamp
- CPU sets / cgroups
- Scheduler controls
- Memory / VM tunables
- I/O queue controls
- GPU / devfreq
- Thermal resources
- Charging / power
- Android runtime and device environment

### ResourceStateModel

`ResourceStateModel` provides a unified state representation:

```text
observed → baseline → desired → verified
```

It also records resource constraints and can revoke mutation eligibility.

The resource model cannot manufacture the final mutation permit.

### ContextEngine

`ContextEngine` classifies runtime conditions including:

- idle
- interactive
- CPU-bound
- GPU-bound
- I/O-bound
- memory-bound
- sustained
- thermal-limited
- power-constrained

It produces contextual eligibility information; it does not directly write kernel state.

### PolicyEngine

`PolicyEngine` converts system context and verified resource state into a plan/candidate representation.

The v1.9 policy layer remains conservative and does not automatically grant mutation authority to newly discovered ecosystem resources.

### ActuatorManager

`ActuatorManager` provides the common registration, discovery and capability boundary for actuators.

CPUFreq remains the existing mutation adapter and remains behind the established safety controller.

### DecisionOutcome

`DecisionOutcome` and `OutcomeClass` represent the result of a decision/action cycle, including:

- neutral
- beneficial
- regressed
- safety blocked
- failed
- rolled back

This representation is intended to support future experience/learning layers without bypassing the safety authority.

### BaselineIntelligence and ExperienceMemory

v1.9.2 includes the baseline-intelligence and experience-memory foundation needed for the next development stage.

These components do not replace the established mutation authority or safety gates.

## Safety model

The default configuration is explicitly fail-closed:

```text
mutation_mode=disabled
allow_cpu_governor=no
mutation_armed=false
```

New ecosystem resources remain observation/plan-only by default.

Autonomous mutation requires the established safety path, including:

1. Adaptive mutation mode
2. Explicit mutation arming
3. Explicit CPU governor permission where applicable
4. Durable mutation journal
5. `MutationPermit` from the mutation controller
6. Actuator capability verification
7. Bounded mutation
8. Read-back verification
9. Restore/rollback handling
10. Safety-hold and recovery gates

The mutation controller remains the final authority for mutation.

A journal entry by itself does not imply that a live mutation exists; mutation state is derived from actuator evidence.

## v1.9.2 audited changes

The audited final release includes:

- Multi-policy rollback bookkeeping fix so one rollback cannot clear another policy's dirty state.
- Journal clearing only when no policy remains changed.
- Reduced `CONTEXT` diagnostic logging to eligibility changes and every 60 samples.
- Default configuration documentation alignment.
- Regression coverage for multi-policy mutation and full restore.
- Preservation of the v1.4.1 Tier-2 safety boundary.

## Default configuration

`module/system/etc/coreflow/default.conf`:

```text
monitor_interval=5
min_confidence=0.70
mutation_mode=disabled
allow_cpu_governor=no
runtime_refresh=true
mutation_armed=false
```

The production-safe default is observation-only.

## Thermal predictor

The source package includes:

```text
module/system/etc/coreflow/thermal_predictor.onnx
```

Pinned SHA-256:

```text
422c64313947688d1cda942f3a6f6612a103b3449991361d66a8bb1839fcab97
```

The CI workflow verifies the model before it is deployed into the production module.

## Source package contents

### Source

```text
include/coreflow/
src/
```

### Tests

```text
tests/architecture_tests.cpp
tests/mutation_tests.cpp
```

### Module

```text
module/
module/system/etc/coreflow/default.conf
module/system/etc/coreflow/thermal_predictor.onnx
```

### Build and validation

```text
.github/workflows/build.yml
tools/build_android.sh
tools/verify_safety_foundation.sh
tools/verify_source_release.sh
```

## Build — host tests

Requirements include CMake, Ninja and a C++17 compiler.

```bash
cmake -S . -B build/host -G Ninja   -DCORE_FLOW_BUILD_TESTS=ON   -DCMAKE_BUILD_TYPE=Debug

cmake --build build/host
ctest --test-dir build/host --output-on-failure --no-tests=error
```

## Build — Android ARM64

The production workflow uses:

```text
NDK:              30.0.16248370
Android SDK:      36
Minimum API:      34
ABI:              arm64-v8a
ONNX Runtime:     1.24.3
```

The workflow:

1. Verifies release version alignment.
2. Installs Java 17 and the Android SDK/NDK.
3. Validates the Android build contract.
4. Verifies the source-package contract.
5. Fetches and validates ONNX Runtime Android.
6. Verifies the thermal model.
7. Configures a Release Android build.
8. Builds and strips `coreflowd`.
9. Assembles the module.
10. Verifies the packaged deployment.
11. Produces the Android ARM64 ZIP artifact.

## Source-package validation

Run:

```bash
tools/verify_source_release.sh
```

The validator checks release metadata, required source files, absence of generated production artifacts, safety defaults, architecture components, documentation versioning and the pinned thermal-model digest.

Run the safety contract validation with:

```bash
tools/verify_safety_foundation.sh
```

## Production status

This source package does not contain:

- `coreflowd`
- `libonnxruntime.so`
- extracted ONNX Runtime SDK/cache
- generated build directories
- CI production artifacts

Therefore the source package itself is **not directly installable**.

A production Android artifact must be generated by the Android build workflow before installation.

## Scope and limitations

v1.9.2 is a source-package release establishing the ecosystem intelligence/control foundation while retaining the existing mutation safety authority.

The package does not claim validation across every Android device, vendor kernel, hardware implementation or kernel-control layout.

Device-specific policy validation remains required before enabling adaptive mutation.

## License

See `LICENSE`.
