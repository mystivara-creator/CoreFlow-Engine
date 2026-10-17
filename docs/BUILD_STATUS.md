# CoreFlow Autonomous v2.3.0 — Build and Validation Status

## Package identity

- Source package version: `v2.3.0` (`versionCode=2300`)
- Language standard: C++20
- Minimum Android API declared by the package: 34
- Default operating mode: `observe`
- Mutation defaults: `mutation_armed=false`, `allow_cpu_governor=no`

## v2.3.0 hardening changes

- Mutation journals are written as checksummed v2 records through a unique, exclusively-created temporary file; v1 journals remain readable for crash recovery across upgrades.
- Journal reads reject symlinks, non-regular files, oversized files/records, duplicate paths, missing terminators, checksum mismatches, and embedded NUL bytes.
- Resource actuator writes use a single no-follow file descriptor, revalidate it before writing, require a kernel tunable filesystem (procfs/sysfs/cgroup), and verify the live value through read-back.
- Resource baseline capture fails closed when no recovery journal is configured.
- Default config remains observe-only; this release does not enable mutation by default.

## Validation status for this source archive

- Host CMake configure/build: passed with GCC 14.2.0 and `CORE_FLOW_BUILD_TESTS=ON`.
- CTest: 10 of 10 passed, including mutation/resource hardening, safety foundation, WebUI integration, and source-release contract.
- ASan + UBSan host build and CTest: 10 of 10 passed.
- Version alignment and source-release checks: passed.
- Android 14+ ARM64 CI has **not yet been run for the v2.3.0 changes**. The earlier v2.2.5 success is not evidence for the updated journal/actuator code. Installation, long-run stability, boot-loop protection, and recovery on a physical device remain operator validation.

## Safety posture

Discovery means an interface was observed; it does not by itself authorize a write. A candidate must pass policy and permit checks, have a durable pre-write journal, verify its actual result, and retain a tested recovery path. A corrupt journal or missing journal fails closed. CRC32 detects accidental journal corruption; it is not an authenticity mechanism against an attacker who can rewrite the journal and recompute the checksum.

## Reproducible host check

```sh
cmake -S . -B /tmp/coreflow-build -DCORE_FLOW_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/coreflow-build -j2
ctest --test-dir /tmp/coreflow-build --output-on-failure
bash tools/verify_safety_foundation.sh
bash tools/verify_source_release.sh
```

Keep build output and extracted ONNX Runtime SDK/cache outside the source archive and repository.
