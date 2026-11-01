# CoreFlow v3.1.2 — trial retention checkpoint

This maintenance checkpoint addresses repeated mutation/rollback of unchanged
candidates and continued numeric tuning after a configuration was accepted.

## Behavioral contract

- A kernel write/read-back is verified first; benefit/safety is measured separately.
- Regressing, unretained neutral and inconclusive measured trials are deferred.
  Foreground/state interruption defers the old trial without learning a false
  regression score. A transient I/O/retention failure is not performance evidence.
- No elapsed timer, restart, trend jitter or nearby numeric target alone unlocks
  another trial of the same scalar surface. A different categorical alternative
  may compete independently; a failed governor does not ban all governors.
- Holds compare relevant, decision-time conditions: thermal, CPU/memory/PSI,
  I/O latency, rendering/GPU, foreground, display period and known charging.
  Missing telemetry never establishes a useful change. A changed starting value can be reconsidered after the controller has a
  trustworthy baseline for that configuration.
- Accepted live values stay held while conditions match, preventing repeated
  escalation or refinement merely because another outcome window became available.
- After a material change and three consecutive samples with no measured tuning
  need, an accepted configuration can restore through ordered recovery. One Idle
  sample or a fixed hold-time expiry does not discard it.
- Stop, disarm, invalid telemetry, unsafe stress and failed recovery keep existing
  immediate safety behavior. No trial-history rule authorizes a privileged write.

## Persistence and attribution

`/data/adb/coreflow/trial_history.db` uses a bounded V1 private store with atomic
replacement and file/directory fsync. Scope includes device, firmware fingerprint
and kernel. Existing sample leases, experience V3 and restore journals are unchanged.
Dirty or unavailable history blocks new trials; a failed write is retried while
in-memory entries remain available. Corrupt/unreadable history is visible and
requires a repaired store plus restart; it is never silently cleared to retry.

Up to 128 member records retain the pre-write value, attempted target, domain,
conditions, measured disposition and cohort fingerprint. This is admission memory,
not independent causal credit for components of a jointly measured configuration.
A same-context reserve prevents eviction solely to cycle through failed candidates;
records from materially different contexts can be replaced as the bounded budget
fills. This is bounded history, not an eternal database of every possible condition.

Raw settings are not blindly replayed after reboot. Journals recover first and
fresh baseline/outcome windows remain mandatory. Historical accepted group members
do not become independently proven retained preferences.

## Verification and device acceptance

Regression fixtures cover hundreds of unchanged-condition decisions without a
repeat write, accepted-value stability, meaningful-context reopening, distinct
governor alternatives, persistence/reload, scope changes, unavailable sensors,
charging/foreground/display changes, admission budget, malformed files and failed
checkpoint retry. Existing coordinated rollback, ownership, installer, source
package, native WebUI and Android build checks remain required.

Local checks passed: strict C++20 core compilation, 29 host suites, 29
ASan/UBSan suites (local leak detection disabled; CI retains it), 646 WebUI
checks, 11 installer scenarios, safety and clean source-package checks. The
Android orchestrator also passed strict syntax with a host log stub.

Validate the exact commit's CI. On device, compare a repeated stable workload,
a materially different workload, and a return to the original workload; inspect
trial-memory holds and mutation events. Exercise thermal transitions, charging,
stop/restart and reboot. A green build checkpoint does not itself establish
long-duration phone stability, power savings or temperature/FPS improvement.
