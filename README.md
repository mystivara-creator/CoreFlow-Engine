CoreFlow Engine

Native C++ Runtime Tuning Engine for Android

""Android" (https://img.shields.io/badge/Android-14%2B-green.svg)" (https://www.android.com/)
""Language" (https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)" (https://en.cppreference.com/w/cpp/17)
""License" (https://img.shields.io/badge/License-MIT-yellow.svg)" (./LICENSE)

CoreFlow Engine adalah engine tuning sistem Android berbasis C++17 yang dirancang untuk melakukan penyesuaian parameter kernel secara dinamis berdasarkan kondisi perangkat.

CoreFlow menggunakan pendekatan runtime state management daripada menerapkan satu konfigurasi performa secara permanen. Engine memantau kondisi sistem, menentukan state yang sesuai, kemudian menerapkan parameter tuning yang tersedia pada perangkat.

«Project status: Active development
Target: Android 14+ / ARM64»

---

Features

Dynamic Runtime State Machine

CoreFlow mengklasifikasikan kondisi perangkat berdasarkan metrik runtime seperti CPU activity, screen state, GPU utilization pada perangkat yang menyediakan interface KGSL, dan temperatur.

State utama yang digunakan:

State| Tujuan
Daily Efficiency| Profil penggunaan normal dengan fokus pada efisiensi sistem.
App Launch / Burst| Menangani lonjakan workload singkat seperti pembukaan aplikasi.
Gaming Unleashed| Profil performa untuk workload GPU/CPU yang lebih berat.
Ultra Deep Sleep| Profil konservatif ketika perangkat berada dalam kondisi idle/screen-off.
Thermal Guardian| Profil proteksi ketika temperatur perangkat mencapai kondisi yang ditentukan.

State machine dirancang agar parameter tidak terus-menerus ditulis ulang ketika state belum berubah.

---

Runtime Tuning

CoreFlow dapat berinteraksi dengan interface kernel yang tersedia melalui filesystem Android, terutama:

- CPU frequency/governor interfaces
- CPU scheduler-related parameters
- Storage I/O parameters
- ZRAM / VM parameters
- GPU interfaces pada perangkat yang mengekspos KGSL
- Thermal information
- Android system properties yang relevan

Tidak semua perangkat menyediakan node kernel yang sama. Karena itu, CoreFlow menggunakan pendekatan capability-based tuning: parameter hanya diterapkan apabila interface yang dibutuhkan tersedia dan dapat digunakan.

---

Native Architecture

CoreFlow dibuat sebagai native C++ daemon dan tidak bergantung pada shell command untuk setiap operasi tuning.

Pendekatan ini memungkinkan engine berinteraksi langsung dengan interface sistem melalui API native seperti:

- "std::ifstream"
- "std::ofstream"
- Android/Bionic system property API
- filesystem dan system interfaces lainnya

Tujuannya adalah menjaga implementasi tetap sederhana, terkontrol, dan menghindari ketergantungan pada rangkaian shell script untuk setiap perubahan parameter.

«CoreFlow tetap memiliki runtime overhead karena melakukan monitoring dan evaluasi kondisi sistem. Fokus desainnya adalah menjaga overhead tersebut tetap rendah, bukan mengklaim zero overhead.»

---

Device Adaptation

CoreFlow tidak mengasumsikan bahwa semua perangkat memiliki struktur kernel yang identik.

Ketika sebuah node tidak tersedia:

Node available
    ↓
Validate
    ↓
Capture current value
    ↓
Apply tuning
    ↓
Verify

Jika node tidak tersedia atau tidak dapat digunakan:

Node unavailable
    ↓
Skip parameter
    ↓
Continue with available capabilities

Pendekatan ini memungkinkan engine beradaptasi dengan variasi kernel dan konfigurasi perangkat.

GPU Detection

Pada perangkat dengan interface KGSL, CoreFlow dapat menggunakan informasi GPU busy untuk membantu menentukan workload GPU.

Pada perangkat yang tidak menyediakan interface tersebut, engine dapat menggunakan metrik lain yang tersedia, seperti CPU activity.

---

Configuration Philosophy

CoreFlow berusaha mempertahankan prinsip:

Detect → Validate → Snapshot → Tune → Monitor → Restore

1. Detect

Mengidentifikasi hardware dan interface kernel yang tersedia.

2. Validate

Memastikan parameter dan node target dapat digunakan sebelum melakukan perubahan.

3. Snapshot

Menyimpan nilai awal parameter yang akan dimodifikasi.

4. Tune

Menerapkan konfigurasi berdasarkan runtime state.

5. Monitor

Mengamati perubahan workload dan kondisi perangkat.

6. Restore

Mengembalikan parameter ke nilai sebelumnya ketika diperlukan.

Pipeline ini penting karena konfigurasi kernel dapat berbeda antar-device dan antar-kernel.

---

Thermal Handling

CoreFlow memiliki Thermal Guardian untuk memantau temperatur yang tersedia dari interface thermal Android.

Ketika temperatur melewati threshold yang telah ditentukan oleh konfigurasi engine, CoreFlow dapat berpindah ke thermal state dan mengurangi agresivitas tuning.

Setelah kondisi kembali normal, engine dapat kembali ke state runtime yang sesuai.

Threshold dan perilaku thermal harus dianggap sebagai engine configuration, bukan sebagai jaminan bahwa perangkat akan selalu berada pada temperatur tertentu.

---

Compatibility

Component| Target
Architecture| ARM64 / AArch64
Android| Android 14+
Language| C++17
Root Framework| Magisk / KernelSU / APatch
GPU telemetry| KGSL jika tersedia

Kompatibilitas aktual bergantung pada kernel, vendor implementation, exposed sysfs/procfs nodes, permission model, dan konfigurasi perangkat.

Tidak semua fitur tersedia pada semua device.

---

Installation

CoreFlow didistribusikan sebagai module untuk environment Android yang mendukung systemless modules.

1. Download release yang sesuai dari halaman Releases.
2. Install module menggunakan root manager yang kompatibel.
3. Reboot perangkat jika diperlukan oleh release tersebut.
4. Periksa log CoreFlow untuk memastikan daemon berhasil berjalan.

«Nama file release dapat berubah pada setiap versi. Gunakan file yang tersedia pada release terkait, bukan nama file yang di-hardcode di dokumentasi.»

---

Runtime Verification

Log runtime dapat digunakan untuk memeriksa state transition dan aktivitas engine.

Contoh:

su
logcat -s CoreFlowEngine

Hal yang dapat diamati antara lain:

- perubahan runtime state
- perubahan workload
- thermal state
- device capability detection
- parameter yang berhasil diterapkan
- parameter yang dilewati karena tidak tersedia

Format dan tag log dapat berubah selama pengembangan.

---

Building

Requirements

- Android NDK
- CMake
- C++17-compatible compiler

Clone

git clone https://github.com/mystivara-creator/CoreFlow-Engine.git
cd CoreFlow-Engine

Build

Build configuration dapat berbeda berdasarkan target release dan environment development.

Untuk build menggunakan CMake:

cmake -S . -B build
cmake --build build

Output binary akan berada di directory build sesuai konfigurasi CMake.

---

Project Structure

CoreFlow-Engine/
├── include/
│   └── ...
├── src/
│   └── ...
├── module_template/
│   └── ...
├── CMakeLists.txt
├── LICENSE
├── CHANGELOG.md
├── CONTRIBUTING.md
└── README.md

Struktur dapat berubah selama pengembangan.

---

Design Goals

CoreFlow dikembangkan dengan beberapa tujuan utama:

- Runtime-aware tuning
- Device capability detection
- Graceful handling of missing kernel interfaces
- Minimal dependency terhadap shell scripting
- State-based configuration
- Snapshot dan restore parameter
- Modular C++ architecture
- Observable runtime behavior

CoreFlow tidak bertujuan menyediakan satu konfigurasi universal yang dianggap optimal untuk setiap perangkat.

Sebaliknya, engine mencoba menggunakan parameter yang benar-benar tersedia pada perangkat yang sedang dijalankan.

---

Limitations

CoreFlow berinteraksi dengan interface kernel yang dapat berbeda antar:

- SoC
- vendor
- kernel version
- custom kernel
- Android version
- device configuration

Karena itu, hasil tuning dapat berbeda antar perangkat.

Tidak adanya suatu node kernel bukan berarti engine gagal secara keseluruhan; fitur yang bergantung pada node tersebut dapat dilewati sementara fitur lain tetap berjalan.

---

Disclaimer

CoreFlow memodifikasi parameter sistem pada perangkat yang memiliki akses root.

Penggunaan konfigurasi kernel yang tidak sesuai dapat menyebabkan:

- perubahan performa
- peningkatan konsumsi daya
- peningkatan temperatur
- ketidakstabilan sistem
- parameter tidak bekerja sebagaimana yang diharapkan

Gunakan pada perangkat yang kamu pahami dan selalu simpan konfigurasi/original state sebelum melakukan eksperimen.

Use at your own risk.

---

Contributing

Bug report, improvement, dan pull request dipersilakan.

Jika melaporkan masalah, sertakan informasi seperti:

- device model
- SoC
- Android version
- kernel version
- root framework
- relevant runtime logs
- parameter/node yang bermasalah

Informasi tersebut membantu reproduksi masalah pada environment yang berbeda.

---

License

CoreFlow Engine is released under the MIT License.

See "LICENSE" (./LICENSE) for the complete license text.

---

Author

Mystivara

GitHub: "@mystivara" (https://github.com/mystivara-creator)

Project: CoreFlow Engine

---

«CoreFlow Engine is an experimental system-tuning project focused on adaptive runtime management for Android.»