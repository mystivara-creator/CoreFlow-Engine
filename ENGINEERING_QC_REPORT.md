# CoreFlow Engineering QC Report — v2.0.1 (2026-10-09)

## Scope

Source baseline: CoreFlow v2.0.0 source package (`rebased/`). This report records the v2.0.1 release-hardening changes and the checks run in the available host environment. It does not certify device-specific behavior.

## Findings addressed

| # | Finding (from v2.0.0 review) | Resolution in v2.0.1 |
|---|---|---|
| 1 | Autonomous kernel mutation armed by default on every device | Default is observe-only (`mutation_mode=observe`, `allow_cpu_governor=no`, `mutation_armed=false`) in engine defaults and `default.conf`. Opt-in is explicit. |
| 2 | Release status claims inconsistent with QC verdict; "validated" model wording | BUILD_STATUS, README, SECURITY, CHANGELOG rewritten as release candidate. CI comment corrected. |
| 3 | Thermal model trained on circular synthetic targets | Not retrained (deferred until device traces exist). Provenance documented as synthetic-only; daemon falls back to heuristic when the model is not applicable. |
| 4 | Missing telemetry encoded as `0` at inference | `src/thermal_features.cpp` marks the vector incomplete when telemetry, history, trends or finiteness requirements fail; model is skipped. |
| 5 | ThermalGuard reacted to a single hot zone | Hottest-only trigger requires two consecutive samples. Rising-near-threshold uses representative sensor only. Hysteresis unchanged. |
| 6 | Two different thermal plausibility windows | Single definition in `include/coreflow/thermal_limits.hpp` (10–120 °C) used by observer and engine. |
| 7 | Training script reproducibility and partial-write risk | Export validated in memory and written atomically; provenance stated in the docstring. Library version pinning deferred with ONNX work. |

Additional defect found during QA: a malformed `mutation_mode` could be re-armed by a later `mutation_armed=true` line, because the parser was order-dependent. Fixed; covered by `testMalformedModeFailsSafe`.

Defect introduced and fixed during this revision: a dangling `thermal_delta` reference in the predictor log, caught by strict compilation of the Android sources.

## Checks run

Environment: Linux x86_64 container, g++ with C++17. CMake and Ninja were not available and could not be installed (no network), so the CMake/CTest flow was reproduced manually with the same source set and flags.

| ID | Check | Result |
|---|---|---|
| A | Core sources (18 files) compiled with `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wformat=2 -Wundef -Werror` | PASS (18/18) |
| B | `mutation_tests` | PASS, 332/332 checks |
| B | `architecture_tests` | PASS |
| B | `resource_tests` | PASS |
| B | `release_hardening_tests` (new, 71 checks) | PASS, 71/71 |
| C | All four host test binaries under AddressSanitizer + UBSan (`-fno-sanitize-recover=all`) | PASS, 0 sanitizer reports |
| D | Android-only daemon sources (`autonomous`, `thermal_predictor`, `observer`, `discovery`, `main`, `cpufreq_actuator`) compiled with strict `-Werror` against compile-only mocks of the ONNX Runtime C++ API and Android headers | PASS (6/6) |
| E | `tools/verify_source_release.sh` (includes 18-feature Python/C++ order check) | PASS |
| F | `tools/verify_safety_foundation.sh` | PASS |
| G | Shell syntax (`module/*.sh`, `tools/*.sh`) and Python compile of training script | PASS |
| H | Model SHA-256 `38a87e82a50fef0f896846a2bb685928b0c0c85bacc683238759ec930fcbb3c6` | Matches CI pin |
| N1 | Negative test: swapped feature names in header | Contract script fails (expected) |
| N2 | Negative test: `mutation_mode=adaptive` in `default.conf` | Safety script fails (expected) |

Full log: `QA_LOG.txt` in this package.

## Not verified in this environment

- CMake configure/build and CTest via the project's `CMakeLists.txt` (the new `coreflow_release_hardening` target and `ctest` registration have not been executed by CMake itself).
- Android NDK/ARM64 build and the packaged `coreflowd` binary.
- ONNX Runtime behavior: the mocks are compile-only. Model load, input-shape checks at init, and inference were not run.
- Real-device thermal sampling, mutation, rollback and ThermalGuard behavior.
- Model retraining on device traces (deferred by decision).

## Known limitations and risks

- Existing installations keep their `/data/adb/coreflow/config.ini`. Users upgrading from v2.0.0 with adaptive settings stay in adaptive mode until they change the file. Recommend a migration note in the release announcement.
- The thermal model remains synthetic-trained. Its prediction is used only inside the existing deviation guard, and the heuristic fallback is the default when telemetry is incomplete.
- Vendor-specific thermal zone naming and exclusion lists are still heuristic (`observer.cpp`).

## QC verdict

**Source-level QC for v2.0.1: PASS** for the checks listed above.

**Production release: NOT CERTIFIED.** Release readiness requires: (1) CI Android ARM64 build producing the artifact with the pinned digest, (2) CMake/CTest green in CI, (3) on-device validation of observe-only operation first, then of each opt-in mutation domain, and (4) rollback validation on at least one reference device. Once those pass, the release can be promoted from release candidate to stable.
