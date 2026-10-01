# 🤝 Panduan Kontribusi

Terima kasih telah tertarik untuk berkontribusi pada CoreFlow Engine! Kami sangat menghargai setiap bantuan Anda.

## 📋 Cara Berkontribusi

### 1. Fork Repository

Klik tombol **Fork** di halaman GitHub untuk membuat salinan repositori Anda sendiri.

### 2. Clone Repository Lokal

```bash
git clone https://github.com/mystivara-creator/CoreFlow-Engine.git
cd CoreFlow-Engine
```

### 3. Buat Branch Baru

```bash
git checkout -b fitur/nama-fitur-anda
# atau untuk bug fix
git checkout -b bugfix/nama-bug-anda
```

### 4. Melakukan Perubahan

- Pastikan kode Anda mengikuti standar **C++17**
- Tambahkan komentar yang jelas dan deskriptif
- Jika menambah fitur baru, update dokumentasi

### 5. Commit Changes

```bash
git add .
git commit -m "Deskripsi singkat perubahan Anda"
```

**Format Commit Message:**
```
[TYPE] Deskripsi singkat

Penjelasan detail (opsional)

- Poin 1
- Poin 2
```

**TYPE options:**
- `feat:` Fitur baru
- `fix:` Perbaikan bug
- `docs:` Perubahan dokumentasi
- `refactor:` Refactoring kode
- `perf:` Peningkatan performa
- `test:` Penambahan/perbaikan test

### 6. Push ke GitHub

```bash
git push origin fitur/nama-fitur-anda
```

### 7. Buat Pull Request

1. Pergi ke halaman GitHub repositori original
2. Klik **New Pull Request**
3. Pilih branch Anda dan isi deskripsi
4. Tunggu review dari maintainer

---

## ✅ Checklist Sebelum Submit PR

- [ ] Kode sudah di-test di perangkat Android 14+
- [ ] Tidak ada compiler warning atau error
- [ ] Dokumentasi sudah diupdate (jika perlu)
- [ ] Commit message jelas dan deskriptif
- [ ] Tidak ada file yang tidak perlu (build artifacts, logs)
- [ ] Mengikuti standar coding C++17

---

## 🐛 Melaporkan Bug

Jika menemukan bug, silakan buat **Issue** dengan:

1. **Judul yang jelas**: Deskripsi singkat bug
2. **Deskripsi lengkap**: Apa yang terjadi dan apa yang diharapkan
3. **Langkah reproduksi**: Cara mengulang bug
4. **Info perangkat**: 
   - Model perangkat
   - Android version
   - Root manager yang digunakan
   - Versi CoreFlow Engine
5. **Screenshot/Logcat** (jika ada)

---

## 💡 Saran Fitur

Ingin menyarankan fitur baru? Buat **Issue** dengan:

1. **Judul**: Deskripsi fitur
2. **Deskripsi lengkap**: Mengapa fitur ini berguna?
3. **Contoh use case**: Kapan pengguna akan menggunakannya?
4. **Mockup/Sketsa** (opsional)

---

## 📖 Standar Coding

### C++ Style Guide

```cpp
// Gunakan namespace yang jelas
namespace CoreFlow {
    // Konstan dengan UPPERCASE_WITH_UNDERSCORE
    constexpr int MAX_CPU_FREQ = 2400000;
    
    // Class names dengan PascalCase
    class ThermalManager {
    private:
        // Member variables dengan m_ prefix
        int m_currentTemp;
        
    public:
        // Method names dengan camelCase
        void updateThermalState();
    };
}
```

### Komentar

```cpp
// Gunakan komentar yang jelas dan singkat
// Jelaskan MENGAPA, bukan APA yang dilakukan

// ❌ Buruk
x = x + 1; // tambah 1 ke x

// ✅ Baik
freq_mhz++; // Increment frequency sesuai thermal threshold
```

---

## 🔨 Setup Development

### Prasyarat
- Android NDK (latest)
- CMake 3.10+
- GCC/Clang C++17 compatible

### Kompilasi

```bash
mkdir build && cd build
cmake ..
make
```

### Testing

```bash
# Compile untuk testing
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

---

## 📝 Dokumentasi

Jika menambah fitur:
1. Update README.md dengan deskripsi
2. Tambah contoh penggunaan (jika perlu)
3. Dokumentasikan di komentar kode
4. Update CHANGELOG (jika ada)

---

## 🚫 Code of Conduct

- Bersikap hormat kepada sesama kontributor
- Tidak ada spam atau konten tidak relevan
- Hindari diskusi politik/agama
- Laporkan perilaku tidak layak kepada maintainer

---

## ❓ Pertanyaan?

Jika ada pertanyaan, silakan:
1. Buka **Discussion** di GitHub
2. Atau kirim issue dengan label `question`

---

**Terima kasih telah berkontribusi! 🙏**
