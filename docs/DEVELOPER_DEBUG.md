# Developer Debug Checklist

Use this checklist before attributing a runtime problem to CoreFlow. Collect evidence first and keep the engine in Observe mode while diagnosing.

## 1. Establish the environment

- Record device model, product/device identifiers, SoC, Android API, kernel release, ABI, and module version.
- Confirm the daemon's actual PID and whether it is running; do not infer health solely from a stale PID file.
- Inspect `install_report.txt`, the daemon log, current `status.json`, and recent decision-trace entries.
- Check `SAFE_MODE`, `DISABLE`, and other runtime guard markers before restarting or changing configuration.

## 2. Validate telemetry and discovery

- Confirm status timestamps are fresh and that missing/invalid sensors are shown as unavailable rather than as zero or a plausible temperature.
- Compare discovered capability counts with policy-ready counts; they are intentionally different.
- For an unexpected resource, inspect the discovered path, access mode, subsystem evidence, source of its candidate value, and policy/consultation reason.
- Treat a truncated filesystem walk as an incomplete inventory.

## 3. Diagnose policy without writing

- Keep `mutation_mode=observe`, `mutation_armed=false`, and `allow_cpu_governor=no` while investigating.
- Read the decision trace from observation through context, candidate ranking, consultation, policy, and final authorization.
- Distinguish a rejected candidate from a missing capability, unavailable telemetry, low confidence, or an active safety hold.
- Do not remove fatal-surface guards or broaden a path-prefix exception to make a candidate appear eligible.

## 4. Test and package hygiene

- Build host tests in an external directory and run `ctest --output-on-failure`.
- Run `tools/verify_safety_foundation.sh` and `tools/verify_source_release.sh` through the configured CTest contract.
- Keep build products and extracted ONNX Runtime SDK/cache outside the source package and repository.
- Verify CMake, module, runtime, and update-feed versions agree before making a release archive.

## 5. Device validation gate

A successful host test is necessary but not sufficient. Before enabling Adaptive mode, verify the Android ARM64 link, installation/uninstallation, service lifecycle, crash recovery, journal restoration, read-back behavior, thermal holds, and the exact kernel interfaces on the target device. Make one bounded change at a time and preserve a recovery path. If evidence is ambiguous, remain in Observe mode.


## Decision trace: cause and effect

`decision_trace.jsonl` is a bounded, best-effort diagnostic journal written by the daemon. Each `coreflow.decision.v1` record describes one engine cycle; it is not proof by itself that a kernel resource changed. The WebUI must distinguish the policy decision from actuator execution and from read-back verification.

The trace reports the selected runtime state, decision, confidence, workload, mutation eligibility, candidate count, safety hold, mode/arming state, CPU-governor opt-in, mutation result fields, policy action, intervention level, and policy explanation. `plan_reason` explains the policy plan; `reason` is the normalized cycle outcome category. They answer different questions and must not be conflated.

Interpret outcomes carefully:

- `OBSERVE_OR_NOT_ARMED`: the mode or arm gate prevents mutation; this is not evidence that a candidate was evaluated as beneficial.
- `POLICY_BLOCKED`: the current plan is not mutation-eligible; inspect `plan_reason`, state, confidence, and candidate count.
- `NO_VERIFIED_MUTATION`: the cycle has no verified mutation result. It does not uniquely identify why no write occurred.
- `MUTATION_VERIFIED`: the mutation path reported verification; this is daemon-reported read-back status, not a performance-benefit claim.
- `MUTATION_ROLLED_BACK` / `MUTATION_FAILED`: inspect mutation journal and runtime logs for the resource-level cause and recovery result.

Trace writing is deliberately non-fatal and best-effort: a filesystem error must not block the control loop or grant mutation authority. Missing trace entries mean observability is incomplete, not that no decision happened. WebUI labels derived from this trace must never claim independent confirmation of a system effect.
