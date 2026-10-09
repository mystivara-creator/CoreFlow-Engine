# Changelog

## v2.1.0 — Fully Autonomous Capability-Driven Engine

### Architecture
- **EffectModel** ranks discovered writable resources using a consistent benefit/risk/confidence utility and structured resource identity. Unknown numeric controls remain ineligible until semantics are defined.
- I/O queue controls require observed I/O activity; CPU utilization or load alone is not sufficient evidence to tune them.
- **ResourceMutationController**: candidate generation is now capability-matrix driven, not a hard-coded resource name list. Baseline capture covers every `mutation_ready` Memory / Io / Scheduler capability.
- **PolicyEngine**: more proactive under Warming / Elevated / active workloads while remaining fail-closed on Idle and when context headroom is missing. Candidates are taken from the resource model.
- **MutationAuthority**: issues permits only for eligible `Candidate` plans; `ReduceIntervention` is hold/recovery-only and never starts a new write.

### Discovery
- Expanded VM tunables: `vm.min_free_kbytes`, `vm.dirty_expire_centisecs`, `vm.dirty_writeback_centisecs` in addition to swappiness / dirty ratios / vfs_cache_pressure.

### Safety (unchanged contract)
- Release default remains observe-only (`mutation_mode=observe`, `mutation_armed=false`).
- Journal → bounded write → read-back verify → rollback.
- Boot-loop guard and SAFE_MODE preserved.

### Context gate
- `ThermalGuard` and `Pressure` block new optimization writes. Recovery and rollback remain available through the controller's restoration path.

### Intent
The engine discovers what the device exposes, scores what is safe and useful to change, applies at most one verified mutation per epoch, measures outcome, and rolls back. Resource identity is not pre-selected by the packager.

## v2.0.1
- Release-candidate source package with observe-only defaults, ONNX thermal predictor, host tests, and safety contracts.
