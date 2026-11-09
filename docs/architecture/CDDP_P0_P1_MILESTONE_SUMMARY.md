# CoreFlow Architecture Milestone — CDDP P0 + Feature Accountability P1

**Milestone baseline:** 2026-11-09  
**Main baseline after P1 merge:** `51839dcdb4671629d535be5bedda6c29b9227da1`  
**Scope:** Architecture and source-accountability milestones; not physical-device acceptance.

## Executive summary

The P0/P1 work establishes a more truthful device-adaptive architecture for CFAE. The engine moves away from assumptions based on a small set of hardcoded SoC identities toward a captured device profile, explicit capability evidence, controlled mutation boundaries, and honest classification of features whose Android-facing actuators do not exist.

A green host/Android build proves build and deterministic-test acceptance only. It does not prove that a device becomes smoother, cooler, or more stable. Those claims remain subject to CF-DVS evidence and physical-device acceptance.

## P0 — CoreFlow Dynamic Device Profiling (CDDP)

### From SoC hardcoding to dynamic profiling

CDDP introduces root-injectable profiling and parsing instead of treating a SoC name or vendor family as a complete hardware description. The profile captures evidence from the running device, including:

- CPU possible/present/online sets and exact CPUFreq policy membership.
- CPUFreq policy bounds, available frequencies, topology, and capability classification.
- GPU devfreq domains and frequency tables where exposed.
- Thermal zones and hwmon sensor inventory/observations.
- Memory, cgroup, and UClamp availability.
- Normalized capability paths, source/domain, semantic identity, units, current values, accepted values, constraints, evidence, and rejection reasons.
- Device/kernel/Android/platform identity fields with privacy-conscious omission of serials and Android IDs.

The immutable `DeviceProfileHandle` is injected into relevant controllers, including `FrequencyController` and `AutonomousTuner`, so a tuning decision can refer to the same captured profile rather than independently rediscovering hardware.

### Capability State Machine and mutation eligibility

Capability discovery is not permission to mutate. A candidate capability must be classified from evidence and semantics; unavailable, malformed, unsupported, or ambiguous targets must fail closed. A write is only eligible when the node's meaning and accepted values are understood and the mutation path can preserve baseline ownership, coordinate access, verify readback, and recover/rollback.

This milestone treats a discovered path, driver feature bit, property, or writable file as evidence to evaluate—not as proof that a useful optimization exists. Capability state and controller eligibility must not be conflated with successful actuation or improved performance.

### P0 validation boundary

The CDDP host fixtures and deterministic tests validate parser behavior, capability classification, profile injection, and synthetic mutation/readback/restore paths. These tests do not establish correctness on every Android OEM/kernel combination. Target-device evidence is still required.

## CF-DVS alignment — PR #9 (not merged into main)

The CF-DVS work on PR #9 aligns device validation with the same immutable CDDP `DeviceProfileHandle` used by the daemon controllers. The session pins profile generation and fingerprint; the report carries the device profile and normalized capability evidence. This avoids a second independent profiler producing a potentially inconsistent view.

CF-DVS adds a versioned report contract, native session recorder, bounded asynchronous writer, root-only local control socket/CLI, integrity manifest, offline validator, and WebUI stop/export workflow. Its collector is capture-only: it does not mutate sysfs/procfs/cgroup/power/GPU/thermal nodes or acquire mutation leases. Device acceptance intentionally remains `INCONCLUSIVE` until evidence is collected and reviewed.

**Repository status at this milestone:** PR #9 remains Draft/open and is not part of `main`. Its implementation and test artifact must not be represented as shipped in the main-branch package until that PR is rebased/reconciled, passes exact-head CI, and is explicitly merged.

## P1 — Correct Feature Classification & Accountability

The P1 audit applies the rule that a class, controller, configuration option, telemetry field, or documentation entry is not proof of an operational feature. The status is based on the actual OS-facing actuator and verification chain.

| Domain | Classification | Current architectural boundary |
|---|---|---|
| ART runtime management | `NOT_IMPLEMENTED` | No ART runtime integration or actuator. |
| DEX2OAT compiler tuning | `NOT_IMPLEMENTED` | No dex2oat/ART service control or verified compiler-policy actuator. |
| JIT/AOT policy control | `NOT_IMPLEMENTED` | No runtime JIT or AOT profile control. |
| SurfaceFlinger refresh/frame control | `NOT_IMPLEMENTED` | No SurfaceFlinger service integration or display-mode actuator. |
| RenderBooster / touch burst | `NOT_IMPLEMENTED` | No trusted frame timeline source or OS mutation; synthetic 120 FPS and 8.33 ms metrics removed. |
| ANGLE backend selection/tuning | `NOT_IMPLEMENTED` | No ANGLE backend/service/property mutation path. |
| OpenGL/GLES optimization | `NOT_IMPLEMENTED` | No EGL/GLES interception or driver actuator. |
| Vulkan ICD/hardware probing | `OBSERVATION_ONLY` | Capability probing only. |
| Vulkan implicit layer | `OBSERVATION_ONLY` | Transparent dispatch/pass-through; not a renderer optimizer. |
| SkiaVK threaded renderer | `NOT_IMPLEMENTED` | No HWUI/Skia integration; runtime active flag forced false. |
| Adaptive VRS | `NOT_IMPLEMENTED` | No command submission/application integration; runtime active flag forced false. |
| Vulkan pipeline-cache prewarming | `NOT_IMPLEMENTED` | No application-owned cache prewarm backend; runtime active flag forced false. |
| Qualcomm Adreno/KGSL tuning | `OBSERVATION_ONLY` | No validated mutation of KGSL controls. |
| ARM Mali `power_policy` | `LIMITED_ACTUATION` | Narrow sysfs policy path with ownership/readback/rollback integration; not general GPU optimization and not yet proof of benefit. |
| Power HAL discovery | `OBSERVATION_ONLY` | Service/interface presence probe only. |
| Power HAL interception/hint submission | `NOT_IMPLEMENTED` | No Binder interception or hint submission; unsupported path reports failure. |

The P1 source changes add compile-time status constants and regression coverage, disable unsupported Vulkan optimization controls, remove synthetic RenderBooster metrics, and correct configuration/documentation claims. See `P1_FEATURE_ACCOUNTABILITY_REPORT.md` for evidence and promotion criteria.

## Acceptance gates and remaining work

1. **Architecture/build acceptance:** P0 CDDP and P1 feature-accountability source/tests must pass exact-head CI.
2. **CF-DVS integration acceptance:** PR #9 must be reconciled with current main and reviewed independently; its Draft status is intentional until accepted.
3. **Physical-device acceptance:** capture CF-DVS evidence on a rooted supported Android 15+ ARM64 device, inspect profile integrity and capability evidence, then test only eligible mutation paths.
4. **Effectiveness acceptance:** do not claim smoother rendering, reduced GPU load, thermal improvement, or stability gains without valid before/after measurements, settling/noise-floor analysis, and no-regression evidence.
5. **Release acceptance:** only a main-branch CI artifact built from the final main SHA is eligible as the current main test package. A PR artifact is not a main/release artifact.

## Final milestone statement

P0 establishes device-specific discovery and evidence-backed capability classification. P1 makes the feature inventory honest by separating observation, limited actuation, and unimplemented functionality. CF-DVS provides the planned evidence-capture bridge but remains a separate unmerged PR at this point. This milestone closes the architecture-accountability workstream; it does **not** declare physical-device validation or performance effectiveness complete.
