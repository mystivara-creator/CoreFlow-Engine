# Changelog

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
