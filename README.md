# CoreFlow Engine (CFAE)

[![Version](https://img.shields.io/badge/version-v5.1.0-blue)](#)
[![Platform](https://img.shields.io/badge/Android-13%2B%20(API%2033%2B)%20ARM64-3DDC84)](#instalasi)
[![License](https://img.shields.io/badge/license-Apache--2.0-orange)](LICENSE)

**CoreFlow Engine** adalah framework sistem otonom berbasis **C++23 + Rust** untuk perangkat Android root (KernelSU / APatch / Magisk). CoreFlow melakukan deteksi topologi perangkat keras secara dinamis (**zero hardcoded path**) dan mengkalibrasi seluruh parameter langsung dari karakteristik silicon serta telemetri kernel real-time (**zero fixed tuning value**), dengan jaminan pemulihan penuh ke konfigurasi pabrik melalui jurnal **CFJ4 OEM Rollback**.

---

## Arsitektur 4-Pilar Otonom

| Pilar | Fokus | Mekanisme Inti |
| :--- | :--- | :--- |
| **1. Frame & UI Smoothness** | Respons sentuhan `< 5 ms` & animasi konsisten | Per-TID `SYS_sched_setattr` (`uclamp_min/max`) pada thread CFS kritis (`RenderThread`, `RenderEngine`, `hwuiTask`, `UnityMain`, `GameThread`, `EmuThread`, `CrRendererMain`) dengan umpan balik *closed-loop* saturasi thread render + kapasitas EAS (`little`/`mid`/`big`) + refresh rate layar — menghindari bentrok PowerHAL cgroup global. |
| **2. Dynamic Storage Burst & QoS** | App launch & loading game cepat | Deteksi foreground cepat `200 ms`, penemuan otomatis antrean blok fisik (UFS/NVMe/eMMC) & `dm-*` aktif dari `/proc/self/mountinfo`; adaptasi `read_ahead_kb` sesuai geometri antrean (`max_sectors_kb`, `nr_requests`) + `io_psi`, serta isolasi cgroup `background`. |
| **3. Anti-Stutter Memory & Smart Pre-Read** | Bebas lag saat multitasking | Aktivasi otomatis Linux **MGLRU**, adaptasi kontinu `min_ttl_ms`, `swappiness`, dan `compaction_proactiveness` dari `MemAvailable` + ZRAM + `mem_psi`, serta prefetch `posix_fadvise(POSIX_FADV_WILLNEED)` terukur khusus aplikasi UI foreground. |
| **4. Thermal PID & Battery Longevity** | Performa stabil tanpa throttling kasar | **Rust PID controller** + prediktor termal **Online RLS + Micro-MLP (10 detik)** memodulasi amplop CPU/GPU (Adreno KGSL, devfreq Mali/Xclipse, dan MediaTek GED), pemutus darurat termal (`SoC ≥ 95 °C` / `Baterai ≥ 58 °C`), deteksi `ScreenOff` OLED/AOD (DRM DPMS & `bl_power`), serta step-charging baterai (termasuk *dual-cell*). |

### Prinsip Desain
- **Zero Fixed Tuning Values & Zero Hardcoded Paths** — setiap perangkat dikalibrasi dari hardware-nya sendiri saat startup & runtime.
- **Anti-Contention OEM dengan Auto-Cooldown** — jika daemon vendor mengubah node sysfs eksternal, CoreFlow mundur secara elegan (tanpa bertarung dengan OEM) dan baru mencoba memulihkan kontrol setelah cooldown eksponensial (`30s → 300s`) jika OEM sudah tenang.
- **CFJ4 Durable Rollback & A/B Pause Instan** — setiap mutasi kernel dicatat sebelum diterapkan dan dapat dibandingkan langsung (*Pause / Resume* 1-klik di WebUI) atau dipulihkan 100% ke baseline OEM tanpa reboot.

---

## Instalasi

- **Syarat**: Android 13+ (API 33+), ARM64 (`arm64-v8a`), root via **KernelSU / KernelSU-Next / APatch / Magisk**.
- Unduh `CFAE-System-Framework-package.zip` dari [GitHub Releases](https://github.com/mystivara-creator/CoreFlow-Engine/releases) atau tab **Artifacts** di GitHub Actions.
- Flash melalui root manager lalu reboot.
- Buka **WebUI MD3 Control Center**:
  - **KernelSU / KernelSU-Next / APatch**: buka langsung tombol **WebUI** pada kartu modul.
  - **Magisk**: gunakan aplikasi **KsuWebUI Standalone** atau **MMRL** untuk membuka WebUI modul.
- Fitur utama WebUI:
  - **Pause (A/B) / Resume & Start CoreFlow** langsung dari header kontrol.
  - **Session Impact & 4-Pillar Readiness** (uptime, jumlah launch burst, MB pre-read, rata-rata/puncak suhu, status 4 pilar).
  - **Installed App Picker** (pencarian otomatis aplikasi terinstal + 1-klik profil per-app: `Auto` / `Gaming` / `Smooth` / `Battery` / `Stock OEM`).
  - **Inspect CFJ4** & **Export Diag** 1-klik untuk pelaporan bug yang rapi dan aman.

---

## CLI Operasional (Root / Termux)

```sh
coreflowd --status                             # Status live, mode, suhu, PID scale, session & aktuator
coreflowd --capabilities                       # Inspeksi topologi hardware (read-only)
coreflowd --journal                            # Audit seluruh node aktif di jurnal CFJ4
coreflowd --diag                               # Bundel diagnostik lengkap (privacy-safe JSON)
coreflowd --set-config profile=smoothness      # Ubah konfigurasi live tanpa restart
coreflowd --set-config inspection_mode=true    # Pause instan ke baseline OEM (A/B compare)
coreflowd --rollback-baseline                  # Hentikan daemon & pulihkan 100% baseline OEM
```

> **Kill switch darurat**: buat file kosong `/data/adb/coreflow/DISABLE` atau nonaktifkan modul di root manager.

---

## Validasi Perangkat & Transparansi

CoreFlow tidak memaksa *unlock thermal* berbahaya dan tidak mengklaim "magic FPS boost" yang merusak baterai. Fokus utamanya adalah **frame pacing lebih konsisten, cold-launch lebih sigap, dan stabilitas termal jangka panjang**.
- Panduan uji perangkat & metrik *Before vs After*: lihat [docs/DEVICE_TEST.md](docs/DEVICE_TEST.md) dan [docs/DEVICE_VALIDATION_SUITE.md](docs/DEVICE_VALIDATION_SUITE.md).
- Bagikan hasil `coreflowd --diag` dan laporan benchmark perangkat Anda di grup komunitas untuk memperluas matriks perangkat teruji.

---

## Build & Pengujian

```sh
cmake -S . -B build/host -G Ninja -DCORE_FLOW_BUILD_TESTS=ON
cmake --build build/host --parallel && ctest --test-dir build/host --output-on-failure
cargo test --manifest-path rust/Cargo.toml && npm run lint && npm run build
```

---

## Lisensi, Hak Cipta & Aturan Fork

Proyek ini dilisensikan di bawah **[Apache License 2.0](LICENSE)** — **Copyright © 2026 mystivara-creator**.

Jika Anda melakukan **fork**, memodifikasi, atau mendistribusikan ulang kode ini (termasuk dalam bentuk modul/binary hasil kompilasi), Anda **wajib**:
1. Menyertakan salinan penuh file [LICENSE](LICENSE) dan [NOTICE](NOTICE) serta mempertahankan seluruh pemberitahuan hak cipta asli milik **mystivara-creator**.
2. Mencantumkan **catatan perubahan yang jelas** (*prominent notice of changes*) pada file atau dokumentasi yang dimodifikasi sesuai Pasal 4(b) Apache-2.0.
3. Tidak menggunakan nama resmi **CoreFlow Engine / CFAE** untuk menyiratkan bahwa fork tidak resmi mendapat dukungan resmi dari pemilik asli.

---

## Komunitas

- [Telegram Channel](https://t.me/VaraProjects) · [Telegram Discussion](https://t.me/VaraProjectsDiscussion) · [GitHub Issues](https://github.com/mystivara-creator/CoreFlow-Engine/issues)
