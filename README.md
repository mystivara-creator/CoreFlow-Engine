# CoreFlow Autonomous Engine

### v1.3.0 — Source Release

CoreFlow Autonomous Engine is the native adaptive system-intelligence daemon for Android (Magisk module).

It observes runtime conditions, evaluates system state, makes confidence-gated adaptive decisions, applies **bounded** CPUFreq governor changes, verifies every write, and restores a known factory baseline when required. Thermal prediction is powered by a pinned ONNX model.

> **Package type:** Source release  
> **Version:** `v1.3.0` (CMake / runtime: `1.3.0`)  
> **Binary:** Produced by CI (`.github/workflows/build.yml`) or `tools/build_android.sh`  
> **Placeholders:** `module/system/bin/coreflowd` and `third_party/jni/arm64-v8a/libonnxruntime.so` are intentional 1-byte stubs. Real artifacts come from the Android NDK + ONNX Runtime build path.

---

## Overview

The v1.3.0 release integrates:

- Runtime observation (memory, CPU utilisation, load, thermal, charging)
- Runtime state evaluation with hysteresis
- Confidence-based decision gating
- CPUFreq policy discovery and capability filtering
- Adaptive governor selection (only governors advertised by the policy)
- Bounded, verified mutation with durable journal
- Baseline restoration / rollback
- Baseline Intelligence efficiency evaluation
- Experience memory and rejection cooldown
- ONNX thermal predictor
- Safe single-instance locking and Magisk supervisor

Mutation is **fail-closed by default**. Enable adaptive mode only after device-specific validation.

---

## Architecture

```text
┌──────────────────────────────┐
│     Runtime Observation      │
│ memory / CPU / thermal /     │
│ charging / runtime signals   │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│      State Evaluation        │
│ IDLE / NORMAL / WARMING /    │
│ ELEVATED / PRESSURE /        │
│ THERMAL_GUARD                │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│     Adaptive Decision        │
│ workload + thermal pressure  │
│ + confidence + capabilities  │
│ + hysteresis + experience    │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│     Bounded Mutation         │
│ supported CPUFreq governor   │
│ journal committed before     │
│ first sysfs write            │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│      Verification            │
│ write → read-back → verify   │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│   Restore / Rollback         │
│ factory baseline when needed │
└──────────────────────────────┘
```

---

## Safety Invariants

1. Factory baseline is captured only while governors are known to be unmutated.
2. Durable journal is committed and fsynced **before** the first sysfs write.
3. Every write is read back and verified. A failed restore blocks new mutations.
4. Corrupt or untrusted journal keeps mutation disabled (fail-closed).
5. Only governors advertised by the discovered CPUFreq policy may be written.
6. Regressed candidates are suppressed for a bounded cooldown.
7. Unknown `mutation_mode` values fall back to Disabled.
8. Single-instance lock prevents concurrent daemons.
9. Shutdown always attempts restore; incomplete restore preserves the journal for the next start.

---

## Adaptive Governor Selection

The adaptive path is not hardcoded to a single governor.

Flow:

```text
CPUFreq Policy Discovery
        ↓
Available Governors
        ↓
Capability Filter
        ↓
Adaptive Evaluation + Runtime-State Bias
        ↓
Hysteresis + Experience Memory
        ↓
Selected Governor
        ↓
Apply → Read-back Verification
```

Typical candidates (only if advertised by the policy):

- `powersave`
- `conservative`
- `schedutil`
- `walt`
- `performance`

This is a bounded heuristic system. It does not claim universal optimality for every device or workload.

---

## Runtime States

```text
IDLE
NORMAL
WARMING
ELEVATED
PRESSURE
THERMAL_GUARD
```

---

## Configuration

Default production-safe configuration (`module/system/etc/coreflow/default.conf`):

```ini
# Mutation remains disabled until a device-specific policy has been validated.
monitor_interval=5
min_confidence=0.70
mutation_mode=disabled
allow_cpu_governor=yes
runtime_refresh=true
```

Runtime override path:

```text
/data/adb/coreflow/config.ini
```

| Key | Meaning | Safe default |
|-----|---------|--------------|
| `monitor_interval` | Seconds between ticks (1–60) | `5` |
| `min_confidence` | Minimum confidence to allow mutation (0.50–1.0) | `0.70` |
| `mutation_mode` | `disabled` / `observe` / `adaptive` | `disabled` |
| `allow_cpu_governor` | Permit CPU governor writes | `yes` |
| `runtime_refresh` | Honour SIGUSR1 rediscovery | `true` |

To enable adaptive mutation after validation on a specific device, set:

```ini
mutation_mode=adaptive
```

---

## Building the Android Binary

Requirements:

- Android NDK `27.3.13750724`
- Android API `35` / ABI `arm64-v8a`
- CMake ≥ 3.23 + Ninja
- An externally extracted ONNX Runtime Android `1.24.3` package

```bash
# Using the helper script
export ANDROID_NDK_HOME=/path/to/android-ndk-27.3.13750724
export COREFLOW_ONNX_ROOT=/path/to/onnxruntime-android-1.24.3
./tools/build_android.sh

# Or let GitHub Actions fetch the pinned ONNX Runtime and produce the
# ARM64 ELF + libonnxruntime.so production module.
```

Source-release integrity check:

```bash
./tools/verify_source_release.sh
```

---

## Magisk Module

After a successful CI/local build, place the real `coreflowd` into `module/system/bin/` and `libonnxruntime.so` into the module library path expected by `service.sh`. The supervisor:

- waits for `sys.boot_completed`
- verifies binary, library and model size
- sets `LD_LIBRARY_PATH` to the module-local ONNX runtime
- runs a bounded-restart supervisor
- recovers any pending mutation journal on start

SELinux rules are supplied in `module/sepolicy.rule`.

---

## Verification Flow

```text
Discover → Capture Baseline → Capability Check
    → (Journal commit) → Apply → Read Back → Verify
    → Keep / Restore
```

A write is never treated as successful merely because the write syscall returned.

---

## Thermal Predictor

- Model: `module/system/etc/coreflow/thermal_predictor.onnx`
- Runtime: ONNX Runtime Android (version pinned in CI)
- Model content hash is pinned in the CI workflow; any change requires deliberate review

---

## Testing

Host deterministic tests (no NDK required):

```bash
cmake -S . -B build/host -DCORE_FLOW_BUILD_TESTS=ON
cmake --build build/host
ctest --test-dir build/host
```

CI also performs a strict Android ARM64 Release build with the full warning set treated as errors.

---

## Release Status

**v1.3.0** is a source release of the Autonomous + Baseline Intelligence + ONNX thermal stack.

- Binary artifacts are produced by CI, not shipped inside this ZIP.
- Mutation defaults to **disabled** for safety.
- Device-specific validation is required before enabling `mutation_mode=adaptive`.

This does **not** claim:

- zero bugs on every vendor kernel
- universal Android compatibility
- guaranteed battery or performance gains on every SoC

---

## License

MIT License — Copyright (c) 2026 Mystivara

See [LICENSE](LICENSE).

---

## Security

See [SECURITY.md](SECURITY.md) for the supported security model and reporting process.
