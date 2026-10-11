# CoreFlow Autonomous Engine — Roadmap

**Current target:** stabilize **v2.1.0** for community evaluation  
**Identity:** Observe. Understand. Balance. Adapt.

## Done in this Balance pass

- Loosened dead-band gates that prevented real-device mutation while keeping hard safety:
  confidence 0.60, thermal headroom 52°C, memory headroom 0.14, CpuBound/Moderate thresholds,
  EffectModel confidence multipliers and utility floor.
- Version identity restored to `v2.1.0` / `versionCode 2100`.
- `IDecisionAgent` foundation present (ScoringDecisionAgent); not yet the primary path in the tick loop.

## Current safety follow-up (still v2.1.0; no release version bump)

- Split discovered OS access evidence from `policy_authorized`; only named VM tunables with semantic priors are eligible through production discovery.
- Quarantine new block-device queue writes at discovery, baseline capture, candidate selection and the central actuator (`scheduler`, `nr_requests`, `read_ahead_kb` on `/sys/block` and equivalent block paths).
- Preserve journal restore for paths changed by earlier builds; recovery must not be disabled by the new-write quarantine.
- Publish the live device/kernel/SoC profile and counts of discovered versus policy-ready capabilities in `status.json` and the WebUI.

## Community evaluation checklist

1. Observe-only soak (default) — no writes, `status.json` + logs healthy.
2. Adaptive + armed, `allow_cpu_governor=no` — evaluate only supported VM candidates; storage queue writes must remain absent.
3. Confirm journal + verify + restore on Idle / disarm / regression and recovery of any pre-existing committed journal.
4. ThermalGuard path only allows policy-approved, stabilizing changes; no new block queue writes.
5. Multi-device notes (SoC, Android/kernel branch, device topology, Magisk vs KSU). Re-enable I/O mutations only after a separate reviewed policy and device-level proof.

## After community accepts v2.1.0

- Phase 2: wire DecisionAgent into the tick loop; optional ReAct-lite.
- Phase 3: stronger experience-aware ranking (still never grants permission).
- Phase 4: tool-style actuators.

## Non-negotiable safety invariants

1. Journal commit before any write
2. Read-back verification
3. Restore on Idle / disarm / kill-switch / SAFE_MODE / measured regression
4. CPU governor requires explicit opt-in
5. Default remains observe-only
