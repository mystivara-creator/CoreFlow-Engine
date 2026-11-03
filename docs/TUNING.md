# CoreFlow Modern v2 tuning and recovery

## Native configuration

Persistent file: `/data/adb/coreflow/coreflow.conf`. Installer copies `module/system/etc/coreflow/default.conf` when the persistent file is missing. Changes take effect on the next daemon start.

Actively applied settings:
- `enable_uclamp`: enables/disables the UClamp heartbeat.
- `temp_monitor_interval_ms`: 1000 to 60000 ms, the periodic kernel timer; epoll itself does not poll continuously.
- `thermal_trip_celsius`: 35–65°C, upper state transition threshold (the WebUI exposes a narrower range).
- `thermal_recover_celsius`: 25°C to at least 1°C below the trip threshold; adds hysteresis.
- `top_app_interactive_min`: 0–1024, utilization floor while interactive; only applied if the kernel interface exists.
- `top_app_interactive_max`: clamped between the configured minimum and 1024.

Experimental or reserved fields in the INI must **not** be treated as active features. In particular, `force_120hz_rendering`, `unlock_vendor_clamping`, `burst_boost_duration_ms`, `top_app_burst_launch_min` and PSI/memory actions do not currently activate hardware changes. Vendor override and forced display writes are disabled regardless of these fields.

## Commands

```sh
su -c '/data/adb/modules/coreflow_engine/system/bin/coreflowd --status'
su -c '/data/adb/modules/coreflow_engine/system/bin/coreflowd --stop'
su -c '/data/adb/modules/coreflow_engine/system/bin/coreflowd --rollback-baseline'
```

To restart after changing configuration, run the installed module's `service.sh` after Android reports `sys.boot_completed=1`. The script respects `DISABLE` and `SAFE_MODE`. Do not delete `baseline.journal` to bypass a failed recovery.

## Safety limitations

Rollback is read-back-verified per node, not an atomic whole-device operation. It does not reverse other modules' edits, physical display instability, nor driver side effects. Same-boot abnormal process termination is handled by the journal on the next start. The daemon refuses to tune if journal recovery is incomplete. Use the `DISABLE` marker for emergency suspension.
