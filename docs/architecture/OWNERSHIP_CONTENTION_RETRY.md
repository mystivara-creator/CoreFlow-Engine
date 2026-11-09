# Ownership Contention Retry Contract

CoreFlow can encounter an external writer changing a node while a transaction is active. A readback mismatch is evidence of contention, not permission to overwrite an arbitrary external value.

## Admission and retry budget

- Retry reservations belong to the exact `(MutationDomain, controller identity, transaction ID)` lease.
- The coordinator enforces a hard maximum of three retry reservations per lease.
- Backoff is timer-tick based: 1, 2, then 4 ticks. The control loop never sleeps or blocks for contention.
- Emergency recovery and preemption close retry admission immediately.
- A successful reassertion restarts the observation window; pre-contention measurements are not mixed with post-retry measurements.

## Reassert safety contract

Reassertion is allowed only when the path remains an active BaselineStore mutation, the tracked active value equals the candidate target, device/inode identity is unchanged, and current readback equals the captured OEM baseline exactly.

A changed inode or a novel external value is not overwritten. The existing path-scoped rollback adopts that state as the new baseline. Failed readback or journal persistence remains fail-closed and retains recovery state.

## Journal and CF-DVS

The write-ahead journal remains durable before reassertion. The tuner releases a lease only after rollback paths pass readback and journal persistence. A failed rollback preserves recovery/ownership state.

CF-DVS records the cumulative number of admitted contention retry attempts per telemetry sample in JSONL and CSV. The tuner verdict is a last-state snapshot and may repeat; use the retry counter to distinguish actual retry attempts from repeated samples.

## Validation

Host tests cover whitespace/comma/range CPU-list parsing, fallback from malformed `related_cpus` to valid `affected_cpus`, exact-baseline-only reassertion, refusal to overwrite novel external values, the hard three-retry limit, tuner observation restart, verified rollback/journal clearing, and CF-DVS retry telemetry.

Passing host/CI checks does not establish physical HyperOS behavior. Device testing is required before claiming CoreFlow has defeated OEM contention.
