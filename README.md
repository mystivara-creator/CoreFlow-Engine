# CoreFlow Autonomous Engine v3.0.0

**Capability-driven, outcome-driven Android kernel adaptation**
**Implementation:** C++17 · **Target ABI:** `arm64-v8a` · **Module format:** Magisk / KernelSU-compatible ZIP

CoreFlow Autonomous Engine is a native Android daemon that discovers kernel controls exposed by the running device, evaluates candidate adjustments, applies validated changes, measures observable system outcomes, and retains or rolls back each change according to the result. The implementation is designed around device capabilities rather than a fixed device profile or a universal tuning preset.

> CoreFlow does not promise absolute stability or performance gains on every device. Kernel interfaces and their effects vary by OEM, SoC, kernel version, memory configuration, storage controller, workload, and root/SELinux environment. A successful host test cannot establish that a particular kernel control is writable or beneficial on a physical device.

## Design contract

```text
Device & System Discovery
          ↓
Runtime Observation
          ↓
Optimization Engine
          ↓
Change Justification
          ↓
Capability & Safety Validation
          ↓
Bounded Mutation + Read-back Verification
          ↓
Outcome Verification
       ↙       ↘
   Retain     Rollback
       ↘       ↙
     Feedback and next cycle
```

The daemon runs this loop continuously. It does not force a named governor or I/O scheduler. It considers the values and metadata exposed by the active kernel, and it must not interpret “discovered” or “writable” as automatic permission to mutate a resource.

## Runtime behavior

### Device and kernel discovery

The discovery layer inventories readable resources under supported `/proc/sys` and `/sys` locations, reads current values, and derives semantic attributes from the discovered resource path. It also collects device context where available, including Android release/API, kernel release, manufacturer/model, SoC model, online CPU count, total RAM, storage type, thermal measurements, memory availability, and Linux pressure-stall information (PSI).

Discrete controls can use choices advertised by the live kernel—for example, available governors or scheduler choices. Numeric controls require a defensible range, sourced from kernel-published companion metadata or a matching semantic policy. The engine has no fixed maximum count of discovered resources, catalogue rules, or advertised choices. The catalogue supplements semantics; it is not a per-device path list or a maximum-candidate setting.

Discovery coverage depends on what the kernel actually exposes and what the process can read. Unknown or unsupported resources may be inventoried without being eligible for mutation.

### Optimization and coordinated decisions

`OptimizationEngine` ranks eligible candidates using the current observation, resource metadata, and feedback retained during the daemon process. Numeric proposals are derived from a fraction of the discovered interval and adjusted using previous outcomes; discrete proposals come from kernel-advertised choices. The candidate selector avoids no-op changes and does not require a hard-coded maximum candidate count.

A retained change to a governor or scheduler marks its discovered subsystem for follow-up consideration, allowing related controls in that domain to be re-evaluated. This is a dependency hint, not a promise that every related tunable exists or should be changed.

### Validation, mutation, and rollback

Before a write, the engine checks that the proposal corresponds to a supported capability, is not a no-op, remains within its accepted numeric range or advertised discrete choices, and still matches the observed current value. The mutation layer uses guarded path traversal, checks resource identity and current value before writing, then reads the value back to verify the result.

If a write fails, read-back differs, the outcome regresses, or the available measurements are inconclusive, the engine attempts a guarded rollback to the previous value. Rollback itself is verified; a failed recovery is reported rather than represented as successful. The implementation also excludes known high-risk control surfaces such as thermal protection, battery/charger, regulator, watchdog, reset, panic, firmware, and similar paths from ordinary optimization.

The engine evaluates outcomes using comparable signals available before and after the change, including thermal burden, available-memory ratio, and CPU/memory/I/O PSI where supported. Signals that are missing on a device are not fabricated. Feedback and the mutation journal are process-local; the journal is emitted through runtime logging rather than a durable learning database.

## C++ architecture

The implementation uses standard C++17 and separates policy decisions from low-level resource access. The primary source layout is:

| Component | Responsibility |
|---|---|
| `src/main.cpp` | Process entry point, startup logging, and top-level exception boundary. |
| `src/discovery/discovery.cpp` | Device/kernel resource discovery, semantic classification, live choice parsing, and range discovery. |
| `src/discovery/strategy_catalog.cpp` | Parses the external TSV policy catalogue, validates its schema/rules, and indexes rules by semantic domain and attribute. |
| `src/profile/device_profile.cpp` | Reads the versioned TSV device-profile format; profile parsing is separate from live discovery and does not itself mutate resources. |
| `src/observation/observation.cpp` | Samples system signals and current values for discovered capabilities. |
| `src/optimization/optimization.cpp` | Selects numeric or discrete proposals, adapts exploration direction/step size from outcomes, and schedules related-domain follow-up after selected policy changes. |
| `src/decision/decision.cpp` | Determines whether the observed context justifies a candidate change. |
| `src/validation/validation.cpp` | Enforces proposal validity and resource safety constraints before mutation. |
| `src/mutation/mutation.cpp` | Performs guarded writes, checks current state and object identity, verifies read-back, and implements rollback. |
| `src/outcome/outcome.cpp` | Compares available before/after measurements using deadbands and chooses retain, rollback, or unresolved disposition. |
| `src/recovery/learning.cpp` | Records the proposal, outcome, disposition, and recovery details in runtime logs. |
| `src/runtime/engine.cpp` | Orchestrates discovery refresh, observation, proposal, validation, mutation, settling, outcome evaluation, rollback, and feedback. |
| `include/coreflow/model/types.hpp` | Shared domain types for capabilities, observations, proposals, applied changes, outcomes, and dispositions. |
| `include/coreflow/runtime/engine.hpp` | Composition root that connects the runtime subsystems. |

The design keeps semantic selectors such as `domain + attribute` separate from absolute device paths. Paths are discovered at runtime; they are not supposed to be enumerated as a fixed list of device-specific entries in the strategy catalogue.

## Strategy catalogue

The optional policy file is `module/strategies.tsv`, schema version `2`. Each rule contains a strategy ID, semantic domain, semantic attribute, and numeric bounds. Example:

```text
schema_version<TAB>2
strategy<TAB>vm.swappiness<TAB>proc_sys.vm<TAB>swappiness<TAB>0<TAB>200
```

The actual file uses tab-separated columns, not the literal `<TAB>` marker shown above. Rules supply semantics where the kernel does not publish a usable bound. They do not define the total number of discoverable resources. Discrete options should be taken from the running kernel rather than assumed from the catalogue.

A numeric bound in a policy file is not proof that a change improves the device. It is one validation input; justification, write verification, outcome measurement, and rollback remain separate requirements.

## Module package

The `module/` directory contains the root of the installable module ZIP:

- `module.prop` — module identity and release metadata.
- `customize.sh` — installation-time ABI/API and package-content checks, extraction, and permissions.
- `service.sh` — waits for Android boot completion and launches/restarts the native daemon.
- `sepolicy.rule` — SELinux policy additions, if needed for the target root-manager/device domain.
- `strategies.tsv` — semantic strategy policy.
- `webroot/index.html` — static MIUIX/WebUI assets.
- `system/bin/coreflow_daemon` — generated ARM64 binary, copied into the package by CI.

The ZIP is intended for a Magisk/KernelSU-style module installer, not the legacy recovery `META-INF/updater-script` format. Do not add `META-INF/` to the module package unless a separate recovery installer is deliberately supported.

## Build and tests

### Host build and regression tests

Requirements: CMake 3.22+, a C++17 compiler, and standard build tools.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

The current CMake test suite contains six targets:

- `strategy_catalog_tests` — catalogue schema and selector validation.
- `architecture_contract_tests` — architecture-level contract checks.
- `optimization_feedback_tests` — proposal and feedback behavior.
- `mutation_safety_tests` — guarded mutation and rollback behavior on test resources.
- `device_profile_tests` — device-profile parsing.
- `outcome_feedback_tests` — outcome comparison and disposition.

### Android ARM64 module build

The GitHub Actions workflow configures the Android NDK toolchain for `arm64-v8a`, compiles `coreflow_daemon` with the Android C++ runtime, checks the resulting ELF architecture, strips the binary, copies it into `module/system/bin/`, and packages the contents of `module/` into `CoreFlow-Engine-Release.zip`.

The module should only be considered package-buildable after the workflow artifact exists and contains the expected root entries, especially `customize.sh`, `service.sh`, `module.prop`, `sepolicy.rule`, `strategies.tsv`, `webroot/index.html`, and `system/bin/coreflow_daemon`.

## Installation and diagnostics

1. Build/download the generated module ZIP from the workflow artifact or a published release.
2. Install it through the supported root manager and reboot.
3. Inspect runtime logs:

   ```sh
   su -c 'logcat -s CoreFlowEngine'
   ```

4. Check daemon and service status if the log shows repeated restarts. A service restart is a fault signal to diagnose, not evidence that optimization succeeded.

Useful log categories include device context, discovery inventory, candidate rejection, mutation/read-back result, outcome disposition, and rollback status. Access denials or unavailable interfaces should be diagnosed against the target device's actual SELinux domain and kernel layout; do not make SELinux globally permissive to bypass them.

## Scope and limitations

- The implementation is capability-driven, but it cannot infer safe semantics for every arbitrary kernel/vendor file. A resource with unknown value grammar, range, side effects, or recovery behavior must not be guessed into a mutation.
- A successful host build/tests do not substitute for CI packaging, module-install checks, or observation of the daemon on a physical device.
- Kernel and OEM differences mean the same setting can have different effects across devices and workloads.
- Runtime feedback is currently in-memory and does not survive daemon restart.
- No software can guarantee “absolute” stability, lower temperatures, better battery life, or higher responsiveness across all possible states.

## Contributing

Keep changes aligned with the discovery → observation → justified optimization → safety validation → bounded mutation → outcome verification → retain/rollback contract. Avoid device-specific absolute-path lists, arbitrary resource-count caps, unverified writes, and unsupported performance claims. Add regression coverage for new decision, validation, mutation, and recovery behavior.

See [CONTRIBUTING.md](CONTRIBUTING.md), [CHANGELOG.md](CHANGELOG.md), and [LICENSE](LICENSE).
