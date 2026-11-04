# CFAE System Framework v3.0.R — Tuning & Policy Reference

Persistent configuration: `/data/adb/coreflow/coreflow.conf`  
Packaged fallback: `/system/etc/coreflow/default.conf`

## Active Controls

General:
- `enable_uclamp`: Master gate for top-app UClamp scheduler transactions.

UClamp (`[uclamp]` / `[uclamp_tuning]`):
- `top_app_interactive_min`: Balanced-state floor in internal `0..1024` capacity scale.
- `top_app_interactive_max`: Upper bound used by Balanced and SustainedLoad states.
- `ui_adaptive_bias`: Optimization intent bias (`battery`, `balanced`, `responsiveness`).

The kernel-facing representation (`0..1024` capacity vs percentage) is discovered dynamically at runtime.

Thermal (`[thermal]` / `[thermal_guardian]`):
- `temp_monitor_interval_ms`: Safety and telemetry cadence, clamped to `1000..60000` ms.
- `thermal_trip_celsius`: Preview/fallback ThermalGuard entry threshold (native production prioritizes Android thermal severity and per-zone OEM trips).
- `thermal_recover_celsius`: Hysteresis recovery threshold.
- `emergency_rollback_on_overheat`: Enables severe-heat baseline rollback.

PSI Stability Gates (`[stability]`):
- `cpu_pressure_boost_pct`: CPU PSI `avg10` required for `SustainedLoad`.
- `memory_pressure_guard_pct`: Memory PSI `avg10` that enters `PressureGuard`.
- `io_pressure_guard_pct`: I/O PSI `avg10` that enters `PressureGuard`.

## Admission & Safety Priority Order

```text
1. Emergency Thermal Exit / OEM Critical Trip
2. ThermalGuard (Active SoC Mitigation)
3. PressureGuard (Memory / I/O Contention Backoff)
4. SustainedLoad (Coordinated Responsiveness Boost)
5. Balanced Equilibrium
```

Component health, OEM thermal ceilings, and system-wide contention always outrank raw performance boosting.

## Runtime Control & Recovery

```bash
su -c '/data/adb/modules/cfae_framework/system/bin/coreflowd --status'
su -c '/data/adb/modules/cfae_framework/system/bin/coreflowd --capabilities'
su -c '/data/adb/modules/cfae_framework/system/bin/coreflowd --stop'
su -c '/data/adb/modules/cfae_framework/system/bin/coreflowd --rollback-baseline'
```

Never delete `/data/adb/coreflow/baseline.journal` manually to bypass a recovery check.
