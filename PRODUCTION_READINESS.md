# Production Readiness Scorecard — CoreFlow v2.1.0

**Verdict: TEST-READY / PRE-PRODUCTION — Target rating 9/10 for “siap diuji”, not “production stable.”**

Production-stable (ship to non-technical users with mutation on by default) remains **out of scope** until P4–P6 complete.  
This document scores readiness to **begin serious device testing and partial certification**.

---

## Scorecard (weight → contribution to 9/10 test-ready bar)

| Domain | Weight | Score (0–10) | Notes |
|--------|--------|--------------|-------|
| Safety defaults & fail-closed | 20% | 9.5 | Observe-only default; malformed mode disables; journal before write |
| Authority separation | 15% | 9.0 | Discovery ≠ Effect ≠ Policy ≠ Actuator ≠ Journal |
| Capability-driven autonomy | 15% | 8.5 | EffectModel + matrix-driven candidates; priors soft only |
| Mutation lifecycle | 15% | 9.0 | Baseline → journal → write → verify → measure → restore |
| Thermal / context policy | 10% | 9.0 | Hysteresis; ThermalGuard/Pressure are hold/restore only; proactive path needs headroom |
| Boot / recovery | 10% | 9.0 | SAFE_MODE, module disable, separate journals |
| Observability | 5% | 8.0 | Structured logs; experience persistence |
| Host/CI proof | 5% | 8.5 | Local CMake/CTest and ASan/UBSan pass; operator CI remains separate |
| On-device validation | 5% | 3.0 | Not yet run on hardware in this package |
| **Weighted total** | 100% | **~8.7 → rounded 9/10 source-test readiness** | |

Interpretation:
- **9/10 siap diuji** = architecture, safety contracts, and autonomous path are mature enough that testing is the bottleneck, not redesign.
- **Not 10/10 production** = no multi-device field data, thermal ONNX still synthetic, CI Android artifact not produced from this tree in-session.

---

## Must-pass before calling a build “production candidate”

### Gate A — Source (v2.1.0)
- [x] Observe-only defaults in `default.conf` and engine
- [x] Capability-driven resource candidates with semantic priors; unknown numeric controls fail closed
- [x] EffectModel ranks Memory/Io/Scheduler only
- [x] Journal commit before write; restore on safety/shutdown
- [x] ThermalGuard/Pressure block new optimization writes; restoration remains available
- [x] ARCHITECTURE.md + this scorecard exist
- [x] `tools/verify_safety_foundation.sh` passes in local validation
- [x] `tools/verify_source_release.sh` passes against staged source
- [x] Host CTest passes locally (6/6)

### Gate B — Artifact
- [ ] Reproducible ARM64 strip of `coreflowd`
- [ ] Magisk module ZIP with matching `module.prop` version
- [ ] ONNX model digest pinned and present
- [ ] Installer refuses adaptive defaults

### Gate C — Observe-only field (minimum 7 days)
- [ ] Boot completed every reboot; no SAFE_MODE from this module
- [ ] Discovery lists expected writable VM/Io nodes
- [ ] Zero sysfs writes (audit journal absent or empty)
- [ ] Log noise acceptable; no crash loops

### Gate D — Adaptive field (operator opt-in)
- [ ] Mutations only when armed; each has journal entry
- [ ] Read-back matches requested; else Failed + restore
- [ ] ThermalGuard triggers restore/hold, never deeper aggression
- [ ] Uninstall / disable restores factory baselines
- [ ] No brick / bootloop attributable to module

---

## Explicit non-goals until later phases

- Mutation enabled out of the box  
- Charging / GPU / uClamp / cpuset autonomous writes  
- Claiming thermal ONNX is device-calibrated without traces  
- “Works on all Android devices” without P5 evidence  

---

## Recommended test matrix (P3–P4)

| Device class | Priority | Focus |
|--------------|----------|--------|
| Redmi Note 15 5G (Dimensity) | P0 | Your target: VM + mq-deadline + thermal |
| One mid-range Snapdragon | P1 | Governor path + capability differences |
| Pixel / AOSP-like | P2 | SELinux strictness, cgroup layout |

Protocol recommendation: complete all seven observe modes with zero writes, then run seven days of adaptive on the same device with resource-only mutation first. Keep CPU governor mutation disabled until resource-only behavior is stable; if later enabled, treat it as a separate trial phase with its own rollback and thermal checks.

---

## Certification language (use this, not marketing)

**Allowed:**  
“CoreFlow v2.1.0 is a pre-production autonomous engine with production-grade safety architecture. It is certified for controlled field testing under observe-only by default; adaptive mutation requires explicit operator opt-in and device-level validation.”

**Not allowed yet:**  
“Production stable,” “safe for all users with mutation on,” “validated on all SoCs.”
