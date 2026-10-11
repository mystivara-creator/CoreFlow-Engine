# Build Status — v2.1.0

## What changed from v2.0.1
- New `EffectModel` component (`include/coreflow/effect_model.hpp`, `src/effect_model.cpp`)
- `ResourceMutationController` uses discovery plus explicit per-resource policy authorization; writable paths alone do not qualify
- Block queue mutations are quarantined at discovery/baseline/candidate/actuator boundaries; journal restore remains enabled for recovery
- Live status/WebUI expose Android, kernel, SoC/board identity and separate discovered vs policy-ready counts
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
For the current source patch, host CMake build and CTest passed (6/6), the WebUI status writer/parser contract passed (166/166), and the end-to-end WebUI render smoke passed (23/23). The Android NDK/ARM64 build, ONNX linking and real-device behavior remain unverified; host tests are not device certification.

## Safety defaults
Unchanged: observe-only until operator sets `mutation_mode=adaptive` and `mutation_armed=true`.
