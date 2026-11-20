# CoreFlow Engine (CFAE)

[![Version](https://img.shields.io/badge/version-v5.0.2-blue)](#)
[![Platform](https://img.shields.io/badge/Android-15%2B%20(API%2035%2B)%20ARM64-3DDC84)](#instalasi)
[![License](https://img.shields.io/badge/license-Apache--2.0-orange)](LICENSE)

**CoreFlow Engine** adalah framework sistem otonom berbasis **C++23 + Rust** untuk perangkat Android root (KernelSU / APatch / Magisk). CoreFlow melakukan deteksi topologi perangkat keras secara dinamis (**zero hardcoded path**) dan mengkalibrasi seluruh parameter langsung dari karakteristik silicon serta telemetri kernel real-time (**zero fixed tuning value**), dengan jaminan pemulihan penuh ke konfigurasi pabrik melalui jurnal **CFJ4 OEM Rollback**.

---

## Arsitektur 4-Pilar Otonom

| Pilar | Fokus | Mekanisme Inti |
| :--- | :--- | :--- |
| **1. Frame & UI Smoothness** | Respons sentuhan `< 5 ms` & animasi halus | Per-TID `SYS_sched_setattr` (`uclamp_min/max`) pada thread CFS kritis (`RenderThread`, `RenderEngine`, `hwuiTask`, `UnityMain`, `GameThread`) yang dikalibrasi dari kapasitas EAS cluster (`little`/`mid`/`big`) + refresh rate layar — menghindari bentrok PowerHAL cgroup global. |
| **2. Dynamic Storage Burst & QoS** | App launch & loading game cepat | Penemuan otomatis antrean blok fisik (UFS/NVMe/eMMC) & `dm-*` aktif dari `/proc/self/mountinfo`; adaptasi `read_ahead_kb` sesuai geometri antrean (`max_sectors_kb`, `nr_requests`) + `io_psi`, serta isolasi cgroup `background`. |
| **3. Anti-Stutter Memory & Smart Pre-Read** | Bebas lag saat multitasking | Aktivasi otomatis Linux **MGLRU**, adaptasi kontinu `min_ttl_ms`, `swappiness`, dan `compaction_proactiveness` dari `MemAvailable` + ZRAM + `mem_psi`, serta prefetch `posix_fadvise(POSIX_FADV_WILLNEED)` terukur khusus aplikasi UI foreground. |
| **4. Thermal PID & Battery Longevity** | Performa stabil tanpa throttling kasar | **Rust PID controller** + prediksi termal **Neural 10 detik** memodulasi amplop CPU/GPU secara halus, pemutus darurat termal (`SoC ≥ 95 °C` / `Baterai ≥ 58 °C`), dan step-charging baterai. |

### Prinsip Desain
- **Zero Fixed Tuning Values & Zero Hardcoded Paths** — setiap perangkat dikalibrasi dari hardware-nya sendiri saat startup & runtime.
- **Anti-Contention OEM** — jika daemon vendor mengubah node sysfs eksternal, CoreFlow mundur secara elegan alih-alih bertarung dengan sistem.
- **CFJ4 Durable Rollback** — setiap mutasi kernel dicatat sebelum diterapkan dan dipulihkan bersih saat daemon berhenti, crash, atau modul dihapus.

---

## Instalasi

- **Syarat**: Android 15+ (API 35+), ARM64 (`arm64-v8a`), root via **KernelSU / KernelSU-Next / APatch / Magisk**.
- Unduh `CFAE-System-Framework-package.zip` dari [GitHub Releases](https://github.com/mystivara-creator/CoreFlow-Engine/releases) atau tab **Artifacts** di GitHub Actions.
- Flash melalui root manager lalu reboot.
- Buka **WebUI MD3 Control Center** langsung dari root manager untuk memilih profil (`Balanced` / `Smoothness` / `Battery` / `Gaming`), toggle fitur, aturan per-aplikasi (`Gaming` / `Excluded OEM Stock`), serta audit jurnal CFJ4.

---

## CLI Operasional (Root / Termux)

```sh
coreflowd --status                             # Status live, mode, suhu, PID scale & aktuator
coreflowd --capabilities                       # Inspeksi topologi hardware (read-only)
coreflowd --journal                            # Audit seluruh node aktif di jurnal CFJ4
coreflowd --set-config profile=smoothness      # Ubah konfigurasi live tanpa restart
coreflowd --rollback-baseline                  # Hentikan daemon & pulihkan 100% baseline OEM
```

> **Kill switch darurat**: buat file kosong `/data/adb/coreflow/DISABLE` atau nonaktifkan modul di root manager.

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
