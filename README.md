# CoreFlow Autonomous Engine

**Observe. Understand. Balance. Adapt.**

Capability-driven native C++ engine for Android. Discovers what the device actually exposes, evaluates candidates with semantic evidence and runtime context, and keeps every mutation behind explicit policy and safety authority.

Not a hard-coded list of tunables. Not aggressive one-size-fits-all tuning.

| | |
|---|---|
| Version | `v2.4.0` (`versionCode` 2400) |
| Language | C++20 |
| ABI | `arm64-v8a` |
| Min API | 34 |
| Default mode | Observe-only |

---

## Principles

1. **Discovery ≠ authority.** A writable interface is only a candidate. Permission comes from policy, context, and a valid mutation permit.
2. **Evidence over assumption.** EffectModel ranks candidates from domain priors, vendor configuration, and measured runtime state — not from path presence alone.
3. **Bounded mutation.** Journal before write, verify after, roll back on failure. Corrupt or missing journal fails closed.
4. **Safe defaults.** Ship observe-only. Adaptive mode and CPU governor changes require explicit operator opt-in after device validation.

```ini
mutation_mode=observe
mutation_armed=false
# Legacy allow_cpu_governor is ignored.
```

---

## v2.4.0 hardening

- **Journal v2** — checksummed records (CRC32), unique exclusive temp file, bounds (256 entries / 4 KiB field / 1 MiB file). Rejects symlinks, non-regular files, truncation, checksum mismatch, embedded NUL, duplicate paths. v1 journals remain readable for upgrade recovery.
- **Actuator write path** — single no-follow descriptor, `fstat` revalidation, requires procfs / sysfs / cgroup semantics, live read-back verification.
- **Baseline capture** — fails closed when recovery journal is not configured.
- ADR 0004 records the journal contract. ADR 0003 (system-property overlays) remains Proposed and is not shipped.

---

## Architecture

| Pillar | Role | Responsibility |
|--------|------|----------------|
| `src/observe/` | Eyes | Mount-derived discovery, capability registry, sampling |
| `src/understand/` | Reasoning | Context, EffectModel, decision agent, thermal, baseline |
| `src/balance/` | Brain | Policy, safety hold, consultation (Allow / Defer / Deny) |
| `src/mutation/` | Hands | Actuators and controllers — write only with a permit |
| `src/conscience/` | Memory | Mutation journal, retained state, decision trace |
| `src/runtime/` | Lifecycle | Config, paths, status export, daemon entry |
| `src/autonomous/` | Loop | Tick orchestration |

Headers under `include/coreflow/`. Sources discovered dynamically (`cmake/CoreFlowSources.cmake`); platform membership is per-file via `@coreflow-platform` or Android header inference.

---

## What it covers

- Mount-derived tunable roots (procfs / sysfs / cgroup) with bounded, non-symlink walks
- VM sysctl, block-queue, CPUFreq, devfreq, uclamp, and related surfaces when capability + policy allow
- ONNX thermal prediction in the packaged ARM64 build
- Persistent experience memory scoped to device/kernel identity
- KernelSU WebUI (Material 3 Expressive) that controls the daemon: start, stop, restart, re-discover, the full runtime policy, and the decision journal. Kernel writes stay with the engine; the UI edits config and sends signals only.

Details: [`docs/CAPABILITY_DRIVEN.md`](docs/CAPABILITY_DRIVEN.md) · [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)

---

## Build

**Host tests**

```sh
cmake -S . -B /tmp/coreflow-build -DCORE_FLOW_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/coreflow-build -j$(nproc)
ctest --test-dir /tmp/coreflow-build --output-on-failure
bash tools/verify_safety_foundation.sh
bash tools/verify_source_release.sh
```

**Android ARM64** — GitHub Actions workflow (`build.yml`) produces the installable module ZIP and daemon binary. This source archive does not ship `coreflowd` or `libonnxruntime.so`.

Keep ONNX Runtime SDK/cache and build trees outside the source tree.

---

## Validation (v2.4.0)

| Gate | Result |
|------|--------|
| Host CMake + deterministic tests | Passed |
| CTest (hardening, safety, WebUI, release contract) | Passed |
| ASan + UBSan | Passed |
| Android 14+ ARM64 production build | Passed |

CI success validates the build pipeline and the journal/actuator hardening. It does **not** certify behavior on every device. Installation, soak, recovery, and KernelSU/Magisk path testing remain operator gates before Adaptive mode.

See [`docs/BUILD_STATUS.md`](docs/BUILD_STATUS.md) and [`docs/DEVICE_VALIDATION.md`](docs/DEVICE_VALIDATION.md).

---

## Install (operator)

1. Use the **CI-produced release ZIP**, not this source archive.
2. Install via KernelSU / Magisk.
3. Leave defaults at observe-only until recovery checks pass on the target device.
4. Enable Adaptive only deliberately; all supported non-fatal resource namespaces follow the global Adaptive/armed authority.

---

## Docs map

| Doc | Purpose |
|-----|---------|
| [`CHANGELOG.md`](CHANGELOG.md) | Release history |
| [`docs/SOURCE_MAP.md`](docs/SOURCE_MAP.md) | What each file is for |
| [`docs/CAPABILITY_DRIVEN.md`](docs/CAPABILITY_DRIVEN.md) | Discovery and eligibility rules |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Authority boundaries |
| [`docs/BUILD_STATUS.md`](docs/BUILD_STATUS.md) | CI and validation status |
| [`docs/DEVICE_VALIDATION.md`](docs/DEVICE_VALIDATION.md) | On-device checklist |
| [`docs/adr/`](docs/adr/) | Architecture decisions (0002 packaging, 0004 journal) |

---

## License

See [`LICENSE`](LICENSE).
