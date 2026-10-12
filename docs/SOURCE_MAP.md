# CoreFlow — Source Map

Baca file ini dulu sebelum membuka folder `src/` atau `include/`.

Engine disusun mengikuti empat fungsi:

**Observe → Understand → Balance → Adapt**  
(+ **Conscience** untuk journal/restore, **Runtime** untuk daemon)

Bukan daftar parameter. Bukan kumpulan file acak.

---

## Peta cepat

```text
Android / Kernel
       │
       ▼
┌────────────── OBSERVE ──────────────┐
│  Apa yang perangkat ini expose?     │
│  discovery · observer · environment │
└──────────────────┬──────────────────┘
                   ▼
┌────────────── UNDERSTAND ───────────┐
│  Apa artinya untuk stabilitas?      │
│  context · effect_model · thermal   │
│  experience · decision_agent        │
└──────────────────┬──────────────────┘
                   ▼
┌────────────── BALANCE ──────────────┐
│  Apakah boleh intervensi sekarang?  │
│  policy · control · safety_hold     │
└──────────────────┬──────────────────┘
                   ▼
┌────────────── ADAPT ────────────────┐
│  Ubah sedikit → ukur → lanjut/undo  │
│  mutation · actuators · baseline    │
└──────────────────┬──────────────────┘
                   ▼
┌────────────── CONSCIENCE ───────────┐
│  Journal, retained state, trace     │
└─────────────────────────────────────┘
```

---

## `src/` (implementasi)

Layout mengikuti tiga peran: **mata** (observe), **otak** (understand + balance), **tangan** (mutation). Lihat `CMakeLists.txt` untuk daftar kompilasi.

### `src/observe/` — mata (melihat, tidak memutuskan)
| File | Fungsi |
|------|--------|
| `discovery.cpp` | Inventaris capability perangkat (VM sysctl, block, scheduler, build prop). Bertanya ke `balance/consultation` untuk kelas surface; **tidak** menentukan izin mutasi sendiri. |
| `capability_registry.cpp` | Registri fakta capability yang sudah ditemukan. |
| `observer.cpp` | Sampel runtime tiap tick (thermal, mem, load, IO). |
| `environment.cpp` | Tipe & matriks capability lingkungan. |
| `ecosystem_knowledge.cpp` | Katalog pengetahuan lunak (risiko/kelas), bukan allow-list. |
| `high_capability.cpp` | Domain perluasan, observe-only. |

### `src/understand/` — penalaran
| File | Fungsi |
|------|--------|
| `context.cpp` | Workload + headroom + boleh mutasi / hanya stabilizing. |
| `effect_model.cpp` | Skor benefit/risk/confidence; prior = bonus; knob tanpa prior ditolak (observe-only). |
| `decision_agent.cpp` | Alur observe → hypothesize → review → propose. |
| `experience.cpp` | Memori outcome terverifikasi per scope perangkat. |
| `baseline_intelligence.cpp` | Jendela sebelum/sesudah untuk klasifikasi outcome. |
| `thermal_features.cpp`, `thermal_guard.cpp`, `thermal_predictor.cpp` | Fitur, guard hysteresis, prediktor (ONNX/heuristic). |

### `src/balance/` — otak (memutuskan izin)
| File | Fungsi |
|------|--------|
| `policy.cpp` | State machine (Idle/Normal/…/ThermalGuard) + keputusan kasar. |
| `control.cpp` | PolicyEngine plan + MutationAuthority (permit akhir). |
| `consultation.cpp` | Konsultasi per kandidat: Allow / Defer / Deny dengan alasan. Urutan: larangan → kemampuan → semantik → stres → kebutuhan → benefit > risk → izin pengguna. |
| `resource_model.cpp` | State resource yang diizinkan policy. |
| `safety_hold.cpp` | Kill switch / SAFE_MODE. |

### `src/mutation/` — tangan (hanya dengan permit)
| File | Fungsi |
|------|--------|
| `resource_mutation.cpp` | Epoch mutasi resource (VM/IO/scheduler). |
| `mutation.cpp` | Epoch mutasi CPUFreq (opt-in terpisah). |
| `resource_actuator.cpp` / `cpufreq_actuator.cpp` | Write + read-back verify. |

### `src/conscience/` — ingatan
| File | Fungsi |
|------|--------|
| `mutation_journal.cpp` | Snapshot pabrik; commit sebelum write. |
| `retained_state.cpp` | State tahan reboot untuk recovery. |
| `decision_trace.cpp` | Jejak keputusan (debug). |

### `src/autonomous/` — loop
| File | Fungsi |
|------|--------|
| `autonomous.cpp` | Tick loop: satukan semua lapisan. |

### `src/runtime/` — daemon
| File | Fungsi |
|------|--------|
| `main.cpp` | Entry process. |
| `config.cpp` | Config / mode / armed / izin pengguna (`grant_*`). |
| `runtime_paths.cpp` | Lokasi file state dan status. |
| `status_snapshot.cpp` | Tulis `status.json` untuk UI. |
| `signal_state.cpp` | Sinyal stop/refresh. |

---

## `include/coreflow/` (kontrak API)

Header = kontrak publik antar-lapisan. Nama file mengikuti modul di atas.

Yang sering membingungkan:
| Header | Artinya |
|--------|---------|
| `types.hpp` | Tipe runtime bersama (sample, state, enum). |
| `environment.hpp` | Domain & capability matrix. |
| `control.hpp` | PolicyPlan + MutationAuthority. |
| `controller.hpp` | Alias tipis ke `MutationController` (kompatibilitas). |
| `actuator.hpp` | Antarmuka aktuator generik. |
| `thermal_limits.hpp` | Hanya konstanta plausibility suhu (header-only). |

---

## Yang bukan tanggung jawab file mana

- **Discovery tidak menulis kernel.**
- **EffectModel tidak mengizinkan write.**
- **Policy tidak memilih path sembarangan** — hanya kapan intervensi masuk akal.
- **Actuator tidak memilih nilai** — hanya menulis nilai yang sudah diizinkan + verify.
- **Tidak ada daftar `vm.swappiness` sebagai syarat mutasi** — kelas permukaan + deny-list + skor.

---

## Cara baca untuk evaluasi

1. Baca peta ini.  
2. Ikuti satu tick di `src/autonomous/autonomous.cpp`.  
3. Loncat ke lapisan yang dipanggil (observe → understand → balance → adapt).  
4. Untuk izin write: `discovery` (class) → `effect_model` (skor) → `control` (authority) → `resource_actuator` (write).  
