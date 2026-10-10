# CoreFlow v2.1.0 — Source Hardening & Field Trial Guide

## What changed in this continuation

This package continues the supplied v2.1.0 source tree; it is not a clean-room rewrite.

- Replaced `EffectScore::reason` identity packing with structured `resource_name`, `path`, `domain`, and `utility` fields. Controller logic no longer parses `name|path|reason` strings.
- Unified candidate ranking around one benefit/risk/confidence utility and deterministic tie-breaking.
- Made unknown numeric controls ineligible until a semantic prior or adapter defines safe bounds and direction.
- Required measured I/O activity before tuning read-ahead or request-queue controls; CPU load alone is not enough.
- Made `endsWith()` useful for classifying scheduler resources by semantic suffix, and added active-token parsing so scheduler fallback cannot reapply the already active token.
- Connected `ExperienceMemory` to resource candidate selection. Compatible, sufficiently confident verified outcomes adjust utility modestly; regression history penalizes a proposal. Experience cannot grant authority or bypass safety checks.
- Changed `ReduceIntervention` semantics to hold/recovery only. (Superseded in the Unreleased changes: `ThermalGuard` and `Pressure` may start a stabilizing-only resource write. Look for `"stabilizing_only":true` in `decision_trace.jsonl`.) Existing restore/rollback paths remain available.
- Added immediate restoration after a resource write/read-back failure where a write may have occurred, and journal cleanup when no write was attempted.
- Corrected v2.1.0 source-release validation, added it to CTest, and updated architecture/security/readiness documentation.
- Added tests for structured ranking metadata and fail-closed behavior on unknown numeric semantics.

## Validation performed here

- Release host CMake/Ninja build with strict warnings and `-Werror`: passed.
- Host CTest: **6/6 passed**.
- ASan/UBSan host CTest: **6/6 passed**.
- Source-release and safety-foundation contracts: passed against the source tree under test conditions.
- Android NDK/ARM64 linking, CI artifacts, ONNX inference on a phone, and real-device mutation/rollback were **not** performed here. CI remains with the operator, as requested.

## Trial recommendations

### Observe-only phase

Keep the shipped default unchanged: `mutation_mode=observe`, `mutation_armed=false`, `allow_cpu_governor=no`. For each of your seven observe modes, record:

- boot/start/stop stability and daemon restart count;
- sensor availability, confidence, stale-sample rejection, thermal/memory/load trends;
- discovered capability count and reasons a capability is not mutation-ready;
- explicit confirmation of zero kernel writes;
- journal state, recovery state, SAFE_MODE/boot-loop guard events;
- CPU and resource controller decisions, including why no candidate was selected.

Do not move to adaptive if any mode has stale or implausible telemetry accepted as current, unexplained journal entries, repeated crashes/restarts, or any kernel write while observe-only.

### Seven-day adaptive phase

Enable adaptive only explicitly and begin with resource-only mutation if the device/config allows it. Keep CPU governor mutation disabled until resource-only behavior is stable. Review daily:

- each proposal's resource/path, before/after value, predicted benefit/risk/confidence, and policy reason;
- journal commit, write result, read-back result, observation window, outcome, and rollback verification;
- thermal peak/trend, available-memory minimum, workload responsiveness, and battery/power signals;
- whether experience changed the ranking and whether the evidence context matched;
- failed writes, restore failures, repeated proposals, and cooldown behavior.

Stop adaptive testing immediately if a restore cannot be verified, a journal is corrupt/pending without successful recovery, the daemon enters repeated SAFE_MODE/restart loops, the device experiences an attributable boot loop, or thermal/memory stability materially regresses. Return to observe-only and preserve logs before trying again.

Do not treat seven days on one device as proof of universal safety. It is a controlled field trial for that specific device, kernel, and workload mix.

## Known remaining limitations

- The EffectModel still relies on semantic priors for the currently supported numeric controls; unknown controls are intentionally not mutated.
- The outcome loop can attribute an outcome to the currently active mutation epoch, but external workload changes can confound causality. Use stable workload windows and retain uncertainty rather than assuming every improvement was caused by the mutation.
- Device-specific SELinux behavior, writable kernel interfaces, scheduler semantics, ONNX runtime integration, and boot/recovery behavior require real-device verification.
