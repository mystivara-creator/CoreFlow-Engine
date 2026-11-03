<p align="center">
  <img src="docs/assets/coreflow-banner.svg" width="960" alt="CoreFlow Engine — Modern C++20 Autonomous Performance Daemon">
</p>

<p align="center">
  <strong>Zero-polling epoll daemon. EAS Uclamp acceleration. Google Pixel Material Design 3 WebUI.</strong><br>
  An autonomous C++20 performance engine for Android 15+ (Linux Kernel 5.15 / 6.1 / 6.6), designed to be lightweight, self-adapting, and safe.
</p>

<p align="center">
  <a href="https://github.com/mystivara-creator/CoreFlow-Engine/releases"><img src="https://img.shields.io/badge/version-v2.0.0-14b8a6?style=flat-square" alt="Version v2.0.0"></a>
  <a href="https://github.com/mystivara-creator/CoreFlow-Engine/actions/workflows/build.yml"><img src="https://github.com/mystivara-creator/CoreFlow-Engine/actions/workflows/build.yml/badge.svg?branch=main" alt="Build and checks"></a>
  <img src="https://img.shields.io/badge/Android-15%2B-38bdf8?style=flat-square" alt="Android 15 or newer">
  <img src="https://img.shields.io/badge/ABI-arm64--v8a-64748b?style=flat-square" alt="ARM64 ABI">
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-Apache--2.0-64748b?style=flat-square" alt="Apache 2.0 license"></a>
</p>

---

## What is CoreFlow Engine Modern v2.0?

CoreFlow Engine v2.0 represents a total architectural restructuring that eliminates over 50% of legacy monolithic complexity. Instead of relying on rigid manual profiles, complex background polling loops, or heavy machine learning dependencies, CoreFlow v2.0 operates as an **autonomous, self-adapting performance manager**:

- **Zero-Polling epoll Reactor (`coreflowd`)**: Employs Linux kernel `epoll` and `signalfd` event notifications instead of continuous sleep-polling loops, achieving near-zero idle CPU wakeups and preserving battery longevity.
- **EAS Uclamp Acceleration**: Dynamically modulates Energy Aware Scheduling (EAS) task utilization clamps (`sched_util_min` / `sched_util_max`) for foreground top-apps, eliminating cold launch delays and touch gesture scroll stutter.
- **Unified cgroups v2 Controller**: Intelligently migrates tasks and pins cpusets across Android 15 unified cgroups hierarchies while bypassing vendor throttling locks.
- **SurfaceFlinger 120Hz Frame Pacing**: Synchronizes HWUI render thread timing with display refresh periods, preventing frame pacing variances during intense multitasking or gaming.
- **Transactional BaselineStore & Immediate Fail-Safe**: Snapshots all factory sysfs nodes before making any modifications. If any node write fails or a fatal signal occurs, it triggers an immediate atomic rollback to OEM defaults to prevent inconsistent kernel states.
- **Defensive Memory Safety & CI Sanitizers**: Safe RAII `UniqueFd` descriptor handling, strict input bounds checking, and CI memory verification via AddressSanitizer (ASan) + UndefinedBehaviorSanitizer (UBSan).
- **Google Pixel / Material Design 3 WebUI**: A lightweight, fluid single-page dashboard built with React 19, Tailwind CSS v4, and React ErrorBoundary, featuring tactile squircle cards, dynamic status pills, and dark/light mode.

---

## Source Tree Migration: Legacy (v3.x) vs Modern (v2.0)

| Area | Legacy Architecture (v3.x) | Modern Architecture (v2.0) | Benefit |
|---|---|---|---|
| **Daemon Core** | 40+ monolithic C++ files across 6 folders (`src/observe`, `src/understand`, `src/balance`, `src/mutation`, `src/conscience`, `src/runtime`) | Streamlined into `src/daemon/` (reactor, entry) and `src/kernel/` (cgroups, uclamp, baseline, render) | **>50% code reduction**, near-instant compile times, zero dynamic memory overhead |
| **Event Loop** | Continuous timer-based sleep polling loops | Linux kernel `epoll_wait` and `signalfd` zero-polling event reactor | Zero idle battery consumption, immediate event response |
| **Scheduling** | Complex trial-history bandit heuristic | Real-time EAS Utilization Clamping (`uclamp`) for top-apps | Native kernel integration, direct scheduler queue impact |
| **Rollback Safety** | Multi-file journal serialization & manual checks | Transactional `BaselineStore` with immediate fail-safe auto-fallback | Guaranteed atomic rollback to stock factory values on error |
| **Dependencies** | External ONNX Runtime binaries (~100MB download) & synthetic ML predictors | Pure C++20 standard library with Linux kernel interfaces | Zero external runtime bloat, no fragile network downloads during CI |
| **WebUI** | 5 fragmented tabs (Sysfs tree explorer, INI text editor, ONNX lab, multi-charts) | Single-Page Google Pixel Material 3 Dashboard with Error Boundary | Fast loading, fluid navigation, glanceable autonomous telemetry |

---

## Clean Source Tree Structure

```
CoreFlow-Engine/
├── CMakeLists.txt              # Unified C++20 build definition (Host tests & Android NDK)
├── config/
│   └── coreflow.conf          # Active daemon configuration
├── docs/                       # Architecture and tuning documentation
│   ├── ARCHITECTURE.md        # Technical subsystem specification
│   ├── TUNING.md              # Tunable parameter guide & safety bounds
│   └── assets/                # Visual branding assets
├── include/coreflow/          # Modern modular C++20 headers
│   ├── baseline_store.hpp     # Transactional OEM rollback store
│   ├── cgroup_v2.hpp          # Unified cgroups v2 controller
│   ├── config.hpp             # Zero-hardcode runtime configuration loader
│   ├── device_profiler.hpp    # Hardware topology & SoC profiler
│   ├── memory_guardian.hpp    # Linux PSI memory pressure guard
│   ├── psi_monitor.hpp        # Pressure Stall Information monitor
│   ├── reactor.hpp            # Zero-polling Linux epoll reactor
│   ├── render_booster.hpp     # SurfaceFlinger 120Hz frame pacing
│   ├── types.hpp              # Engine definitions and bounds
│   ├── uclamp.hpp             # EAS Utilization Clamping governor
│   └── unique_fd.hpp          # Safe RAII file descriptor wrapper
├── module/                    # KernelSU / Magisk flashable package
│   ├── customize.sh           # Module installation script
│   ├── module.prop            # Module identity & versioning (v2.0.0)
│   ├── service.sh             # Late-boot supervisor daemon launcher
│   ├── system/etc/coreflow/   # Packaged default daemon configuration
│   ├── update.json            # KernelSU update feed metadata
│   └── webroot/               # Compiled offline WebUI dashboard
├── src/
│   ├── daemon/                # Native daemon core (main.cpp, reactor.cpp)
│   └── kernel/                # Hardware controllers (cgroups, uclamp, baseline, etc.)
├── tests/
│   └── test_baseline_rollback.cpp  # Host CTest verification suite
└── webui/                     # Google Pixel Material 3 Single-Page Dashboard
    ├── App.tsx                # Unified single-page reactive dashboard
    ├── components/            # Pixel-style modular components
    ├── engine/                # Autonomous reactive simulation
    └── bridge/                # KernelSU & local storage bridge
```

---

## Configuration (`config/coreflow.conf`)

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
touch_boost_uclamp_min = 380
burst_boost_duration_ms = 120

[thermal_guardian]
thermal_trip_celsius = 44.0
emergency_rollback_on_overheat = true
```

---

## Building

### 1. Build WebUI Dashboard
```bash
npm ci
npm run build
```
Compiles the offline React WebUI directly into `module/webroot/`.

### 2. Run Host Unit Tests (Linux / Host CI)
```bash
cmake -S . -B build/host -DCMAKE_BUILD_TYPE=Debug -DCORE_FLOW_BUILD_TESTS=ON
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

### 3. Build Native Android ARM64 Daemon (NDK)
```bash
export ANDROID_NDK_HOME=/path/to/android-ndk
./tools/build_android.sh
```

---

## License

Apache License 2.0. Copyright (c) 2026 mystivara-creator.
