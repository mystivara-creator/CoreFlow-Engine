# Changelog

## v3.1.0 — coordinated tuning, real console bridge, correctness fixes

### Fixed
- Numeric probes learned the wrong direction when the bounds forced a downward probe; learning now uses the direction actually written.
- Scheduler and governor follow-ups never ran: tunables beneath `queue/iosched/` and `policyN/<governor>/` were not discovered after the selector changed. A retained selector change now triggers an immediate rediscovery and follow-ups are scoped to the new directory.
- Writability was inferred from `access(W_OK)`, which root always passes. Mode bits are now checked as well.
- The WebUI called the KernelSU bridge with an incorrect API and had no daemon data to show.
- Documentation stated C++17 and the wrong version and test count.

### Added
- Domains `sysfs.block.iosched` and `sysfs.cpu.governor`, with catalogue rules for the common elevator and governor tunables.
- Thermal headroom from the kernel's passive trip points as a retention guard, replacing the need for any absolute temperature target.
- Operator state directory (`/data/adb/coreflow`): atomic `status.json` and `capabilities.json`, rotated `journal.jsonl`, and a `command` file accepting `pause`, `resume`, `rescan`, `reset_learning`.
- Pipeline stage reporting so the console shows the C++ stage that is running.
- Four host test targets: discovery classification, runtime state store, thermal headroom, optimization coordination.

### Changed
- WebUI rewritten: reads daemon state through the KernelSU bridge and sends only fixed tokens. No terminal needed.
- `service.sh` tails the daemon log from the start and rotates it at 1 MiB.
- Discovery prunes debugfs, tracing, firmware and pstore subtrees.

### Not yet
- cpuset and uCLAMP mutation, Performance Hint API integration, app-launch boost, UFS/eMMC policy.
- Device validation of v3.1.0 changes.

## v3.0.0 — initial Autonomous Engine package

### Added
- Native C++17 closed-loop runtime: discovery, observation, optimization, decision justification, safety validation, mutation, outcome evaluation, rollback and feedback.
- Runtime discovery of accessible kernel controls under `/sys` and `/proc/sys`, without a fixed resource or candidate-count cap.
- Device context discovery for Android build/API, kernel release, SoC properties, online CPU count, RAM and storage/controller type when exposed by the running system.
- Kernel-advertised discrete choice discovery and policy changes that use the choices actually exposed by the device, rather than fixed governor or I/O-scheduler names.
- Numeric candidate generation from discovered/semantic bounds and the resource's representation, with runtime pressure-aware direction and per-resource outcome feedback.
- Symlink-resistant path opening, current-value and object-identity checks, write/read-back verification, and guarded rollback.
- Outcome comparison using available memory, thermal data and CPU/memory/I/O PSI signals.
- KernelSU/Magisk-compatible service and ARM64 NDK build workflow.

### Changed
- Strategy catalog is a semantic extension, not a per-device absolute-path allowlist or an arbitrary cap on how many kernel controls may be discovered.
- Retained CPU governor, devfreq governor, EPP or block scheduler changes trigger follow-up evaluation of related controls within the same discovered subsystem.
- Potentially dangerous thermal, battery/power, regulator, watchdog, reset, debug, firmware and crash-control surfaces are excluded from mutation.

### Validation status
- Host build and all six host regression tests pass in the build environment used to prepare this source package.
- Android NDK packaging and behavior against a physical Android kernel have not been executed in this environment. Kernel-specific effectiveness and safety cannot be inferred from host tests alone.
