# Changelog

Semua perubahan penting pada proyek ini akan didokumentasikan di file ini.

Format ini didasarkan pada [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
dan proyek ini mengikuti [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [1.0.0-rebuild] - 2026

### ✨ Added
- **5-Pillar Dynamic State Machine** dengan deteksi beban real-time
  - Daily Efficiency (Idle mode)
  - App Launch / Burst acceleration
  - Gaming Unleashed mode
  - Ultra Deep Sleep optimization
  - Thermal Guardian dengan monitoring suhu baterai
- **Zero-Overhead Architecture** menggunakan C++ File Stream dan Bionic Native API
- **Anti-Bootloop & Graceful Fallback** system dengan safety nets
- **Kunzite Privilege Profile** khusus untuk Redmi Note 15 5G (Snapdragon 6 Gen 3 + Adreno 710)
- **Hybrid Universal Mode** untuk kompatibilitas device Android 14+
- **DSP/NPU QoS Management** untuk optimasi gaming
- **ZRAM Swappiness dynamic tuning** (60%-120%)
- **CPU WALT Scheduler** management
- **I/O Buffer optimization** (256KB-2048KB adaptive)
- **Adreno GPU TZ Governor** tuning untuk Snapdragon devices
- Support untuk Magisk v24+, KernelSU-Next, APatch
- Logcat verification tool untuk monitoring
- Graceful fallback untuk SoC non-Snapdragon

### 🔧 Changed
- Refactored thermal monitoring dengan direct battery temp sensing
- Improved CPU frequency scaling algorithm
- Enhanced I/O scheduler tuning untuk smooth app launching
- Optimized ZRAM swappiness calculation

### 🐛 Fixed
- Fixed bootloop pada device tanpa sysfs tertentu
- Fixed thermal throttling overshoot
- Fixed I/O buffer size mismatch pada gaming mode
- Fixed Adreno GPU detection untuk non-Snapdragon

### 🚀 Performance
- **Zero Overhead** - No shell/bash script execution overhead
- **Real-time Response** - Instant state transitions dengan <100ms latency
- **Memory Efficient** - Minimal RAM footprint melalui C++ native implementation

---

## [2.2] - 2026

### ✨ Added
- Initial Kunzite Privilege Profile
- Basic thermal monitoring
- Core WALT scheduler management
- ZRAM integration

### 🔧 Changed
- Improved device compatibility checks
- Enhanced error handling

### 🐛 Fixed
- Fixed crashes pada certain Android 14 variants
- Fixed file permission issues

---

## [2.1] - 2026

### ✨ Added
- Foundation code architecture
- CMake build system integration
- Basic I/O management
- CPU frequency scaling

---

## [2.0] - 2026

### ✨ Added
- Initial release
- Core daemon functionality
- Magisk module integration

---

## Versi Depan (Roadmap)

### 🔜 Planned Features (v2.0.0)
- [ ] Machine Learning-based workload prediction
- [ ] Per-app customizable profiles
- [ ] Web-based control panel
- [ ] Real-time monitoring dashboard
- [ ] Advanced GPU clock tuning
- [ ] Memory pressure relief optimization
- [ ] Battery health monitoring
- [ ] Vibration/haptic response tuning
- [ ] GPU power efficiency mode

### 🔜 Improvements
- [ ] Support untuk Android 15+
- [ ] Dual cluster scheduling untuk big.LITTLE
- [ ] Extended MediaTek/Exynos GPU profiling
- [ ] Advanced thermal prediction (AI-based)

---

## Release Notes

### Cara Update

1. Unduh versi terbaru dari [Releases](../../releases)
2. Flash menggunakan Magisk/KernelSU/APatch
3. Reboot
4. Verifikasi dengan `logcat -s CoreFlowEngine`

### Backup Rekomendasi

Sebelum update:
```bash
# Backup current module
adb pull /data/adb/modules/CoreFlow_Engine backup/

# Atau gunakan Magisk backup feature
```

---

## Kontribusi

Jika Anda ingin berkontribusi pada project ini, lihat [CONTRIBUTING.md](./CONTRIBUTING.md).

---

## Lisensi

Proyek ini dilisensikan di bawah MIT License. Lihat [LICENSE](./LICENSE) untuk detail.

---

**Last Updated**: Oct 1, 2026
