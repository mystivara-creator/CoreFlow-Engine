# Build Status — v2.0.0

This is the **v2.0.0 audited final source package** of CoreFlow Autonomous Engine.

## Release identity

| Component | Value |
|---|---|
| CoreFlow | `v2.0.0` |
| Module version | `v2.0.0` |
| Module versionCode | `2000` |
| Android ABI | `arm64-v8a` |
| Android minimum API | `34` |
| Android SDK API | `36` |
| Android NDK | `30.0.16248370` |
| ONNX Runtime Android | `1.24.3` |
| Thermal model SHA-256 | `422c64313947688d1cda942f3a6f6612a103b3449991361d66a8bb1839fcab97` |
| C++ standard | C++17 |

## Package contract

This archive is a **source package**, not a directly installable production module.

It contains:

- Complete C++17 source under `src/` and `include/`
- Android/Magisk module scaffolding under `module/`
- Deterministic host tests under `tests/`
- Android ARM64 production CI workflow under `.github/workflows/`
- Pinned thermal predictor model
- Source-release and safety verification scripts
- Release documentation and security/safety notes

It intentionally does **not** contain:

- `build/` or generated build directories
- Pre-built ARM64 `coreflowd`
- Pre-built `libonnxruntime.so`
- Extracted ONNX Runtime SDK/cache
- CI-generated production artifacts

Production Android binaries and the packaged ONNX Runtime library are produced by the CI workflow or the Android build helper from the externally supplied/pinned ONNX Runtime package.

## v2.0.0 release scope

v2.0.0 is the audited production source release of the autonomous adaptive control engine.

The source package includes:

- `ResourceStateModel` for observed/baseline/desired/verified resource state
- `ContextEngine` for workload and system-context classification
- `PolicyEngine` for plan/candidate generation
- `ActuatorManager` for common actuator registration and discovery
- `DecisionOutcome` and outcome classification
- `BaselineIntelligence`
- `ExperienceMemory` feedback for verified CPU/resource candidate outcomes
- Persistent scoped ExperienceMemory (device/kernel scoped, atomic, capped at 128 records)
- Runtime I/O throughput and throttled top-process workload profiling
- Battery level/status/current/voltage/temperature awareness
- Power-aware and I/O-aware mutation eligibility
- Existing mutation journal, safety hold, rollback and actuator safety boundaries

Generic resources are adaptive-capable only through the centralized `MutationAuthority`, scoped permit, policy allow-list, durable dirty-set journal, verification and causal outcome loop.

## v2.0.0 audited changes

- Fixed multi-policy rollback bookkeeping so rolling back one policy does not clear dirty state belonging to another policy.
- Journal cleanup now occurs only when no policy remains changed.
- Reduced `CONTEXT` diagnostic logging to eligibility changes and every 60 samples instead of every monitor tick.
- Aligned the default configuration documentation with the v2.0.0 release.
- Added regression coverage for multi-policy mutation and full restore behavior.
- Unified CPUFreq and generic-resource mutation under one authority and one causal adaptive loop.
- Restricted journals/restores to CoreFlow-owned dirty resources only.
- Runtime refresh restores both mutation domains before discovery and re-baselining.
- Added ASan/UBSan CI coverage and raw ARM64 binary provenance artifacts.
- Added persistent, scoped ExperienceMemory with atomic replacement and bounded retention.
- Expanded ContextEngine with I/O-bound, memory-bound and power-constrained workload states.
- Added low-overhead I/O telemetry and throttled process profiling; these paths are observational only.
- Added battery level/status awareness to power safety decisions.
- Integrated historical experience into candidate ranking without granting mutation authority.

## Safety status

The default module configuration is autonomous adaptive operation; the C++ safety authority remains the final gate:

```text
mutation_mode=adaptive
allow_cpu_governor=yes
mutation_armed=true
```

The safety contract requires, before autonomous mutation:

- Adaptive mutation mode
- Explicit mutation arming
- Explicit CPU governor permission
- A durable mutation journal committed before kernel-control writes
- A `MutationPermit` issued by the established mutation controller
- Read-back verification
- Restore/rollback handling
- Fail-closed behavior on journal corruption, failed recovery or missing actuator capability evidence

No Android/device write validation is claimed by this source package alone. A production Android artifact must be rebuilt by CI before installation.

## Validation contract

The source package provides:

```bash
tools/verify_source_release.sh
tools/verify_safety_foundation.sh
```

Host deterministic tests:

```bash
cmake -S . -B build/host -G Ninja   -DCORE_FLOW_BUILD_TESTS=ON   -DCMAKE_BUILD_TYPE=Debug

cmake --build build/host
ctest --test-dir build/host --output-on-failure --no-tests=error
```

Android production builds use the pinned CI contract:

- NDK `30.0.16248370`
- SDK platform `android-36`
- minimum native API `34`
- ABI `arm64-v8a`
- ONNX Runtime Android `1.24.3`

The CI workflow validates the minimum API input, source-package contract, thermal model presence, ONNX Runtime layout, Android Release configuration, ARM64 binary output, module assembly and packaged deployment.

## Thermal model integrity

The source package contains:

```text
module/system/etc/coreflow/thermal_predictor.onnx
```

Pinned SHA-256:

```text
422c64313947688d1cda942f3a6f6612a103b3449991361d66a8bb1839fcab97
```

Any model change requires an intentional digest update and release review.

## Release interpretation

`v2.0.0` is an audited source-package release. It is the stable public autonomous-adaptive release: the v1.x safety/control foundations are consolidated here, and the v2 workload, power, I/O and persistent-learning capabilities are active where the device exposes the required telemetry/capabilities. Device-specific resources remain capability-driven and fail closed.

It does **not** claim that a generated Android binary has been validated on every Android device, kernel, vendor implementation or hardware configuration.
