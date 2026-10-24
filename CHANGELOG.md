# Changelog

## Current release line: v3.0.0 — Autonomous Architecture baseline

This release deliberately resets the **displayed** version label to `v3.0.0` to mark CoreFlow's Autonomous Architecture baseline. It does not revert the source to the original historical v3.0.0 package: the release carries forward the outcome-driven and adaptive-workload hardening developed on the previous version line. The internal module `versionCode` remains monotonic at `30300` so package update ordering does not move backwards.

### Added
- `docs/architecture-contract.md`: CoreFlow-owned stage contracts, hard safety invariants, module boundaries, learning rules, reference sources and release acceptance criteria.
- Explicit end-to-end architecture flow: discovery → observation → workload assessment → justified candidate → capability/safety validation → bounded mutation → read-back/outcome verification → retain or verified rollback → evidence-based learning.
- A phone-portrait-only WebUI layout. Landscape and wide viewports show an orientation message; no frontend framework or network listener is introduced.
- CI JavaScript actions updated to Node.js 24-compatible major versions and the runner is pinned to `ubuntu-24.04` to avoid implicit `ubuntu-latest` image migration.

### Retained
- Adaptive noise-floor and workload assessment; per-signal unit separation and fallbacks; outcome-based retain/rollback; pause-with-observation; runtime state and atomic command handling; native C++20 engine and Android ARM64 packaging workflow.
- Encore Tweaks remains a comparison source for patterns. This patch does not copy Encore implementation files; all adaptations must be reviewed against CoreFlow contracts and external license notices.

### Validation boundary
- The source baseline at commit `388bfda` had passed host regression tests and the Android NDK packaging workflow before this release-specific delta. This patch must run through CI again.
- Physical-device behavior, thermal interactions, SELinux permissions and performance/energy effects still require direct device validation.

## Historical engineering notes (previous display-version line)

### Previous v3.2.0 — hardening freeze

No new features. Every tuning value that remained in the daemon was either derived from the device or removed.

### Changed (behavior)
- Outcome deadbands, the temperature-rise guard, and the thermal headroom margin are learned from each device's own sample-to-sample variation (`NoiseFloorTracker`). Previously they were fixed numbers. A signal without a learned floor makes the outcome inconclusive and blocks retention.
- Workload elevation is measured against each signal's own median and variation, in units of that variation. Fixed pressure, memory and CPU-busy gates are removed.
- The thermal headroom margin is the kernel-published hysteresis of the passive trip point.
- A frame is slow when it misses one whole refresh after the dominant frame interval. The mean-based threshold is removed from both frame paths.
- The shipped strategy catalogue has no numeric rules. Numeric mutation requires a kernel-published range or an operator extension.
- The probe size is a power-of-two multiple of the representation quantum, grown on neutral results and shrunk on regressions. Previously it was a fraction of the span.
- Selector and dependency ranking is lexicographic. The additive priority offsets are removed.
- The WebUI staleness bound comes from the daemon (`stale_after_s`) instead of a constant in the page.

### Fixed
- A failed probe that regressed grew instead of shrinking when judged through the outcome-aware API.
- Outcome deadbands mixed PSI percentage units with ratio units.
- The module service forwarded daemon output through a shell loop that could outlive the daemon. The daemon now writes its own log file; Android diagnostics go to logcat.
- The last error message persisted after successful cycles; transient cycle exceptions now clear after a subsequent complete observation while unresolved recovery errors remain visible.
- Memory PSI absence no longer disables workload detection when MemAvailable is valid. PSI pressure and memory headroom are scored independently against their own adaptive baselines, without mixing units or double-counting.
- CPU-busy is a per-device fallback when CPU PSI is unavailable, and now participates in CPU-domain outcome verification as well as workload scoring. A regression in either measurable CPU signal can veto retention.
- The noise estimator no longer infers representation resolution from a workload transition. It uses source precision/counter granularity, ignores non-finite samples, takes a lower median, excludes adverse excursions from the training history, and publishes the pre-sample baseline for decisions.
- Concurrent observations are serialized so CPU deltas, frame telemetry and the noise-floor history cannot be applied out of order.
- Pause now keeps discovery and telemetry current while preventing new mutations, matching the WebUI status and runtime log.
- Rollback counters reflect verified restoration only. Exceptions or failed recovery are reported as unresolved rather than silently counted as successful rollback.
- StateStore operations serialize writes/rotation/command claims, use unique atomic-write temporaries, claim command files with rename, flush journal entries before reporting success, and fail closed when journal rotation fails.
- WebUI commands are validated as own-property tokens and published through unique `mktemp` files plus atomic rename so concurrent UI requests cannot reuse a temporary path or expose a partial command.
- An unused legacy step table was written and erased but never read.

### Retained invariants (not tuning)
- Loop timing and the stale bound derive from `runtime/limits.hpp`: idle 2–10 s, paused poll 2 s, discovery refresh 2 min, settle equal to the kernel PSI avg10 window (10 s).
- Physical sanity bounds on thermal readings (10–125 °C) and PSI percentages (0–100).
- Safety denylists of protected kernel surfaces, pruned subtrees, and writable-mode and object-identity checks.
- Algorithmic constants: the power-of-two probe growth, the 2× missed-refresh definition, the signal-to-noise test (an elevation must exceed one deadband), and the exploration budgets (six numeric trials, two misses before deferral).
- Memory budgets for tracked contexts and capabilities.

### Previous v3.1.0 — coordinated tuning, real console bridge, correctness fixes

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

### Original v3.0.0 — initial Autonomous Engine package

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
