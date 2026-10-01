# 🚀 CoreFlow Engine v2.3

Advanced Native C++ Daemon for Android 14+

[![Android](https://img.shields.io/badge/Android-14+-green.svg)](https://www.android.com/)
[![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE)

## 📋 Table of Contents

- [Deskripsi](#deskripsi)
- [Fitur Utama](#fitur-utama)
- [Kompatibilitas](#kompatibilitas)
- [Persyaratan Instalasi](#persyaratan-instalasi)
- [Cara Instalasi](#cara-instalasi)
- [Verifikasi Instalasi](#verifikasi-instalasi)
- [Kontribusi & Kompilasi](#kontribusi--kompilasi)
- [Disclaimer](#disclaimer)
- [Lisensi](#lisensi)

---

## 📖 Deskripsi

CoreFlow Engine adalah **systemless module** (Magisk/KernelSU/APatch) yang beroperasi sebagai native background daemon. Ditulis murni dalam **C++17 modern**, modul ini mengambil alih manajemen:

- 🧠 CPU WALT Scheduler
- 💾 I/O Storage
- 🔄 ZRAM Swappiness
- 🎮 QoS DSP/NPU

Semuanya berjalan secara **real-time tanpa membebani sistem** (Zero Overhead).

### Profil Khusus

Modul ini memiliki **Kunzite Privilege Profile** yang dirancang eksklusif untuk memaksimalkan potensi **Redmi Note 15 5G** (Snapdragon 6 Gen 3 + Adreno 710), namun tetap memiliki kapabilitas **Hybrid Universal** untuk beradaptasi dengan perangkat Android 14+ lainnya.

---

## ✨ Fitur Utama

### 🧠 5-Pillar Dynamic State Machine

Mesin pendeteksi beban berbasis Jiffies (`/proc/stat`) dan GPU Busy (`/sys/class/kgsl`) yang secara cerdas merespons kondisi perangkat ke dalam 5 status:

| Status | Deskripsi |
|--------|-----------|
| **Daily Efficiency** (Idle) | Menurunkan beban latar belakang dan mempertahankan profil I/O stock pabrik demi baterai. |
| **App Launch / Burst** | I/O sweet spot (1024KB) dan sinkronisasi WALT Scheduler untuk akselerasi instan tanpa cache bloat. |
| **Gaming Unleashed** | Membuka limitasi I/O hingga 2048KB, mengubah target beban CPU, memaksimalkan governor Adreno TZ, dan mengaktifkan DSP QoS untuk render assistance. |
| **Ultra Deep Sleep** | Saat layar mati, CPU diubah ke mode powersave, I/O buffer ditekan ke 256KB, dan Swappiness didorong ke 120% untuk membersihkan RAM. |
| **Thermal Guardian** | Memantau suhu baterai langsung. Jika menyentuh 43°C, sistem melakukan throttling perlindungan secara native dengan notifikasi peringatan. Pemulihan performa otomatis saat suhu turun. |

### ⚡ Zero-Overhead Architecture (Anti-Lag)

Tidak menggunakan eksekusi shell/bash script kuno yang memakan siklus CPU. Seluruh manipulasi parameter sysfs dan properti Android dilakukan via:

- C++ File Stream (`std::ofstream`)
- Bionic Native API (`__system_property_set`)

### 🛡️ Anti-Bootloop & Graceful Fallback

- Berjalan pada fase **late-start service**
- Jika perangkat tidak memiliki direktori sysfs yang dituju, engine akan mengabaikannya secara otomatis
- **Graceful Fallback** tanpa Kernel Panic atau Bootloop

---

## ⚠️ Persyaratan Instalasi

**⚠️ WAJIB DIBACA!** Agar perangkat Anda tetap stabil, patuhi aturan berikut:

### 🚫 Larangan

- **Jangan menggabungkan modul performa lain** seperti Magnetar, Uperf, KTweak, NFS, atau modul Thermal Unlocker. Hapus semua modul tweaks CPU/GPU pihak ketiga.
- **Matikan Auto-Profile** di Kernel Manager (FKM, EXKM, SmartPack). Biarkan CoreFlow yang mengatur segalanya secara dinamis.

### ✅ Persyaratan

- **ZRAM harus aktif** di sistem Anda. CoreFlow bergantung pada manipulasi swappiness (60% hingga 120%).

### ℹ️ Catatan untuk Non-Snapdragon

- Modul ini **100% aman** (Anti-Bootloop) untuk HP MediaTek/Exynos/Unisoc
- Deteksi GPU presisi saat bermain game hanya aktif di perangkat Adreno GPU (Snapdragon)
- Untuk SoC lain, engine mengandalkan kalkulasi persentase CPU Jiffies murni

---

## ⚙️ Kompatibilitas

| Aspek | Persyaratan |
|-------|-----------|
| **Arsitektur** | ARM64 (aarch64) |
| **OS Target** | Android 14+ (HyperOS, PixelOS, LineageOS, dll) |
| **Root Manager** | Magisk v24+, KernelSU-Next, APatch |

---

## 📦 Cara Instalasi

1. Unduh file `.zip` rilis terbaru dari halaman [Releases](../../releases)
2. Buka aplikasi **Magisk** / **KernelSU** / **APatch**
3. Pilih menu **Modules** → **Install from storage**
4. Pilih file `CoreFlow_Engine_v2.3.zip`
5. Tunggu proses flashing selesai, lalu **Reboot**

---

## 🔍 Verifikasi Instalasi

Untuk memastikan CoreFlow Engine berjalan di latar belakang Anda, gunakan aplikasi terminal (Termux) atau PC via ADB:

```bash
su
logcat -s CoreFlowEngine
```

Anda akan melihat transisi log saat:
- Layar dihidupkan/dimatikan
- Membuka aplikasi berat/game
- Perubahan status termal

---

## 👨‍💻 Kontribusi & Kompilasi

### Prasyarat

- Android NDK (rilis terbaru)
- CMake
- C++17 compiler

### Langkah-langkah

1. Clone repositori ini:
   ```bash
   git clone <repository-url>
   cd CoreFlow-Engine
   ```

2. Pastikan Anda memiliki Android NDK terbaru terinstall

3. Jalankan CMake pada folder root:
   ```bash
   cmake .
   make
   ```

4. Binary akan tersimpan di folder output

---

## 📝 Disclaimer

Modul ini memodifikasi parameter tingkat kernel. Meskipun dirancang dengan pengaman **SafeTuner (Anti-Bootloop)**, pengembang **tidak bertanggung jawab** atas:

- Kerusakan perangkat keras
- Kehilangan data
- Perangkat yang meleleh akibat kondisi termal eksternal

**DWYOR** (Do With Your Own Risk)

---

## 📄 Lisensi

Proyek ini dilisensikan di bawah **MIT License**. Lihat file [LICENSE](./LICENSE) untuk detail lengkap.

### MIT License Summary

Anda diizinkan untuk:
- ✅ Menggunakan perangkat lunak ini untuk tujuan komersial
- ✅ Memodifikasi perangkat lunak
- ✅ Mendistribusikan perangkat lunak
- ✅ Menggunakan secara pribadi

Dengan syarat:
- ⚠️ Sertakan pemberitahuan lisensi dan hak cipta
- ⚠️ Tanggung jawab pengguna atas penggunaan

---

## 👤 Kredit

- **Author**: Mystivara (mystivara-creator)
- **Codename**: Kunzite Privilege
- **Version**: 2.3

---

## 📞 Support & Feedback

Jika Anda menemukan bug atau memiliki saran, silakan buka **Issue** di repositori ini.

---

**Last Updated**: 2024
