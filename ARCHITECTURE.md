# CoreFlow Autonomous — Target Architecture (v2.1 → Production)

## 1. Design thesis

CoreFlow is a **universal, fail-closed autonomous kernel tuner** for Android.

It does **not** ship a fixed list of “resources we like.”  
On every device it:

1. **Discovers** what the kernel exposes
2. **Separates evidence**: discovered access, explicit policy authorization, model inference, and verified outcome
3. **Scores** only supported resources where a semantic prior and bounded change are defined
4. **Gates** through policy, confidence, config opt-in, and experience
5. **Applies** at most one verified mutation per epoch
6. **Measures** outcome against a factory baseline window
7. **Rolls back** always on shutdown, safety state, or failed verification

The packager never decides “this phone should change dirty_ratio.”  
The machine decides from capability + effect + context.

---

## 2. Layered architecture

```
┌─────────────────────────────────────────────────────────────┐
│  CONFIG (observe-only default, explicit opt-in)             │
├─────────────────────────────────────────────────────────────┤
│  AUTONOMOUS ENGINE (tick loop, epoch lifecycle)             │
├──────────────┬──────────────────┬───────────────────────────┤
│  OBSERVER    │  CONTEXT ENGINE  │  ADAPTIVE POLICY          │
│  samples     │  workload+headroom│  state machine            │
├──────────────┴──────────────────┴───────────────────────────┤
│  POLICY ENGINE  →  candidates from ResourceStateModel       │
│  MUTATION AUTHORITY → permit (mode/armed/confidence/scope)  │
├─────────────────────────────┬───────────────────────────────┤
│  EFFECT MODEL (brain)       │  EXPERIENCE MEMORY (memory)   │
│  rank benefit/risk          │  advisory → hard filter later │
├─────────────────────────────┴───────────────────────────────┤
│  RESOURCE MUTATION CTRL  │  CPUFREQ MUTATION CTRL           │
│  capability-driven       │  governor bounded                │
├──────────────┬───────────┴──────────────────────────────────┤
│  ACTUATORS   │  JOURNALS (cpu + resource, separate)         │
│  write+verify│  commit before write, clear after restore    │
├──────────────┴──────────────────────────────────────────────┤
│  DISCOVERY → DeviceProfile + EnvironmentCapabilityMatrix    │
│  BASELINE INTELLIGENCE → causal before/after windows        │
│  THERMAL PREDICTOR (ONNX) + THERMAL GUARD (hysteresis)      │
│  SAFETY HOLD + BOOT-LOOP GUARD                              │
└─────────────────────────────────────────────────────────────┘
```

### Authority boundaries (non-negotiable)

| Owner | Owns | Must not |
|-------|------|----------|
| Discovery | device identity, interface presence, effective R/W evidence | infer benefit or silently authorize a path |
| Resource policy | explicit per-domain/path authorization | infer benefit or write kernel |
| EffectModel | model inference: benefit/risk score, requested value | write kernel or grant authority |
| PolicyEngine | *when* intervention is justified | pick arbitrary paths |
| MutationAuthority | final permit | bypass config opt-in |
| Actuator | bounded write + read-back | choose values |
| Journal | durable factory snapshot | be skipped before write |
| Experience | historical outcome filter | override safety gates |
| BaselineIntelligence | causal measurement | authorize mutation |

---

## 3. Runtime state machine

```
IDLE ──► NORMAL ──► ELEVATED / WARMING ──► THERMAL_GUARD
  ▲         │              │                      │
  │         ▼              ▼                      ▼
  └──── PRESSURE ◄── memory collapse      ReduceIntervention
                                              + restore epoch
```

| State | Policy action | Intervention |
|-------|---------------|--------------|
| Idle | Hold | ObserveOnly |
| Normal (weak signal) | Hold | ObserveOnly |
| Normal (strong CpuBound + headroom) | Candidate | Moderate |
| Normal (active workload) | Candidate | Low |
| Elevated / Warming | Candidate | Low |
| Pressure / ThermalGuard | ReduceIntervention (hold/restore only) | ObserveOnly |
| Config not adaptive/armed | any → Skipped + restore | — |

---

## 4. Mutation epoch contract

1. Capture factory baseline only for explicitly policy-authorized and mutation-ready resources + supported CPU governor targets
2. Authority issues permit
3. EffectModel ranks eligible candidates with structured identity; controller applies ≤1 best candidate this cycle
4. Journal **commit** (fsync) **before** first write
5. Actuator write + read-back verify
6. BaselineIntelligence observation window
7. Experience record (outcome)
8. Hold for cooldown **or** safety restore → `restoreAll()` → journal clear

One resource mutation epoch and one CPU epoch are independent journals so recovery cannot cross-corrupt.

---

## 5. Capability-driven resource path (v2.1.0+)

```
for cap in capability_matrix where mutation_ready && policy_authorized
    && domain in {supported Memory tunables}:
        score = EffectModel.evaluate(cap, baseline, context)
        if score.eligible: rank by one benefit/risk/confidence utility

pick top eligible that policy.allows
apply bounded value only when semantic bounds/direction are known
```

Soft priors for supported VM knobs (swappiness, dirty ratios/timers, vfs_cache_pressure and min_free_kbytes) provide candidate bounds and preferred directions. They are not a replacement for discovery, explicit policy authorization or live evidence. Unknown numeric writables remain observe-only. The model retains historical priors for I/O, but production discovery does not authorize block queue writes in this safety follow-up. The production `ResourceActuator` allow-list admits only the seven exact approved `/proc/sys/vm/` paths; all other resource paths are denied for new writes. Thus `/sys/block/*/queue/{scheduler,nr_requests,read_ahead_kb}`, equivalent `/sys/devices/.../block/...` paths, kernel scheduler sysctls and unknown controls remain read-only until separately reviewed and validated. I/O throughput remains telemetry, not permission to write. CPUFreq uses a separate actuator/authority path and remains disabled by default through `allow_cpu_governor=no`.

**Never autonomous (telemetry only until proven):** charging limits, GPU governors, uClamp, cpuset, ART, Android properties.

---

## 6. Safety stack (production invariants)

1. Release default = observe-only  
2. Explicit triple opt-in for aggressive CPU path: `mutation_mode=adaptive` + `mutation_armed=true` + `allow_cpu_governor=yes`  
3. Resource path: adaptive + armed (CPU allow not required)  
4. Fail-closed on corrupt journal, failed restore, boot-loop (≥3 SAFE_MODE, ≥5 module disable)  
5. No SELinux permissive / no wildcard allows  
6. Malformed config → mutation disabled  
7. ThermalGuard hysteresis 55 °C enter / 52 °C exit  

---

## 7. Roadmap to production certification

| Phase | Goal | Exit criteria |
|-------|------|----------------|
| **P0 — Source hardening** (done in v2.1.0) | Capability-driven brain, clear architecture | Source contracts, observe-only default, EffectModel + policy path |
| **P1 — Host proof** | CMake/CTest + sanitizers green | All unit tests pass; ASan/UBSan clean |
| **P2 — Android artifact** | Reproducible ARM64 `coreflowd` + Magisk ZIP | CI artifact digest pinned |
| **P3 — Observe-only field** | 7 days on ≥1 reference device | No boot loop, stable discovery logs, zero writes |
| **P4 — Adaptive field** | Opt-in on same device | Every mutation journaled, verified, restorable; thermal/memory not worsened |
| **P5 — Multi-device** | 3 SoC families | Same safety invariants; per-device experience scope |
| **P6 — Production stable** | Signed release | Thermal model device-trace trained optional; docs + incident process |

v2.1.0 targets **end of P0 + ready to enter P1–P3** (test-ready, not production-stable).

---

## 8. What “fully autonomous” means here

- **Hands:** actuators only write what discovery found writable and authority permitted  
- **Brain:** EffectModel + PolicyEngine + Context + Experience decide *if* and *what*  
- **Conscience:** journals, verify, rollback, boot-loop guard, observe-only default  
- **Not:** silent always-on kernel rewriting without measurement or recovery  

Autonomy without a conscience is a brick risk. This architecture keeps the conscience mandatory.
