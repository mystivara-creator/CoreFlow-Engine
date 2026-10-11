# CoreFlow Engineering QC Report — v2.1.0 source safety follow-up

## Scope

This patch starts from the supplied `CoreFlow-Engine-Autonomous-Engine` source archive and keeps its `v2.1.0` / `versionCode 2100` identity. It is a source hardening continuation, not a release declaration.

## Changes in this patch

| Area | Change |
|---|---|
| Capability evidence | Added `policy_authorized` separately from OS access/readability and model inference. A discovered writable node is not automatically eligible. |
| Storage safety | Quarantined new writes to block queue `scheduler`, `nr_requests`, and `read_ahead_kb` paths at discovery, baseline capture, fallback synthesis, selection and central actuator. |
| Recovery-first | The write quarantine does not prevent restore of previously journaled paths, so old changes can still be rolled back. |
| Device intelligence | Live status exports Android/API, manufacturer/model, SoC/board, kernel/ABI, interface facts, and discovered vs policy-ready counts. |
| WebUI | Added an observed device/ecosystem profile and explicit indication that block queue mutations are quarantined. |
| Knowledge base | Added `KERNEL_KNOWLEDGE_BASE.md` with evidence levels, policy scope, review gates and authoritative documentation links. |

## Checks performed in this work session

- Host configure/build with strict warnings and `-Werror`: **passed**.
- CTest: **6/6 passed** (including source-release and safety-foundation contracts).
- WebUI status writer/parser contract: **166/166 passed**.
- End-to-end WebUI render smoke test: **23/23 passed** when run independently after the combined invocation hit its time limit.
- Strict-warning syntax-only compile of `src/discovery.cpp` with a stub Android system-properties header: **passed**; this is not an Android NDK build.
- ASan/UBSan: **not run in this session**.

## Not verified here

- Android NDK/ARM64 `coreflowd` link and ONNX Runtime integration.
- GitHub Actions workflow or artifact digest.
- Actual Android `status.json` permissions/SELinux behavior.
- Real-device Observe soak, Adaptive VM mutation, old-journal recovery, or storage regression root cause.

## Acceptance state

**Host source checks passed; Android/device release is not certified.** Keep the device on Observe until the patch is reviewed, the operator's ARM64 CI is green, and staged device validation confirms no unexpected writes, stable storage and working recovery. The earlier storage issue correlated with an observed `loop0` scheduler mutation but has not been causally proven; the current policy treats it as a safety incident worth preventing against, not as confirmed root cause.
