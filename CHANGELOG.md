# Changelog

## Unreleased — Dynamic source packaging and property policy

### Changed
- Source and test discovery is dynamic. `cmake/CoreFlowSources.cmake` globs `src/**/*.cpp`. The platform of each file is set by an `@coreflow-platform` tag, or inferred from Android platform headers. `CMakeLists.txt` globs `tests/*_tests.cpp` and creates one executable and one CTest entry per file. Adding a module or a test no longer needs a CMake edit (ADR 0002).
- Test executable names follow their files: `coreflow_mutation_tests`, `coreflow_high_capability_tests`, and so on.
- `observer.cpp`, `decision_trace.cpp`, and `thermal_predictor.cpp` are now platform-neutral and belong to `coreflow_core`. They compile on the host.
- `tools/verify_safety_foundation.sh` checks structure (glob coverage, the test glob, and that `resource_mutation.cpp` is not device-only) instead of names written in CMake.

### Added
- ADR 0002 (dynamic source packaging) and ADR 0003 (system property mutation policy, Proposed). ADR 0003 defines evidence-based eligibility, boot-time overlays, and bootloop rollback tied to the existing boot-attempt counter. It does not allow runtime writes to `ro.*` properties.

### Validation
- The host build was run with an equivalent script because `cmake` was not available. All eight tests passed. A new source and test added with no CMake edit were built and passed.
- Real CMake configure and Android ARM64 build are not yet run and must be the first CI step.

## Unreleased — Mount-derived capability discovery

### Changed
- Tunable roots are read from the kernel mount table (`/proc/mounts`), not from a list of paths in source. Procfs mounts contribute their sysctl tree, sysfs mounts their attribute tree, and cgroup mounts their controller files. A root exists only if the kernel mounted it.
- The tunable walk is breadth-first, bounded by depth (`kSysfsDepth`, `kSysctlDepth`, `kCgroupDepth`), by a visit budget (`kMaxVisits`), and by filesystem boundary. Symlinks are never followed. A truncated walk is reported in the profile (`tunables_truncated`).
- Admission is by evidence: a writable node is a candidate only if its subsystem is recognised from its location, or the vendor's own configuration declares it. Writable nodes that match neither are counted as `tunables_unclaimed` and not admitted.
- `TunableDiscovery` is the single authority for writable kernel surfaces. Inventory probes for classes (thermal, power, cpufreq, devfreq, block queues) and pressure files remain.
- Availability flags (`uclamp_available`, `cpuset_available`, `scheduler_controls_available`) are derived from discovered, readable capabilities. cgroup v2 is derived from the mount table.

### Removed
- Hard-coded tunable tables (`vm.*`, `kernel.sched_*`, cpu, cgroup, gpu, runtime) and their duplicate probes in discovery.
- The `vm_swappiness`, `uclamp_min`, `uclamp_max` and `cpuset_effective_cpus` profile fields, and the baseline fallbacks that named `vm.swappiness` and block-queue leaves. Baselines come from the same capability matrix as every other candidate.
- Binder, ion, dma_heap and lmkd marker probes (observe-only inventory, not used by policy).

### Fixed
- Tunable walk followed symlinks through `stat()`, which could duplicate nodes or loop through `/sys/bus/*/devices`.
- The comment on `UserGrants` said grants default to none. The code and `default.conf` default them to opt-out (`yes`). The comment now matches the code.

### Tests
- `coreflow_capability_driven` adds: tunable roots come from the mount table; unrecognised writable nodes are not admitted; symlinks do not duplicate nodes.
- Resource fixtures build capabilities the way discovery does, instead of the removed profile fields.

### Documentation
- Restored `docs/` to the source package. `tools/verify_source_release.sh` passes, including the WebUI status and render checks.
- `docs/CAPABILITY_DRIVEN.md` now describes mount-derived roots and the unclaimed admission rule. `docs/ANDROID_ECOSYSTEM_MAP.md` no longer refers to the removed `expandEcosystemSurface`.

### Known remaining hard-coded policy (deliberately not changed in this release)
- `observer.cpp` ranks and excludes thermal zones by type name. Replacement evidence (trip points bound to cooling devices) needs validation on device logs.
- `ecosystem_knowledge.cpp` is a name-keyed risk catalog that gates ObserveOnly, FrameworkOwned and Quarantined surfaces. Removing it without replacement would weaken safety.
- `fatal_surface.cpp` is a path-prefix denylist. It is kept as the only hard-coded safety rule.
- `high_capability.cpp` matches name tokens for observation-only status.

### Validation
- Host suite (8 binaries) passes. Safety foundation contract passes.
- Key suites pass under AddressSanitizer and UBSan.
- `tools/verify_source_release.sh` fails on the source package because `docs/BUILD_STATUS.md` is not in the archive.
- Android ARM64 build and device validation have not been run.


## v2.2.5 — Evidence-Based Resource Reasoning & Release Validation

### Changed

- Refined EffectModel eligibility to use semantic evidence, runtime context, and risk rather than assuming that a writable interface implies benefit.
- Memory-pressure handling no longer assumes arbitrary numeric tunables, such as swappiness, will stabilize memory. Stabilizing candidates require relevant domain and vendor evidence.
- Utilization-clamp candidates are considered stabilizing only in relevant thermal or throttling contexts.
- Physical block-queue controls (`read_ahead_kb` and `nr_requests`) remain eligible for evaluation when device capability, policy, and runtime evidence permit them. They are not globally disabled; stress and safety gates remain.
- Aligned release version metadata across CMake, runtime configuration, module metadata, updater metadata, README, and packaged default configuration.

### Safety

- Default mode remains `mutation_mode=observe`.
- Mutation remains disarmed by default (`mutation_armed=false`).
- CPU governor changes remain disabled unless explicitly permitted (`allow_cpu_governor=no`).
- Discovery does not grant mutation authority. Policy authorization, capability checks, bounded writes, journaling, read-back verification, safety holds, and rollback remain required.
- Fatal and protected surfaces remain outside the normal mutation path.

### Validation

- Host deterministic tests: passed in GitHub Actions.
- Android 14+ ARM64 production build and package validation: passed in GitHub Actions.
- ASan + UBSan: passed in GitHub Actions.

### Scope and limitations

- CI success does not certify behavior on every Android device.
- KernelSU/Magisk installation, device-specific behavior, sustained observation, and recovery testing should be validated on the target device before enabling Adaptive mode.
- Publish the public release only with the verified CI-produced ZIP and matching release metadata.

---

## v2.2.0 — Final development snapshot (not yet device-certified)

### Merged improvements (post-merge audit)
- Expanded `ResourceActuator::mutationTargetAllowed` beyond the hard-coded seven VM paths: now admits all EffectModel-prior VM sysctls (`watermark_scale_factor`, `page-cluster` included) and physical block-queue numeric nodes (`/sys/block/<dev>/queue/{read_ahead_kb,nr_requests}`). Virtual devices and scheduler tokens remain denied.
- Aligned actuator allow-list with consultation `SurfaceClass::Admissible` and the existing EffectModel I/O priors so `grant_block_queue=yes` can actually take effect.
- Updated host tests to assert physical queue admission and continued virtual/scheduler denial.

### Included
- Capability-bound resource authorization and consultation Allow/Defer/Deny ledger exposed in runtime status and WebUI.
- Expanded domain evidence summaries and aligned module/CMake/runtime version identity (`v2.2.0`, `versionCode 2200`).
- Updated handoff and readiness documentation to distinguish completed host validation from outstanding Android/device gates.

### Not certified
- Android NDK/ARM64 + ONNX Runtime link, KernelSU/Magisk install tests, and real-device soak/recovery validation remain outstanding.
- Settings DB observation, grant/revoke control API, and grant TTL remain intentionally out of scope until their permission and authority boundaries are reviewed.

## Unreleased — Consultation brain and user grants

### Changed
- Source tree reorganized by role: `observe/` (eyes), `understand/` and `balance/` (reasoning and brain), `mutation/` (hands), `conscience/` (memory), `runtime/`, and `autonomous/`.
- Discovery no longer decides mutation admissibility by itself. It asks `balance/consultation` through `classifySurface`.

### Added
- Expose the bounded consultation ledger in the read-only `status.json` snapshot: cumulative counts by reason plus each namespace's latest reason and whether runtime need was present.
- Render consultation reason counts and last namespace decisions in the WebUI's live Brain panel. Older daemon snapshots remain readable and show an explicit unavailable message.
- Consultation: every resource candidate gets an Allow, Defer, or Deny verdict with a reason. Order: observe-only namespaces and kernel-critical surfaces are denied; missing observation, baseline, semantic knowledge, stress compatibility, runtime need, and EffectModel benefit are deferred; user permission is checked last.
- Per-namespace user grants: `grant_memory`, `grant_block_queue`, `grant_scheduler` (default `no`). A device need never grants permission on its own.
- Bounded consultation ledger for status and diagnostics.
- `coreflow_consultation_tests` and an end-to-end test proving that no kernel value changes without a user grant.

### Safety
- Build properties and settings database are observed, never written, in this build.
- Benefit judgment reuses EffectModel's single eligibility definition; no second threshold was introduced.

## v2.2.0 — Adaptive reasoning + 15-domain capability expansion

### Added
- Bounded multi-stage decision review: observation validation, candidate generation,
  evidence/risk review, ambiguity handling, and proposal-only output.
- Fifteen additional domain capability descriptors with detected/observable/unavailable
  evidence levels; domain discovery never grants actuator authority.
- Live status JSON and WebUI summary for expanded domain coverage.

### Safety
- Existing policy permits, actuator verification, rollback, and quarantined interfaces
  remain authoritative. Newly catalogued domains are observation-only.


## v2.1.0 — Hardening (status truth + agent visibility)

### Fixed
- **Adaptive shown as Observe on Live page**: `status.json` now publishes `mode` as
  `adaptive`/`observe` (config tokens). `primaryBlocker` accepts case-insensitive mode
  so legacy `ADAPTIVE`/`DISABLED` log tokens cannot force `OBSERVE_MODE`.
- **Confidence gate in status blocker** aligned to 0.60 (Balance default).

### Improved
### Polish
- `skip_reason` + observation epoch progress (`observation_samples` / `observation_target=5`) in status.json
- WebUI Brain shows observation epoch and skip/wait reason (no silent NO_ACTION)
- `default.conf` `min_confidence=0.60` aligned with Balance EngineConfig

- **DecisionAgent** proposal summaries include resource, utility, benefit, risk, confidence.
- **ResourceMutationController** records `lastAgentReason()`; published in status as
  `brain.agent_mode` / `brain.agent_reason`.
- **WebUI** Brain panel shows Decision agent row.
- Host tests: `ADAPTIVE` mode must not map to `OBSERVE_MODE`.
- `DEVICE_VALIDATION.md` staged checklist for real-device RC evidence.


## Unreleased — v2.1.0 safety and observability follow-up

This is a source hardening continuation of the supplied v2.1.0 tree, not a version reset or new public release.

- Added `policy_authorized` as a distinct capability fact; `writable`/`permission_granted` evidence and model inference no longer substitute for the explicit policy gate.
- Restricted the production `ResourceMutationController` and central resource actuator to seven exact VM paths with semantic priors; other resource paths are denied for new writes. The separate CPUFreq path remains behind its explicit `allow_cpu_governor` opt-in (default `no`). Block-device queue writes (`scheduler`, `nr_requests`, `read_ahead_kb`) are quarantined across discovery, baseline capture, ranking/selection and the central actuator, including fallback paths.
- Kept journal-based restore available for recovery from changes recorded by older builds; the new-write guard is intentionally not applied to restore.
- Added live device/ecosystem data to `status.json` and the WebUI: manufacturer/model, Android/API, kernel/ABI, SoC/board, interface presence, discovered capabilities and policy-ready count.
- Added regression coverage for policy authorization and protected block queue targets, plus WebUI contract/render assertions.
- Migrated the native build contract to C++20 for host and Android NDK builds.
- Connected `ScoringDecisionAgent` to the real resource-candidate ranking path. It returns explainable ranked proposals only; policy authorization, MutationAuthority, journal, actuator verification, ExperienceMemory scoring and rollback remain downstream.
- Added a KernelSU / KernelSU Next `updateJson` feed. A GitHub Release workflow validates the versioned install ZIP and attaches a release-specific `update.json` so the manager can detect newer `versionCode` values.
- Host build/CTest passed in this work. Android NDK/ARM64 linking, Android ONNX integration and device-level behavior still require the operator's CI and staged validation.

## v2.1.0 — Balance pass (community evaluation target)

Identity: **Observe. Understand. Balance. Adapt.**

Authority was fail-closed to a fault: real devices often never reached
`mutation_allowed_by_context`, so Adaptive could not adapt. This pass keeps every
hard safety stop (journal, verify, rollback, Idle/disarm/kill-switch, CPU opt-in)
and opens the dead bands that blocked useful Low/Moderate resource mutation.

### Balance changes (real-device impact)
- **Context confidence** gate: `0.70` → `0.60` (mutation + stabilizing).
- **Config `min_confidence`**: `0.70` → `0.60` (MutationAuthority / controllers).
- **Thermal headroom**: `< 50°C` → `< 52°C` (still below ThermalGuard enter 55°C).
- **Memory headroom**: `≥ 0.18` → `≥ 0.14` (Pressure still enters at 0.10).
- **CpuBound classification**: util `≥ 0.80` → `≥ 0.72`; Sustained rising util `≥ 0.65` → `≥ 0.60`.
- **Policy Normal / CpuBound**: Moderate confidence `0.85` → `0.72`.
- **Policy Normal**: mild activity with headroom may take **Low** (was Hold-only).
- **EffectModel**: confidence multipliers raised (`0.85`→`0.92` numeric, `0.75`→`0.88` scheduler);
  utility floor `0.05` → `0.02`; scheduler justify includes Normal / moderate load.

### Still hard (unchanged on purpose)
- Default remains observe-only until Adaptive + armed.
- CPU governor still requires `allow_cpu_governor=yes`.
- ThermalGuard / Pressure: only `stabilizing_only` resource writes; never CPU governor.
- Journal before write, read-back verify, regression restore, one write per epoch.

### Also in this tree
- `IDecisionAgent` + `ScoringDecisionAgent` (interface ready; ranking still EffectModel).
- `DEVELOPMENT_ROADMAP.md` for post-community agentic work.

## Unreleased (audit fixes originally on v2.1.0 source)

### Added: live engine status for the WebUI (eyes, brain, hands)
The WebUI used to re-read `/proc` through a shell and guess decisions from log regexes, so
it never saw what the engine itself saw or decided.
- **C++**: `status_snapshot` (host-tested). The daemon writes `/data/adb/coreflow/status.json`
  atomically (temp file + rename, mode 0600) once per cycle, and a final `running:false`
  snapshot at shutdown. Writing is best-effort and cannot affect control.
- **Content**: eyes (the engine's own telemetry), brain (state, workload, context gates, plan,
  mode/armed, what `MutationAuthority` would grant now, baseline and cooldown), hands (changes
  held, applied values, verified/rolled-back/failed counters, last change).
- **`blocker` code**: one stable answer to "why is the engine not acting?", from a fixed
  evaluation order (stopped, safety hold, observe, not armed, baseline, observing, change
  held, cooldown, context idle/confidence/headroom, no candidates, ready).
- **WebUI**: new default page "Live engine" with a decision pipeline and Eyes/Brain/Hands
  panels. Read-only; polls only while visible; reports a missing, stale or stopped daemon.
  Rendering uses `textContent` only.
- **Tests**: strict-JSON validation of the writer (NaN, quotes, control characters, size caps),
  blocker ordering, atomic write; a cross-language test that feeds real C++ output to the
  WebUI parser and checks both sides agree on every field and blocker code; an end-to-end
  smoke test running the real `app.js` against a fake DOM and KernelSU bridge. Writer and
  reader mutants were all caught.

### Changed (behavior): graded safety instead of a hard block
Before, ThermalGuard and Pressure blocked every write at three separate layers, and
the daemon restored any change the moment such a state began. Those are the moments
the tuning priors were written for, so the engine was idle exactly when needed.
- **Context**: new `stabilizing_allowed_by_context` (non-Idle, confidence >= 0.70, and
  stressed: ThermalGuard/Pressure or missing thermal/memory headroom). Heavy I/O no
  longer blocks proactive tuning.
- **Policy**: a stressed but trustworthy context yields a `stabilizing_only` plan at
  Low intervention (Memory/Io candidates). Low confidence or Idle is still a hard hold.
- **EffectModel**: marks a candidate `stabilizing` only when it answers thermal stress
  by lowering load or answers memory pressure in the prior's memory direction.
  Schedulers are never stabilizing.
- **Authority**: a stabilizing plan gets a Resource permit only. It never gets a CPU
  governor permit. In ThermalGuard/Pressure any non-stabilizing plan gets nothing.
- **Daemon**: the immediate restore under stress is skipped only for a held
  stabilizing resource write while the plan still permits one (`shouldRestoreEpochNow`,
  host-tested). Idle, disarm, kill switch, SAFE_MODE, low confidence, any CPU governor
  change, and a measured regression still restore. The observation window, outcome
  hold and one-write-per-epoch limit still apply.
- **Resource rejection**: `rejectLastMutation()` previously only cleared a list. A
  regressed candidate is now suppressed for 120 controller cycles, as the CPU
  controller already did.
- Trace and log gain `stabilizing_only`.
- Unchanged: journal before write, read-back verify, rollback, boot-loop guard,
  kill switch, SAFE_MODE, `allow_cpu_governor` opt-in.

### Fixed
- **Scheduler selector values** (`queue/scheduler`): the kernel reads back the
  active token in brackets (`[mq-deadline] kyber none`) but accepts only the bare
  token on write. Verification and baseline capture now use the comparable active
  token, so rollback writes a value the kernel accepts. Journals written by earlier
  builds are normalized on restore.
- **Installer legacy flag**: `/sdcard/CoreFlow/enable_adaptive` no longer arms Adaptive
  (shared storage is writable by any app). Only an explicit Volume Down press does;
  timeout means observe, as the on-screen text states.
- **CPU governor permit**: `MutationAuthority` now refuses the CPUFreq permit unless
  `allow_cpu_governor=yes`, matching the controller-side check.
- **Resource apply result**: a `NoChange` outcome is reported as `Skipped`, not
  `Verified`. The journal is no longer dropped while another resource is still changed.
- **WebUI shell interpolation**: `mutation_mode` is quoted in the config-write script.

### Tests
- Regression tests for selector comparison, scheduler rollback to the bare token,
  legacy journal normalization, and CPU permit gating. Verified to fail on the
  pre-fix behavior.
- Installer harness: 10/10 (previously 2 of 12 cases failed). Flag cases now assert the flag is ignored.

### Removed
- Dead `webui_adaptive_pending` branch in `service.sh` (nothing ever created the marker).

### Known gaps
- Scheduler behavior is verified with regular files, not real sysfs. Device validation is still required.
- Stabilizing priors (direction and step of each knob) are source-level assumptions, not device measurements.
- `autonomous.cpp` and `decision_trace.cpp` cannot be built on the host; they were syntax-checked with `-Werror` against a stub `android/log.h` only. The NDK build is still required.
- No dedicated test yet for the `ReadFailed`/journal-retention path in `applyCandidate`.
## v2.1.0 — Fully Autonomous Capability-Driven Engine

### Architecture
- **EffectModel** ranks discovered writable resources using a consistent benefit/risk/confidence utility and structured resource identity. Unknown numeric controls remain ineligible until semantics are defined.
- I/O queue controls require observed I/O activity; CPU utilization or load alone is not sufficient evidence to tune them.
- **ResourceMutationController**: candidate generation is now capability-matrix driven, not a hard-coded resource name list. Baseline capture covers every `mutation_ready` Memory / Io / Scheduler capability.
- **PolicyEngine**: more proactive under Warming / Elevated / active workloads while remaining fail-closed on Idle and when context headroom is missing. Candidates are taken from the resource model.
- **MutationAuthority**: issues permits only for eligible `Candidate` plans; `ReduceIntervention` is hold/recovery-only and never starts a new write.

### Discovery
- Expanded VM tunables: `vm.min_free_kbytes`, `vm.dirty_expire_centisecs`, `vm.dirty_writeback_centisecs` in addition to swappiness / dirty ratios / vfs_cache_pressure.

### Safety (unchanged contract)
- Release default remains observe-only (`mutation_mode=observe`, `mutation_armed=false`).
- Journal → bounded write → read-back verify → rollback.
- Boot-loop guard and SAFE_MODE preserved.

### Context gate
- `ThermalGuard` and `Pressure` block new optimization writes. Recovery and rollback remain available through the controller's restoration path.

### Intent
The engine discovers what the device exposes, scores what is safe and useful to change, applies at most one verified mutation per epoch, measures outcome, and rolls back. Resource identity is not pre-selected by the packager.

## v2.0.1
- Release-candidate source package with observe-only defaults, ONNX thermal predictor, host tests, and safety contracts.

