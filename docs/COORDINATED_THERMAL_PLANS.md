# CoreFlow v3.1.0 — coordinated plans and thermal stabilization

## Why this changed

The old orchestrator always attempted a CPU governor before the resource planner.
A successful CPU write opened the only measurement window, starving justified
I/O, memory, cgroup and GPU candidates. The resource controller also loaded its
own active journal as if a different process had crashed; a subsequent apply
could restore accepted values before coupling could build on them.

The replacement executes one **coordinated plan** per measurement epoch. Resource
candidates get first choice. Each plan can contain up to three resource writes;
a governor fallback can add up to three justified resource companions. Every
member still needs its own capability admission, grant, exact-target permit,
pre-write journal and read-back verification. A plan does not fill its budget
with unrelated writes just to touch every domain.

## Coupling uses live configuration

- Candidate generation reads current owned values, while immutable factory
  baselines remain separate for recovery. An accepted scheduler B or governor B
  therefore informs subsequent ranking instead of stale factory A.
- After a scheduler transition, a bounded read-only refresh inventories its
  `iosched` subtree. After a governor transition, policy-local and shared
  governor attribute trees are refreshed. Newly found controls need baselines
  and the same grants/authority as previously discovered controls.
- A governor fallback adds only companions with coupling evidence. Inactive
  advertised governor directories are not treated as active tunables.
- A scheduler/governor is not switched out from under still-owned dependent
  controls. Existing dependencies must first settle through the normal recovery
  path. There is no universal scheduler-to-governor preset table.
- Kernel option lists, advertised OPPs, vendor operating points and reviewed ABI
  bounds supply target values. Matching/risk semantics and finite work budgets
  remain explicit code; unknown writable nodes do not gain guessed semantics.

## ThermalGuard remains active

ThermalGuard withdraws performance boosts and settles unsafe performance epochs
before opening a cooling experiment. It can still authorize **stabilizing-only**
resource plans when measured evidence and configured confidence permit them.

Examples of eligible evidence include one step down an advertised frequency
scale, a lower utilization request, or a bounded change to known DVFS ramp/decay
semantics. OPP indices with inverted frequency order retain their existing
semantics. The standard cgroup `cpu.uclamp.max` value `max` is understood as its
ABI percentage ceiling. Min/max sibling constraints are checked when available.
Repeated thermal capping is bounded to half the device's original cgroup ceiling,
so successive small steps cannot silently converge to a near-zero CPU allowance.

CPU governor experimentation and performance-increasing boosts remain excluded
from a stabilizing-only plan. Framework-owned CPU frequency limits and kernel
thermal protections keep their existing exclusions. Uclamp affects frequency
requests on supporting governors; it does not promise a particular physical
clock on every kernel. Charging, ambient heat or another subsystem can dominate
real temperature, so cooling effectiveness still requires device measurements.

An already verified cooling configuration can stay active through a transient
confidence/plan dip. That dip blocks **new** writes; it does not automatically
raise an already-reduced demand back to factory values. Stop, disarming, failed
recovery, invalid telemetry and the other existing safety paths remain operative.
A matched thermal baseline is collected before scoring a new cooling experiment.

## One outcome, one identity

A multi-member plan receives a deterministic, length-delimited `cohort` identity.
Its measured outcome is recorded under that identity, not credited separately to
each component. A regressing plan rolls back together and its candidates are
suppressed under the existing rejection cooldown. No measured joint benefit is
invented as proof that each individual member was beneficial on its own.

Accepted cohorts remain active in the session with durable recovery journals.
Their individual members are **not** written as independent retained preferences.
After reboot, journals restore the original settings and the engine rebuilds
fresh baselines; scoped group experience and lifetime sample identity persist.
Single-member trials keep the existing per-candidate retained-store behavior.
Group records provide outcome provenance; per-member scoring continues to use
only independently measured records, not unproven component credit.

## Recovery and diagnostics

Dependent resource controls restore before their scheduler/governor anchor.
Journal snapshots preserve write order and crash recovery walks it in reverse.
A failed dependent restore prevents the anchor from being removed; journals and
retry state remain available. Incomplete startup recovery cannot clear a pending
journal using an empty in-memory dirty inventory. An external override is handed off without writing
over that actor's value; remaining cohort members can then be canceled safely.

The WebUI exposes `THERMAL_STABILIZING`, the latest cohort identity/member count,
applied controls, and the existing separate heartbeat/telemetry timestamps.
Decision trace events include coordinated plan verification and observation start.
This describes executed/held plans, not simulated success or guaranteed cooling.

## Verification scope

Host fixtures cover thermal CPU/GPU/cgroup plans, resource-first scheduling,
governor plus newly discovered companions, I/O scheduler plus queue companions,
whole-cohort rollback, failed companion commits, dependent rollback failure,
external override, cumulative capping bounds and restart recovery. Existing
policy, ownership, journal, installer and WebUI checks remain part of validation.

A passing host or sanitizer run does not establish Android ABI correctness or
phone thermal outcomes. The exact commit needs a green Android CI build and
on-device workload, thermal transition, stop/restart and reboot acceptance before
it is called a field-validated release checkpoint.

## Semantic references

- [Linux utilization clamping](https://docs.kernel.org/scheduler/sched-util-clamp.html)
- [Linux CPU performance scaling](https://docs.kernel.org/admin-guide/pm/cpufreq.html)
- [Linux block queue sysfs](https://github.com/torvalds/linux/blob/master/Documentation/ABI/stable/sysfs-block)


### Local checkpoint results

- Strict C++20 host compilation: 48 core sources, 28 host suites passed.
- ASan + UBSan: all 28 host suites passed locally; leak detection disabled in
  the traced local environment, with CI leak detection unchanged.
- Offline WebUI build and contracts: 391 checks passed.
- Installer harness: 11 scenarios passed; safety and clean source-release
  contracts passed. Android orchestrator strict syntax was checked with a log stub.
- Android linking/packaging and device behavior require their own validation;
  these local results do not claim a phone thermal soak or measured FPS gain.
