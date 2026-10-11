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

### `src/observe/` — mata
| File | Fungsi |
|------|--------|
| `discovery.cpp` | Scan capability perangkat (VM sysctl, block, cgroup, uClamp path, …). **Tidak** mengizinkan mutasi. |
| `observer.cpp` | Ambil sampel runtime tiap tick (thermal, mem, load, IO, …). |
| `environment.cpp` | Tipe & matriks capability lingkungan. |
| `ecosystem_knowledge.cpp` | Katalog pengetahuan lunak (risiko/kelas) — **bukan** allow-list mutasi. |
| `high_capability.cpp` | Domain perluasan (observe-only evidence). |

### `src/understand/` — otak
| File | Fungsi |
|------|--------|
| `context.cpp` | Workload + headroom + boleh mutasi / hanya stabilizing. |
| `effect_model.cpp` | Skor benefit/risk/confidence; prior = bonus; unknown numeric pakai batas struktural. |
| `decision_agent.cpp` | Alur observe → hypothesize → review → propose. |
| `experience.cpp` | Memori outcome terverifikasi per scope perangkat. |
| `thermal_*.cpp` | Fitur, guard hysteresis, prediktor (ONNX/heuristic). |

### `src/balance/` — penyeimbang
| File | Fungsi |
|------|--------|
| `policy.cpp` | State machine (Idle/Normal/…/ThermalGuard) + keputusan kasar. |
| `control.cpp` | PolicyEngine plan + MutationAuthority (permit akhir). |
| `safety_hold.cpp` | Kill switch / SAFE_MODE. |

### `src/adapt/` — tangan terukur
| File | Fungsi |
|------|--------|
| `resource_mutation.cpp` | Epoch mutasi resource (VM/IO class). |
| `mutation.cpp` | Epoch mutasi CPUFreq (opt-in terpisah). |
| `resource_actuator.cpp` / `cpufreq_actuator.cpp` | Write + read-back verify. |
| `resource_model.cpp` | State resource yang diizinkan policy. |
| `baseline_intelligence.cpp` | Jendela sebelum/sesudah untuk outcome. |
| `outcome.cpp` | Klasifikasi hasil. |

### `src/conscience/` — nurani
| File | Fungsi |
|------|--------|
| `mutation_journal.cpp` | Snapshot pabrik; commit sebelum write. |
| `retained_state.cpp` | State tahan reboot untuk recovery. |
| `decision_trace.cpp` | Jejak keputusan (debug). |

### `src/runtime/` — daemon
| File | Fungsi |
|------|--------|
| `main.cpp` | Entry process. |
| `autonomous.cpp` | Tick loop: satukan semua lapisan. |
| `config.cpp` | Config / mode / armed. |
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
2. Ikuti satu tick di `src/runtime/autonomous.cpp`.  
3. Loncat ke lapisan yang dipanggil (observe → understand → balance → adapt).  
4. Untuk izin write: `discovery` (class) → `effect_model` (skor) → `control` (authority) → `resource_actuator` (write).  
