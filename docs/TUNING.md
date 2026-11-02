# CoreFlow Engine v2.0 Tuning Guide

This document describes the parameters available in `config/coreflow.conf` and the autonomous dials on the Google Pixel Material 3 WebUI.

---

## 1. Configuration File (`config/coreflow.conf`)

Located at `/data/adb/modules/coreflow_engine/coreflow.conf`:

```ini
[general]
version = 2.0.0
log_level = info
zero_polling_mode = true
enable_cgroups_v2 = true
enable_uclamp = true

[device_adaptation]
tier_override = auto
unlock_vendor_clamping = true
force_120hz_rendering = true

[uclamp_tuning]
top_app_interactive_min = 320
top_app_interactive_max = 1024
top_app_burst_launch_min = 800
top_app_game_min = 512
top_app_game_max = 980
foreground_min = 60
background_max = 250
sched_util_clamp_min_rt_default = 128

[thermal_guardian]
temp_monitor_interval_ms = 1000
thermal_trip_celsius = 44.0
thermal_recover_celsius = 40.0
emergency_rollback_on_overheat = true

[render_pipeline]
jank_detection_threshold_ratio = 1.25
burst_boost_duration_ms = 120
touch_boost_uclamp_min = 380
```

---

## 2. Key Tunables & Safety Boundaries

| Parameter | Default | Range | Description |
|---|---|---|---|
| `zero_polling_mode` | `true` | `true/false` | Uses Linux kernel `epoll` instead of periodic polling. Reduces CPU idle wakeups to 0. |
| `top_app_interactive_min` | `320` | `0 – 1024` | EAS Uclamp capacity floor for the active foreground top-app. Prevents frame drops during interactive tasks. |
| `top_app_burst_launch_min`| `800` | `500 – 1024` | Momentary uclamp capacity allocated during application cold launch transitions. |
| `touch_boost_uclamp_min` | `380` | `200 – 600` | Instantaneous uclamp floor allocated upon touchscreen scroll/swipe input events. |
| `burst_boost_duration_ms` | `120` | `60 – 300 ms`| Duration for which touch or launch boost clamps remain active before gracefully fading. |
| `thermal_trip_celsius` | `44.0` | `40.0 – 48.0°C` | Proactive thermal trip threshold. If the SoC reaches this temperature, the engine clamps the maximum uclamp ceiling to prevent aggressive hardware throttling. |
| `force_120hz_rendering` | `true` | `true/false` | Directs SurfaceFlinger render booster to align with 120Hz display refresh cadence. |
| `unlock_vendor_clamping` | `true` | `true/false` | Overrides restrictive OEM proprietary throttling constraints on foreground interactive cgroups. |

---

## 3. Transactional Baseline Rollback

The `BaselineStore` subsystem snapshots all modified sysfs nodes at daemon initialization:
- **Emergency Automatic Rollback**: Triggered if the SoC temperature exceeds critical limits with `emergency_rollback_on_overheat = true` or on clean shutdown.
- **Manual WebUI Rollback**: Pressing "Restore OEM Baseline" in the WebUI reverts all modified nodes to their factory boot state within 10 milliseconds.
