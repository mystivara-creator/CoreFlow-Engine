# CoreFlow — Developer Debug Checklist

Untuk **developer engine**, bukan pengguna modul biasa.  
Tujuan: menguji Observe → Understand → Balance → Adapt secara penuh dan terarah.

Baca juga `docs/SOURCE_MAP.md`.

---

## 0. Instrumen wajib

| Sumber | Path / perintah |
|--------|------------------|
| Log | `logcat -s CoreFlowAutonomous:V` |
| Status UI | Live engine (baca `status.json`) |
| Status file | `/data/adb/coreflow/status.json` |
| Config | `/data/adb/coreflow/config.ini` |
| Journal CPU | `/data/adb/coreflow/mutation_journal.txt` |
| Journal resource | (path resource journal di log/status) |
| Experience | `/data/adb/coreflow/experience.db` |
| Decision trace | `/data/adb/coreflow/decision_trace.jsonl` |
| Kill switch | `/data/adb/coreflow/DISABLE` |
| Safe mode | `/data/adb/coreflow/SAFE_MODE` |

---

## 1. Observe — inventory benar?

**Config:** mode bebas; mutasi boleh off.

- [ ] Daemon jalan, `status.json` terisi tiap cycle  
- [ ] `uClamp` = yes jika `/dev/cpuctl/*/cpu.uclamp.*` ada  
- [ ] `cgroup v2` / `devfreq` sesuai perangkat  
- [ ] `discovered` dan `policy-ready` masuk akal (policy-ready ⊂ discovered)  
- [ ] VM nodes muncul dari **scan** `/proc/sys/vm`, bukan dari daftar statis di kepala  

**Gagal di sini → jangan lanjut adaptive.** Perbaiki `src/observe/`.

---

## 2. Understand — konteks & skor?

**Config:** masih boleh observe-only / belum armed.

- [ ] `workload` berubah wajar (Idle / Interactive / CpuBound / …)  
- [ ] `confidence` tidak selalu 0 atau selalu 1  
- [ ] Decision agent mengisi summary (ReActLite / reason string)  
- [ ] Thermal trend + prediksi muncul saat suhu bergerak  
- [ ] Di UI: Context Open vs blocked selaras headroom  

**Gagal →** `src/understand/` (context, effect_model, thermal).

---

## 3. Balance — gate & hold?

- [ ] ThermalGuard enter/exit tidak bounce (histeresis)  
- [ ] SAFE_MODE / DISABLE mematikan mutasi dan memicu restore  
- [ ] Satu epoch: tidak ada dua mutasi resource bersamaan  
- [ ] `skip_reason` / hold reason terbaca di status (bukan diam tanpa sebab)  

**Gagal →** `src/balance/` + bagian hold di `autonomous.cpp`.

---

## 4. Adapt resource — jalur penuh (debug developer)

**Config disarankan untuk debug resource:**
```ini
mutation_mode=adaptive
mutation_armed=true
allow_cpu_governor=no
```

- [ ] Authority **Permit** resource saat context terbuka  
- [ ] Ada kandidat dari EffectModel (path + requested value)  
- [ ] Journal **commit sebelum** write  
- [ ] Read-back verify sukses  
- [ ] Observation window jalan → outcome (beneficial / neutral / regression)  
- [ ] Restore / rollback membersihkan ke baseline  
- [ ] Experience mencatat outcome untuk scope perangkat ini  

Ulangi untuk beberapa kandidat (VM class dan IO fisik jika ada).

**Gagal verify/restore → hentikan adaptive.** Perbaiki `src/mutation/` + `src/conscience/`.

---

## 5. Adapt CPU — terpisah

Hanya setelah langkah 4 bersih.

```ini
allow_cpu_governor=yes
```

- [ ] Tanpa flag ini, CPU path tidak menulis  
- [ ] Dengan flag: governor experiment ter-journal dan restoreable  
- [ ] Tidak mengganggu resource journal (jurnal terpisah)  

---

## 6. Tes nurani (wajib sebelum klaim “full engine OK”)

- [ ] Sentuh `DISABLE` saat ada mutasi aktif → restore, tidak nulis lagi  
- [ ] `SAFE_MODE` → perilaku sama / lebih ketat  
- [ ] Restart daemon dengan journal pending → recovery, bukan state kotor  
- [ ] Thermal tinggi → ReduceIntervention / tidak ada mutasi agresif  

---

## 7. Hypotesis debug (satu per sesi)

Contoh yang baik:
- “uClamp tidak terdeteksi” → hanya `src/observe/discovery.cpp`  
- “kandidat tidak pernah eligible” → `effect_model` + context gates  
- “mutasi jalan tapi tidak restore” → journal / actuator restore  
- “UI bilang holding selamanya” → outcome window + cooldown di `autonomous.cpp`  

Jangan ubah config + kode + thermal threshold dalam satu percobaan.

---

## Definisi sukses debug (developer)

Engine boleh disebut siap evaluasi penuh jika:

1. Inventory capability-driven benar di perangkat nyata  
2. Minimal satu resource epoch: propose → journal → write → verify → observe → outcome → restore  
3. Safety path (DISABLE / thermal / SAFE_MODE) membosankan-andalan  
4. CPU path terbukti terpisah dan opt-in  

Bukan: “banyak yang berubah di sysfs”.  
Melainkan: **setiap perubahan dijelaskan, diukur, dan bisa dikembalikan.**
