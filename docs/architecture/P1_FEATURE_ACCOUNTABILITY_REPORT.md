# P1 — Correct Feature Classification & Accountability Report

**Audit baseline:** 2026-11-14  
**Audited ref:** `feat/p4-native-actuators-end-to-end` at `57fd39ac505792780f62559dd89f3ecf697a5c08`

## Acceptance rule

A class, UI toggle, config key, telemetry field, layer manifest, or documentation statement is not proof of an operational feature. Promotion requires a concrete Android-facing actuator, capability/semantic validation, ownership/recovery integration, readback or platform acknowledgement, and deterministic tests.

## Domain classification

| Domain / feature | Status | Evidence and boundary |
|---|---|---|
| ART runtime management | `NOT_IMPLEMENTED` | No ART runtime integration, service client, or actuator in current source. |
| DEX2OAT compiler tuning | `NOT_IMPLEMENTED` | No dex2oat invocation, ART service interface, compiler-filter/threads actuator, or verified compiler configuration path. Historical v3.0.1 claim is superseded. |
| JIT/AOT policy control | `NOT_IMPLEMENTED` | No JIT runtime control, compilation daemon interface, or AOT profile actuator. |
| SurfaceFlinger refresh/frame control | `NOT_IMPLEMENTED` | No SurfaceFlinger service integration or display-mode actuator. |
| RenderBooster / touch burst | `LIMITED_ACTUATION` | Verified evdev touch events request bounded UClamp bursts; actual FrameTimeline windows feed jank response through a worker source. Physical Perfetto availability and device effects remain unverified. No display refresh-rate actuation. |
| ANGLE backend selection/tuning | `NOT_IMPLEMENTED` | No ANGLE service/property/backend integration or verified mutation path. |
| OpenGL / GLES optimization | `NOT_IMPLEMENTED` | No EGL/GLES interception, driver control, or application rendering actuator. |
| Vulkan ICD / physical-device probing | `OBSERVATION_ONLY` | Native ICD probing reads hardware capabilities; it does not alter the ICD or application rendering. |
| Vulkan implicit layer | `LIMITED_ACTUATION` | Transparent dispatch forwarding and VkPipelineCache lifecycle persistence hooks exist. Android process-level implicit-layer loading and persistent reuse are not proven by daemon layer-file detection. No shader, command-buffer, frame pacing, or renderer optimization. |
| SkiaVK threaded renderer | `NOT_IMPLEMENTED` | No HWUI/Skia integration. Native snapshot flag hardcoded false regardless of config. |
| Adaptive VRS | `NOT_IMPLEMENTED` | Feature discovery is not an actuator; no VRS command submission or app integration. Native flag hardcoded false. |
| Vulkan pipeline-cache prewarming | `NOT_IMPLEMENTED` | No application pipeline-cache ownership/prewarm path. Native flag hardcoded false. |
| Qualcomm Adreno / KGSL tuning | `DORMANT_GUARDED_ACTUATOR` | `FrequencyController::add_kgsl()` validates paired writable power-level nodes and table cardinality; `apply_gpu_frame_policy()` has ordered writes, readback and rollback. The daemon does not currently call this actuator from a GPU candidate path, so autonomous runtime remains observation-only. |
| ARM Mali GPU policy | `LIMITED_ACTUATION` | A discovered `/device/power_policy` path may be set to `coarse_demand` through BaselineStore, coordinator lease, and readback/rollback. Not general Vulkan/GPU-frequency optimization; physical acceptance remains required. |
| Power HAL discovery | `OBSERVATION_ONLY` | Probes Binder/HwBinder access and AIDL `android.hardware.power.IPower/default` presence only. |
| Power HAL interception / hint submission | `NOT_IMPLEMENTED` | `enable_interception()` returns an explicit unsupported error; no OEM/AOSP integration or transparent Binder interception exists. |

## Current branch implementation status (2026-11-14)\n\n- SurfaceFlinger Perfetto FrameTimeline capture is implemented through `PerfettoFrameTimelineEventSource` and connected to `RenderBooster` or workload evidence after lifecycle promotion. Actual Perfetto permission and sample availability remain device-dependent.\n- KGSL paired-level transaction code is present, but no GPU candidate provider currently calls `FrequencyController::apply_gpu_frame_policy()`. Keep this mutation path dormant in production until candidate routing, thermal admission, and device ABI acceptance are complete.\n- The Vulkan layer implements pipeline-cache hooks, but `VulkanController` previously inferred interception from installed files and configured mode. That does not establish process-level loader activation. Runtime activation is therefore reported as unverified.\n- v4.0.0 metadata and CI package artifacts exist on this branch; public GitHub Release/tag publication is separate and has not occurred.\n\n## Changes in this batch

1. Added `include/coreflow/feature_accountability.hpp` with compile-time status constants.
2. Exposed status constants from RenderBooster, PowerBridge, and VulkanController.
3. Removed fabricated RenderBooster defaults and reject invalid/unavailable frame samples.
4. Forced SkiaVK, Adaptive VRS, and pipeline-cache-prewarm runtime flags to false.
5. Disabled those WebUI controls and displayed `NOT_IMPLEMENTED`.
6. Set packaged optimization toggles to false and documented them as reserved.
7. Corrected current architecture/tuning documentation and added a changelog correction for historical overclaims.
8. Added compile-time regression coverage via `FeatureAccountabilityTest`.

## Acceptance boundaries

- Host CI verifies statuses, inactive flags, and deterministic paths; it cannot prove Android service integration or device behavior.
- Mali policy is limited actuation, not proven beneficial tuning. Promote only after driver semantics, target-device readback, ownership, and rollback acceptance.
- Vulkan layer dispatch tests prove pass-through mechanics only; they do not prove higher FPS, lower frame time, reduced GPU load, or lower temperature.
- Power HAL service presence must never be represented as interception-active.
- No ART/DEX2OAT/JIT/AOT, SurfaceFlinger, ANGLE, or OpenGL optimization claim is acceptable until a concrete OS actuator and tests exist.

## Promotion criteria

1. Concrete Android-facing actuator with documented API/ABI and permission model.
2. Capability and semantic discovery that rejects unsupported devices.
3. Central mutation ownership, baseline/journal, bounded actuation, readback/acknowledgement, and rollback where applicable.
4. Deterministic tests for success, rejection, ownership loss, and recovery.
5. Android integration tests and physical-device evidence before claiming runtime effectiveness.
6. UI and documentation status driven by implementation state, never configuration intent alone.

**P1 conclusion:** The audited rendering/runtime domains above must not be marketed as operational optimization until they meet these criteria. Other CPUFreq/cgroup/thermal mutation paths retain their own acceptance requirements.
