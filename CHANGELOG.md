# CoreFlow v2.0.0

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
