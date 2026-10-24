# CoreFlow Autonomous Engine v3.1.0

**Capability-driven, outcome-driven Android kernel adaptation**
**Implementation:** C++20 · **Target ABI:** `arm64-v8a` · **Module format:** Magisk / KernelSU-compatible ZIP

CoreFlow Autonomous Engine is a native Android daemon that discovers kernel controls exposed by the running device, evaluates candidate adjustments, applies validated changes, measures observable system outcomes, and retains or rolls back each change according to the result. The implementation is designed around device capabilities rather than a fixed device profile or a universal tuning preset.

> CoreFlow does not promise absolute stability or performance gains on every device. Kernel interfaces and their effects vary by OEM, SoC, kernel version, memory configuration, storage controller, workload, and root/SELinux environment. A successful host test cannot establish that a particular kernel control is writable or beneficial on a physical device.

## Design contract

```text
Device & System Discovery
          ↓
Runtime Observation  (CPU/memory/IO PSI, thermal, SurfaceFlinger timestats)
          ↓
Optimization Engine  (workload-scoped proposals, follow-ups after selector changes)
          ↓
Change Justification → Capability & Safety Validation
          ↓
Bounded Mutation + Read-back Verification
          ↓
Outcome Verification (subsystem signals + thermal headroom)
       ↙       ↘
   Retain     Rollback
       ↘       ↙
  Feedback, state export (WebUI), next cycle
```

The daemon does not force a named governor or I/O scheduler. It acts on values and metadata exposed by the active kernel.

## Coordinated follow-up (v3.1.0)

Some kernel controls exist only beneath another control:

| Retained change | Kernel layout that appears | Re-evaluated controls |
|---|---|---|
| `queue/scheduler` → X | `queue/iosched/` for elevator X (absent for `none`) | `iosched/*` tunables, plus same-queue controls such as `nr_requests` |
| `policyN/scaling_governor` → G | `policyN/G/` for governor G | `policyN/G/*` tunables, plus policy-level limits |
| `devfreq ... governor` | governor tunables beside the governor | same-device siblings |

A retained selector change sets a rediscovery request. The runtime runs a fresh inventory before proposing follow-ups, so new directories are seen immediately rather than after the 2-minute refresh. Only controls under the new directory, or same-subsystem siblings, are eligible; a same-named tunable on another queue or policy is not.

The tunable *names* are discovered live. Numeric bounds come from `module/strategies.tsv`, a semantic policy keyed by domain and attribute. A live value outside those bounds is never proposed.

## Thermal guard

Retention is vetoed when a change consumes headroom beyond the device's own noise and leaves the hottest zone inside the hysteresis band the kernel publishes for its **passive trip point** (`trip_point_N_type == passive`, `trip_point_N_hyst`). A rise of the hottest zone beyond its own noise also vetoes retention. Both thresholds are learned from the device; no absolute temperature is assumed.

## Runtime behavior

### Device and kernel discovery

Discovery walks `/sys` and `/proc/sys`, skipping debugfs, tracing, firmware and pstore. A resource is considered writable only when its mode has a write bit and `access()` allows it; root bypasses DAC, so `access()` alone is insufficient. Each resource is classified into a semantic domain (see `include/coreflow/discovery/classification.hpp`). Protected surfaces (thermal, battery/charger, regulator, watchdog, reset, debug, firmware, and similar) are never optimization targets.

Discrete controls use choices advertised by the live kernel. Numeric controls require a range published by the kernel, or an operator-supplied catalogue rule. The shipped build provides no numeric bounds of its own.

### Optimization

`OptimizationEngine` ranks eligible candidates by workload-scoped signals. Numeric probes move in the direction that was actually attempted; a failed probe reverses it. Learning is per workload signature, is bounded in memory, and is not updated by inconclusive outcomes.

### Validation, mutation, and rollback

Before a write, the engine checks capability identity, no-op status, bounds or advertised choices, and the currently observed value. Mutation opens files without following symlinks, verifies object identity, writes, reads back, and rolls back through the same verified path on failure or regression.

### Outcome

Outcome verification compares before/after signals valid on both sides. Every deadband is the device's measured sample-to-sample variation of that signal; a signal without a learned floor is inconclusive. Scheduler/governor probes are judged on their own subsystem: I/O pressure for elevator tunables, CPU pressure and SurfaceFlinger frame timing for CPU governor tunables. Missing signals are never fabricated.

## Operator console (WebUI) and state directory

The WebUI does not need a terminal and does not open a network port. The daemon and the console communicate through `/data/adb/coreflow` (override with `COREFLOW_STATE_DIR`):

| File | Written by | Contents |
|---|---|---|
| `status.json` | daemon, every cycle (atomic replace) | engine state, current pipeline stage, counters, device, system signals |
| `capabilities.json` | daemon, after each inventory | discovered controls, domain, strategy, value, eligibility |
| `journal.jsonl` | daemon, per decision/mutation (rotated at 256 KiB) | decisions, writes, outcomes, rollbacks |
| `command` | WebUI, one token | consumed and deleted by the daemon |

Accepted command tokens are exactly `pause`, `resume`, `rescan`, and `reset_learning`. Anything else is discarded. The WebUI (`module/webroot/index.html`) reads these files through the KernelSU WebUI bridge (`ksu.exec`) and displays the C++ pipeline stage that is running. A status older than the `stale_after_s` the daemon publishes (three worst-case loop intervals) is shown as not reporting, so a stopped daemon is never presented as live.

Pausing keeps observation running but issues no kernel writes.

## C++ architecture

Standard C++20 (the code uses `std::string_view::starts_with` and `std::from_chars` for `double`).

| Component | Responsibility |
|---|---|
| `src/main.cpp` | Process entry point, startup logging, top-level exception boundary. |
| `src/discovery/discovery.cpp` | Inventory, writability, live choice and range discovery, catalogue application. |
| `src/discovery/classification.cpp` | Pure domain classification, protected surfaces, writability bits, pruned subtrees. |
| `src/discovery/strategy_catalog.cpp` | Validates and indexes the TSV policy catalogue. |
| `src/profile/device_profile.cpp` | Reads the versioned device-profile TSV. Does not mutate. |
| `src/observation/observation.cpp` | Samples CPU, memory, PSI, thermal zones and passive-trip headroom. |
| `src/observation/surface_flinger.cpp` | Read-only SurfaceFlinger timestats parsing via `dumpsys`. |
| `src/optimization/optimization.cpp` | Proposals, learning, dependency follow-ups, rediscovery requests. |
| `src/optimization/workload.cpp` | Workload signatures and per-domain scores. |
| `src/decision/decision.cpp` | Justifies a candidate against the observed workload. |
| `src/validation/validation.cpp` | Proposal validity and resource safety before mutation. |
| `src/mutation/mutation.cpp` | Guarded writes, identity and value checks, read-back, rollback. |
| `src/outcome/outcome.cpp` | Before/after comparison per domain, retain/rollback/unresolved. |
| `src/recovery/learning.cpp` | Logcat record of each outcome. |
| `src/runtime/engine.cpp` | Orchestration, stage reporting, command handling. |
| `src/runtime/state_store.cpp` | Command parsing, JSON formatting, atomic state files. |

## Strategy catalogue

`module/strategies.tsv` ships schema version `2` with no rules. Numeric bounds come from the kernel, or from an operator-supplied extension file selected with `COREFLOW_STRATEGY_CATALOG`. Operator rules are validated strictly and never override a kernel-published limit. A value outside a rule's bounds is never mutated.

## Build and tests

### Host

Requirements: CMake 3.22+, a C++20 compiler.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

There are 16 test targets: strategy catalogue, architecture contract, optimization feedback, optimization coordination, mutation safety, device profile, outcome feedback, workload policy, noise-floor estimation, pressure parsing, SurfaceFlinger parsing, thermal policy, thermal headroom, rollback guard, discovery classification, and runtime state store.

### Android ARM64 module

The GitHub Actions workflow builds `coreflow_daemon` with the NDK for `arm64-v8a`, verifies the ELF, strips it, and packages `module/` into `CoreFlow-Engine-Release.zip`.

## Installation and diagnostics

1. Install the module ZIP through a supported root manager and reboot.
2. Open **CoreFlow Console** from the root manager's module WebUI. No terminal is needed.
3. Logs, if needed: `logcat -s CoreFlowEngine`.

## Scope and limitations

- Host tests and CI packaging do not substitute for observation on a physical device. The v3.1.0 changes have not yet been exercised on hardware.
- Not implemented: mutation of cpuset and uCLAMP controls (their layout uses cgroup paths and `max` tokens that need device-specific semantics), Android Performance Hint API integration, app-launch I/O boost, and UFS/eMMC-specific policy. The Performance Hint API is an app-side NDK interface, so a root daemon cannot receive hints without a broker service.
- Feedback and the journal survive daemon restarts only in the rotated `journal.jsonl`; learned direction and step state are process-local.
- SELinux rules for `/data/adb/coreflow` are not added by default. If the daemon is denied, add narrowly scoped rules after inspecting AVC denials on the target device. Do not set SELinux permissive.
- No software can guarantee absolute stability, lower temperatures, battery life, or responsiveness across all states.

## Contributing

Keep changes aligned with discovery → observation → justified optimization → safety validation → bounded mutation → outcome verification → retain/rollback. Avoid device-specific absolute-path lists, arbitrary resource-count caps, unverified writes, and unsupported performance claims. Add regression coverage for new decision, validation, mutation, and recovery behavior.

See [CONTRIBUTING.md](CONTRIBUTING.md), [CHANGELOG.md](CHANGELOG.md), [docs/miuix-webui.md](docs/miuix-webui.md), and [LICENSE](LICENSE).
