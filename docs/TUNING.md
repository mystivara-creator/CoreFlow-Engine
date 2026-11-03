# CoreFlow Modern v2 tuning

Persistent configuration: /data/adb/coreflow/coreflow.conf

## Active controls

General:
- enable_uclamp: master actuator gate.

UClamp:
- top_app_interactive_min: Balanced-state floor in internal 0..1024 capacity scale.
- top_app_interactive_max: upper bound used by Balanced and SustainedLoad.

The kernel-facing representation is discovered dynamically. Do not enter percentages here.

Thermal:
- temp_monitor_interval_ms: safety/state cadence, clamped to 1000..60000 ms.
- thermal_trip_celsius: ThermalGuard entry threshold.
- thermal_recover_celsius: hysteresis recovery threshold.
- emergency_rollback_on_overheat: enables severe-heat exit.
- thermal_headroom_celsius: margin below trip required for SustainedLoad.
- thermal_emergency_margin_celsius: margin above trip that restores baseline and stops daemon.

PSI stability gates:
- cpu_pressure_boost_pct: CPU PSI avg10 required for SustainedLoad.
- memory_pressure_guard_pct: memory PSI avg10 that enters PressureGuard.
- io_pressure_guard_pct: I/O PSI avg10 that enters PressureGuard.

Priority:

    Emergency thermal exit
    ThermalGuard
    PressureGuard
    SustainedLoad
    Balanced

This order is deliberate: component health and system-wide contention outrank raw performance.

Recovery:

    su -c '/data/adb/modules/coreflow_engine/system/bin/coreflowd --status'
    su -c '/data/adb/modules/coreflow_engine/system/bin/coreflowd --stop'
    su -c '/data/adb/modules/coreflow_engine/system/bin/coreflowd --rollback-baseline'

Never delete baseline.journal to bypass a recovery failure.
