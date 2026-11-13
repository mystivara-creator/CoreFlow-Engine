# CFAE System Framework

**CFAE System Framework (CoreFlow Engine)** adalah framework optimasi sistem Android berbasis kapabilitas perangkat. CoreFlow mengevaluasi workload, tekanan sistem, dan kondisi termal sebelum menerapkan perubahan kernel yang terukur.

> OEM baseline is a reference, not an authority ceiling. CoreFlow earns operational authority through capability discovery, controlled actuation, and verified outcomes.

## Prinsip inti

- **Discover before actuation:** kenali semantik, batas, dan kemampuan interface perangkat.
- **Coordinate mutations:** controller berbagi ownership dan prioritas melalui transaction coordinator.
- **Verify every change:** journal, readback, settling, dan evaluasi hasil mendahului retensi.
- **Protect OEM ownership:** perubahan eksternal tidak ditimpa; recovery tetap menghormati baseline dan journal.

## Engine

| Komponen | Tanggung jawab |
|---|---|
| C++23 | Native daemon, device profiling, kernel discovery, controller, transaction dan recovery |
| Rust 2024 | Thermal/pressure state machine, hysteresis, dan slew-bounded PID |
| Kotlin 2.1 | Integrasi Android, Quick Settings dan thermal receiver |
| TypeScript / React | WebUI dan native daemon bridge |

CoreFlow mencakup CPUFreq/UClamp, cgroup v2, block I/O, VM pressure, thermal policy, serta hardware-longevity controls sesuai bukti dan dukungan kernel. GPU/Power HAL yang hanya terdeteksi tetap berstatus observation-only; keberadaan interface tidak otomatis memberi kewenangan mutasi.

## Instalasi dan kompatibilitas

- Android 15+ (API 35+), ARM64, dan akses root.
- Paket flashable tersedia melalui [GitHub Releases](https://github.com/mystivara-creator/CoreFlow-Engine/releases).
- Dukungan controller ditentukan ulang saat runtime; fitur yang tidak lolos validasi akan tetap nonaktif.

## Status pengembangan

v3.1.0. Pekerjaan native input dan foreground render-thread pada [PR #29](https://github.com/mystivara-creator/CoreFlow-Engine/pull/29) masih berstatus Draft. Implementasi saat ini menyediakan touch-triggered transient top-app UClamp dengan expiry dan ownership-checked restore serta foreground-task placement yang dibatasi identitas dan admission thermal/workload. Ini belum merupakan integrasi SurfaceFlinger FrameTimeline, frame pacing end-to-end, atau kontrol refresh-rate layar. CI hijau bukan pengganti validasi runtime pada perangkat.

## Dokumentasi

README ini sengaja ringkas. Penjelasan arsitektur, alur transaksi, controller, konfigurasi, lifecycle, serta sejarah refactor sejak PR #7 dipusatkan pada:

- [CoreFlow — Website & Overview](https://github.com/mystivara-creator/CoreFlow-Engine/blob/website/cfae-landing-page/docs/index.html)
- [Engineering Reference](https://github.com/mystivara-creator/CoreFlow-Engine/blob/website/cfae-landing-page/docs/reference.html)

## Operasional

Jalankan sebagai root:

- `coreflowd --status` — status daemon.
- `coreflowd --capabilities` — inspeksi kapabilitas read-only.
- `coreflowd --stop` — hentikan daemon.
- `coreflowd --rollback-baseline` — hentikan dan pulihkan baseline terverifikasi.

Kill switch: `/data/adb/coreflow/DISABLE` atau nonaktifkan modul melalui root manager. Jangan menghapus `baseline.journal` secara manual untuk melewati recovery.

## Komunitas

- [Telegram Channel](https://t.me/VaraProjects)
- [Telegram Discussion](https://t.me/VaraProjectsDiscussion)
- [GitHub Issues](https://github.com/mystivara-creator/CoreFlow-Engine/issues)

## Lisensi

[MIT License](LICENSE).
