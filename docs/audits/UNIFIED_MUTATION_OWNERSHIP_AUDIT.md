# Unified Mutation Ownership Audit

**Repository:** `mystivara-creator/CoreFlow-Engine`  
**Audit ref:** `audit/main-correctness-remediation`  
**Audit basis:** PR head `474405884771e02b3a6854c07d4cca69f6ce1748`  
**Purpose:** Establish a source-grounded inventory and migration contract before consolidating every mutation-capable subsystem under one transaction authority.

> This document records verified source paths and required remediation. It does not claim that the coordinator integration is already implemented.

## 1. Current architectural finding

`CentralMutationCoordinator` currently arbitrates leases used by `AutonomousTuner`. It is not yet a system-wide mutation authority.

The coordinator's current `acquire_lease()` behavior preserves the current owner and marks `preemption_requested` when a higher-priority request arrives. `emergency_preempt_all()` marks active leases but does not dispatch a cancellation callback, wait for an acknowledgement, or itself verify rollback. This is a useful ownership-preservation primitive, but it is not complete cancellation orchestration.

`BaselineStore` serializes its own operations and records snapshots/journal state. Its mutex does not serialize controller-level decisions across the duration of a multi-node transaction, nor does it provide domain ownership, controller cancellation, priority arbitration, or conflict policy across independent controllers.

## 2. Verified mutation-path inventory

| Owner / subsystem | Source path | Current mutation route | Required coordinator domain | Audit status |
|---|---|---|---|---|
| AutonomousTuner | `src/kernel/autonomous_tuner.cpp` | Acquires leases for tuner transactions and mutates through `BaselineStore` | CpuPolicy, BlockIo, VmMemory (per actual candidate/resource) | Partial: owns its own transactions; emergency cancellation of other owners is not implemented |
| FrequencyController | `src/kernel/frequency_controller.cpp` | `apply()` calls `BaselineStore::apply_mutation()` for CPU/GPU maximum-frequency nodes | CpuPolicy / GpuDevfreq | Not integrated with CentralMutationCoordinator |
| UclampGovernor | `src/kernel/uclamp.cpp` | `write_top_app()` performs paired min/max `BaselineStore::apply_mutation()` calls | UclampCgroup | Not integrated; pair must be one transaction with dependency ordering |
| HardwareLongevityManager | `src/kernel/hardware_longevity.cpp` | Storage `io.weight` and battery charging-step writes through BaselineStore | BlockIo or PowerLongevity | Not integrated; multiple storage targets must not leave partial ownership silently |
| VulkanController | `src/kernel/vulkan_controller.cpp` | `apply_workload_policy()` writes Adreno/Mali policy nodes through BaselineStore | GpuDevfreq or a dedicated GPU policy domain | Not integrated; safety-control writes must be classified separately from optimization candidates |
| Daemon lifecycle helper | `src/daemon/main.cpp` | `write_sys_node_quiet()` directly opens and writes cgroup membership and `/proc/self/oom_score_adj` | Lifecycle-only authority, not workload optimization | Direct write ignores short/failed write and does not verify post-write state |

The inventory is based on the current PR source and targeted repository-wide searches. It is a verified starting set, not proof that no other write sites exist. A complete source scan for `open(O_WRONLY)`, `write()`, `ofstream`, shell redirection, resetprop, and indirect bridge writes remains a required acceptance gate.

## 3. Required transaction contract

Every mutation-capable controller must use one common transaction lifecycle:

1. **Resolve identity:** canonical resource identity, device/inode or equivalent kernel-resource identity, semantic domain, and current capability fingerprint.
2. **Validate capability:** existence, readability, write eligibility, accepted format/choices, bounds, dependencies, and explicit mutation eligibility. Writable permission alone is insufficient.
3. **Acquire ownership:** acquire all required resource/domain leases in deterministic order. If any lease fails, release leases acquired by this attempt and do not write.
4. **Revalidate immediately before write:** identity, current value, external ownership, capability and dependency state.
5. **Record transaction:** persist baseline/prior/target values and transaction metadata before the first write.
6. **Apply bounded mutation:** one controlled write at a time; no untracked direct writes.
7. **Read back:** verify the exact resource identity and normalized active value after each write.
8. **Observe and settle:** retain ownership for the entire settling and outcome-verification window where competing mutation would invalidate attribution.
9. **Classify outcome:** IMPROVED, NEUTRAL, REGRESSED, INCONCLUSIVE, OWNERSHIP_LOST, REJECTED, or RECOVERY_REQUIRED. Missing measurements must not be interpreted as zero.
10. **Retain or recover:** retain only with adequate comparable evidence; otherwise rollback with ownership checks and readback verification.
11. **Release ownership:** only after verified retain or verified rollback. If recovery is unresolved, keep ownership/recovery state visible and block conflicting writes.

### Transaction invariants

- BaselineStore remains the durable snapshot/journal and safe-write layer; it is not a substitute for the coordinator.
- The coordinator owns arbitration and transaction lifetime; controllers own domain-specific policy and must expose cancellation/rollback acknowledgement.
- No controller may release a lease merely because cancellation was requested.
- A higher-priority request may request preemption, but cannot take ownership until the previous owner confirms its operation is stopped and recovery is verified.
- If rollback fails, ownership and recovery state remain visible. Never report a successful rollback based only on an attempted write.
- External OEM/kernel changes must never be overwritten during rollback. Resource identity and current value must still match the state CoreFlow owns.
- Lifecycle writes such as daemon cgroup migration are not optimization candidates. They still require explicit allowlisted targets, complete-write checks, readback, and a failure policy.

## 4. Emergency cancellation protocol

Emergency preemption must become an acknowledged state machine, not a boolean hint:

`ACTIVE → PREEMPT_REQUESTED → CANCELLING → ROLLING_BACK → VERIFYING → RELEASED`

Failure path:

`PREEMPT_REQUESTED / ROLLING_BACK / VERIFYING → RECOVERY_REQUIRED`

Requirements:

- Coordinator records reason, priority, owner, transaction ID, resource set, request timestamp, and current resolution state.
- Every registered controller receives a cancellation request at a safe interruption point.
- Controller stops issuing new writes immediately after observing the request.
- Controller resolves any in-flight write, rolls back only resources it still owns, and verifies restoration.
- Controller returns an explicit acknowledgement with outcome and unresolved resource list.
- Coordinator releases ownership only after successful acknowledgement and verification.
- Timeout or controller disappearance produces `RECOVERY_REQUIRED`; it must not silently clear the lease.
- Emergency safety and OEM thermal protections remain authoritative. The coordinator must not disable or bypass platform thermal mitigation.

## 5. Controller-specific integration notes

### FrequencyController

- Acquire CPU/GPU domain ownership before beginning the per-domain apply loop.
- Keep ownership while validating current min/max and selecting a supported frequency.
- A changed OEM maximum or minimum retires the domain and must not be overwritten.
- Rollback must use the same identity/ownership contract as forward mutation.
- Distinguish CPUFreq and GPU devfreq resources; do not infer that all devfreq surfaces are GPUs.

### UclampGovernor

- Treat `cpu.uclamp.min` and `cpu.uclamp.max` as one coupled transaction.
- Preserve current dependency ordering when the new maximum is below the current minimum.
- On a partial pair failure, rollback the first successful write before releasing Uclamp ownership.
- If an external owner changes either clamp, relinquish control and do not reassert the old pair.

### HardwareLongevityManager

- Battery charging current and storage arbitration are separate domains with separate safety policy.
- Never raise an OEM charging limit.
- Treat multiple `io.weight` targets as a bounded transaction or record each independently with explicit partial-failure recovery.
- Do not claim storage health or bandwidth guarantees from `io.weight` changes.

### VulkanController

- Separate optimization controls from safety controls. Thermal throttling must remain enabled and must never be treated as an optimization variable.
- Acquire GPU policy ownership before changing Adreno/Mali policy nodes.
- A failed optional write must not leave the controller reporting a fully applied policy.
- GPU capability evidence must be current and vendor-specific; unknown GPU surfaces remain observation-only.

### Daemon lifecycle writes

- Replace `write_sys_node_quiet()` with an allowlisted, checked lifecycle operation.
- Check full write completion and report failure.
- Verify the process's actual cgroup membership and `oom_score_adj` readback where supported.
- Do not continue as though migration succeeded when a write failed.
- Keep lifecycle ownership separate from adaptive workload-controller leases.

## 6. Required tests

### Coordinator unit tests
- Same-domain conflict rejection.
- Deterministic multi-domain acquisition and partial-acquisition cleanup.
- Higher-priority preemption request without lease transfer.
- Controller acknowledgement required before release.
- Timeout, controller disappearance, rollback failure, and recovery-required persistence.
- Thread-safe concurrent acquire/release/preempt operations.

### Controller integration tests
- FrequencyController and tuner competing for CPU policy.
- Uclamp pair write interrupted after first successful node.
- Vulkan policy update competing with another GPU-domain owner.
- Longevity storage pacing competing with block-I/O tuner.
- Emergency request while a controller is in an active multi-node transaction.
- OEM/external writer changes value or inode before rollback.
- Verify no controller continues issuing writes after cancellation acknowledgement.

### Lifecycle tests
- Short write and failed write.
- Symlink substitution and path replacement.
- Missing or unreadable readback.
- Cgroup migration success and failure.
- OOM score adjustment success and failure.
- Confirm status/logs never claim success when readback does not match.

## 7. Acceptance criteria

Unified mutation ownership is complete only when all of the following are demonstrated:

- Every mutation path is inventoried, including direct sysfs/procfs writes, resetprop, shell helpers, Rust bridge calls, and lifecycle operations.
- Every optimization mutation is mediated by the coordinator for its complete transaction lifetime.
- No controller can bypass capability checks, baseline capture, journaling, readback, and rollback verification.
- Preemption reaches the actual controller and is acknowledged only after safe resolution.
- Conflicting controllers cannot mutate the same resource/domain concurrently.
- Partial multi-node transactions are recovered deterministically.
- External OEM changes are preserved.
- Failed rollback remains visible as `RECOVERY_REQUIRED`.
- Integration tests exercise real controller behavior with mocked resources, not only synthetic lease records.
- Full host tests, sanitizers, Rust, WebUI, shell/module validation, and Android ARM64 packaging pass on the exact candidate SHA.
- Physical-device validation remains a separate gate and is not inferred from host CI.

## 8. Implementation order

1. Complete the full write-path inventory and classify lifecycle versus optimization writes.
2. Define coordinator transaction/owner registration and cancellation acknowledgement interfaces.
3. Add thread-safety and deterministic multi-domain lease acquisition.
4. Integrate one controller at a time, beginning with FrequencyController and UclampGovernor.
5. Integrate VulkanController and HardwareLongevityManager with domain-specific policy.
6. Harden daemon lifecycle writes separately.
7. Add cross-controller concurrency and emergency-preemption integration tests.
8. Run complete CI and audit all remaining write sites before claiming system-wide ownership.

**Current status: NOT COMPLETE.** The existing coordinator is a foundation, not yet a universal mutation authority.


## 9. Implementation update — 2026-11-06

The following coordinator and telemetry hardening has now been implemented on the audit remediation branch:

- Coordinator lease state is protected by a mutex; the conflict counter is atomic.
- Concurrent acquisition of one domain is covered by a 12-contender regression test; exactly one owner may hold the lease.
- Released lease records are reused per domain to prevent unbounded growth during long-running daemon operation.
- Lease release can be transaction-ID scoped. A stale completion from an earlier transaction cannot release a newer lease held by the same controller/domain.
- PSI availability is propagated independently for CPU, memory, and I/O through daemon, Rust controller, predictor, tuner, status JSON, and WebUI. Missing PSI is not treated as zero; incomplete evidence withholds normal workload mutation while thermal stabilization remains independently actionable.
- Thermal trends are now tracked independently per discovered sensor. Invalid sensor reads reset only that sensor's trend history; the most rapidly warming valid compute/board sensor contributes the conservative trend used by the tuner.
- WebUI reports unavailable PSI as N/A and exposes per-sensor thermal velocity instead of retaining stale pressure values.

Validation for the complete candidate head `49ab32fcb781d9096f83e2d9b9020ef400e3a7c7`:
- [GitHub Actions run 42266487313](https://github.com/mystivara-creator/CoreFlow-Engine/actions/runs/42266487313) passed all seven jobs: Host C++, ASan/UBSan, Rust/Clippy, WebUI, Module validation, Android ARM64 package, and CI gate.

### Remaining ownership blockers at the 2026-11-06 baseline

At this historical candidate, the following work was still unresolved and is superseded by the implementation update below:

- Timeout policy and controller-disappearance recovery.
- HardwareLongevityManager coordinator integration.
- Real cross-controller conflict and partial-failure integration tests.
- Checked, allowlisted daemon lifecycle writes with readback.
- Physical-device behavior validation.

**Status at that candidate: NOT COMPLETE.**


## 10. Implementation update — controller integration and persistence hardening

Since the previous update, the audit branch has advanced with the following verified changes:

### Coordinator and transaction ownership
- Added registered preemption handlers. Emergency and higher-priority requests are dispatched outside the coordinator mutex.
- A controller acknowledgement releases only the matching domain and transaction ID; failed or missing acknowledgement leaves the lease active.
- Same-owner requests cannot silently replace an unresolved transaction. A controller must release the current transaction before acquiring a different transaction ID.
- FrequencyController now coordinates CPU policy and GPU devfreq domains separately, retains leases while mutations are active, and verifies rollback before release.
- UclampGovernor now coordinates the coupled `cpu.uclamp.min` / `cpu.uclamp.max` transaction and preserves fail-closed behavior when resynchronization cannot be verified.
- VulkanController now coordinates GPU sysfs mutations and has an emergency rollback path with readback checks.

### Persistence
- Baseline journal directory traversal now opens each directory component with `O_NOFOLLOW`.
- Journal temporary creation, rename, and recovery use directory-file-descriptor-relative operations (`openat`, `renameat`, `unlinkat`) to avoid parent-directory symlink substitution.
- Added a regression test proving a symlinked journal state directory cannot redirect a journal write into its target.

### Validation
- Candidate SHA: `6d8aea73c1b427f43bd83cab2d500da492958c4a`.
- [GitHub Actions run 42270168256](https://github.com/mystivara-creator/CoreFlow-Engine/actions/runs/42270168256) passed all seven jobs.
- Host C++ suite: 12/12 tests passed, including BaselineRollbackTest, FrequencyOwnershipTest, UclampTest, and VulkanControllerTest.
- ASan/UBSan, Rust/Clippy, WebUI, Module validation, Android ARM64 production package, and CI gate all passed on that exact SHA.

### Remaining work before the 3.1.0 freeze
- Integrate HardwareLongevityManager battery and storage mutations with separate PowerLongevity and BlockIo leases, callbacks, and verified rollback.
- Replace daemon lifecycle direct writes with allowlisted, checked operations and readback.
- Complete cross-controller multi-domain and partial-acquisition recovery tests.
- Review Vulkan sysfs policy values against discovered per-node semantics and allowed values; path existence alone is not sufficient capability evidence.
- Validate actual behavior on supported physical devices. CI does not establish device compatibility or thermal safety.

**Status remains: NOT COMPLETE.** The passing CI run validates the candidate commit, not the entire 3.1.0 freeze criteria. Do not merge or publish the freeze until the remaining ownership and device-validation gates are closed.



## 11. Implementation update — transaction isolation and recovery hardening

**Audit basis:** PR head `1de5d818af6c5298506b382d15372645c36b6638`  
**Base:** `5ccc5a23598471833fc058f61e45ae9a13b74bf7`

### Implemented on the remediation branch

- `BaselineStore::apply_mutation()` no longer invokes a global rollback when one path fails. Runtime failure recovery is path-scoped; global `rollback_locked()` remains reserved for journal recovery and an explicit global safety sweep.
- Added two-path regression coverage proving that a failed mutation on a missing path does not roll back unrelated active mutations.
- Added coordinator emergency-admission barrier. Daemon safety recovery closes lease admission before controller callbacks, waits for all active owners to acknowledge, performs the global BaselineStore sweep only after leases are clear, and reopens admission only after no active mutations remain.
- Preemption callback exceptions are contained. An exception is a failed acknowledgement: the lease remains active and emergency admission remains closed.
- FrequencyController handler identity is per instance. Frequency discovery and apply preflight reject a journaled path that is not owned by that controller; failed write/readback paths retain or recover ownership conservatively.
- CgroupV2Controller rejects a foreign journaled path even when it already owns a different path in the CPU-policy domain. Failed multi-node writes roll back only its own transaction.
- Uclamp discovery, apply, and resync reject pre-modified clamp nodes that are not owned by the governor.
- HardwareLongevityManager rejects foreign journaled battery-current and storage-weight entries before using them as a baseline.
- VulkanController refuses to treat a foreign journaled GPU value as an already-applied policy.
- AutonomousTuner preflight rejects journaled paths left active by another or unresolved owner. Partially committed failed steps are retained in rollback state rather than dropped.
- Daemon lifecycle cgroup migration and `oom_score_adj` writes now use exact allowlisted paths, full-write checks, and readback verification.
- Legacy `ScopedBaselineLock` is deprecated because it has no coordinator ownership identity. Its implementation now skips already-active journal paths and tracks unresolved path-scoped rollback for a final retry.

### Regression coverage added or extended

- Baseline path-scoped rollback isolation.
- Coordinator admission remains closed until leases are resolved.
- Throwing preemption callbacks do not terminate dispatch or release ownership.
- Cgroup failed-write lease cleanup and foreign-node isolation while another Cgroup path remains leased.
- FrequencyController multiple-instance callback identity, failed CPU write handling, and foreign journaled CPUFreq path isolation while GPU remains independently coordinated.
- Uclamp foreign journal ownership rejection before apply/resync.
- HardwareLongevityManager foreign battery and storage journal rejection.
- Vulkan foreign GPU policy journal rejection.

### Validation state

- Workflow run [42281030503](https://github.com/mystivara-creator/CoreFlow-Engine/actions/runs/42281030503) was observed on the exact source head `1de5d818af6c5298506b382d15372645c36b6638` and was **pending** at the time of this update.
- The documentation-only commit that adds this section does not alter runtime source. The resulting new PR head must still be checked for its own CI result.
- No local build/test execution or physical-device validation is claimed.
- No update to `main`, merge, release publication, or device flashing was performed.

**Status remains: NOT COMPLETE.** The remaining acceptance gate is a successful complete CI run on the resulting latest PR head, followed by physical-device validation and a final audit of every mutation path and controller interaction. Do not mark PR #7 ready or merge it solely on the basis of source-level review.



## 12. CI-driven correction — no-exceptions toolchain compatibility

**Audit basis before this note:** `5c334c5f6b11a813ec29e5af990b3f12c58a80a6`.

### CI result and root cause

Workflow run [42281445106](https://github.com/mystivara-creator/CoreFlow-Engine/actions/runs/42281445106) failed on that exact head. Rust, WebUI and Module validation passed; Host C++, Android ARM64 and ASan/UBSan failed during compilation.

Two introduced compile defects were confirmed from the job logs:

1. The production C++ toolchain explicitly compiles with `-fno-exceptions`. A `try/catch` block in the coordinator and a test callback that used `throw` therefore failed to compile.
2. Clang/Android NDK rejected the placement of the `[[deprecated]]` attribute before the class declaration.

### Corrections applied

- Removed exception syntax from the coordinator. The callback contract now explicitly requires non-throwing handlers; failed recovery is represented by a normal `false` acknowledgement, preserving the lease and closed admission.
- Replaced the throwing callback test with a non-throwing handler that returns `false`, testing the supported fail-closed contract under `-fno-exceptions`.
- Moved the deprecation attribute to the class declaration position accepted by Clang/NDK.
- Adjusted the CPUFreq foreign-baseline regression so an OEM-retired CPU domain does not incorrectly require an independent GPU domain operation to fail.

### Current validation

- Run [42281951246](https://github.com/mystivara-creator/CoreFlow-Engine/actions/runs/42281951246), on an intermediate source head, was cancelled when a newer commit arrived before its full matrix completed.
- Run [42281996391](https://github.com/mystivara-creator/CoreFlow-Engine/actions/runs/42281996391) was created for the corrected CPUFreq regression head `014efe418bd5f4b93f0bd44fbd949c2a0fc38524`; it was pending when this note was written.
- The exact latest PR head must receive a complete successful workflow result before the code can be considered CI-validated.
- No local compiler/test execution or physical-device validation is claimed.

**Status remains: NOT COMPLETE / DO NOT MERGE.** Reinspect the next exact-head CI result and address all additional compiler, test, sanitizer and Android packaging failures before moving PR #7 out of Draft.

## 13. Implementation update — exact-head CI recovery and remaining acceptance gates

**Source head validated:** `65960da3303ab57a9a984030cf5b822468937aaf`  
**Base:** `5ccc5a23598471833fc058f61e45ae9a13b74bf7`

### Exact-head validation

GitHub Actions run [42282266384](https://github.com/mystivara-creator/CoreFlow-Engine/actions/runs/42282266384) completed with **success** on the exact source head above. All seven jobs passed:

- Host C++ tests, including the complete CTest suite.
- Android ARM64 production package build and module assembly validation.
- Rust tests and Clippy.
- ASan and UBSan build plus sanitized test execution.
- WebUI tests, type-check, and production build.
- Module lifecycle and shell validation.
- Aggregate CoreFlow CI gate.

The CPUFreq foreign-ownership regression was corrected so the aggregate CPU/GPU policy call correctly reports failure when CPU policy is rejected, while an independently valid GPU operation may still succeed and retain its own lease. The test verifies GPU emergency rollback and preserves the foreign CPU value until its explicit owner restores it.

### Correction to earlier audit wording

This production build uses `-fno-exceptions`. Therefore the earlier Section 11 wording that described callback exceptions as contained is not an accurate description of the final implementation. Exception handling is not used: preemption handlers are required to be non-throwing and must communicate unsuccessful recovery by returning `false`. A failed acknowledgement does not release the lease and does not reopen emergency admission. Section 12's description of the final supported callback contract supersedes the earlier wording.

### Remaining blockers — system-wide audit is still incomplete

Passing CI validates this candidate's host tests, sanitizers, Rust, WebUI, module checks, and Android ARM64 compilation/package assembly. It does **not** close these remaining acceptance gates:

- HardwareLongevityManager has foreign-journal protections, but its battery-current and storage mutations are not yet fully mediated by coordinator-owned PowerLongevity and BlockIo leases with transaction-scoped callbacks and verified release.
- Cross-controller multi-domain acquisition, partial-acquisition cleanup, controller disappearance, and bounded timeout/recovery policy still need explicit integration coverage.
- Vulkan node semantics and allowed values must be validated from discovered vendor/device capability evidence; path existence alone is insufficient.
- The complete mutation-path inventory must be rechecked against the final tree, including all direct sysfs/procfs, resetprop, Rust bridge, and lifecycle paths.
- Physical-device validation is still required for supported firmware/device combinations. No device behavior or thermal safety is inferred from CI.

**Status: CI-VALIDATED CANDIDATE, NOT SYSTEM-WIDE OWNERSHIP COMPLETE, NOT READY FOR 3.1.0 FREEZE.** Keep PR #7 Draft. Do not merge, publish a release, or flash a device based only on this CI result.

## 14. Audit correction — HardwareLongevityManager coordinator integration

A final source inspection of the current branch confirms that the HardwareLongevityManager integration blocker listed in Section 13 is **already implemented** in the candidate tree and must not be reported as outstanding.

The current implementation in `include/coreflow/hardware_longevity.hpp` and `src/kernel/hardware_longevity.cpp`:

- Registers a per-instance preemption handler.
- Uses separate `PowerLongevity` and `BlockIo` domain leases with transaction IDs.
- Rolls battery-current and storage-weight paths back independently.
- Verifies BaselineStore state and node readback before releasing each lease.
- Rejects foreign or unresolved journal ownership rather than adopting it as an OEM baseline.
- Preserves lease/recovery state when rollback cannot be verified.

`tests/test_longevity.cpp` exercises battery and storage lease acquisition, emergency preemption, baseline restoration, foreign-journal rejection, and preservation of an OEM thermal-service update.

Accordingly, the HardwareLongevityManager coordinator integration is **implemented and host-tested**; it is not a remaining blocker. The remaining system-wide gates are cross-controller multi-domain/partial-acquisition and disappearance/timeout recovery coverage, full final write-path audit, Vulkan semantic validation against actual device capability evidence, and physical-device validation. This correction does not change the CI result or claim that the remaining gates are complete.



## 11. Implementation update — path-scoped recovery and controller integration

**Current remediation head at this update:** `dc447993a09044bb33c5590e8e3b6bfdf42d03c2`.

### BaselineStore transaction isolation

- Runtime mutation admission failures no longer invoke the global `rollback_locked()` sweep. Invalid input, unreadable nodes, and write/journal failures are scoped to the path being operated on.
- Added `rollback_path_locked()` for single-resource recovery. Global rollback remains reserved for explicit recovery/safety sweep paths.
- Added regression coverage proving a rejected operation on one path does not reset unrelated active paths.
- AutonomousTuner now inspects the journal after a failed step and includes any still-modified resource in reverse-order recovery. Optional failed steps are also explicitly restored if their path remains journaled as active.

### Emergency admission barrier

- `begin_emergency_recovery()` closes new lease admission before emergency callbacks are dispatched.
- New lease attempts are rejected while recovery is active, including higher-priority requests for otherwise-free domains.
- Admission reopens only after every active lease has acknowledged rollback and the BaselineStore journal has no active mutations.
- Failed/missing callback acknowledgement retains the original owner and lease. Regression tests cover admission closure and unresolved recovery.

### Controller identity, partial failure, and cross-controller isolation

- FrequencyController now uses a per-instance handler identity rather than one shared static name. Failed writes are evaluated against both actual readback and BaselineStore ownership; unreadable or unresolved nodes retain recovery ownership.
- CgroupV2Controller restores its existing transaction if a later node write fails, and releases a lease only after verified rollback.
- HardwareLongevityManager now registers an acknowledged preemption handler for separate PowerLongevity and BlockIo transactions. Battery and storage rollback are verified before their respective leases are released.
- Existing UclampGovernor and VulkanController callback integrations remain in place.
- New CgroupV2/ FrequencyController integration coverage verifies that both compete for the CpuPolicy domain and that the cgroup mutation proceeds only after frequency rollback completes.
- Controller-specific tests cover battery and storage preemption, UClamp emergency rollback, Vulkan emergency rollback, frequency partial failures, and cgroup partial failures.

### Daemon lifecycle operations

- Replaced the silent lifecycle write helper with an allowlisted operation for the known cgroup membership targets and `/proc/self/oom_score_adj`.
- The operation rejects non-allowlisted paths, rejects symlink final components, checks complete write length, reopens the target, and requires readback confirmation.
- Failed migration or OOM adjustment is logged as unverified rather than reported as success. These lifecycle writes remain separate from adaptive optimization leases.

### Remaining acceptance gates

- The coordinator intentionally retains a lease and keeps admission closed when an owner cannot acknowledge rollback. There is no automatic timeout-based lease theft; operational recovery after controller disappearance still requires an explicit recovery/restart path and must be validated.
- The full write-path scan must remain part of acceptance; this update scanned the current C++/Rust/shell source tree for direct write primitives, but static searches cannot prove the absence of indirect writes in dynamically loaded vendor code.
- The latest verified full CI result must correspond to the exact final PR head. A prior green run is not evidence for a later commit.
- Physical-device validation remains mandatory for supported Xiaomi/HyperOS targets. Host CI cannot validate actual sysfs semantics, OEM thermal arbitration, or charging behavior.

**Status: NOT COMPLETE.** These changes materially reduce cross-controller rollback risk but do not authorize merge or the 3.1.0 freeze until the remaining acceptance gates are closed.
