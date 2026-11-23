# CoreFlow v6.1.0-R — Zero-Profile Unified Autonomous Design & Honesty Contract

CoreFlow Companion (Miuix Compose) and MD3 WebUI follow six strict engineering & UX rules so every screen stays aligned, calm, and truthful across devices.

## 1. 100% Pure Zero-Profile Autonomy
- Manual global profiles (`Balanced / Smoothness / Gaming / Battery`) and manual per-app override buckets are intentionally absent.
- One continuous physics-driven closed loop adapts automatically to touchscreen input (`< 5 ms` + Hz-adaptive OverScroller fling hold), automatic 3D game engine signatures, **Multi-Cgroup EAS Thermal-Power Pacing** (`cpu.uclamp.max` + EWMA slow-thermal velocity), **Low-Battery Auto-Eco (`<= 20%` discharging)**, and per-app **App DNA (`CFDNA2`)**.
- The only non-manual escape hatches exposed to the user are **Pause (Stock OEM A/B Compare)** and per-app **Exclude (Stock OEM)**.

## 2. Capability-Driven UI ("Absent, Not Disabled")
- Every hardware toggle is gated on live **CoreFlow Atlas** (`coreflowd --atlas` / `--status`) evidence.
- If a kernel does not expose a safe, writable charging control node (`input_suspend`, `charging_enabled`, `battery_charging_enabled`), Bypass Charging and Charge Ceiling controls are **omitted entirely** — never shown as broken switches.

## 3. Honest Unknowns
- Missing sensors, unavailable FPS counters, or uninitialized temperatures MUST render as `--` (never fabricated zeros or synthetic `60 FPS`).
- Real FPS in `CoreFlowOverlayService` is derived strictly from SurfaceFlinger frame flip deltas.

## 4. Uniform Grid Alignment Contract
- Every horizontal card or metric pill row uses `Modifier.fillMaxWidth().height(IntrinsicSize.Min)` with `Modifier.weight(1f).fillMaxHeight()` on each child.
- Metric pill labels enforce `maxLines = 1, overflow = TextOverflow.Ellipsis` so pills never grow unevenly across locales (`en` and `id`).

## 5. Single Audited Write Path
- The Companion App and WebUI are strictly **thin clients** — they never mutate `/sys` directly.
- Every state change routes through `coreflowd --set-config` → `BaselineStore (CFJ4)` → read-back verification → `DecisionJournal` episode entry.

## 6. Respectful Coexistence & Bilingual Accessibility
- External OEM sysfs mutations trigger exponential-backoff cooldown (`30s → 300s`) and are surfaced transparently as `OEM Yield` episodes instead of fighting the OEM daemon.
- All user-facing Companion App tabs support instant live switching between **English** and **Bahasa Indonesia** via `Strings.kt`.

Arsitektur lengkap (pipeline, fisika, invarian, cara membaca dump): [ARCHITECTURE.md](ARCHITECTURE.md).
