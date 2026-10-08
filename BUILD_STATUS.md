# Build Status — v1.9.2

This is the **v1.9.2 audited final source package** of CoreFlow Autonomous Engine.

## Release identity

| Component | Value |
|---|---|
| CoreFlow | `v1.9.2` |
| Module version | `v1.9.2` |
| Module versionCode | `1902` |
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

## v1.9.2 release scope

v1.9.2 is the audited final form of the ecosystem-intelligence/control foundation.

The source package includes:

- `ResourceStateModel` for observed/baseline/desired/verified resource state
- `ContextEngine` for workload and system-context classification
- `PolicyEngine` for plan/candidate generation
- `ActuatorManager` for common actuator registration and discovery
- `DecisionOutcome` and outcome classification
- `BaselineIntelligence`
- `ExperienceMemory` foundation for future learning integration
- Existing mutation journal, safety hold, rollback and actuator safety boundaries

Newly discovered ecosystem resources remain observation/plan-only by default. The policy/resource layers do not create the final mutation authority.

## v1.9.2 audited changes

- Fixed multi-policy rollback bookkeeping so rolling back one policy does not clear dirty state belonging to another policy.
- Journal cleanup now occurs only when no policy remains changed.
- Reduced `CONTEXT` diagnostic logging to eligibility changes and every 60 samples instead of every monitor tick.
- Aligned the default configuration documentation with the v1.9.2 release.
- Added regression coverage for multi-policy mutation and full restore behavior.
- Preserved the existing v1.4.1 Tier-2 safety boundary.

## Safety status

The default module configuration is observation-only:

```text
mutation_mode=disabled
allow_cpu_governor=no
mutation_armed=false
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

`v1.9.2` is an audited source-package release. It establishes the ecosystem intelligence/control foundation while retaining the existing safety authority for mutation.

It does **not** claim that a generated Android binary has been validated on every Android device, kernel, vendor implementation or hardware configuration.
