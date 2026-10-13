# Capability-Driven Resource Autonomy

This document supersedes every earlier description of a per-parameter
allow-list (the nine reviewed VM paths, physical-only block queue numerics,
scheduler-token quarantine, and EffectModel semantic priors).

## Principle

CoreFlow does not decide by parameter name what it may tune. It discovers the
tunable surface of the running device, learns the operating points its vendor
already configured, and only **blocks the surface classes that are fatal on
every Android device**. Everything else is a candidate. Measured outcomes
decide what is kept.

```
VendorKnowledge ─┐
                 ├─> TunableDiscovery ─> EffectModel ─> Consultation ─> Authority ─> Actuator
Kernel walkers ──┘        (matrix)        (candidates)    (need/grant)    (permit)    (fatal guard,
                                                                                         journal,
                                                                                         read-back)
                                         BaselineIntelligence ─> keep / rollback ─> ExperienceMemory
```

## 1. Vendor knowledge (`src/observe/vendor_knowledge.cpp`)

Read-only harvest of the firmware image at discovery time:

| Source | What is extracted | Profile tag |
|---|---|---|
| `init*.rc` under `/vendor`, `/odm`, `/system`, `/system_ext`, `/product` | `write <path> <value>` | From the `on ...` trigger text (e.g. `on property:...lowpower=1` → efficiency) |
| Vendor shell (`*.sh`, e.g. `init.*.post_boot.sh`) | `echo <value> > <path>` | From function/case labels (`configure_sustained_mode()` → efficiency) |
| `powerhint*.json` | `Nodes[].Values` + `Actions[]` per power hint | `DefaultIndex` → default; `LAUNCH`/`INTERACTION`/... → performance; `SUSTAINED`/`LOW_POWER`/... → efficiency |

Only literal values for kernel tunable paths are kept (no `$VAR`, globs or
properties). Paths are canonicalized with `realpath`, so
`/sys/devices/system/cpu/cpu4/cpufreq/...` and `.../cpufreq/policy4/...` match.

## 2. Generic tunable discovery (`src/observe/tunable_discovery.cpp`)

Walkers cover every capability class:

| Domain | Roots |
|---|---|
| Memory | `/proc/sys/vm/*` |
| Scheduler | `/proc/sys/kernel/sched*`, `/proc/sys/walt/**`, cpufreq governor tunable dirs, `cpuN/core_ctl/*` |
| CpuFreq (tunables) | `cpufreq/policyN/*` (the governor node stays with `MutationController`) |
| Io | `/sys/block/<dev>/queue/*` and `queue/iosched/*` (loop/ram skipped) |
| Gpu | `/sys/class/devfreq/*`, `/sys/class/kgsl/*` (+ `devfreq/`), `mali0`, `/sys/kernel/gpu` |
| UClamp | `/dev/cpuctl/**/cpu.*`, `/dev/stune/**`, cgroup v2 `cpu.*` |
| CpuSet | `/dev/cpuset/**` |
| Vendor | any other path the vendor's configuration writes |

A node is recorded only if it is a regular file with a kernel write bit (root
bypasses `access(W_OK)`, so the mode bits are checked) and reads back a value.
Each node carries capability evidence:

- `options`: tokens from a `"[a] b c"` selector line or a sibling `available_*` list
- `ordered_scale`: the options are an ordered hardware scale (frequencies)
- `vendor_values`: what the vendor writes, with its profile tag

## 3. Fatal surface guard (`src/conscience/fatal_surface.cpp`)

The **only** hard restriction. Denied classes:

| Class | Examples |
|---|---|
| Action triggers | `drop_caches`, `compact_memory`, `sysrq-trigger`, `uevent`, `bind`/`unbind`, `reset`, `delete`, `rescan`, `cgroup.*`, `memory.reclaim` |
| Crash control | `*panic*`, `*watchdog*`, `*lockup*`, `hung_task*`, `core_pattern`, `reboot`, `kexec` |
| Security | SELinux, `kptr_restrict`, `bpf`, `modules_disabled`, `randomize_va_space`, `mmap_min_addr`, `printk`, `perf_event*` |
| Thermal protection | `/sys/class/thermal/**`, `*thermal*`, trip points, cooling devices, `*throttl*` |
| Power delivery | `/sys/power/*`, power_supply, charging, battery, regulators, clocks, voltage, suspend/wakelocks |
| Hotplug | `online`, `offline`, memory hotplug |
| Hard limits | `overcommit_*`, hugepages, `max_map_count`, `memory.max/high/min/low`, `pids.max`, `pid_max`, `threads-max`, RT runtime/period, cpuset `mems`/exclusive/load-balance |
| Storage integrity | `write_cache`, `fua`, `discard_*`, `ro`, RPMB, UFS host |
| Raw interfaces | debugfs, tracefs, securityfs, pstore, firmware, `/sys/bus`, `/proc/sys/fs`, `/proc/sys/user`, anything outside the tunable roots |

Value invariants: a value must be a single printable line of at most 256 bytes,
and a node whose factory value is a non-negative integer never receives a
negative integer (`-1` means "unlimited/disabled" on many knobs).

## 4. Candidate generation (`src/understand/effect_model.cpp`)

No parameter tables. Candidates come from evidence, strongest first:

1. **Vendor values** (efficiency / performance / default)
2. **Kernel options** (advertised tokens)
3. **Ordered scale** (one step down / up the advertised scale)
4. **Bounded step** (±12.5% of the factory value; never for 0/1 toggles; within ½× to 2×)

Opaque multi-field values (cpu lists, masks) are only ever set to a
vendor-declared value.

Context rules:

- **Thermal stress** (Warming / ThermalGuard): only load-reducing candidates:
  vendor efficiency values, a step down an ordered scale, or a lower
  utilization clamp.
- **Memory pressure** (without thermal stress): the Memory domain itself is
  explored; the measured outcome decides direction.
- **Headroom** (Elevated): every candidate. Vendor values outrank exploration.
- With no other evidence, a step down is tried before a step up.

## 5. Unchanged safety net

Adaptive mode + `mutation_armed` remain the user's consent; per-namespace
grants are now opt-out (`grant_<ns>=no`). Every write still requires a
capability-bound permit, a journal entry before the write, read-back
verification, measured observation, rollback on regression, epoch restore on
stress / idle / authority loss, and SAFE_MODE recovery.
