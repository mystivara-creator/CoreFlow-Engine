# Build Status — CoreFlow v2.2.0

## Package state

This is the final development snapshot for the current source line. It consolidates the capability-bound mutation safety work, consultation brain, read-only consultation ledger in `status.json`/WebUI, and expanded domain capability summaries. It is **not** an Android/device-certified production release.

## Current behavior and safety boundaries

- Installed defaults are Observe-only: `mutation_mode=observe`, `mutation_armed=false`, and `allow_cpu_governor=no`.
- New generic resource writes require exact policy authorization, a target-bound permit, journaling, bounded application, and read-back verification.
- New block queue writes (`scheduler`, `nr_requests`, `read_ahead_kb`) remain quarantined; journal-based restore of prior changes remains available.
- Build properties and Android Settings Database are observation-only; Settings DB collection is not implemented in this package.
- Consultation ledger and grants are informational/config-backed respectively. There is no remote WebUI grant/revoke endpoint and no grant TTL.

## Local validation in this package

The package must pass a fresh host CMake build, all CTest targets, WebUI status contract, WebUI render smoke, source-release contract, safety-foundation contract, and installer harness before handoff. Exact results are recorded in `FINAL_HANDOFF.md` and `FINAL_AUDIT_REPORT.txt`.

## Remaining external gates

1. Android NDK/ARM64 build and ONNX Runtime link in the operator's Android toolchain.
2. KernelSU/Magisk install/uninstall validation on supported devices.
3. Observe-only soak, journal recovery checks, then a separately reviewed controlled Adaptive trial.
4. Review and implement Settings DB observation only if there is a supported, permission-correct API path.

Host success does not imply Android or device certification. Keep installed devices in Observe mode until these gates pass.
