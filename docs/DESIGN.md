# CoreFlow v6.0 NextGen — Design & Honesty Contract

CoreFlow Companion (Miuix Compose) and WebUI follow five strict engineering & UX rules so every screen stays aligned, calm, and truthful across devices.

## 1. Capability-Driven UI ("Absent, Not Disabled")
- Every hardware toggle is gated on live **CoreFlow Atlas** (`coreflowd --atlas` / `--status`) evidence.
- If a kernel does not expose a safe, writable charging control node (`input_suspend`, `charging_enabled`, `battery_charging_enabled`), Bypass Charging and Charge Ceiling controls are **omitted entirely** — never shown as broken switches.

## 2. Honest Unknowns
- Missing sensors, unavailable FPS counters, or uninitialized temperatures MUST render as `--` (never fabricated zeros or synthetic `60 FPS`).
- Real FPS in `CoreFlowOverlayService` is derived strictly from SurfaceFlinger frame flip deltas.

## 3. Uniform Grid Alignment Contract
- Every horizontal card or metric pill row uses `Modifier.fillMaxWidth().height(IntrinsicSize.Min)` with `Modifier.weight(1f).fillMaxHeight()` on each child.
- Metric pill labels enforce `maxLines = 1, overflow = TextOverflow.Ellipsis` so pills never grow unevenly across locales (`en` and `id`).

## 4. Single Audited Write Path
- The Companion App and WebUI are strictly **thin clients** — they never mutate `/sys` directly.
- Every state change routes through `coreflowd --set-config` → `BaselineStore (CFJ4)` → read-back verification → `DecisionJournal` episode entry.

## 5. Respectful Coexistence & Bilingual Accessibility
- External OEM sysfs mutations trigger exponential-backoff cooldown (`30s → 300s`) and are surfaced transparently as `OEM Yield` episodes instead of fighting the OEM daemon.
- All user-facing Companion App tabs support instant live switching between **English** and **Bahasa Indonesia** via `Strings.kt`.
