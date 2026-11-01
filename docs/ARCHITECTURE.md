# Architecture and Authority Boundaries

## Control pipeline

1. **Observe (`Device & Runtime Discovery` -> `Capability Registry` -> `Telemetry & Context`):** discover mounted `procfs`, `cgroup`, and `sysfs` interfaces, harvest vendor PowerHAL/perf profiles, and sample thermal, PSI, memory, CPU, GPU busy, and dynamic refresh-period frame telemetry.
2. **Understand (`Baseline Intelligence` -> `Outcome Evaluator` -> `Strategy Catalog` -> `Effect Model & Candidate Ranking`):** maintain rolling pre-mutation baselines, classify workload regimes (`DailyMultitasking`, `SustainedGaming`, `MemoryRelief`, `ThermalStabilization`), couple dependent domains (I/O scheduler <-> queue depth/readahead <-> CPU governor/rate-limit <-> foreground UClamp <-> GPU floor), and rank candidates strictly from live capability evidence.
3. **Balance (`Policy Authority` -> `Safety Gate`):** evaluate headroom, cooperative Power HAL / Performance Hint posture, `SAFE_MODE` / `DISABLE` holds and stabilizing-only thermal plans, and issue target-bound `MutationPermit` tokens (or route to `Keep configuration / Explain hold`).
4. **Adapt (`Mutation Transaction` -> `Durable Mutation Journal` -> `Resource Actuator`):** commit pre-write baselines to the checksummed v2 journal before writing through a single `O_NOFOLLOW` + `fstat` kernel-FS descriptor.
5. **Verify (`Read-back Verification` -> `Outcome Measurement` -> `Experience & Model Update` / `Rollback & Recovery`):** verify live read-back immediately, measure multi-sample runtime outcomes, retain verified beneficial or stable zero-harm improvements, and roll back regressions or crash leftovers deterministically.

## Non-negotiable invariants

- The installer configures automatic bounded optimization; missing/invalid configuration fails closed. Observe/Standby is not a user-selectable mode.
- Discovery/readability/writability is evidence, not permission.
- Inference, confidence, experience memory, and UI selections cannot bypass the final mutation authority.
- CPUFreq governor changes use the global `mutation_mode=adaptive` + `mutation_armed=true` authority; the legacy `allow_cpu_governor` key is compatibility/diagnostic data, not a separate permission.
- A write must be bounded, journaled, read-back verified, and recoverable.
- Unknown or insufficiently described interfaces should remain observation-only rather than receive guessed values.
- A missing sensor or invalid telemetry value must not be interpreted as a safe reading.

## Epoch and freshness

A plan should be based on current telemetry and the current discovered capability state. If the environment or relevant capability state changes, stale candidates should be reevaluated rather than blindly replayed. Persistent experience is advisory and scoped; it cannot replace fresh measurements or authorization.

## Release boundary

Passing host tests does not establish Android ABI/link correctness, vendor-kernel compatibility, or device-level safety. Those require a successful Android ARM64 build and staged device validation, including recovery behavior.


## Coordinated execution

Read-only CPU and resource proposals compete by condition, utility and risk
against keeping the current configuration. No class has a fixed turn or priority.
Independent profitable options can share a bounded plan: up to three resource
members plus a selected governor share one outcome
identity. Live coupling and bounded subtree rediscovery follow anchor changes.
Reverse-order resource journals settle dependents before CPU recovery. Verified
cooling can remain active during ThermalGuard; confidence dips block new writes.
[Detailed semantics and acceptance scope](COORDINATED_THERMAL_PLANS.md).


## v3.1.1 diagnostic and measurement boundary

Native CPU inventory is independent from applied/owned settings. A governor can
be fully visible without a justified replacement; per-class selection reasons
explain the difference. The console displays actuator activity separately from
state-policy recommendations. Outcome evidence includes means, coverage and
scores for seven dimensions. Zero-origin pressure is measurable, noise below
an absolute or combined-standard-error threshold is neutral, and changed charging
context is inconclusive. These are bounded short-window comparisons, not a causal
benchmark. See [the checkpoint](CHECKPOINT_3.1.1.md).


## Context-aware trial admission (v3.1.2)

`TrialHistory` is a separate bounded, private, atomically replaced admission store.
It is never a restore journal or per-member causal benefit record. Each verified
plan snapshots member paths, pre-write values and targets before observation.
Resolved and interrupted trials record decision-time conditions, not a context
chosen after the result. New plans consult history before ranking, with a second
per-target check for categorical alternatives. A timer alone cannot reopen a
failed trial; accepted values cannot continuously drift under the same conditions.
Missing/corrupt history or a pending durable checkpoint holds new trials while
mandatory recovery remains available. [Acceptance and limits](CHECKPOINT_3.1.2.md).
