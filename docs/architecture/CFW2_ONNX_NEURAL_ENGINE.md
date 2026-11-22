# CFAE System Framework — Hybrid CFW2 + ONNX Neural Engine Architecture

Status: **Phase A Implemented & Verified (`feat/v6.1-onnx-cfw2-neural-engine`, PR #49)**  
Target Runtime Budgets (`coreflowd`): **`< 0.3%` idle CPU · `< 3 MB` RSS · `< 16 KB` model footprint · `< 15 µs/tick` inference**

---

## 1. Engineering Principle & Safety Invariant

- **Zero Mandatory Heavyweight Runtime:** Standard `CFAE-System-Framework` ZIP packages **never** bundle `libonnxruntime.so` (~10–18 MB).
- **Advisory-Only Outputs:** Neural predictions (`predicted_pressure_10s`, `jank_risk`, `recommended_scale`, `workload_class`) are strictly bounded advisory signals. They can **never** bypass OEM thermal ceilings, `BaselineStore (CFJ4)` write allowlists, `PowerLongevity` SOC floors, or `Sovereign Tamper Guard`.
- **Root Trust Boundary:** All model files (`.onnx`, `.cfw2`, `.cfw`) and optional `libonnxruntime.so` paths are strictly absolute, opened with `O_NOFOLLOW`, bounded in size (`<= 16 KB` for `CFW2`/`CFW1`, `<= 16 MiB` for `.onnx`, `<= 64 MiB` for `.so`), non-world-writable, and verified root-owned (`uid == 0`, non-group-writable) when running under root (`geteuid() == 0`). No relative `dlopen("libonnxruntime.so")` lookup is permitted.
- **Three-Tier Deterministic Fallback:**
  1. **Tier 1 (Optional Add-on):** Dynamic ONNX Runtime C-API v1 (`dlopen` on trusted absolute paths + `/data/adb/coreflow/models/equilibrium.onnx`, single-threaded, arena-free, telemetry-off, startup dry-run self-test, and 3-strike runtime failure circuit breaker).
  2. **Tier 2 (Primary Native Engine):** Compact binary **`CFW2`** compiled from PyTorch/ONNX offline training (`1,700 B` on disk, `< 16 KB` RAM, zero heap allocations per tick) + **Online RLS + NLMS residual adaptation** per device.
  3. **Tier 3 (Built-in Fallback):** Embedded physics-calibrated `10→16→12→3` RLS-MLP weights compiled directly into `coreflowd` (plus backward-compatible legacy `CFW1` loader).

---

## 2. Input & Output Feature Contract (10 → 16 → 12 → 3)

### Input Vector `x ∈ ℝ¹⁰` (strictly finite & clamped before inference)
| Index | Feature | Normalization | Range |
|---|---|---|---|
| `0` | `thermal_pressure` | `(p - 60.0) / 40.0` | `[-1.5, 2.0]` |
| `1` | `thermal_velocity_per_s` | `clamp(vel_per_s / 2.0, -2.0, 2.0)` | `[-2.0, 2.0]` |
| `2` | `cpu_psi` | `psi / 50.0` | `[0.0, 2.0]` |
| `3` | `memory_psi` | `psi / 25.0` | `[0.0, 2.0]` |
| `4` | `io_psi` | `psi / 25.0` | `[0.0, 2.0]` |
| `5` | `gpu_busy_pct` | `gpu / 100.0` | `[0.0, 1.0]` |
| `6` | `render_pressure` | `clamp(rp, 0, 1)` | `[0.0, 1.0]` |
| `7` | `jank_ratio` | `clamp(jr, 0, 1)` | `[0.0, 1.0]` |
| `8` | `current_scale` | `(scale - 0.4) / 0.6` | `[0.0, 1.0]` |
| `9` | `charging_power_norm` | `clamp(mw / 15000.0, 0, 1.5)` | `[0.0, 1.5]` |

### Output Vector `y ∈ ℝ³`
| Index | Output | Post-Processing & Safety Clamp | Consumer |
|---|---|---|---|
| `0` | `delta_pressure_10s` | `clamp(p + rls_rate * 6.0 + tanh(clamp(y[0], -6, 6)) * 10.0, 0.0, 130.0)` | Thermal pre-cooling & 10s forecast |
| `1` | `jank_risk` | `sigmoid(clamp(y[1], -8, 8)) ∈ [0.0, 1.0]` | Feed-forward UClamp rescue & App DNA |
| `2` | `recommended_scale` | `0.40 + 0.60 * sigmoid(clamp(y[2], -8, 8)) ∈ [0.40, 1.00]` | Symbiotic headroom advisor |

---

## 3. Binary Format Specification: `CFW2`

Little-endian, fixed-header, CRC32-verified binary file (`<= 16,384 bytes` hard cap, opened with `O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK`):

```text
Offset  Size  Field
------  ----  ----------------------------------------------------------
0       4     magic          = "CFW2" (0x43 0x46 0x57 0x32)
4       2     format_ver     = 2 (uint16)
6       2     flags          = bit0: activation (0=tanh, 1=silu), bits 1..15 must be 0
8       2     in_dim         = 10 (uint16)
10      2     h1_dim         = 16 (uint16)
12      2     h2_dim         = 12 (uint16)
14      2     out_dim        = 3  (uint16)
16      4     payload_crc32  = IEEE CRC32 over bytes [24 .. EOF)
20      4     reserved       = 0 (uint32)
24      ...   float32 tensors in contiguous row-major order:
                W1 [h1_dim * in_dim]   (16 * 10 = 160 floats = 640 B)
                b1 [h1_dim]            (16 floats            =  64 B)
                W2 [h2_dim * h1_dim]   (12 * 16 = 192 floats = 768 B)
                b2 [h2_dim]            (12 floats            =  48 B)
                W3 [out_dim * h2_dim]  (3  * 12 =  36 floats = 144 B)
                b3 [out_dim]           (3  floats            =  12 B)
```

- **Total size for `10→16→12→3` (FP32):** `24 + (160 + 16 + 192 + 48 + 36 + 12) * 4 = 1,700 bytes` (**1.66 KB**).
- **MAC operations per tick:** `160 + 192 + 36 = 388` FMAs → **`< 2 µs`** on ARM64 Cortex-A55/A78.
- **Validation Gate:** Reject file if size > 16 KB, symlink detected, world-writable, non-root-owned (when `uid == 0`), magic/flags/dims mismatch, CRC32 mismatch, or any weight is `NaN`/`Inf`/`|w| > 50.0`.

---

## 4. Phase A Implementation (`coreflowd`)

1. **Compiler & Offline Training Pipeline (`tools/onnx_to_cfw2.py`, `tools/train_cfw2_from_diag.py`):**
   - Extract `Gemm`/`MatMul`+`Add` weights (including `alpha`/`beta` scaling and `SiLU` detection) from `.onnx` MLPs into verified `CFW2` binaries.
   - Fine-tune directly from real-device `coreflowd --diag` JSON captures blended with physics boundary anchors, with best-checkpoint rollback.
2. **Native `CFW2` Micro-Engine (`include/coreflow/neural_predictor.hpp`, `src/kernel/neural_predictor.cpp`):**
   - 10-feature `NeuralInput` & enriched `NeuralPrediction` (`jank_risk`, `workload_class`, `backend`, `cfw2_crc32`).
   - Zero-allocation `CFW2` loader (`O_NOFOLLOW`, CRC32, finite-weight check), O(1) lossless `reset()`, and online RLS + NLMS residual head.
3. **Real Dynamic ONNX Runtime C-API Execution (Optional Add-on):**
   - Full `OrtApi` v1 lifecycle (`CreateEnv`, `CreateSessionOptions` with `intra_op=1`, `inter_op=1`, `DisableCpuMemArena`, `DisableMemPattern`, `DisableTelemetryEvents`, `CreateSession`, startup dry-run self-test, `CreateTensorWithDataAsOrtValue`, `Run`, clean teardown in destructor).
   - Strict NaN/bounds guard, automatic per-tick fallback, and 3-strike circuit breaker closing broken sessions cleanly.
4. **Daemon & Telemetry Integration (`src/daemon/main.cpp`, `tests/mock_onnxruntime.cpp`, `tests/test_neural_predictor.cpp`, `tests/test_cli_lifecycle.py`):**
   - Expose `aiJankRisk`, `aiWorkloadClass`, and `aiBackend` (`onnx-runtime-live` | `cfw2-micro-mlp` | `custom-cfw-rls-mlp` | `embedded-rls-mlp`) in `--status` / `--diag`.
   - Unit tests under Host C++, ASan+UBSan+LSan, and ThreadSanitizer (covering malformed/corrupt/symlink/world-writable `CFW2`, `reset()` preservation, NaN input rejection, mock ORT C-API shared library, startup dry-run rejection, and 3-strike circuit breaker with 0 leaked handles).

---

## 5. Phase B Handoff (Companion App On-Demand ONNX Runtime)

Planned for the follow-up PR after Phase A merges:
- Add `com.microsoft.onnxruntime:onnxruntime-android` strictly inside `android/` (zero impact on `coreflowd` daemon).
- **Lifecycle Discipline:** Lazy-load `OrtSession` only when the user opens `CFAE TELEMETRY LINK` or runs Session Anomaly Analysis; deterministically `close()` on `Lifecycle.Event.ON_STOP` so background RAM overhead is `0 MB`.
- Use compact INT8 intent/anomaly `.onnx` models to augment [`CoreFlowDialogueEngine.kt`](../../android/src/main/kotlin/io/coreflow/CoreFlowDialogueEngine.kt) with free-form Indonesian/English NLU while keeping deterministic telemetry grounding authoritative.
