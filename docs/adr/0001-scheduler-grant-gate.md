# 0001. Scheduler grant gate (queue/scheduler)

## Status

Proposed. Scheduler writes remain blocked. `grant_scheduler` is parsed but has no effect.

## Context

`/sys/block/<dev>/queue/scheduler` is quarantined at multiple layers: `ResourceActuator::mutationTargetAllowed` denies scheduler tokens, and consultation does not classify them as Admissible. The current EffectModel stores numeric priors only, and scheduler values are categorical tokens (for example `mq-deadline`, `none`, `bfq`, `kyber`). A numeric prior system cannot rank them safely.

## Decision

Keep scheduler writes denied until all of the following exist and are reviewed:

1. A categorical EffectModel for scheduler tokens, with per-token evidence (latency, throughput, power) collected on named device/SoC/storage combinations.
2. A token-safety model that rejects tokens unavailable in the device's `scheduler` list at discovery time and never writes a token not read back from that list.
3. Device-matrix validation on at least two storage stacks (UFS and eMMC) in Observe mode for 7 days, then Adaptive with rollback verified.
4. A separate source change adding the scheduler path to the actuator allow-list, with host tests that assert denial for virtual devices and for unknown tokens.

## Consequences

- `grant_scheduler=yes` changes nothing until the above is delivered. The WebUI should show it as "blocked by policy", not "granted".
- The installer does not offer a scheduler prompt.
- Reopening this gate requires a new ADR (0002+) that supersedes this one.
