# ADR 0006: Interactivity boost authorization and journal recovery

- Status: Accepted (implemented after v2.4.0, host-tested under ASan and UBSan)

## Context

The boost actuators (GPU minimum frequency, top-app uclamp, read-ahead) write kernel
state for a bounded window and must restore it afterwards. An audit found that:

1. Per-domain grants were constant `true` and ignored config keys. Capability alone
   could write.
2. Boost writes had no grant gate at all.
3. Journal commit and clear results were discarded during expiry and restore.
4. A successful restore reports `RolledBack`, but the boost controller accepted only
   `Ok` and `NoChange`. Every restore was treated as a failure, so a boost could never
   leave the journal, and its path stayed blocked.
5. Nothing read the journal on start. A crash during a boost left the boosted value in
   place until reboot.

## Decision

Authorization (default-deny, each gate traceable):

- Mutation requires `mutation_mode=adaptive`, `mutation_armed=true`, and
  `interactivity_mode=adaptive` for boosts.
- Each domain requires an explicit grant: `allow_cpu_governor` for governors, and
  `grant_<domain>` for memory, block_queue, scheduler, gpu, cgroup, cpufreq, and kernel.
  A missing key grants nothing. `default.conf` states the shipped grants explicitly.
- Boost domain mapping: GPU minimum frequency to `grant_gpu`, top-app uclamp to
  `grant_cgroup`, read-ahead to `grant_block_queue`.
- A rejected `mutation_mode` also withdraws every grant (fail safe).

Journal and restore:

- A boost entry carries a `restored` flag. It is removed only after a journal commit
  succeeds without that entry. A failed restore keeps the entry, journaled and
  blocking its path, until a later pass restores it.
- Journal failures set `recovery_required_`. New boosts are refused until a commit
  succeeds. The next `expire()` performs that retry, so recovery is idempotent.
- A restore counts as successful when the status is `Ok`, `NoChange`, or `RolledBack`.
  This follows the convention in `mutation.cpp`. `succeeded()` is not changed globally,
  because `RolledBack` is also used for failed applies.
- One boost per path. A path that is boosted or pending is never re-read as a baseline.
- On start, `recoverFromJournal()` adopts pending entries as already due, so the next
  pass restores the true baseline. A corrupt journal blocks boosts and is never
  overwritten, so it can be inspected.

## Consequences

- With the shipped `default.conf`, behaviour after arming is unchanged for every domain.
  An operator who writes a partial config gets less than before, and that is intended.
- Boosts can now be restored and re-applied. Previously they ran once per path. This
  changes on-device behaviour and must be validated on device.
- `recoverFromJournal` must be called once before the first `apply()`. The autonomous
  loop calls it during source initialisation.
- Known gap: the journal is a single file for all boosts. A crash between a successful
  write and its journal entry is covered by the write-after-commit order, but a torn
  journal file depends on the atomicity of `FileMutationJournal`. That is not yet proven
  on device.
