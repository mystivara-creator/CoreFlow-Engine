# CoreFlow Kernel Knowledge Base and Evidence Policy

This is a compact, version-aware reference map for maintainers, not a runtime feed of copied kernel source. CoreFlow must not download or apply tunables by scraping documentation while the phone is running.

## Evidence layers

1. **Observed fact:** device properties, Android/API level, kernel release/ABI, SoC/board properties, filesystem interface presence, and read/write access. Missing properties stay unknown rather than being guessed.
2. **Policy authorization:** an explicit source-controlled rule for a specific supported resource. Discovery alone must never set this merely because a node is writable.
3. **Model inference:** a named semantic prior computes a bounded candidate from the current workload and fresh telemetry. This remains an estimate, not a proven benefit.
4. **Proven action:** a valid `MutationPermit`, committed baseline journal, bounded write and successful read-back.
5. **Measured outcome:** a subsequent observation window reports beneficial/neutral/regressed/unknown. A verified write is not itself proof of performance improvement.

## Current policy in this source tree

- `mutation_mode=observe`, `mutation_armed=false`, `allow_cpu_governor=no` remain the installed defaults.
- Production resource policy and the central `ResourceActuator` allow-list currently admit only seven exact `/proc/sys/vm/` paths with named `EffectModel` priors; all other target paths are denied for new resource writes. CPUFreq is a separate path and remains off by default (`allow_cpu_governor=no`).
- New writes to block queue nodes—`scheduler`, `nr_requests`, and `read_ahead_kb` under `/sys/block/.../queue/` and equivalent `/sys/devices/.../block/.../queue/` paths—are quarantined even if writable or proposed by a fallback candidate.
- Restores from a previously committed resource journal remain allowed so recovery from an older run can complete. A restore failure must remain visible and must never clear an unresolved journal.
- CPU governor mutation is a separate authority path controlled by `allow_cpu_governor`; it is not authorized by this resource policy.

## Why I/O queue controls are quarantined

These controls affect the kernel block layer rather than a private CoreFlow setting. Upstream kernel documentation describes `scheduler` as switching the active scheduler for a block device; `read_ahead_kb` controls filesystem read-ahead; and `nr_requests` controls how many block-layer requests may be allocated. Android storage stacks may expose virtual and physical devices with dependencies that are not represented by the current `IoDevice` model. Without dependency mapping and device-specific recovery evidence, writability and observed throughput are insufficient authorization.

This is a conservative safety decision for the current source, not a claim that all such controls are inherently unsafe. Re-enable any queue control only in a separately reviewed patch with dependency classification, supported-kernel scope, baseline/restore proof, bounded per-device values, and real-device soak/rollback evidence.

## Maintainer references

- [Android kernel overview](https://source.android.com/docs/core/architecture/kernel): Android kernels combine upstream LTS with Android-specific changes.
- [Android Common Kernels](https://source.android.com/docs/core/architecture/kernel/android-common): ACK branches carry Android-relevant backports and changes; do not infer behavior from a generic Linux version string alone.
- [Linux block queue sysfs](https://docs.kernel.org/block/queue-sysfs.html): semantics of queue nodes including scheduler, read-ahead, and request depth.
- [Linux VM sysctl](https://docs.kernel.org/admin-guide/sysctl/vm.html): documented meanings of VM tunables.

Reference pages are context, not proof of the exact vendor kernel implementation. Before authorizing a path, match the device's Android release, kernel branch/vendor changes, actual node semantics, and rollback behavior. Record the version/source used to justify a policy change in this file or the review notes.

## Required review checklist for a new tunable

- [ ] Exact target and resource class are explicit; unknown paths cannot be synthesized into production permission.
- [ ] Semantic bounds and direction are supported by the matching kernel documentation/source and the controller enforces the bounds.
- [ ] `policy_authorized` is separate from existence/readability/writability and cannot be granted by model score or experience memory.
- [ ] Baseline journal is committed before writing; immediate read-back verifies the exact requested value.
- [ ] Recovery handles stale/corrupt journal and missing/replaced device paths without erasing unresolved state.
- [ ] Regression tests cover denial, apply, verify failure, restore and rollback failure.
- [ ] The relevant Android/ACK/vendor kernel family is built and tested; real-device Observe and controlled trial evidence is recorded.
