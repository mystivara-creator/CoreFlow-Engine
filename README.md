# CoreFlow Autonomous Engine v2.1.0


## WebUI Command Center

The installable module includes a mobile-first KernelSU WebUI in `module/webroot/` with Overview, Mode & Policy, Diagnostics, Shell Console, and Project & AI pages. Observe remains interactive for log inspection and discovery refresh without arming mutation. Adaptive can be explicitly enabled from the UI after confirmation; CPU governor mutation requires a separate opt-in. The UI writes only the runtime configuration and sends a discovery-refresh signal—it does not write kernel tunables directly. CoreFlow's existing capability/policy gates, mutation journal, verification, rollback, thermal guards, and safety holds remain authoritative. See [`module/webroot/README.txt`](module/webroot/README.txt).

The About page acknowledges Mystivara as the author and Gemini, ChatGPT, Grok AI, Claude, and GitHub Copilot as assistants used during selected development tasks. These acknowledgements do not imply endorsement or an official partnership.

---


## KernelSU / KernelSU Next update notifications

`module/module.prop` points `updateJson` to the latest GitHub Release asset at `releases/latest/download/update.json`. The `KernelSU Update Feed` workflow attaches a version-specific JSON file when a release is published. That JSON carries the packaged `version`, increasing integer `versionCode`, exact release ZIP URL, and that release's changelog URL. It reads version metadata from the ZIP itself rather than guessing from a moving branch.

For each release, attach the CI-produced ZIP named `CoreFlow-Autonomous-vX.Y.Z-minapi34-arm64.zip` before publishing the GitHub Release. The workflow then uploads `update.json` to that release. For an already-published release, run **Actions → KernelSU Update Feed → Run workflow** with the tag, or leave it blank to sync the latest release. The workflow must be present on the repository's default branch and GitHub Actions must allow release asset writes. Installed clients only offer an update when the published `versionCode` is greater than the installed module's code.

## v2.1.0 — Capability-Driven Autonomous Decision Loop

This source continuation adds an explicit policy authorization gate to the capability-driven resource path. A readable or writable interface is only a discovery fact; model inference does not grant permission. The live WebUI now shows the device/SoC/kernel identity and separates discovered capabilities from policy-ready resources. Unknown controls remain observe-only. Following the storage regression reported during field use, new writes to block queue nodes (`scheduler`, `nr_requests`, `read_ahead_kb`) are quarantined at discovery, baseline capture, candidate selection and the actuator, regardless of measured I/O activity. The production `ResourceMutationController` and central resource actuator are limited to seven exact policy-approved VM paths with semantic priors; other resource paths are denied for new writes. CPUFreq is a separate authority path and remains off by default (`allow_cpu_governor=no`). Existing journal restore remains available for recovery from older changes. The package remains v2.1.0 source and is not certified for Android/device behavior until operator CI and staged testing pass.

The source-release validator is versioned with the package, and the host CTest suite includes the source-release contract. This does not certify Android ARM64 or device-level behavior; those remain part of the operator's build and staged device validation.

**Fully autonomous, capability-driven adaptive intelligence for Android.**

The engine discovers writable kernel interfaces on the device, scores semantically described candidates with an EffectModel (thermal / memory / workload / I/O context), and applies bounded verified mutations only when policy and confidence gates pass. Experience is advisory evidence that adjusts candidate utility; it never replaces fresh telemetry or safety authorization. Mutations are journaled and rollback is verified.

Release default remains **observe-only**. Opt in with `mutation_mode=adaptive` + `mutation_armed=true`.

---

## v2.0.1 — Release Hardening

v2.0.1 is the release-hardened revision of v2.0.0. It changes the default to **observe-only**, makes the thermal model inputs fail-safe, unifies thermal plausibility bounds, adds hottest-zone confirmation to ThermalGuard, and adds regression tests. See `CHANGELOG.md`.

## v2.0.0 — Ecosystem Autonomous Control

Autonomous adaptive engine with device-aware capability discovery, workload/power intelligence, persistent scoped experience, centralized mutation authorization, causal baseline evaluation, bounded CPUFreq/approved VM mutation and durable recovery. Block-device queue writes are currently quarantined pending device-topology validation.

### Autonomous resource coverage
- Read-only discovery of block-device queue controls (`/sys/block/*`); control writes remain quarantined in the current source policy.
- VM swappiness discovery and adaptive intervention under real memory pressure.
- Historical I/O queue/read-ahead effect priors retained for future evaluation; the current production discovery path does not authorize writes to those nodes.
- CPUFreq governor adaptation with bounded, verified experiments.
- Runtime I/O-rate and throttled process profiling for workload classification.
- Battery level/status/current/voltage/temperature awareness and power-constrained policy.
- Persistent, atomically-written ExperienceMemory scoped to device/kernel identity and capped at 128 records.
- Resource baseline, durable recovery journal, read-back verification and rollback.
- Writable resources are policy-eligible only after runtime discovery; the final mutation authority remains `MutationPermit`.

### v2.0.1 — Release Hardened Adaptive Engine

CoreFlow Autonomous Engine is a native C++20 adaptive system-intelligence daemon for Android.

The source package consolidates environment/capability discovery, unified resource state, context classification, policy planning, centralized mutation authority, CPUFreq and generic-resource actuators, causal outcome evaluation and durable recovery.

> **Package type:** Source release  
> **Version:** `v2.0.1`  
> **versionCode:** `2001`  
> **ABI:** `arm64-v8a`  
> **Android minimum API:** `34`  
> **Build SDK:** `36`  
> **NDK:** `30.0.16248370`  
> **ONNX Runtime Android:** `1.24.3`

## Release position

v2.0.1 is a **release-candidate source package**. Host tests and source/safety contracts run in CI. The Android ARM64 artifact is produced by GitHub Actions; the source archive intentionally contains no generated binary. Device-level validation has not been performed (see *Scope and limitations*).

The package contains the source, tests, module scaffolding, thermal model, CI workflow and validation tooling required to produce the Android ARM64 artifact.

Production binaries and the ONNX Runtime shared library are intentionally produced during the build process rather than shipped in the source package.

## Architecture

```text
Android / Kernel Environment
          │
          ▼
┌──────────────────────────────┐
│ Environment Discovery        │
│ Android / kernel / cgroup    │
│ CPUFreq / UClamp / CPUSet    │
│ Memory / I/O / GPU / thermal │
│ Charging / power / runtime   │
└──────────────┬───────────────┘
               ▼
┌──────────────────────────────┐
│ Capability Matrix             │
│ existence / read / write     │
│ permission / runtime verify  │
│ mutation readiness           │
└──────────────┬───────────────┘
               ▼
┌──────────────────────────────┐
│ Unified Resource State       │
│ observed → baseline →        │
│ desired → verified           │
│ constraints / eligibility    │
└──────────────┬───────────────┘
               ▼
┌──────────────────────────────┐
│ Context Engine               │
│ workload / thermal / memory  │
│ I/O / battery / power        │
│ process-profile / confidence │
└──────────────┬───────────────┘
               ▼
┌──────────────────────────────┐
│ Policy Engine                │
│ plan / candidate / risk     │
│ no final mutation authority  │
└──────────────┬───────────────┘
               ▼
┌──────────────────────────────┐
│ Actuator Manager             │
│ registration / discovery     │
│ CPUFreq adapter boundary     │
└──────────────┬───────────────┘
               ▼
┌──────────────────────────────┐
│ Mutation Safety Foundation   │
│ permit / journal / verify    │
│ rollback / safety hold       │
└──────────────────────────────┘
```

## v1.9 foundation components

### Environment & Capability Intelligence

The discovery layer records device identity, Android/kernel environment and exposed interfaces, including existence, readability, writability and runtime-read evidence. The separate `policy_authorized` flag must also be true before a capability can become mutation-ready.

A writable path is **not** treated as permission to mutate it. In the current v2.1.0 safety follow-up, production policy authorizes only named VM controls with semantic priors. Block queue tunables are discovery-only, and a central actuator guard denies new queue writes even if another path proposes them.

The resource vocabulary covers:

- CPUFreq and governors
- UClamp
- CPU sets / cgroups
- Scheduler controls
- Memory / VM tunables
- I/O queue controls (observed; new writes quarantined pending topology validation)
- GPU / devfreq
- Thermal resources
- Charging / power
- Android runtime and device environment

### ResourceStateModel

`ResourceStateModel` provides a unified state representation:

```text
observed → baseline → desired → verified
```

It also records resource constraints and can revoke mutation eligibility.

The resource model cannot manufacture the final mutation permit.

### ContextEngine

`ContextEngine` classifies runtime conditions including:

- idle
- interactive
- CPU-bound
- GPU-bound
- I/O-bound
- memory-bound
- sustained
- thermal-limited
- power-constrained

It produces contextual eligibility information; it does not directly write kernel state.

### PolicyEngine

`PolicyEngine` converts system context and verified resource state into a plan/candidate representation.

The v1.9 policy layer remains conservative and does not automatically grant mutation authority to newly discovered ecosystem resources.

### ActuatorManager

`ActuatorManager` provides the common registration, discovery and capability boundary for actuators.

CPUFreq remains the existing mutation adapter and remains behind the established safety controller.

### DecisionOutcome

`DecisionOutcome` and `OutcomeClass` represent the result of a decision/action cycle, including:

- neutral
- beneficial
- regressed
- safety blocked
- failed
- rolled back

This representation is intended to support future experience/learning layers without bypassing the safety authority.

### BaselineIntelligence and ExperienceMemory

v2.0.0 makes these components part of the stable autonomous loop. Verified mutation outcomes are evaluated against the factory/runtime baseline and stored as advisory experience. Experience is persisted atomically, capped at 128 records, and scoped to the device/kernel identity so one device cannot inherit another device's tuning history. Historical experience can influence candidate ranking only; it can never grant mutation permission or bypass fresh safety checks.

## Safety model

The release default is **observe-only**:

```text
mutation_mode=observe
allow_cpu_governor=no
mutation_armed=false
```

No kernel control is written until an operator explicitly opts in by setting `mutation_mode=adaptive`, `mutation_armed=true` and `allow_cpu_governor=yes`. Even after opt-in, only policy-allow-listed, preflight-verified resources can receive a scoped mutation permit, and safety holds, journaling, bounded writes, verification, rollback, and ownership checks remain mandatory.

Autonomous mutation requires the established safety path, including:

1. Adaptive mutation mode
2. Explicit mutation arming
3. Explicit CPU governor permission where applicable
4. Durable mutation journal
5. `MutationPermit` from the mutation controller
6. Actuator capability verification
7. Bounded mutation
8. Read-back verification
9. Restore/rollback handling
10. Safety-hold and recovery gates

`MutationAuthority` is the single final authorization gate; CPUFreq and generic-resource controllers cannot manufacture permits themselves.

A journal entry by itself does not imply that a live mutation exists; mutation state is derived from actuator evidence.

## v2.0.1 release-hardening changes

- Default configuration changed to observe-only (`mutation_mode=observe`, `allow_cpu_governor=no`, `mutation_armed=false`). Autonomous mutation is an explicit opt-in.
- A malformed `mutation_mode` now fails safe regardless of key order in the config file.
- Thermal plausibility bounds (10–120 °C) are defined once in `thermal_limits.hpp` and shared by the observer and engine validation.
- The thermal model feature vector is built by a pure, host-tested module (`thermal_features.cpp`). Missing telemetry is never encoded as `0`; an incomplete vector skips the model and uses the bounded heuristic.
- ThermalGuard: a trigger that comes only from the hottest policy-eligible zone must persist for two consecutive samples. Representative and predicted triggers still act immediately; the rising-near-threshold trigger uses the representative sensor only.
- The 18-feature order is cross-checked between the Python training script and the C++ header by `tools/verify_source_release.sh`.

## v2.0.0 production changes

The v2.0.0 release included:

- Multi-policy rollback bookkeeping fix so one rollback cannot clear another policy's dirty state.
- Journal clearing only when no policy remains changed.
- Reduced `CONTEXT` diagnostic logging to eligibility changes and every 60 samples.
- Default configuration documentation alignment.
- Regression coverage for multi-policy mutation and full restore.
- Preservation of the v1.4.1 Tier-2 safety boundary.
- Persistent scoped ExperienceMemory across reboot with atomic replacement and bounded storage.
- Workload classification expanded with I/O-bound, memory-bound and power-constrained states.
- Runtime I/O throughput telemetry and throttled top-process profiling for workload context; profiling is observational only.
- Battery level/status telemetry incorporated into power headroom and autonomous eligibility.
- Experience is now fed back into CPU candidate scoring while remaining advisory and safety-neutral.

## Default configuration

`module/system/etc/coreflow/default.conf`:

```text
monitor_interval=5
min_confidence=0.70
mutation_mode=observe
allow_cpu_governor=no
runtime_refresh=true
mutation_armed=false
```

See [Enabling adaptive mode](#enabling-adaptive-mode-community-testing) for the full procedure, rollback and safety controls.


## Enabling adaptive mode (community testing)

CoreFlow ships in **observe-only** mode. In this mode the daemon samples the device, builds plans and writes logs, but it does not write any kernel control. Start here and move to Adaptive only after the Observe logs look healthy. Adaptive remains limited to the resource classes explicitly approved by the source policy; for this source continuation, block-device queue controls are quarantined in every mode.

### 1. Observe first

1. Install the module with Magisk or KernelSU and reboot.
2. Check that the daemon started:

   ```sh
   su -c 'tail -n 50 /data/adb/coreflow/logs/coreflowd.log'
   su -c 'logcat -d -s CoreFlowAutonomous CoreFlowThermalML | tail -n 50'
   ```

   The startup line `CORE v2.0.1 | mode=DISABLED ...` confirms observe-only operation. Look for `THERMAL_ML status=...`, `CONTEXT`, and any `SAFETY_HOLD` or `THERMAL_GUARD_TRIGGER` lines.
3. Use the device normally for a day or two. Mutation stays off, so this period only shows whether sampling and thermal reads are stable on your hardware.

### 2. Opt in

Edit `/data/adb/coreflow/config.ini` (root shell, for example `su`) and set:

```text
mutation_mode=adaptive
allow_cpu_governor=yes
mutation_armed=true
```

`mutation_mode=adaptive` and `mutation_armed=true` are both required before any mutation can occur. `mutation_mode` selects the adaptive engine and `mutation_armed` is the explicit arming switch. `allow_cpu_governor` controls only CPU governor changes; set it to `no` to keep CPU governors untouched during the first test. An unknown `mutation_mode` value disables mutation.

The configuration is read when the daemon starts, so **reboot after editing it**. Editing the file while the device is running has no effect. Even when Adaptive is explicitly armed, the current source policy does not authorize block-device queue writes.

### 3. Verify

After the reboot, the startup line should read `mode=ADAPTIVE`. Mutations are logged with their result. Each change is written to the mutation journal first, read back, and rolled back when the device leaves the protected state.

### Stop or roll back

- **Immediate stop (kill switch):** `su -c 'touch /data/adb/coreflow/DISABLE'`. The daemon logs `SAFETY_HOLD`, switches to observe-only, and restores the values it changed through the normal relax path. This takes effect while the device is running. Restores happen gradually, not instantly. To resume, remove the file: `su -c 'rm /data/adb/coreflow/DISABLE'`.
- **Return to observe-only:** set `mutation_mode=observe`, `mutation_armed=false` and `allow_cpu_governor=no`, then reboot.
- **Uninstall:** removing the module stops the daemon with SIGTERM so it can restore the factory baseline. The journal and logs are kept on purpose; they are recovered on the next start.

### Automatic safe mode

CoreFlow protects against boot loops and crash loops:

- Three consecutive boots that never reach `boot_completed` create `/data/adb/coreflow/SAFE_MODE`. The daemon then stays observe-only.
- Five consecutive failed boots make the module disable itself, using Magisk's `disable` marker. Re-enable it in the Magisk or KernelSU manager after investigating.
- Repeated daemon crashes also create `SAFE_MODE`.

**`SAFE_MODE` is not cleared automatically.** After you have read the logs and resolved the cause, remove it: `su -c 'rm /data/adb/coreflow/SAFE_MODE'`. Until you do, the daemon stays observe-only even after a successful boot.

### What to report

When you open an issue, include:

- the CoreFlow version (`v2.1.0`), device model, Android version and kernel (`uname -r`),
- the relevant lines of `/data/adb/coreflow/logs/coreflowd.log` and `logcat`,
- your `config.ini` with serial numbers and other identifiers removed (see `SECURITY.md`).

All logs and state stay on the device. CoreFlow does not send any data off the device.

## Thermal predictor

The thermal model is a **synthetic-data baseline**. Its training script generates trajectories when no real traces are supplied, so its holdout metrics describe that generator and not a specific device. It is not device-validated. When the model cannot be used (incomplete telemetry, too little history, unknown trend, load failure, or prediction outside the safe deviation), the daemon uses the bounded heuristic. ONNX retraining on per-device traces is a planned follow-up.

The source package includes:

```text
module/system/etc/coreflow/thermal_predictor.onnx
```

Pinned SHA-256:

```text
a605b046086f2f30e0a625c64547de5399e407887d69461d3c5642f7344f0703
```

The CI workflow verifies the model digest before it is packaged into the module.

### Per-device accuracy

The bundled model uses the same 18 inputs on every device, so it always runs as a generic predictor. It does not learn your device on its own. Predictions are accurate only as far as the bundled synthetic baseline matches your hardware. When the inputs are incomplete, or the prediction is far from the live sensor, the daemon uses the bounded heuristic instead.

The training script can build a per-device model from real traces (`--data`, a CSV with the 18 feature columns and `target_thermal_c`). The daemon does not record these traces yet. Collecting them on device, with explicit opt-in and stored locally, is the planned next step. Until a trace-based model is released with its own digest, the bundled model remains the one in use.

## Source package contents

### Source

```text
include/coreflow/
src/
```

### Tests

```text
tests/architecture_tests.cpp
tests/mutation_tests.cpp
```

### Module

```text
module/
module/system/etc/coreflow/default.conf
module/system/etc/coreflow/thermal_predictor.onnx
```

### Build and validation

```text
.github/workflows/build.yml
tools/build_android.sh
tools/verify_safety_foundation.sh
tools/verify_source_release.sh
```

## Build — host tests

Requirements include CMake, Ninja and a C++20 compiler.

```bash
cmake -S . -B build/host -G Ninja   -DCORE_FLOW_BUILD_TESTS=ON   -DCMAKE_BUILD_TYPE=Debug

cmake --build build/host
ctest --test-dir build/host --output-on-failure --no-tests=error
```

## Build — Android ARM64

The production workflow uses:

```text
NDK:              30.0.16248370
Android SDK:      36
Minimum API:      34
ABI:              arm64-v8a
ONNX Runtime:     1.24.3
```

The workflow:

1. Verifies release version alignment.
2. Installs Java 17 and the Android SDK/NDK.
3. Validates the Android build contract.
4. Verifies the source-package contract.
5. Fetches and validates ONNX Runtime Android.
6. Verifies the thermal model.
7. Configures a Release Android build.
8. Builds and strips `coreflowd`.
9. Assembles the module.
10. Verifies the packaged deployment.
11. Produces the Android ARM64 ZIP artifact.

## Source-package validation

Run:

```bash
tools/verify_source_release.sh
```

The validator checks release metadata, required source files, absence of generated production artifacts, safe-autonomous defaults, architecture components, documentation versioning and the pinned thermal-model digest.

Run the safety contract validation with:

```bash
tools/verify_safety_foundation.sh
```

## Production status

This source package does not contain:

- `coreflowd`
- `libonnxruntime.so`
- extracted ONNX Runtime SDK/cache
- generated build directories
- CI production artifacts

Therefore the source package itself is **not directly installable**.

A production Android artifact must be generated by the Android build workflow before installation.

## Scope and limitations

v2.0.1 is a release-candidate source package. Host deterministic tests, the source-release contract and the safety-foundation contract pass. The Android ARM64 build and on-device behavior have not been validated in the source-package environment.

The package does not claim validation across every Android device, vendor kernel, hardware implementation or kernel-control layout.

Device-specific policy validation, a real-device thermal trace set, and ONNX retraining remain required before enabling adaptive mutation on a given device.

## License

See `LICENSE`.


## Structured Decision Trace

The daemon writes bounded, line-delimited JSON decision records to
`/data/adb/coreflow/decision_trace.jsonl`. Each record captures the evaluated
runtime state, policy decision, confidence, workload context, mutation
eligibility, candidate count, safety-hold reason, effective mode/arming flags,
and the actual CPU/resource mutation result returned in that cycle.

The journal is observational only: trace-write failures are non-fatal and do
not grant mutation authority or change safety gates. The current journal is
rotated at approximately 512 KiB to a single `.1` file. The WebUI reads only
the most recent 200 records through its allowlisted diagnostics path. A missing
journal means no structured records are currently readable; it is not proof
that no decisions occurred.
