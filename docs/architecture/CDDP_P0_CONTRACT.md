# CoreFlow Dynamic Device Profiling (CDDP) — P0 Contract

## Objective

Replace fragmented hardware discovery with one OS-derived profile consumed by
all runtime controllers. Profiling is read-only. A discovered node is not
automatically a safe actuator.

## Published data contract

- `DeviceProfileSnapshot` is published as `DeviceProfileHandle`
  (`std::shared_ptr<const DeviceProfileSnapshot>`).
- One controller decision/transaction pins one profile generation.
- A refresh creates a new snapshot; it never mutates a published snapshot.
- `generation`, monotonic capture time and a deterministic fingerprint make
  stale-profile detection explicit.
- Identity strings are descriptive. Decisions must use discovered topology,
  interfaces, units, constraints and evidence, never a guessed SoC tier.
- CPU membership is represented as exact CPU ID vectors. Do not reduce a
  non-contiguous set to first/last CPU IDs.

## Capability eligibility state machine

`AVAILABLE -> READABLE -> WRITABLE -> SEMANTICS_VERIFIED ->
CONSTRAINTS_VERIFIED -> MUTATION_ELIGIBLE`

Every transition requires new evidence. A failed check leaves the capability
at its last proven state and records a rejection reason. A writable permission
check alone can never advance a node beyond `WRITABLE`.

The final state requires:
1. Exact canonical node path and owning hardware domain.
2. Successful read and a known value representation.
3. A verified interface/driver ABI semantic identifier.
4. Explicit unit and accepted-value/range constraints.
5. A successful preflight against the current profile generation.

The state is a discovery-time eligibility result, not a mutation lease.
Controllers must still capture baseline, acquire the mutation-domain lease,
check current ownership, perform the write, and verify readback.

## Source policy

| Data | Preferred source |
|---|---|
| CPU possible/present/online sets | `/sys/devices/system/cpu/{possible,present,online}` |
| CPU policy membership and limits | `/sys/devices/system/cpu/cpufreq/policy*/` |
| GPU domains | `/sys/class/devfreq/*` and driver-specific interfaces |
| Thermal sensors | `/sys/class/thermal/` and `/sys/class/hwmon/` |
| Memory and pressure | `/proc/meminfo`, `/proc/pressure/*` |
| Cgroup topology | `/proc/self/mountinfo`, cgroup controller files |
| Device/SoC identity | Device Tree, `/sys/devices/soc0`, Android properties |
| Android API/runtime | system properties and documented Android runtime interfaces |

Source precedence is field-specific. A device-tree `model` string is not
automatically an SoC identifier. Unknown, unreadable, malformed or conflicting
data stays explicitly unknown; never synthesize a successful value.

## Refresh policy

- Initial profile: daemon startup before controllers discover resources.
- Refresh: topology/capability invalidation, relevant hotplug or explicit
  controlled rescan.
- Fast-changing values (frequency, temperature, PSI, workload) remain runtime
  telemetry and are not frozen into a long-lived hardware identity profile.
- A transaction must abort/replan if its pinned generation is invalidated
  before actuation.

## P0 integration order

1. Implement a root-injectable read-only profiler and strict parsers.
2. Build deterministic synthetic sysfs/procfs fixtures (A–J).
3. Test exact CPU membership, malformed/missing nodes, permissions, units,
   constraints, refresh generations and stable fingerprints.
4. Inject one `DeviceProfileHandle` into FrequencyController and
   AutonomousTuner. Remove their independent hardware discovery only after
   equivalent profile coverage exists.
5. Add end-to-end fixture tests: Profile -> Classify -> Baseline -> Decide ->
   Act -> Readback -> Verdict.
6. Keep Qualcomm KGSL unverified controls observation-only. No ART, ANGLE,
   OpenGL or SurfaceFlinger actuator is claimed by this profiling contract.

## Non-goals

CDDP does not tune by itself, infer performance tiers from CPU count, mutate
OEM boot properties, override driver semantics, or treat logcat/dumpsys as
primary hardware discovery sources. CF-DVS remains a post-execution evidence
collector.
