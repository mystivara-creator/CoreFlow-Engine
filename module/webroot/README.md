# CoreFlow Engine v5.0.0 WebUI

Lightweight Material Design 3 offline dashboard synchronized into `module/webroot` for KernelSU, APatch, and Magisk.

- **Home**: Live 5-mode context monitor (`LAUNCH_BURST`, `INTERACTIVE`, `GAMING`, `EFFICIENCY`, `SCREEN_OFF`), active foreground app, SoC/Battery thermal telemetry, 4 performance profiles (`Balanced`, `Smoothness`, `Battery`, `Gaming`), and 5 core feature toggles.
- **Apps**: Per-app override management (`Gaming Apps` and `Excluded / OEM Stock` apps) applied safely via `coreflowd --set-config`.
- **System & Safety**: Dynamic hardware discovery summary (CPU clusters, GPU, UFS/eMMC storage, RAM/ZRAM, `sched_setattr` per-thread UClamp support, MGLRU), Battery Power telemetry, CFJ4 Baseline Journal viewer, and one-tap OEM Baseline Restore.
