# ADR 0007: Evidence-driven tuning expansion

- Status: Proposed for v2.4.x validation
- Decision: Expand candidate generation from runtime capability evidence without
  converting option names or device-name patterns into authority.

## Context

The engine already separates capability discovery, EffectModel proposal ranking,
consultation, MutationAuthority, journal persistence, bounded actuator writes,
read-back verification and rollback. A categorical quarantine for a discovered
block scheduler prevents the reasoning layer from proposing a trial even when
the target exposes its accepted selector tokens. Conversely, a short blacklist
of `loop`, `ram`, `dm-*`, or `zram` names is not a reliable device-topology model
and can exclude legitimate layered targets.

## Decision

1. Treat the block scheduler selector as `BoundedTune`, not universally safe. The
   EffectModel only considers kernel-advertised selector tokens; it does not
   manufacture token values.
2. I/O PSI / latency can move an otherwise idle/normal CPU into the soft
   `Elevated` workload path. In that I/O-only case, EffectModel opens scheduler
   candidates only; it does not use I/O pressure as a blanket reason to explore
   CPU and memory tunables. Shared thresholds are defined once for policy/model.
3. Token availability is not proof of benefit. Scheduler trials are not described
   as stabilizing during thermal/memory stress unless I/O pressure is also present
   and there is matching vendor-efficiency evidence or a strong positive outcome
   prior. In ordinary headroom/elevated contexts, advertised choices can be ranked
   and evaluated by the existing experience layer.
4. `none`, `mq-deadline`, and `kyber` receive only a small prior under observed I/O
   pressure. This is a ranking hint, not a safety grant, hard-coded allow-list, or
   claim that one scheduler is optimal on every device. Other kernel-advertised
   choices (for example, vendor-specific schedulers) remain eligible.
5. CPU-set exploration is scoped to `cpus` / `cpuset.cpus`, using only kernel or
   vendor-provided alternatives; `mems` and partition control need separate
   semantics. UClamp/CPU-set controller selection requires a complete capability
   record, including policy authorization and runtime verification.
6. Interactivity discovery prefers actual `top-app`/`foreground` path components
   over background cgroups. A path preference affects ranking only; it never lowers
   the mutation risk score or grants authority.
7. Read-ahead paths are selected from admitted I/O capabilities where possible,
   supporting both `/sys/block/...` and `/sys/devices/.../block/...`; fallback
   discovery still relies on the existing actuator and journal gates.
8. Numeric bounded exploration avoids signed overflow at integer extrema.

## Consequences

This widens the engine's candidate space without removing permission grants,
mutation arming, resource-domain policy, fatal-surface checks, retained-state
persistence, bounded writes, read-back checks, rejection cooldowns, or rollback.
A candidate is a hypothesis: real benefit remains workload/device dependent and
must be established through post-mutation telemetry and rollback-capable trials.

## Validation required

Host deterministic tests and sanitizers cover proposal gating and edge arithmetic.
Android ARM64 CI and device-specific trials remain required before describing the
expanded scheduler behavior as production-validated.
