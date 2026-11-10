# P2-D.6 — Generic Linux Ceiling Provider and External Ownership Quarantine

## Device evidence motivating this change

The Redmi Note 15 5G report showed:

- `scaling_driver=qcom-cpufreq-hw` for policy0 and policy4.
- Four explicitly searched `qcom_lmh_*` attributes were absent from the inspected policy and cooling-device directories.
- CPU policy maximums were at 691200 kHz while hardware maxima were 1804800/2400000 kHz.
- Runtime logs demonstrated memory-domain trials and outcome-based rollback/retention, but did not demonstrate a CPU OPP transaction.
- The journal contained a boot identity record only; it did not establish an active CPU frequency transaction.

The empty ABI search is scoped evidence, not proof that every possible vendor interface is absent. In particular, it is not permission to infer an LMh ceiling from generic cooling state.

## Generic provider contract

`GenericLinuxFrequencyCeilingProvider` is a conservative fallback for explicitly supported non-Qualcomm cpufreq drivers:

- `cpufreq-hw`, `cpufreq-dt`, `acpi-cpufreq`, `intel_pstate`, and `amd-pstate`.
- It rejects `qcom-cpufreq-hw` unconditionally. A generic provider must not bypass an unavailable Qualcomm LMh contract.
- It requires readable `cpuinfo_min_freq`, `cpuinfo_max_freq`, `related_cpus`, and at least one readable generic thermal zone with a standard `passive`, `hot`, or `critical` trip point.
- Its ceiling is the kernel-reported static hardware maximum. It is not represented as a dynamic thermal/LMh cap.
- Thermal admission remains independently mandatory and is re-evaluated by the actuator. If Qualcomm-specific sensor names are unavailable on a non-Qualcomm platform, thermal admission requires at least two fresh, trip-backed compute/board sensors; a single generic sensor cannot grant admission. Discrete OPP discovery and target validation remain separate.
- The capability fingerprint binds driver identity, policy index, related CPUs, and hardware bounds. It does not claim cryptographic authenticity.

Generic trip points are not converted into MHz, and cooling-device `cur_state`/`max_state` are not translated into frequency. Dynamic platform limits require an independent kernel/vendor contract.

## External ownership / Power HAL behavior

A coordinator lease serializes CoreFlow controllers; it cannot prevent an unrelated kernel client or Android Power HAL from writing a sysfs node. The new ownership-loss state therefore provides quarantine, not forceful control:

1. The transactional actuator checks coordinator lease validity, node readback, and the BaselineStore audit record during outcome observation.
2. If an external writer changes the leased node, the lease is marked `external_ownership_lost` and preemption/retry admission is closed.
3. The actuator attempts its existing ownership-aware rollback. It must not overwrite a value it can no longer prove it owns.
4. If rollback/readback cannot be verified, the lease remains quarantined and recovery-required. It is not silently released.
5. The quarantine can be cleared only after the actuator verifies OEM baseline readback and journal ownership, or after the existing detached-owner recovery verifies that the durable journal is clear.

No periodic reassertion of `scaling_max_freq` is added. Reapplying a CoreFlow target against Power HAL can fight thermal policy, create oscillation, and obscure who owns the effective limit. Sysfs inotify is not assumed reliable for kernfs attributes; the existing bounded runtime observation/readback path is the authority check.

## Device qualification boundary

This change does not authorize CPU mutation on the reported Redmi device. Its driver is explicitly excluded from the generic fallback, and the searched LMh ABI was not found. Until a valid independent Qualcomm ceiling contract is exposed, CPU frequency mutation remains denied. Memory and other independently validated domains remain separately eligible.

The generic provider is a cross-platform capability expansion, not a universal guarantee that every Linux or Android kernel can safely raise CPU policy limits.
