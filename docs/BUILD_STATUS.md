# CoreFlow Autonomous v2.2.5 — Build and Validation Status

## Package identity

- Source package version: `v2.2.5` (`versionCode=2250`)
- Language standard: C++20
- Minimum Android API declared by the release: 34
- Default operating mode: `observe`
- Mutation defaults: `mutation_armed=false`, `allow_cpu_governor=no`

## Validation performed for this source archive

- Host CMake configure/build: passed with GCC 14.2.0 and `CORE_FLOW_BUILD_TESTS=ON`.
- Deterministic host test binaries: 8 of 8 passed.
- CTest suite: 10 of 10 tests passed, including safety foundation, WebUI status/render integration, and source-release contract.
- Source-release contract: passed after restoring the missing release documentation and synchronizing the WebUI fixture version with the engine version.

These are host-environment results, not Android certification. An Android ARM64/NDK production build, installation test, and device/recovery validation are **not verified by this document**. Do not interpret host tests as proof that any kernel control is safe on every device.

## Safety posture

The packaged defaults are observe-only. Mutation remains disarmed unless an operator explicitly configures Adaptive mode and arms it. CPU governor changes additionally require the separate CPU-governor opt-in. Discovery means an interface was observed; it does not by itself authorize a write. A candidate must still pass policy, permit, bounded-write, journal, read-back, and rollback safeguards.

## Reproducible host check

```sh
cmake -S . -B /tmp/coreflow-build -DCORE_FLOW_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/coreflow-build -j2
ctest --test-dir /tmp/coreflow-build --output-on-failure
```

Keep build output and extracted ONNX Runtime SDK/cache outside the source archive and repository.
