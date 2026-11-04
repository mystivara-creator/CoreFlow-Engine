<p align="center">
  <img src="docs/assets/coreflow-banner.svg" width="680" alt="CFAE System Framework">
</p>

<p align="center">
  <a href="https://t.me/VaraProjects"><img src="https://img.shields.io/badge/Channel-Telegram-0088cc?style=for-the-badge&logo=telegram&logoColor=white" alt="Telegram Channel"></a>
  <a href="https://t.me/VaraProjectsDiscussion"><img src="https://img.shields.io/badge/Discussion-Telegram-26A5E4?style=for-the-badge&logo=telegram&logoColor=white" alt="Telegram Discussion"></a>
  <a href="https://instagram.com/mystivara"><img src="https://img.shields.io/badge/Instagram-mystivara-E4405F?style=for-the-badge&logo=instagram&logoColor=white" alt="Instagram"></a>
  <a href="https://github.com/mystivara-creator/CoreFlow-Engine/releases"><img src="https://img.shields.io/badge/Version-v3.0.R-0ea5e9?style=for-the-badge&logo=github&logoColor=white" alt="Latest Version"></a>
  <img src="https://img.shields.io/badge/License-MIT-10b981?style=for-the-badge" alt="MIT License">
</p>

## About CFAE System Framework

**CFAE System Framework** (formerly CoreFlow Engine) is an autonomous, stability-first Android optimizer engineered in native C++20 and Rust. Rather than applying blind benchmark boosts or fragile device-specific overrides, CFAE dynamically discovers semantic kernel interfaces (cgroup-v2, UClamp, PSI pressure, and SoC thermal zones) to deliver sustained fluid responsiveness while strictly protecting hardware longevity and thermal safety.

Every mutation is atomic, journaled, and 100% reversible back to factory OEM baselines.

---

## Highlights & Capabilities

- **Autonomous Mutation Safety**: Zero hardcoded sysfs nodes; broad runtime discovery paired with narrow, verified actuator mutations.
- **Dynamic UClamp 2.0 Discovery**: Semantic discovery of top-app cgroups with capacity encoding awareness (0..1024 or percentage).
- **Multi-Sensor Thermal Matrix & PSI Guard**: Real-time evaluation of thermal severity and CPU/Memory/IO pressure headroom.
- **Hardware Longevity**: Paced storage I/O and dynamic battery charging step control during elevated temperatures.
- **Atomic Rollback Journal**: Fsync write-ahead journal ensures immediate, idempotent recovery to factory OEM values upon exit or safety halt.
- **Modern Offline WebUI**: Interactive dashboard integrated directly into root manager web views for real-time monitoring and control.

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

Join our official channels for updates, discussions, configuration sharing, and direct developer contact:

- 📢 **Telegram Channel**: [@VaraProjects](https://t.me/VaraProjects) — Official releases, announcements, and news
- 💬 **Telegram Discussion Group**: [@VaraProjectsDiscussion](https://t.me/VaraProjectsDiscussion) — Community support, testing, and feedback
- 📸 **Instagram**: [@mystivara](https://instagram.com/mystivara) — Project creator and developer profile
- 🐛 **Issue Tracker**: [GitHub Issues](https://github.com/mystivara-creator/CoreFlow-Engine/issues) — Report bugs or submit feature suggestions

---

## License

CFAE System Framework is open-source software licensed under the **[MIT License](LICENSE)**.
