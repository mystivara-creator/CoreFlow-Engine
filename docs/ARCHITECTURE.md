# CoreFlow Engine v2.0 Architecture

CoreFlow Engine v2.0 is a lean, high-performance native systems engine designed for Android 15+ (Linux Kernels 5.15 / 6.1 / 6.6). It has been restructured from the ground up to prune over 50% of legacy monolithic complexity, focusing on event-driven kernel reactivity, modern EAS Uclamp scheduling, and zero-polling power efficiency.

---

## 1. Native Subsystems Architecture

```
┌────────────────────────────────────────────────────────┐
│             Linux Kernel (sysfs, procfs, cgroups)      │
└──────────────────────────┬─────────────────────────────┘
                           │ epoll & signalfd
┌──────────────────────────▼─────────────────────────────┐
│          coreflowd — Native C++20 Daemon Core          │
│                                                        │
│  ┌──────────────────────────────────────────────────┐  │
│  │ EpollReactor (Zero-Polling Event Engine)         │  │
│  └──────┬────────────────────┬──────────────────┬───┘  │
│         │                    │                  │      │
│         ▼                    ▼                  ▼      │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────┐  │
│  │ EAS Uclamp   │    │ cgroups v2   │    │ Render   │  │
│  │ Dynamic Clamps    │ Task Pinning │    │ Booster  │  │
│  └──────────────┘    └──────────────┘    └──────────┘  │
│         │                    │                  │      │
│         └────────────────────┼──────────────────┘      │
│                              │                         │
│  ┌───────────────────────────▼──────────────────────┐  │
│  │ Transactional BaselineStore (OEM Safety & Rollback)│ │
│  └──────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────┘
```

### Core Subsystems:
1. **EpollReactor (`include/coreflow/reactor.hpp`, `src/daemon/reactor.cpp`)**:
   - Replaces CPU-intensive polling loops with Linux kernel `epoll_wait()` and `signalfd()`.
   - Wakes up instantaneously only when kernel events, task transitions, or system signals occur.
   - Preserves maximum battery efficiency while maintaining zero-latency responsiveness.

2. **EAS Uclamp Governor (`include/coreflow/uclamp.hpp`, `src/kernel/uclamp.cpp`)**:
   - Controls Energy Aware Scheduling (EAS) task utilization clamping via `/dev/cpuctl/` and `/sys/fs/cgroup/`.
   - Modulates `sched_util_min` and `sched_util_max` dynamically:
     - **Cold Launch Burst**: Momentarily raises clamp floor (e.g. 800/1024) during application startup.
     - **Touch Response**: Fast-tracks touch gesture events (floor 380/1024) to eliminate scroll jitter.
     - **Deep Idle**: Sets floor to 0 during screen off or low-refresh periods.

3. **Unified cgroups v2 Controller (`include/coreflow/cgroup_v2.hpp`, `src/kernel/cgroup_v2.cpp`)**:
   - Manages top-app task migration and cpuset pinning across CPU clusters.
   - Detects Android 15 unified cgroups v2 hierarchy and unlocks proprietary vendor throttling clamps.

4. **SurfaceFlinger Render Booster (`include/coreflow/render_booster.hpp`, `src/kernel/render_booster.cpp`)**:
   - Synchronizes HWUI render thread timing with the display refresh period (60Hz, 90Hz, 120Hz).
   - Dynamically stabilizes frame cadence during heavy GPU/composition load.

5. **Transactional BaselineStore (`include/coreflow/baseline_store.hpp`, `src/kernel/baseline_store.cpp`)**:
   - Snapshots OEM factory sysfs values prior to any modification.
   - Enforces transactional guarantees: any crash, SIGTERM, or WebUI "Restore Baseline" request cleanly reverts all mutated nodes back to stock OEM values.

---

## 2. WebUI: Google Pixel Material 3 Dashboard

- Built with **React 19**, **Tailwind CSS v4**, and **Lucide Icons**.
- Implements Google Pixel / Material You design philosophy:
  - Tonal squircle cards (`rounded-3xl` and `rounded-2xl`).
  - Dynamic status pills with real-time epoll event metrics.
  - Autonomous adaptation status hero (Top-App detection, Uclamp meter, 120 FPS gauge, and SoC temperature).
  - Autonomous tuning dials (Adaptive bias, target refresh rate, thermal trip guard).
  - Transactional OEM baseline protection with 1-click restore.
  - Monospace real-time reaction and event stream with category filtering.
