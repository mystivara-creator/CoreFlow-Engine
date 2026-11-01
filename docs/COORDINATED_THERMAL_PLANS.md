# CoreFlow — measured autonomous checkpoint

The planner evaluates CPU governor and resource proposals against the live
configuration before granting one exact-target write. It ranks predicted benefit,
risk and confidence; keep-current wins when measured need or safe evidence is
absent. No domain rotation, bulk resource cohort or independent interactivity
boost may write from the runtime loop. Each trial has exactly one member.

## Causal outcome and recovery contract

1. Discovery inventories observable interfaces; write access and vendor values
   confer no semantic safety contract. Existing capability fields independently
   describe discovery, observability, OS permission and policy readiness. The
   consultation ledger distinguishes missing semantics, baseline, evidence and
   authority. Active/retained records and verified rollback events describe the
   subsequent lifecycle; quarantine never grants mutation permission.
2. Both controllers preview before the planner chooses one. The winning value is
   checked against the live current value, journaled before writing, and read back.
   A pending outcome blocks the next plan, including across actuator classes.
3. Four pre-change and four post-change samples are compared using paired metrics,
   noise floors and uncertainty. Foreground/ownership/refresh changes invalidate
   attribution. Read-back alone never proves benefit. Only Beneficial is retained;
   Neutral, Inconclusive and Regression restore the pending candidate while
   preserving earlier accepted settings. This is observational evidence, not a
   guarantee of causal benefit under every future workload.
4. The next baseline is rebuilt against the entire accepted configuration. A
   governor's dependent inventory is refreshed only after measured acceptance.
   Resource dependents block another governor transition. Scheduler changes may
   inventory iosched controls but cannot write a companion in that epoch.
5. Trial history persists path, before/target, workload and thermal/pressure
   conditions, outcome, acceptance and configuration identity. Similar failed
   scalar targets wait for materially changed evidence; categorical alternatives
   can compete independently. V1 history remains readable; new writes use V2.
6. An accepted configuration survives ordinary cycles. After sustained workload
   change with no measured need, one accepted resource is restored at a time;
   iosched children precede the scheduler anchor. Resource settings settle before
   CPU governors. Each release resets measurement and requires a fresh baseline
   and cooldown before another change. Emergency recovery and authority loss can
   restore all owned settings. Selective release is conservative: it does not
   claim that an older individual setting caused a new workload regression.
7. Journals survive failed writes/cleanup and restart recovery must settle before
   new trials. External replacements are withdrawn and shared across actuators;
   the engine does not overwrite an observed PowerHAL replacement. This detects
   changes, and cannot prevent every possible race with an external writer.

DRM/display and UFS host power controls are hard denied, including recovery
writes from stale journals. Generic GPU/vendor controls remain observation-only.
ObserveOnly, FrameworkOwned and Quarantined semantic catalog entries produce no
proposals. Failed recovery keeps the engine held and preserves evidence. The
cc87642 reboot has no proven kernel-panic cause; these guards address observed
risk-control failures without asserting a cause.

## Controlled device test procedure

Use only the exact commit with all three required CI jobs green and its uploaded
ARM64 module. CI verifies ELF architecture, model digest, packaged files and
includes build-provenance.txt with the commit in the module ZIP. Green CI is not
proof of device stability. Do not call this a general stable release.

Before installing, in rooted Termux:

```sh
su -c 'mkdir -p /data/adb/coreflow; touch /data/adb/coreflow/DISABLE'
su -c 'tar -czf /sdcard/Download/coreflow-before-test.tar.gz /data/adb/coreflow'
```

Keep the previous known-working module ZIP and a tested way into OrangeFox.
The installer preserves DISABLE and SAFE_MODE. Install the CI ZIP through
KernelSU, reboot and observe for at least 10 minutes with DISABLE present.
Check status.json, logs and decision trace: no new optimization write should be
present. Recovery of existing owned journal entries can still occur. If recovery
is pending or SAFE_MODE exists, stop here; preserve the journal rather than
clearing safety evidence. A reboot resets many runtime nodes but does not prove
that every firmware interface has returned to its original state.

For staged mutation, first turn every domain grant off in config.ini; enable
only grant_block_queue=yes, then restart the daemon. Config is read at daemon
start; creating/removing DISABLE is checked on ticks. Use the WebUI restart or
stop and restart the service as below. Inspect the resulting configuration and
status before removing DISABLE. Retain the initial backup.

```sh
su -c 'rm -f /data/adb/coreflow/DISABLE'
```

Run the same short workload and inspect one trial's write, verification, four
fresh outcome samples, and retention/rollback. Enable memory and CPU/cgroup
namespaces individually in later sessions, with matching control workloads and
logs. The engine still chooses the best eligible candidate rather than following
this operator staging order. GPU/vendor grants cannot bypass quarantine. Do not
start with Facebook Reels; reproduce the incident workload only after simpler
sessions remain clean. Stop if evidence is insufficient to attribute outcomes.

To hold mutations immediately, then request graceful shutdown:

```sh
su -c 'touch /data/adb/coreflow/DISABLE; pkill -TERM -x coreflowd'
```

A normal TERM exit is not restarted by the supervisor. Verify the process exits;
if rollback fails, keep the module disabled and retain journals. To restart:

```sh
su -c 'sh /data/adb/modules/coreflow_autonomous/service.sh >/dev/null 2>&1 &'
```

For a freeze, artifacts, reboot, repeated VERIFY_FAILED, unexpected competing
writer, thermal regression or worse frame pacing: stop testing immediately. If
Android is unresponsive, enter OrangeFox and create
`/data/adb/modules/coreflow_autonomous/disable` (with /data mounted/decrypted),
then reboot. Do not clear CoreFlow journals or SAFE_MODE. If /data cannot be
accessed, use the already tested KernelSU module-disable recovery procedure.
No boot partition is changed by this module test.

After any issue, capture what is available before restarting experiments:

```sh
su -c 'touch /data/adb/coreflow/DISABLE; pkill -TERM -x coreflowd'
su -c 'dmesg > /sdcard/Download/coreflow-test-dmesg.txt'
su -c 'logcat -b all -d > /sdcard/Download/coreflow-test-logcat.txt'
su -c 'tar -czf /sdcard/Download/coreflow-test-state.tar.gz /data/adb/coreflow /sys/fs/pstore'
```

Missing pstore or dmesg permission is a capture limitation, never evidence that
no kernel fault occurred. The state archive can contain foreground identities;
review it before sharing publicly.
