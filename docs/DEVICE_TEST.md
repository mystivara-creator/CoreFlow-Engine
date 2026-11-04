# Device Acceptance Test — CFAE System Framework v3.0.R

Use only an artifact from a green CI run matching the tested commit.

## Before Install

1. Ensure no older CoreFlow daemon is active (`cfae_framework` automatically migrates legacy `coreflow_engine` directories).
2. Keep recovery or root-manager module disable access available.
3. Record visible top-app UClamp values for baseline comparison:

```bash
su -c '
for p in /dev/cpuctl/top-app /sys/fs/cgroup/top-app; do
  [ -r "$p/cpu.uclamp.min" ] && echo "$p min=$(cat "$p/cpu.uclamp.min") max=$(cat "$p/cpu.uclamp.max")"
done
'
```

The daemon itself discovers mount points dynamically via `/proc/self/mountinfo`.

## First Boot Verification

After Android finishes booting (`sys.boot_completed=1`):

```bash
su -c '/data/adb/modules/cfae_framework/system/bin/coreflowd --status'
su -c '/data/adb/modules/cfae_framework/system/bin/coreflowd --capabilities'
su -c 'tail -n 80 /data/adb/coreflow/logs/coreflowd.log'
```

Acceptance requires:
- `"running":true`
- `"uclampSupported":true`
- Log line confirming discovered CPU policies, GPU domains, and UClamp target
- Zero repeated mutation failure or backoff loops
- Zero unexpected reboots, display artifacts, freezes, or SystemUI crashes

## Functional Sequence

1. 10 minutes idle and daily interactive navigation.
2. High-refresh scrolling and web browsing.
3. Multitasking under memory and I/O contention (`PressureGuard` verification).
4. Sustained compute/gaming load while monitoring temperature (`SustainedLoad` → `ThermalGuard` transition).
5. Graceful stop and ownership-verified baseline rollback:

```bash
su -c '/data/adb/modules/cfae_framework/system/bin/coreflowd --rollback-baseline'
su -c '/data/adb/modules/cfae_framework/system/bin/coreflowd --status'
```

After rollback, the daemon must report `"running":false` and `"recoveryPending":false`.

## Emergency Kill Switch

```bash
su -c 'touch /data/adb/coreflow/DISABLE'
```

The daemon checks safety markers on every tick and exits cleanly through verified baseline rollback.
