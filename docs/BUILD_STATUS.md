# CoreFlow v3.0.0 — Release freeze validation status

Honest status for this source tree. Prefer CI logs and device evidence over marketing language.

## Package

| Item | Value |
|------|--------|
| Version | `v3.0.0` (`versionCode=3000`) |
| Language | C++20 |
| Min Android API | 34 |
| Default mode | `observe` (`mutation_armed=false`) |

## What CI proves (when green on a commit)

GitHub Actions workflow `build.yml` runs:

1. **Host deterministic tests** — configure, build, `ctest`
2. **ASan + UBSan** — same tests with `-fsanitize=address,undefined`, leak detection enabled
3. **Android 14+ ARM64 Production** — NDK arm64-v8a build + release artifacts (`coreflowd`, ONNX package)

A green run on a given SHA (for example `8af1984` on `refactor/source-consolidation`) is evidence for **that SHA**, not for an arbitrary local tree.

## What this does **not** claim

- Universal device compatibility. If `coreflowd` starts and runs its observe/adapt loop on a device, that behavior is real on **that** device only.
- Field soak, boot-loop immunity, or SELinux path validation on every OEM.
- That the bundled thermal ONNX is calibrated per SoC. The model is shipped as an optional inference aid; the daemon remains functional without per-device model validation. Operators should treat model quality as device-specific.

## Hardening posture (source)

- Mutation writes: single FD, `O_NOFOLLOW`, `fstat` on that FD, kernel FS check, read-back verify, journal before write.
- Journals / status files: exclusive temps where applicable; reject symlink and non-regular journal inputs.
- Directory fsync for durable stores: `O_DIRECTORY | O_NOFOLLOW`.
- Defaults remain observe-only; Adaptive still requires arming and capability/policy/permit gates.

## Host reproduce

```sh
cmake -S . -B /tmp/coreflow-build -DCORE_FLOW_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/coreflow-build -j
ctest --test-dir /tmp/coreflow-build --output-on-failure
```

Keep build trees and NDK/ONNX caches outside the source package.
