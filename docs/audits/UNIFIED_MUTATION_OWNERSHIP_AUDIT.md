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

## 15. Exact-head correction — CMake test linkage and multi-domain acquisition coverage

**Source head validated:** `82f30edc189727ec1d2623dc01548175aa9a35f0`  
**Base:** `5ccc5a23598471833fc058f61e45ae9a13b74bf7`

### CI failure and correction

- Workflow run [42284168051](https://github.com/mystivara-creator/CoreFlow-Engine/actions/runs/42284168051) failed on the preceding head because `test_cgroup_v2` began exercising `FrequencyController` but its CMake target did not include `src/kernel/frequency_controller.cpp`.
- Added the missing implementation source to the `test_cgroup_v2` executable. This was a test-target linkage defect, not a production-daemon link failure.
- Workflow run [42284536538](https://github.com/mystivara-creator/CoreFlow-Engine/actions/runs/42284536538) passed all seven jobs on the corrected source head `e0ff3c0a7ea82c32bb272fcd72f8ad0ca953cc0d`.

### Multi-domain partial-acquisition regression

- Extended `tests/test_cgroup_v2.cpp` to reserve the `GpuDevfreq` domain under a separate test owner while leaving CPU/GPU frequency nodes at their discovered baselines.
- A second `FrequencyController` discovers both domains and requests a combined operation. CPU lease acquisition succeeds, GPU lease acquisition is rejected because the test owner holds it, and the operation must return failure before writing either node.
- The test asserts that the temporary CPU lease is released, the original GPU reservation remains the only active lease, both frequency nodes remain unchanged, and BaselineStore has no active mutation.
- The explicit cleanup then releases the test reservation and verifies that the coordinator has no remaining leases.
- The test-source scope correction is included in the validated head `82f30edc189727ec1d2623dc01548175aa9a35f0`.

### Exact-head validation

Workflow run [42285053683](https://github.com/mystivara-creator/CoreFlow-Engine/actions/runs/42285053683) completed with **success** on exact source head `82f30edc189727ec1d2623dc01548175aa9a35f0`. All seven jobs passed:

- Host C++ tests: all 13 CTest tests passed, including the expanded `CgroupV2OwnershipTest`.
- ASan and UBSan.
- Android ARM64 production package.
- Rust validation.
- WebUI validation.
- Module validation.
- Aggregate CoreFlow CI gate.

### Corrected remaining acceptance gates

The CPU/GPU partial-acquisition path now has explicit host regression coverage. This does not close every multi-domain failure mode or establish physical-device behavior. Remaining gates are:

- Controller disappearance while the daemon remains alive: unresolved leases intentionally remain held and emergency admission remains closed. No automatic timeout-based lease theft is permitted. The controlled recovery/restart procedure still needs operational validation.
- Additional concurrency coverage for simultaneous controller destruction, preemption dispatch, and multi-path rollback under real scheduling pressure.
- Vulkan node semantics and accepted values require evidence from actual device/vendor capability discovery; host path fixtures are not sufficient.
- The final mutation-path inventory must continue to be checked against direct sysfs/procfs, lifecycle, resetprop, Rust bridge, and dynamically loaded vendor paths. Static source scanning cannot prove behavior of opaque vendor code.
- Physical-device validation remains mandatory for supported Xiaomi/HyperOS firmware combinations, including OEM arbitration, thermal behavior, and rollback after interrupted writes.

**Status: EXACT-HEAD CI-VALIDATED CANDIDATE, NOT SYSTEM-WIDE OWNERSHIP COMPLETE, NOT READY FOR 3.1.0 FREEZE.** Keep PR #7 Draft. Do not merge, publish a release, or flash a device until remaining operational and device-validation gates are independently reviewed.

## 16. Controller identity lifetime hardening

**Source head validated:** `e49d389493709f9649450d80f919aa56b176b683`  
**Base:** `5ccc5a23598471833fc058f61e45ae9a13b74bf7`

### Finding

Using an object address (`this`) as a controller identity is unique only while two objects occupy different addresses. After a controller is destroyed, an allocator may reuse its address. If the previous controller left an unresolved lease because rollback could not be verified, a replacement instance could receive the same controller key. Per-instance transaction counters can also restart from the same initial value. That makes address-derived identity weaker than the coordinator's lifetime ownership contract.

### Correction

- Added `CentralMutationCoordinator::make_controller_identity()`, backed by a process-wide atomic monotonic counter.
- Migrated `AutonomousTuner`, `CgroupV2Controller`, `FrequencyController`, `HardwareLongevityManager`, and `UclampGovernor` from address-derived keys to non-reusable process-lifetime identities.
- Kept the diagnostic prefix format (`ControllerType@N`) stable, so existing log filtering and owner-prefix checks remain compatible.
- Kept `VulkanController` on its fixed identity because its constructor is private and the class is an enforced singleton.
- Added regression assertions that two identities for the same controller type are distinct.

This prevents an object-address reuse from silently making a newly constructed controller appear to be the owner of an older unresolved lease. It does not automatically recover that unresolved lease; the coordinator remains fail-closed until explicit recovery.

### Exact-head validation

Workflow run [42286398292](https://github.com/mystivara-creator/CoreFlow-Engine/actions/runs/42286398292) completed with **success** on exact source head `e49d389493709f9649450d80f919aa56b176b683`. All seven jobs passed:

- Host C++ tests: 13/13 CTest tests passed.
- ASan and UBSan: 13/13 CTest tests passed.
- Android ARM64 production package.
- Rust validation.
- WebUI validation.
- Module validation.
- Aggregate CoreFlow CI gate.

### Remaining acceptance gates

- Controlled recovery after controller disappearance while the daemon remains alive still needs an explicit operational contract and validation. No timeout-based lease theft is permitted.
- Expand concurrency coverage for overlapping destruction, callback dispatch, multiple owned paths, and emergency recovery under real scheduler pressure.
- Validate Vulkan per-node semantics and accepted values from device/vendor capability evidence; path presence is insufficient.
- Complete the final source/write-path audit, including indirect and dynamically loaded vendor paths.
- Perform physical-device validation on supported Xiaomi/HyperOS firmware combinations. Host CI cannot establish OEM arbitration or thermal safety.

**Status: EXACT-HEAD CI-VALIDATED CANDIDATE, NOT SYSTEM-WIDE OWNERSHIP COMPLETE, NOT READY FOR 3.1.0 FREEZE.** Keep PR #7 Draft and do not merge, publish a release, or flash a device until all remaining acceptance gates are reviewed.

## 17. Detached-controller recovery — durable orphan reconciliation and monitor retry

**Source head validated:** `de67bbb2e116e7a086ce87588217d8a0b408833d`  
**Base:** `5ccc5a23598471833fc058f61e45ae9a13b74bf7`

### Lifecycle state and ownership retention

- `DomainLease` now records `owner_detached`. The coordinator sets it only when `unregister_preemption_handler()` has completed its wait for all in-flight callbacks and the controller can no longer acknowledge rollback.
- Unregister does not release the lease. The active lease remains visible and continues to block its domain.
- Controller identities are retired for the rest of the process lifetime. Registration and acquisition attempts using a retired identity are rejected, including attempts after orphan reconciliation. A new controller must receive a new process-lifetime identity.
- Orphan retirement also requires the lease to have an explicit `preemption_requested` marker and no remaining registered handler. An active owner that refuses or has not yet acknowledged preemption cannot be treated as detached.

### Controlled recovery procedure

1. Emergency recovery closes mutation admission and requests preemption from all active owners.
2. If any still-live owner has not acknowledged, recovery stops. The global journal sweep is withheld and admission stays closed.
3. Detached-owner leases are eligible for reconciliation only after all remaining live owners have acknowledged.
4. `BaselineStore::verify_and_persist_clear_state()` checks that no tracked node is marked modified and atomically rewrites an empty CFJ4 journal, including file and directory durability checks.
5. Only after that verification succeeds may the coordinator retire detached-owner leases. Emergency admission reopens only when no active lease remains.
6. The daemon monitor checks for detached owners and for an already-active recovery barrier on each tick. It retries incomplete recovery without a timeout-based lease steal; if recovery remains unresolved, mutation admission remains closed and the next monitor tick retries.

The durable empty-journal check is deliberately stronger than an in-memory `has_active_mutations()` check. A prior rollback can clear in-memory flags while a stale write-ahead record remains on disk after a persistence failure; the explicit empty-journal write must succeed before orphan leases can be retired.

### Regression coverage

- A live controller that returns a failed preemption acknowledgement cannot be released by the orphan API, even if the caller presents a clear BaselineStore.
- Simulated controller disappearance unregisters its callback while preserving its active lease; the lease is marked detached and remains active.
- Orphan retirement is rejected while BaselineStore still has a mutation and while the durable empty-journal check fails.
- After verified path rollback and durable empty-journal persistence, the orphan lease is retired, the admission barrier reopens, and a controller with a fresh identity can acquire the domain.
- A detached identity cannot register a replacement callback or acquire a new lease after recovery.
- Baseline journal tests prove a symlinked state directory fails closed and that durable empty-journal verification succeeds only after the state path is repaired.

### Exact-head validation

Workflow run [42290529938](https://github.com/mystivara-creator/CoreFlow-Engine/actions/runs/42290529938) completed with **success** on exact source head `de67bbb2e116e7a086ce87588217d8a0b408833d`. All seven jobs passed:

- Host C++ tests: 13/13 CTest tests passed.
- ASan and UBSan: 13/13 CTest tests passed.
- Android ARM64 production package.
- Rust validation.
- WebUI validation.
- Module validation.
- Aggregate CoreFlow CI gate.

### Remaining acceptance gates

- The detached-owner recovery path is implemented and host-tested. It still requires physical/operational validation that the daemon monitor observes a real controller teardown on supported Android firmware and that OEM-owned node changes are preserved during the global BaselineStore sweep.
- Expand concurrency testing beyond deterministic callback/unregister and admission tests to stress overlapping controller destruction, preemption dispatch, multi-path rollback, and monitor retries under real scheduler pressure.
- Validate Vulkan node semantics and accepted values from actual per-device/vendor capability evidence; path presence alone is insufficient.
- Complete the final source/write-path audit, including indirect and dynamically loaded vendor paths.
- Perform physical-device validation on supported Xiaomi/HyperOS firmware combinations, including OEM arbitration, thermal behavior, and rollback after interrupted writes.

**Status: EXACT-HEAD CI-VALIDATED CANDIDATE, DETACHED-OWNER RECOVERY IMPLEMENTED, NOT SYSTEM-WIDE OWNERSHIP COMPLETE, NOT READY FOR 3.1.0 FREEZE.** Keep PR #7 Draft. Do not merge, publish a release, or flash a device until remaining concurrency, source-audit, Vulkan-semantic, and device-validation gates are reviewed.

## 18. Single-flight preemption dispatch and immutable callback registration

**Source head:** `f99ab0671e5b2c457c94a398bf3ddee1634a3d5f`  
**Base:** `5ccc5a23598471833fc058f61e45ae9a13b74bf7`

### Race identified

The coordinator previously allowed two concurrent emergency/preemption paths to observe the same active lease with `preemption_requested=true` and both copy/invoke the owner's callback. Controller mutexes reduce the likelihood of overlapping writes, but duplicate rollback entry is unnecessary and leaves callback semantics dependent on every controller implementing idempotent re-entry correctly.

The callback registry also allowed an idle registration under an existing identity to be silently replaced. Since controller identities are process-lifetime ownership tokens, a second registration should not replace the callback associated with the current owner.

### Correction

- Added a per-lease `preemption_dispatch_in_flight` marker guarded by the coordinator mutex.
- A dispatch claims the exact domain/controller/transaction lease before calling the handler outside the lock. Concurrent dispatchers for that same lease return without invoking a second callback.
- On successful acknowledgement, the exact lease is released atomically under the coordinator mutex. This closes the small interval between acknowledgement and release in which a second dispatcher could otherwise enter.
- On failed acknowledgement, the in-flight marker is cleared and the lease remains active/preemption-requested so a later daemon monitor or emergency pass can retry.
- Handler registration now rejects any already-registered controller identity instead of replacing an existing handler. Unregister remains responsible for retiring the identity after callbacks drain.

### Regression coverage

- Duplicate callback registration under one controller identity is rejected; the original callback remains the only callback invoked.
- A deterministic two-thread test blocks the first rollback callback while a second emergency-preemption pass runs. The callback count remains exactly one, the active lease exposes its in-flight marker, and the first callback's successful acknowledgement releases the lease.
- The existing failed-acknowledgement test now invokes a second emergency pass and verifies that retry occurs after the in-flight marker clears.
- The existing unregister/callback lifetime test remains in place to verify teardown waits for copied callbacks.

### CI status

Workflow run [42291892269](https://github.com/mystivara-creator/CoreFlow-Engine/actions/runs/42291892269) was created for exact head `f99ab0671e5b2c457c94a398bf3ddee1634a3d5f`. At the time this audit entry was written, the run was still pending; it is not recorded as passed. The preceding source commit run [42291881468](https://github.com/mystivara-creator/CoreFlow-Engine/actions/runs/42291881468) was still in progress and is not treated as validation for the final head.

### Vulkan semantics review — evidence boundary

- ARM's Mali driver ABI documents the `power_policy` sysfs attribute and lists `coarse_demand` and `always_on` as supported values, with the active policy bracketed in readback. See [Android common kernel Mali sysfs ABI](https://android.googlesource.com/kernel/google-modules/gpu/+/3372cb0ffe779efd9b7507174c61d0e48822b5e2/mali_kbase/Documentation/ABI/testing/sysfs-device-mali). This supports the token vocabulary, but does not replace per-device discovery and OEM regression testing.
- At the time of this earlier review, the implementation still applied numeric targets to Qualcomm KGSL `devfreq/adrenoboost`, `bus_split`, and `throttling`. No normative, version-matched ABI had been established for the target device. Section 21 supersedes that implementation and disables these writes while retaining read-only observations.

### Remaining acceptance gates

- Exact-head CI for `f99ab0671e5b2c457c94a398bf3ddee1634a3d5f` must complete successfully; no result from an earlier source head substitutes for it.
- Add repeated stress coverage for controller destruction racing with dispatch, multi-domain owners, multiple simultaneous leases, and monitor retry under varied scheduling. The new two-thread test is deterministic regression coverage, not a substitute for long-running stress or device execution.
- Resolve the Adreno KGSL semantic evidence gate above before authorizing those mutations for any target firmware.
- Complete the final source/write-path inventory and perform physical Xiaomi/HyperOS validation, including OEM arbitration and interrupted-write recovery.

**Status: PREEMPTION DUPLICATE-DISPATCH RACE ADDRESSED IN SOURCE, EXACT-HEAD CI PENDING, VULKAN ADRENO WRITE SEMANTICS AND DEVICE VALIDATION OPEN. PR #7 MUST REMAIN DRAFT; NOT READY FOR 3.1.0 FREEZE.**

## 19. Lease-retirement state normalization

**Source head:** `eba8e0b601ba2f5130c21032607804c33a621b61`

A follow-up consistency pass found two state-normalization details after introducing `preemption_dispatch_in_flight`:

- At this point in the branch history, the bulk controller release helper cleared the in-flight marker in addition to the active, transaction, and preemption fields. That unscoped API was subsequently removed in Sections 20 and 21.
- Detached-owner reconciliation now explicitly rejects a lease whose callback-dispatch marker is still set, and clears that marker when the orphan record is retired.

This is defense in depth: unregister already waits for registered callbacks to drain before marking the owner detached, but orphan eligibility now independently encodes the no-callback-in-flight requirement rather than relying only on the unregister implementation.

Workflow run [42292156748](https://github.com/mystivara-creator/CoreFlow-Engine/actions/runs/42292156748) was created for exact source head `eba8e0b601ba2f5130c21032607804c33a621b61` and was pending at the time of this entry. Earlier runs for intermediate heads `f99ab067` and `a375149a` were cancelled when subsequent commits arrived; they do not count as validation.

**Status: SOURCE STATE NORMALIZATION APPLIED; EXACT-HEAD CI PENDING.**
## 20. Mutation write-path inventory and removal of unscoped lease release

**Source implementation head:** `e0439d8b0cc06db62794d98505079cf9cb38270e`

### Write-path inventory

The source review separated kernel-control mutations from daemon lifecycle/state-file operations:

| Surface | Observed write path | Ownership / verification |
|---|---|---|
| Sysfs/procfs tuning nodes | `BaselineStore::write_raw_node()` | Canonical mutation path; no-follow open, descriptor type and inode identity checks, write-ahead CFJ4 persistence, readback verification, path-scoped rollback |
| AutonomousTuner | `BaselineStore::apply_mutation()` | Mutations are journaled and tracked through the coordinator's transaction/domain ownership |
| CPU/GPU frequency | `BaselineStore::apply_mutation()` | CPU and GPU domains acquire separate leases; rollback verifies the restored baseline and journal ownership |
| Cgroup v2 CPU policy | `BaselineStore::apply_mutation()` | CPU-policy lease, transaction path set, verified multi-node rollback; task migration remains disabled rather than using an unverified direct membership write |
| UClamp | `BaselineStore::apply_mutation()` | CPU-policy/cgroup lease; paired min/max writes are rolled back together and checked by readback |
| Battery charge / storage pacing | `BaselineStore::apply_mutation()` | Separate PowerLongevity and BlockIo leases; charging control is restricted to the documented `constant_charge_current_max` node and never raises the OEM limit |
| Vulkan GPU policy | `BaselineStore::apply_mutation()` | GpuDevfreq lease; active node set is rolled back and verified before lease release |
| Daemon cgroup / OOM lifecycle | `write_sys_node_verified()` | Exact target allowlist, `O_NOFOLLOW`, complete write, reopen/readback; intentionally separate from adaptive mutation domains |
| Daemon PID and Vulkan layer state | Direct file writes | State/coordination files, not kernel tuning controls; PID file uses exclusive flock and fsync; Vulkan state uses a temporary file and rename |
| Reactor shutdown signal | `write(eventfd)` | Event notification only, not a system policy mutation |
| Xiaomi boot properties | `resetprop -n` in `post-fs-data.sh` | Xiaomi/Redmi/POCO brand gate, explicit mode validation, property readback against requested value, failure logged and ignored rather than reported as success |
| Rust bridge / Vulkan layer / power bridge / telemetry helpers | No direct mutation primitive found in the reviewed files | Operate as bridge, rendering, or telemetry surface; adaptive node writes remain routed through the C++ controller/BaselineStore layer |

The shell install/service scripts also write boot tokens, attempt counters, safety markers, module files, and logs. These are installer/boot-state operations, not adaptive kernel tunables. Their state-directory and file replacement paths are subject to the script's symlink checks and temporary-file/rename pattern; they still require target-firmware installation testing.

### Unscoped lease release API removed

- The API could mark every active lease belonging to a controller inactive without a domain/transaction identifier, rollback acknowledgement, or BaselineStore verification.
- The first removal exposed one internal caller in `AutonomousTuner::rollback_active()` through Host C++ CI. That caller was corrected to release only `domain_for_candidate(current_candidate_id_)` plus the exact candidate transaction ID, and only after all tracked paths passed rollback/readback verification.
- Removed the unscoped declaration and implementation. Normal release is transaction-scoped through `release_lease(domain, controller, transaction_id)`; the detached-owner path remains a distinct, explicitly verified global recovery operation.
- The CI compile failure at source head `deb3c3fdaf49adb100898180ca18eca973f517f8` was therefore a real integration regression, not dismissed as a transient runner issue. The fix is in source head `e0439d8b0cc06db62794d98505079cf9cb38270e` and must be validated by a subsequent exact-head CI run.

### Scope and residual limitations

- This inventory covers the repository's current tracked C++/Rust/module shell implementation, not behavior inside opaque vendor services, dynamically loaded GPU drivers, root-manager internals, or firmware daemons.
- File writes used for logs, configuration, PID coordination, and Vulkan layer state are not sysfs/procfs tunable writes, but their state-directory hardening and crash durability remain independent review topics.
- Device-level ownership is not proven by host fixtures: Xiaomi thermal services and kernel drivers may change a node between read, write, and readback. The engine must preserve OEM values when ownership/readback changes and leave unknown semantics observation-only.
- The Qualcomm KGSL `adrenoboost`, `bus_split`, and `throttling` mutation semantics remain unapproved for production enablement until matched to the target kernel source/ABI and validated on target firmware.

### Exact-head validation

The source removal/follow-up fix commit is `e0439d8b0cc06db62794d98505079cf9cb38270e`. The earlier CI run `42292602289` failed compilation because the original `AutonomousTuner::rollback_active()` call remained; it is now changed to exact domain/transaction release. The following audit-document commit and its exact-head CoreFlow CI must complete before this PR head is recorded as validated. No local build is claimed.

**Status: INTERNAL UNOWNED BULK-RELEASE API REMOVED; REVIEWED WRITE SURFACES CLASSIFIED; EXACT-HEAD CI AND PHYSICAL DEVICE VALIDATION STILL REQUIRED.**
## 21. Qualcomm KGSL controls downgraded to observation-only; strict transaction release

**Source/test heads:**
- `bf7b4ee68779c04ad6a54ed4c7ae858b1bdee975` — disable unverified Qualcomm KGSL mutations
- `164cab8234e5410e9b0583e591302cfb7f18a160` — clarify raw KGSL observation fields
- `29599e0ec27ecef62b946e624f74db07eee7f249` — observation-only regression tests
- `71234465d43f34e945a5fdabfb5a11a9b52b4ff1` — reject empty transaction IDs
- `cf392676a93c7db1ffeb3d5e9c8a5868c36b95b9` — test empty transaction ID rejection

### Device-specific ABI evidence boundary

- The public [kunzite kernel development repository](https://github.com/xiaomi-kunzite/android_device_xiaomi_kunzite-kernel) contains extracted kernel images and device trees; it is not a version-matched KGSL driver source tree for the target HyperOS kernel.
- The Xiaomi GPL source request for [Redmi Note 15 5G / POCO M8 5G, codename kunzite](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/issues/41125) remains open in the repository's issue tracker as reviewed for this audit.
- Upstream Qualcomm KGSL source shows `_throttling_store()` delegates to `_pwrctrl_store(..., ADRENO_THROTTLING_CTRL)`, while `_throttling_show()` reads the driver's `pwrctrl_flag`. That is not evidence that this node toggles the OEM hardware thermal-protection mechanism. The previous code comment implying it preserved/controlled OEM thermal throttling was therefore not defensible without the target driver source.
- Upstream evidence for a related KGSL `bus_split` implementation describes a boolean bus-control setting, but that generic implementation does not establish the exact accepted semantics or safety contract of the target firmware's vendor driver. Likewise, no normative, version-matched definition for `adrenoboost` and its `0/1/2` values was established.

### Source correction

- `VulkanController::discover_gpu_kernel_surface()` continues to read `adrenoboost`, `bus_split`, and `throttling` into diagnostic snapshot fields. These fields are explicitly documented as raw observations and do not authorize mutation.
- `VulkanController::apply_workload_policy()` now treats Qualcomm Adreno KGSL as observation-only and returns a safe no-op without acquiring a GPU policy lease or writing any of the three nodes.
- If an earlier in-process transaction still owns mutation paths when this policy path is reached, it first attempts `rollback_gpu_transaction_locked()` and keeps the lease/recovery state if verified rollback fails.
- ARM Mali `power_policy` remains separately gated to the documented `coarse_demand`/`always_on` token set; its evidence still does not replace actual device validation.

### Regression tests

- The synthetic Adreno fixture now asserts sustained-load and balanced requests leave `adrenoboost`, `bus_split`, and `throttling` unchanged and create no GPU mutation lease.
- A pre-existing journaled external change to `adrenoboost` is preserved; the observation-only policy does not adopt or overwrite it.
- Existing Mali test continues to exercise `always_on` to `coarse_demand` and emergency rollback to its OEM baseline.
- `release_lease()` now rejects an empty controller identity or transaction ID and compares the exact transaction ID unconditionally. Removing the default argument in the public declaration was not enough by itself because the implementation previously treated an explicitly empty string as a wildcard.
- Multi-domain regression coverage proves a CPU release cannot affect a GPU lease for the same controller, and an empty transaction ID leaves both leases active.

### Validation and remaining gates

- The previous exact-head CoreFlow CI run [42293276595](https://github.com/mystivara-creator/CoreFlow-Engine/actions/runs/42293276595) passed for source/test head `ae597ac66f19708d7d1cfd72b0c97af7306550b4` before the new KGSL observation-only and strict empty-ID changes. It does not validate this section's new changes.
- Exact-head CI is required for the final head containing `bf7b4ee`, `164cab8`, `29599e0`, `7123446`, and `cf39267`.
- Long-running concurrent stress, target Xiaomi/HyperOS tests, thermal/OEM arbitration, interrupted-write recovery, and actual vendor KGSL source matching remain open. Adreno policy writes must stay disabled until the ABI contract is verified.

**Status: UNVERIFIED ADRENO KGSL WRITES DISABLED; EMPTY TRANSACTION-ID WILDCARD RELEASE CLOSED; EXACT-HEAD CI AND TARGET-DEVICE VALIDATION PENDING. PR #7 MUST REMAIN DRAFT; NOT READY FOR 3.1.0 FREEZE.**
## 22. Controller-wide single-flight callback dispatch

**Source/test heads:**
- `6ea5fc789ece2bcab1c6c84c74ff841fcc978694` — add handler-level dispatch state
- `de8636cc57d72e151924f0765c8dabcb39993865` — serialize callback dispatch per controller identity
- `dd8a317f4b481a0bd6b7157fe6fec6fb6a4132cc` — deterministic multi-domain regression test

### Race addressed

Per-lease single-flight prevented duplicate callbacks for the same transaction, but did not prevent the same callback object from being invoked concurrently for two different leases owned by one controller. Controllers with more than one domain can legitimately hold multiple leases; their callback object must not be assumed reentrant or thread-safe.

### Correction

- Added `PreemptionHandlerEntry::callback_dispatch_in_flight`, guarded by the coordinator mutex.
- `dispatch_preemption()` now claims both the exact lease and the controller callback before releasing the coordinator lock and invoking the handler.
- While one callback for a controller identity is running, dispatch attempts for any other lease owned by that identity are skipped without setting a lease-dispatch marker. The owning emergency pass can continue to the next lease once the first callback returns; a later monitor/emergency pass remains able to retry any still-pending lease.
- Handler in-flight accounting is decremented and the controller-wide marker cleared after callback completion. Unregister still waits for all in-flight callbacks before retiring the identity.
- Successful callback acknowledgement remains scoped to the exact domain/controller/transaction. No other lease is released by the acknowledgement.

### Regression coverage

- A deterministic two-domain test gives one controller both a CPU-policy and GPU-devfreq lease.
- The CPU callback is blocked while a second emergency-preemption pass runs. The callback count stays at one, both leases remain present, and the GPU lease is not incorrectly marked in-flight.
- After the first callback returns, the original emergency pass processes the GPU lease serially; both exact transactions are acknowledged and released, with exactly two total callback invocations.
- This complements the earlier same-lease duplicate-dispatch test and multi-domain exact-release/empty-transaction-ID tests.

### CI status and remaining gates

- The preceding exact-head run [42293823121](https://github.com/mystivara-creator/CoreFlow-Engine/actions/runs/42293823121) passed all seven jobs at head `91a338323c811fd4bb9ff706ac8faa6865d349fd`. That head included the observation-only KGSL changes and strict empty-ID transaction release, but not this controller-wide callback change.
- Exact latest head `dd8a317f4b481a0bd6b7157fe6fec6fb6a4132cc` has a CI run pending/in progress and must complete before this additional concurrency change is marked validated.
- Repeated long-running scheduler stress, physical Xiaomi/HyperOS validation, target-specific KGSL source proof, and interrupted-write recovery remain open.

**Status: CONTROLLER-WIDE CALLBACK SINGLE-FLIGHT IMPLEMENTED WITH DETERMINISTIC MULTI-DOMAIN TEST; EXACT-HEAD CI PENDING. PR #7 REMAINS DRAFT; NOT READY FOR 3.1.0 FREEZE.**
## 23. Recovery admission waits for preemption callback drain

**Source/test heads:**
- `d30d9d3ac559f4eef64f955f426046b784a7c361` — require callback drain in recovery completion and orphan reconciliation
- `b8373e17ddd9dbb96bd3ec16ed0d998d6446f757` — deterministic early-release callback regression test

### Race addressed

A controller callback may complete its BaselineStore rollback and release its exact lease from inside the callback, then execute a small amount of callback epilogue before returning its acknowledgement. During that interval the active-lease list can be empty while the coordinator still has a callback copied and executing. Checking only active leases could therefore reopen mutation admission before callback execution has drained.

### Correction

- `complete_emergency_recovery()` now requires both zero active leases and zero in-flight preemption callbacks across the handler registry.
- `release_orphaned_leases_after_verified_recovery()` now checks that all callbacks are drained both before the durable BaselineStore clear-state verification and again after it. This is in addition to requiring every remaining active lease to be detached, preemption-requested, and free of lease-level dispatch.
- The checks use the coordinator mutex; callback invocation remains outside that mutex. Callback completion decrements in-flight accounting and notifies the condition variable.
- If callback execution remains in flight, the recovery barrier stays closed and the daemon's later monitor retry can complete recovery after the callback drains.

### Regression coverage

- The test callback releases its exact lease from inside the callback, signals that the lease list is empty, then blocks before returning its acknowledgement.
- While the callback is still executing, `active_leases()` is empty but `complete_emergency_recovery()` must return false and leave the emergency barrier active.
- After the callback is released and the dispatcher joins, recovery completion succeeds. This proves the barrier tracks callback lifetime, not only lease lifetime.

### Validation

- Exact preceding head `109cef2215906854450725b9af8e25a141ceaf9b` passed all seven CoreFlow CI jobs in [run 42294428826](https://github.com/mystivara-creator/CoreFlow-Engine/actions/runs/42294428826).
- Latest source/test head `b8373e17ddd9dbb96bd3ec16ed0d998d6446f757` has workflow run [42294756956](https://github.com/mystivara-creator/CoreFlow-Engine/actions/runs/42294756956) in progress; it is not yet recorded as passed.
- The documentation update following this section will require its own exact-head CI result. No local build or physical-device result is claimed.

**Status: EARLY ADMISSION REOPEN WINDOW CLOSED IN SOURCE; DETERMINISTIC TEST ADDED; EXACT-HEAD CI PENDING. PR #7 REMAINS DRAFT; NOT READY FOR 3.1.0 FREEZE.**