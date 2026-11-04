<p align="center">
  <img src="docs/assets/coreflow-banner.svg" width="680" alt="CFAE System Framework">
</p>

<p align="center">
  <a href="https://t.me/VaraProjects"><img src="https://img.shields.io/badge/Channel-Telegram-0088cc?style=for-the-badge&logo=telegram&logoColor=white" alt="Telegram Channel"></a>
  <a href="https://t.me/VaraProjectsDiscussion"><img src="https://img.shields.io/badge/Discussion-Telegram-26A5E4?style=for-the-badge&logo=telegram&logoColor=white" alt="Telegram Discussion"></a>
  <a href="https://instagram.com/mystivara"><img src="https://img.shields.io/badge/Instagram-mystivara-E4405F?style=for-the-badge&logo=instagram&logoColor=white" alt="Instagram"></a>
  <a href="https://github.com/mystivara-creator/CoreFlow-Engine/releases"><img src="https://img.shields.io/badge/Version-v3.0.R-0b57d0?style=for-the-badge&logo=github&logoColor=white" alt="Latest Version"></a>
  <img src="https://img.shields.io/badge/License-MIT-10b981?style=for-the-badge" alt="MIT License">
</p>

## About CFAE System Framework

**CFAE System Framework** (formerly CoreFlow Engine) is an autonomous, mutation-safe Android system optimization framework engineered across **ISO C++23**, **Rust 2024**, and **Kotlin 2.1**. Rather than applying blind single-parameter tweaks or fragile device-specific overrides, CFAE coordinates changes through **Mutation Transactions**—combining optimization intent, dependency provenance, multi-dimensional risk budgets, immediate readback verification, and OEM ownership guards.

Every mutation is journaled prior to execution, verified on readback, and 100% reversible to factory OEM baselines.

---

## Programming Language Architecture

- **ISO C++23 (`src/`, `include/`)**: Zero-polling `epoll`/`timerfd`/`signalfd` daemon runtime, semantic `cgroup v2` & `sysfs` capability discovery, `O_NOFOLLOW` + `fstat` object identity verification, and durable write-ahead journal (`CFJ3`).
- **Rust 2024 (`rust/`)**: Memory-safe thermal PID equilibrium controller, hysteresis state machine, and PSI contention arbitration bridged via `cxx`.
- **Kotlin 2.1 (`android/`)**: Native Android 15 (API 35) Quick Settings `TileService` and thermal broadcast receiver.
- **Material Design 3 WebUI (`webui/`, `module/webroot/`)**: Minimalist Google Material You interface with frosted soft-blur tonal surfaces, real-time transaction lifecycle telemetry, and Architecture Studio.

---

## Highlights & Capabilities

- **5-Layer Coordinated Mutation Safety**: Reasoning Engine → Dependency Analyzer → Mutation Planner → Safety Admission Gate & Transaction Coordinator → Outcome Evaluator & Learning.
- **12 Non-Negotiable Safety Invariants**: Enforces valid baselines before every write, immediate readback verification, external OEM ownership protection (`OWNERSHIP_LOST` safe release), and idempotent crash recovery.
- **Semantic UClamp 2.0 Discovery**: Dynamic discovery of top-app cgroups across `/sys/fs/cgroup` and `/dev/cpuctl` with automatic capacity encoding detection (`0..1024` vs percentage).
- **Multi-Sensor Thermal Matrix & PSI Guard**: Evaluates compute, battery, skin, and board thermal zones alongside Android NDK thermal severity and Linux `/proc/pressure` (CPU, Memory, I/O).
- **Hardware Longevity**: Bounded battery step-charging ceiling reduction during elevated temperatures and cgroup v2 relative I/O weighting.

---

## Supported Root Managers

- **[KernelSU](https://github.com/tiann/KernelSU)** & **[KernelSU Next](https://github.com/rifsxd/KernelSU-Next)** (Native WebUI support)
- **[APatch](https://github.com/bmax121/APatch)** (Native WebUI support)
- **[Magisk](https://github.com/topjohnwu/Magisk)** *(Full daemon optimization; WebUI via standalone providers)*

### WebUI Providers & Standalone Viewers
- **[KsuWebUI](https://github.com/5ec1cff/KsuWebUIStandalone)**
- **[WebUI-X](https://github.com/MMRLApp/WebUI-X-Portable)**
- **[MMRL](https://github.com/MMRLApp/MMRL)**

---

## Runtime & Control

Default runtime directories on device:
- **Configuration**: `/data/adb/coreflow/coreflow.conf`
- **OEM Recovery Journal**: `/data/adb/coreflow/baseline.journal`
- **Daemon Log**: `/data/adb/coreflow/logs/coreflowd.log`
- **Safety Trip / Kill Switch**: `/data/adb/coreflow/DISABLE` or `/data/adb/modules/cfae_framework/disable`

Direct shell control commands:
```bash
# Check daemon status and active state
su -c '/data/adb/modules/cfae_framework/system/bin/coreflowd --status'

# Inspect discovered kernel capabilities & UClamp nodes
su -c '/data/adb/modules/cfae_framework/system/bin/coreflowd --capabilities'

# Gracefully stop daemon and restore OEM baseline values
su -c '/data/adb/modules/cfae_framework/system/bin/coreflowd --rollback-baseline'
```

---

## Community & Support

- **Telegram Channel**: [@VaraProjects](https://t.me/VaraProjects) — Official releases, announcements, and news
- **Telegram Discussion Group**: [@VaraProjectsDiscussion](https://t.me/VaraProjectsDiscussion) — Community support, testing, and feedback
- **Instagram**: [@mystivara](https://instagram.com/mystivara) — Project creator and developer profile
- **Issue Tracker**: [GitHub Issues](https://github.com/mystivara-creator/CoreFlow-Engine/issues) — Report bugs or submit feature suggestions

---

## License

CFAE System Framework is open-source software licensed under the **[MIT License](LICENSE)**.
