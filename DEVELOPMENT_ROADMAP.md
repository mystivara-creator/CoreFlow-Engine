# CoreFlow Autonomous Engine — Roadmap

**Current target:** stabilize **v2.1.0** for community evaluation  
**Identity:** Observe. Understand. Balance. Adapt.

## Done in this Balance pass

- Loosened dead-band gates that prevented real-device mutation while keeping hard safety:
  confidence 0.60, thermal headroom 52°C, memory headroom 0.14, CpuBound/Moderate thresholds,
  EffectModel confidence multipliers and utility floor.
- Version identity restored to `v2.1.0` / `versionCode 2100`.
- `IDecisionAgent` foundation present (ScoringDecisionAgent); not yet the primary path in the tick loop.

## Community evaluation checklist

1. Observe-only soak (default) — no writes, status.json + logs healthy.
2. Adaptive + armed, `allow_cpu_governor=no` — Memory/I/O/Scheduler Low mutations appear under load.
3. Confirm journal + verify + restore on Idle / disarm / regression.
4. ThermalGuard path only allows stabilizing resource writes.
5. Multi-device notes (SoC, kernel, Magisk vs KSU).

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
