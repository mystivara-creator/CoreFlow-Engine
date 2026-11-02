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

CoreFlow Engine v2.0 has been completely restructured from the ground up to eliminate over 50% of legacy bloat, focusing strictly on high-performance native Linux kernel mechanics and an autonomous, self-adapting experience:

- **Zero-Polling epoll Reactor (`coreflowd`)**: Replaces resource-heavy polling loops with Linux kernel `epoll` and `signalfd` event notifications for near-zero idle CPU and battery drain.
- **EAS Uclamp Acceleration**: Dynamically tunes Energy Aware Scheduling (EAS) task utilization clamping (`sched_util_min` and `sched_util_max`) for foreground top-apps, reducing cold launch latency and touch gesture stutter.
- **Unified cgroups v2 Task Migration**: Seamlessly manages cpuset pinning and priority hierarchy across `/sys/fs/cgroup/top-app` and background cgroups.
- **SurfaceFlinger & HWUI Frame Pacing Booster**: Unblocks silky 120Hz display refresh rates and mitigates frame misses during intense rendering.
- **Transactional BaselineStore**: Snapshots all factory sysfs nodes at boot time and guarantees an atomic, clean rollback to stock OEM values upon exit or via 1-click WebUI action.
- **Pixel Material Design 3 Single-Page Dashboard**: A lightweight, fluid WebUI built with React 19, Tailwind CSS v4, and Google Pixel Material 3 aesthetic (tonal surfaces, squircle cards, dynamic pill indicators, and dark/light mode).

---

## Architecture Overview

```
CoreFlow-Engine/
├── CMakeLists.txt              # Clean C++20 build definition (Host & Android NDK)
├── config/
│   └── coreflow.conf          # Modern INI daemon configuration
├── include/coreflow/          # Modern modular C++20 headers
│   ├── baseline_store.hpp     # Transactional OEM rollback store
│   ├── cgroup_v2.hpp          # Unified cgroups v2 controller
│   ├── device_profiler.hpp    # Hardware topology & SoC profiler
│   ├── memory_guardian.hpp    # Linux PSI memory pressure guard
│   ├── psi_monitor.hpp        # Pressure Stall Information monitor
│   ├── reactor.hpp            # Zero-polling Linux epoll reactor
│   ├── render_booster.hpp     # SurfaceFlinger 120Hz frame pacing
│   ├── types.hpp              # Common types and limits
│   └── uclamp.hpp             # EAS Utilization Clamping governor
├── module/                    # KernelSU / Magisk flashable package
│   ├── customize.sh           # Magisk/KernelSU installer
│   ├── module.prop            # Module identity & versioning
│   ├── service.sh             # Late-boot supervisor daemon launcher
│   └── system/etc/coreflow/   # Default daemon configuration
├── src/
│   ├── daemon/                # Native daemon core (main.cpp, reactor.cpp)
│   └── kernel/                # Hardware controllers (cgroups, uclamp, baseline)
├── tests/
│   └── test_baseline_rollback.cpp  # Host CTest verification suite
└── webui/                     # Google Pixel Material 3 Single-Page Dashboard
    ├── App.tsx                # Unified reactive dashboard
    ├── components/            # Pixel-style modular components
    ├── engine/                # Autonomous reactive simulation
    └── bridge/                # KernelSU & local storage bridge
```

---

## Building & Installation

### 1. Build WebUI Dashboard
```bash
npm ci
npm run build
```
The compiled offline assets are bundled into `module/webroot/`.

### 2. Build Native Android ARM64 Daemon (using NDK)
```bash
export ANDROID_NDK_HOME=/path/to/android-ndk
./tools/build_android.sh
```

### 3. Run Host Tests
```bash
cmake -S . -B build/host -DCMAKE_BUILD_TYPE=Debug -DCORE_FLOW_BUILD_TESTS=ON
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

---

## License

Apache License 2.0. Copyright (c) 2026 mystivara-creator.
