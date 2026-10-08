# Changelog

## v1.3.0 — Source Package

- Canonically aligned CoreFlow versioning to `1.3.0` / `versionCode=1300`.
- Aligned the source package, CMake project, runtime version constant, module metadata, CI artifact naming, release checks and documentation.
- Removed generated build output from the source package contract.
- Removed misleading one-byte binary/library placeholders from the source package; production binaries remain CI/local-build outputs.
- Unified the local Android build contract with CI: Android API 35, NDK 27.3.13750724 and externally supplied ONNX Runtime 1.24.3.
- Strengthened source-release validation around generated artifacts and version consistency.
- Kept the fail-closed default (`mutation_mode=disabled`) and existing journal/verification/restore safety model unchanged.

## v1.2.0-ONNX — Previous Source Release

- Integrated ONNX thermal predictor with pinned model and CI-fetched ONNX Runtime.
- Default configuration was fail-closed: `mutation_mode=disabled`.
- Added durable mutation journal, write-verify-restore path, and single-instance locking.

## v1.1.0-A — Baseline Intelligence (Development)

- Added `BaselineIntelligence` as a separate efficiency-evaluation layer.
- Preserved `MutationController` restore baseline as the safety baseline.
- Added five-sample factory/runtime baseline capture before mutation is allowed.
- Added five-sample post-mutation observation windows.
- Added Beneficial / Neutral / Regression / Inconclusive outcomes.
- Regression outcomes restore the MutationController baseline.
- Added deterministic Baseline Intelligence host tests.
- Integrated the feature into `AutonomousEngine::tick()`.
- Reset efficiency state on startup and runtime rediscovery.

## v1.0.0-A — Autonomous Foundation

- Runtime observation, state evaluation and confidence-based decisions.
- CPUFreq policy discovery and adaptive governor selection.
- Bounded, verified CPU-governor mutation with baseline restoration.
- Hysteresis, capability filtering and safe shutdown handling.
- Magisk module scaffolding and SELinux rules.

## Earlier foundation notes

- Native instance locking to prevent duplicate daemons.
- Safe SIGUSR1 runtime rediscovery.
- Persistent runtime configuration under `/data/adb/coreflow/`.
- Hardened Android build flags and CI checks.
