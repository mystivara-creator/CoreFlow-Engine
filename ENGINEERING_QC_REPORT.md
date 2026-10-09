# CoreFlow Engineering QC Report — v2.1.0

## Scope

Upgrade from v2.0.1 rebased source to **capability-driven fully autonomous** resource path while preserving fail-closed safety defaults.

## Architectural changes

| Area | Change |
|------|--------|
| EffectModel | New component: scores discovered writables by stability benefit/risk under thermal/memory/load context |
| ResourceMutationController | Candidates from capability matrix + EffectModel rank; baseline captures all mutation_ready Memory/Io/Scheduler |
| PolicyEngine | Supported active workload → Candidate; ThermalGuard/Pressure → hold/restore; Idle → Hold |
| ContextEngine | New optimization writes require non-safety state, headroom, and confidence≥0.70; ThermalGuard/Pressure are hold/recovery-only |
| Discovery | Additional VM tunables: min_free_kbytes, dirty_expire_centisecs, dirty_writeback_centisecs |
| Docs | ARCHITECTURE.md, PRODUCTION_READINESS.md, CHANGELOG v2.1.0 |

## Safety regressions checked

| Check | Expected | Status in source |
|-------|----------|------------------|
| default.conf observe-only | mutation_mode=observe, armed=false | PASS (file content) |
| Engine default MutationMode::Disabled | no write without opt-in | PASS (config.hpp) |
| Journal before write | commit then apply | PASS (resource_mutation.cpp) |
| Domain filter | no charging/GPU/ART mutation via EffectModel | PASS (evaluate domain switch) |
| Unknown numeric semantics | Observe-only until a semantic prior/adapter exists | PASS (EffectModel) |
| I/O controls | Require measured I/O activity; CPU load alone is insufficient | PASS (EffectModel) |
| Candidate identity and utility | Structured metadata and one consistent utility formula | PASS (EffectModel + controller) |
| ThermalGuard/Pressure | Block new optimization writes; restoration remains available | PASS (context + policy + authority) |

## Not verified in this environment

- Android NDK link of coreflowd
- ONNX inference on device
- Real-device mutation/rollback

## QC verdict

**Host source tests: PASS in the current local environment (CTest + ASan/UBSan).**  
**Android/device release: NOT CERTIFIED** until the operator's ARM64 build and staged device validation are complete.
