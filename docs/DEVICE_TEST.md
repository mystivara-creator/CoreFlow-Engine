# Device acceptance test — Modern v2

Use only an artifact from a green CI run matching the tested commit.

## Before install

1. Ensure no older CoreFlow daemon/module is active.
2. Keep recovery or root-module disable access available.
3. Record any visible top-app UClamp values for comparison.

Diagnostic examples only:

    su -c '
    for p in /dev/cpuctl/top-app /sys/fs/cgroup/top-app; do
      [ -r "$p/cpu.uclamp.min" ] && echo "$p min=$(cat "$p/cpu.uclamp.min") max=$(cat "$p/cpu.uclamp.max")"
    done
    '

The daemon itself does not depend on those example paths.

## First boot

After Android finishes booting:

    su -c '/data/adb/modules/coreflow_engine/system/bin/coreflowd --status'
    su -c 'tail -n 80 /data/adb/coreflow/logs/coreflowd.log'

Acceptance requires:
- running:true
- uclampSupported:true
- log line showing a discovered top-app UClamp target
- no repeated mutation failure/backoff loop
- no unexpected reboot, display artifact, freeze, or SystemUI crash

## Functional sequence

1. 10 minutes idle/daily UI.
2. Browsing and scrolling.
3. Multitasking under memory pressure.
4. Sustained game/load while monitoring temperature.
5. Stop and rollback.

    su -c '/data/adb/modules/coreflow_engine/system/bin/coreflowd --rollback-baseline'
    su -c '/data/adb/modules/coreflow_engine/system/bin/coreflowd --status'

After rollback the daemon must be stopped and recoveryPending:false.

## Emergency

    su -c 'touch /data/adb/coreflow/DISABLE'

The daemon checks the marker on its next safety tick and exits through normal rollback. If Android cannot boot, disable/remove the module from the root manager or recovery environment.

A successful device test validates only that device, ROM, and kernel combination.
