# CoreFlow v3.1.1 — Release checkpoint validation status

Honest status for this source tree. Prefer CI logs and device evidence over marketing language.

## Package

| Item | Value |
|------|--------|
| Version | `v3.1.1` (`versionCode=3110`) |
| Language | C++20 |
| Min Android API | 34 |
| Installed operation | Automatic bounded optimization after boot |

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

## Hardening & low-overhead posture (source)

- Mutation writes: single FD, `O_NOFOLLOW`, `fstat` on that FD, kernel FS check, read-back verify, journal before write.
- Authority boundaries: `scaling_governor` belongs exclusively to `MutationController`; `scaling_min_freq`, `scaling_max_freq`, and `scaling_cur_freq` remain PowerHAL/observe-only across `Consultation`, `TunableDiscovery`, `EcosystemKnowledge`, and `ResourceActuator`.
- Journals / status files: exclusive temps where applicable; `EINTR`-safe writes; reject symlink and non-regular journal inputs.
- Directory fsync for durable stores: `O_DIRECTORY | O_NOFOLLOW`.
- Low-overhead daemon loop: single-timer signal-interruptible `clock_nanosleep` (zero slice-polling wakeups during deep sleep), exact interactivity boost-expiry wake clamping, allocation-free numeric helpers and ASCII matching on selected telemetry/classification paths, and non-spinning arena-free ONNX Runtime CPU execution.
- Installed configuration enables bounded optimization automatically. Uninitialized/invalid configuration still fails closed; capability, domain-grant, journal and safety-hold checks remain.

## Host reproduce

```sh
cmake -S . -B /tmp/coreflow-build -DCORE_FLOW_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/coreflow-build -j
ctest --test-dir /tmp/coreflow-build --output-on-failure
```

Keep build trees and NDK/ONNX caches outside the source package.

## v3.1.1 maintenance candidate

See [the checkpoint record](CHECKPOINT_3.1.1.md) for the exact local checks,
persistence behavior, known environment limits and required device acceptance.
Source checks are complete locally; a new Android CI run and on-phone acceptance
for the final commit are still required before calling a binary release validated.


## Native maintenance audit

See [the native audit](NATIVE_AUDIT_2026-11.md) for allocation-failure boundaries,
strict vendor parsing, ranking work reduction, advisory permission revocation and
experience V3 compatibility. These changes require their own green Android CI run
and device acceptance; earlier green commits do not validate the new tree.


## Coordinated thermal plans

The [coordinated plan record](COORDINATED_THERMAL_PLANS.md) defines the new
condition-driven cross-domain selection, thermal stabilization, dependency recovery and group
learning boundaries. Validate this commit's CI and phone behavior independently
of the preceding native maintenance audit.
