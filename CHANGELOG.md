# Changelog

## v2.0.1 — Release hardening — 2026-10-09

Release-candidate revision of v2.0.0 with changes from the engineering review. No model retraining; the thermal model digest is unchanged.

### Behavior changes

- **Default is observe-only.** Engine defaults and `default.conf` now set `mutation_mode=observe`, `allow_cpu_governor=no`, `mutation_armed=false`. Adaptive mutation is an explicit operator opt-in. New installations receive this default. Existing `/data/adb/coreflow/config.ini` files are not overwritten and keep their previous values; operators upgrading from v2.0.0 should review that file.
- **ThermalGuard confirmation.** A guard trigger that comes only from the hottest policy-eligible zone must persist for two consecutive samples. The representative-sensor and predicted triggers act immediately as before. The rising-near-threshold trigger now uses the representative sensor only (previously the larger of representative and hottest), so a single hot zone cannot enter the guard early.

### Fixes

- A malformed `mutation_mode` now disables mutation regardless of the order of the other keys. Previously a later `mutation_armed=true` line could re-arm the engine (mutation still required `mutation_mode=adaptive`, so kernel writes were not reachable, but the documented fail-safe was not met).
- Thermal model inputs: missing telemetry (battery, I/O, CPU, battery level) and unknown thermal or load trends are no longer encoded as `0`. The model is skipped and the bounded heuristic is used. The model input contract (18 features, frozen order, `float_input`, `[N,18]` float32) is unchanged.
- Thermal plausibility bounds (10–120 °C) are defined once in `thermal_limits.hpp`. The observer and the engine's sample validation previously used different windows.
- Training script: the ONNX file is written only after the in-memory checker and parity check pass, using an atomic replace. The module docstring states that the default model is trained on synthetic data.

### Engineering

- New pure modules, host-testable and free of Android/ONNX headers: `thermal_features` (18-feature builder), `thermal_guard` (enter/hold decisions), `thermal_limits` (plausibility).
- New regression suite `tests/release_hardening_tests.cpp` (71 checks): default and opt-in config, fail-safe parsing, plausibility bounds, index-by-index feature mapping, completeness rules, and guard confirmation/hysteresis.
- `mutation_tests` default assertion updated to the observe-only release contract.
- Source and safety contract scripts now check the observe-only defaults, the release version, the new modules, and the Python/C++ 18-feature order.
- CI model comment corrected (the model is synthetic-data-trained and not device-validated).

### Validation

See `ENGINEERING_QC_REPORT.md` for the commands run and their results.

### Not in this release

- ONNX retraining on real per-device traces (requires device telemetry).
- Android NDK/ARM64 build verification and on-device validation in the source-package environment.

## v2.0.0 source QC revision — 2026-10-09

- Added per-sample `ThermalReading` records for every discovered thermal zone, preserving zone/type, raw millidegree value, read status, validity, and policy eligibility.
- Thermal reads now distinguish unreadable sensors, implausible values, and valid readings excluded from policy selection; diagnostics no longer silently discard those zones.
- Thermal guard entry and exit now consider the hottest valid policy-eligible thermal reading in addition to the representative sensor and model prediction.
- Added periodic per-zone telemetry diagnostics and total/valid sensor counters.
- Reconciled the documented thermal model SHA-256 with the digest already pinned in the CI workflow.
- Validation performed: host build and 4/4 CTest checks, source-release contract, safety-foundation contract, and strict-warning host compilation of `observer.cpp` and `autonomous.cpp` using an Android logging stub. A real Android NDK/ARM64 build and on-device behavior were not run in this environment.


## Production adaptive engine

- Unified CPUFreq and generic-resource mutation behind `MutationAuthority` and scoped `MutationPermit`.
- Journal records contain only resources actually owned by CoreFlow, preventing recovery from overwriting untouched external changes.
- Runtime refresh restores both CPU and generic-resource mutations before discovery and re-baselining.
- Generic resource mutation is explicitly allow-listed by `PolicyPlan`; `Pressure` and `ThermalGuard` are intervention-blocking states.
- CPU and generic-resource mutation are causally isolated to one actuator class per decision cycle.
- Verified CPU/resource candidates feed the same BaselineIntelligence + ExperienceMemory evaluation loop.
- Added ownership, authority, and external-change regression tests.
- Public defaults enable safe autonomous operation; mutation remains bounded by the centralized C++ safety authority.
- v2.0.0 Stable closes the planned capability gaps from v1.9.x: persistent scoped experience, I/O/workload profiling, battery/power-aware policy, and adaptive feedback integration.
- Historical experience is advisory only and cannot authorize a mutation or bypass fresh telemetry/safety gates.

# CoreFlow v1.9.5

## Ecosystem Autonomous Control

- Added dynamic ecosystem resource mutation for discovered VM and block-device resources.
- Added adaptive `vm.swappiness` intervention driven by memory pressure.
- Added per-device I/O resource discovery for `read_ahead_kb`, `nr_requests`, and scheduler controls.
- Added bounded generic resource actuator with read-back verification.
- Added separate durable resource mutation journal and recovery path.
- Preserved `MutationPermit`, baseline capture, rollback, and fail-closed defaults.

## v1.9.2 — Audited final

- Fix: a rolled-back write on one policy no longer clears the dirty state of other policies. Each policy is tracked individually, and the journal is cleared only when no policy remains changed. Previously a rollback on one policy could leave another policy's mutation unrestored until reboot, with no journal record.
- Fix: the CONTEXT diagnostic is logged on eligibility change and every 60 samples, instead of every monitor tick.
- Docs: default configuration comment version aligned.
- Test: multi-policy mutation and full restore regression added.

## v1.9.1 — Ecosystem Intelligence & Control Foundation (Production Source)

### Added
- Hardened CI Android API input validation so the requested minimum API cannot exceed the installed SDK platform.

- Consolidated the pre-v2 architecture on top of the v1.5 environment/capability layer.
- Added `ResourceStateModel` for observed/baseline/desired/verified resource state.
- Added explicit resource constraints and fail-closed mutation eligibility.
- Added `ContextEngine` for workload and system-context classification.
- Added `PolicyEngine` for plan-only ecosystem decisions.
- Added `ActuatorManager` for a common actuator registration/discovery boundary.
- Added `DecisionOutcome` and outcome classification for future experience learning.
- Integrated context and policy planning into the runtime observation loop without granting new mutation authority.
- Added deterministic architecture-foundation tests.

### Safety / production hardening

- New ecosystem resources remain mutation-disabled by default.
- Policy and resource layers cannot manufacture the final `MutationPermit`.
- Existing v1.4.1 journal, verification, rollback, safety-hold and boot-loop safeguards remain authoritative.
- Default config is fully fail-closed:
  - `mutation_mode=disabled`
  - `allow_cpu_governor=no`
  - `mutation_armed=false`
- In-code defaults match the config file (`allow_cpu_governor_` and `mutation_armed_` both default false).
- Source package ships no production binary or ONNX library (CI produces them).

### Validation

- 330/330 host mutation safety checks passed
- Architecture foundation tests passed
- Safety foundation contract and source-release integrity scripts passed

## v1.5.0 — Environment & Capability Intelligence

- Added Android environment identity and capability discovery.
- Added resource capability matrix covering CPUFreq, UClamp, CPUSet, scheduler, memory, I/O, GPU, thermal, charging, power, cgroup and Android runtime.

## v1.4.1 — Tier-2 Safety Consistency Fix

- Corrected `NoChange` mutation bookkeeping so a journaled plan with no kernel write does not force a restore write.
- Aligned rollback and journal clear with actuator evidence (`writes_attempted`, `RolledBack`).
- Safety hold and boot-loop guards remain authoritative.

## v1.4.0 / earlier

- Durable mutation journal, explicit arm gate, MutationPermit, read-back verification
- Bounded CPUFreq governor mutation, rejection cooldown, max mutation hold
- Magisk supervisor, SAFE_MODE, boot-loop disable marker
- ONNX thermal predictor foundation
