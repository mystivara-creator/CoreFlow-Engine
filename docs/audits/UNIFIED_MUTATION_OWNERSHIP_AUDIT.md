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
