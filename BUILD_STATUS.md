# Build Status — v2.0.1

This is the **v2.0.1 release-candidate source package** of CoreFlow Autonomous Engine. It supersedes v2.0.0 with release-hardening changes (observe-only defaults, fail-safe thermal inputs, shared plausibility bounds, confirmed hottest-zone guard, regression tests).

## Release identity

| Component | Value |
|---|---|
| CoreFlow | `v2.0.1` |
| Module version | `v2.0.1` |
| Module versionCode | `2001` |
| Android ABI | `arm64-v8a` |
| Android minimum API | `34` |
| Android SDK API | `36` |
| Android NDK | `30.0.16248370` |
| ONNX Runtime Android | `1.24.3` |
| Thermal model SHA-256 | `38a87e82a50fef0f896846a2bb685928b0c0c85bacc683238759ec930fcbb3c6` |
| C++ standard | C++17 |

## Package contract

This archive is a **source package**, not a directly installable production module.

It contains:

- Complete C++17 source under `src/` and `include/`
- Android/Magisk module scaffolding under `module/`
- Deterministic host tests under `tests/`
- Android ARM64 CI workflow under `.github/workflows/`
- Pinned thermal predictor model (synthetic-data baseline, see below)
- Source-release and safety verification scripts under `tools/`
- Release documentation and security/safety notes

It intentionally does **not** contain `build/` directories, a pre-built `coreflowd`, a pre-built `libonnxruntime.so`, an extracted ONNX Runtime SDK, or CI-generated artifacts.

## Release defaults

The release default is **observe-only**:

```text
mutation_mode=observe
allow_cpu_governor=no
mutation_armed=false
```

Kernel control is written only after an explicit operator opt-in (`mutation_mode=adaptive`, `mutation_armed=true`, `allow_cpu_governor=yes`). Existing installations keep their existing `/data/adb/coreflow/config.ini`; this default applies to new installations.

## v2.0.1 changes

- Default configuration changed from autonomous to observe-only (engine defaults, `default.conf`, and verification contracts updated accordingly).
- A malformed `mutation_mode` disables mutation regardless of the order of other keys in the file.
- Thermal plausibility bounds (10–120 °C) are defined once in `include/coreflow/thermal_limits.hpp` and used by both the observer and engine validation. The engine's previous -50 to 150 °C validation window was removed.
- Thermal model input vector (18 features) is built by `src/thermal_features.cpp`, a host-tested module. Missing telemetry is no longer encoded as `0`. An incomplete vector skips the model and uses the bounded heuristic.
- ThermalGuard: a trigger from the hottest policy-eligible zone alone must persist for two consecutive samples (`kHottestOnlyConfirmSamples`). Representative and predicted triggers still act immediately. The rising-near-threshold trigger now uses the representative sensor only. Hold/exit hysteresis is unchanged.
- Training script: export is validated in memory and written atomically; a failed parity check leaves no model file behind.
- New regression suite `tests/release_hardening_tests.cpp` (71 checks).
- The 18-feature order is cross-checked between the Python training script and the C++ header by `tools/verify_source_release.sh`.

## Thermal model status

`module/system/etc/coreflow/thermal_predictor.onnx` was trained by `tools/train_coreflow_thermal_predictor_v1_2_18f.py` **on synthetic trajectories**. Its holdout metrics describe the generator, not a real device, and the model is **not device-validated**. The daemon uses it only when the feature vector is complete and the prediction stays within the safe deviation of the live sensor; otherwise it falls back to the heuristic.

Retraining on real per-device traces (`--data`) is a planned follow-up and requires real telemetry from the target devices. Any model change requires an intentional digest update in `.github/workflows/build.yml` and in this document.

## Validation contract

Host deterministic tests:

```bash
cmake -S . -B build/host -G Ninja -DCORE_FLOW_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build/host
ctest --test-dir build/host --output-on-failure --no-tests=error
```

Source and safety contracts:

```bash
tools/verify_source_release.sh
tools/verify_safety_foundation.sh
```

Android production builds use the pinned CI contract (NDK `30.0.16248370`, SDK `android-36`, minimum API `34`, ABI `arm64-v8a`, ONNX Runtime Android `1.24.3`). CI validates the minimum API input, the source-package contract, the thermal model digest, the ONNX Runtime layout, the Release configuration, the ARM64 binary, the module assembly and the packaged deployment, and runs host tests under AddressSanitizer/UBSan.

## Release interpretation

`v2.0.1` is a **release candidate**. Source-level checks listed above are the evidence in this package. It does **not** claim:

- an Android ARM64 build was produced in the source-package environment,
- on-device behavior on any Android device, kernel, vendor implementation or hardware configuration,
- a device-validated thermal model.

Production distribution requires the CI Android artifact, plus device-level validation of the enabled mutation domains, before a release is marked stable.
