# CoreFlow on-device acceptance checklist

This checklist validates one exact commit and packaged ARM64 artifact on a real
device. It is NOT a certificate of universal hardware compatibility.

## Before enabling the installed module

1. Keep a known-good boot/recovery route and a copy of the previous module.
2. Identify the exact Git commit SHA and the CI-built minapi34 ARM64 module ZIP.
   A GitHub source-code ZIP is not a flashable module.
3. CoreFlow's shipped configuration starts bounded Adaptive tuning automatically
   after installation. There is no standalone user-selectable observe-only mode.
   If you want an observation-only acceptance period, **hold writes** with the
   documented SAFE_MODE marker (`/data/adb/coreflow/SAFE_MODE`) or Stop in the
   WebUI; do not modify kernel-owned tunables manually.
4. With writes held, confirm clean boot, continuous daemon heartbeat (distinct
   from telemetry timestamp), accurate CPUFreq topology and vendor discovery.
5. Validate that no tuning occurs while SAFE_MODE/stop is active. Release an
   operator hold only after basic boot/daemon checks pass and recovery is ready.

## Model startup and execution

- Verify `system/lib64/libonnxruntime.so` and
  `system/etc/coreflow/thermal_predictor.onnx` in the packaged ZIP.
- Check logcat `THERMAL_ML status=MODEL_READY backend=ONNX` for a successfully
  initialized ONNX session. `HEURISTIC_ONLY` is an explicit fallback, not a
  successful ONNX model startup.
- In the native WebUI, `onnx_session_ready` reports session readiness.
  `onnx_inferences` counts finite model predictions; `heuristic_fallbacks`
  counts heuristic predictions. A ready session and zero model predictions do
  not establish successful inference.
- Compare real thermal measurements and predicted behavior under idle, warm
  and gaming workloads; synthetic model provenance is not per-device calibration.

## Adaptive device acceptance

1. Compare repeated stable workload, changed workload and return to original.
   Existing accepted values should hold rather than re-write on every tick.
2. Observe A followed by B against the live accepted configuration. Governor
   transitions must rediscover live tunables; previously selected resource
   candidates are re-scored, not blindly replayed.
3. Confirm every candidate stays in the kernel-advertised discrete OPP set and
   ABI numeric range, including vendor-provided values. A stock MHz/GHz
   specification is NOT permission to overclock or bypass thermal controls.
4. Inspect decision trace: prewrite journal, target-bound permission,
   readback verification, measured outcome, acceptance/rollback and trial holds.
5. Confirm that one cohort produces one measured outcome, not fabricated
   independent benefit for each member.
6. Test battery charging, thermal guard, external OEM overrides, interrupted
   journal recovery, Stop/Restart and reboot. Changed external values must not
   be overwritten by an old CoreFlow journal.
7. Record jank/FPS, PSI, latency, temperature and power/battery during repeated
   test sessions, including idle/normal/high-load soaks. Missing sensors are
   UNAVAILABLE and do not imply a stable result.

## Evidence to export

Exact commit SHA and CI run; module install/flash outcome; device/ROM/kernel;
start/stop timestamps; `status.json`, decision-trace JSON and logcat;
ONNX session/inference counters; mutating settings and rollback verification.

A green host + sanitizer + Android ARM64 CI proves source/build/package
contracts for that exact SHA, not phone compatibility or thermal efficiency.
Do not mark stable until these real-device checks pass.
