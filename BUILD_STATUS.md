# Build Status — v2.1.0

## What changed from v2.0.1
- New `EffectModel` component (`include/coreflow/effect_model.hpp`, `src/effect_model.cpp`)
- `ResourceMutationController` is capability-driven (no fixed resource name allow-list for candidates)
- `PolicyEngine` authorizes Low intervention under Warming/Elevated/active workloads
- Discovery matrix includes additional VM tunables
- Version bumped to 2.1.0 / versionCode 2100

## Still required before production certification
1. Android NDK ARM64 build of `coreflowd` (operator-managed CI)
2. On-device observe-only validation across the planned seven modes
3. On-device adaptive validation with rollback checks over the planned seven-day trial
4. Confirm telemetry quality, stale-sample handling, journal recovery, and boot-loop recovery
5. Optional: retrain thermal ONNX on representative device traces

## Local source validation
The source revision is exercised with the host CMake/CTest suite when available. This is a source-level check only and does not claim Android or device certification.

## Safety defaults
Unchanged: observe-only until operator sets `mutation_mode=adaptive` and `mutation_armed=true`.
