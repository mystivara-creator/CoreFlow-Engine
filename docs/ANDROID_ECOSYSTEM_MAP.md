# CoreFlow — Android / Linux ecosystem map

Embedded daemon catalog (`ecosystem_knowledge.cpp`) indexes surfaces the engine
may **observe**. Mutation still requires explicit `policy_authorized` + journal.

## Layers

| Layer | Meaning |
|-------|---------|
| LINUX_KERNEL | Upstream sysctl / sysfs semantics ([docs.kernel.org](https://docs.kernel.org)) |
| ANDROID_COMMON_KERNEL | ACK + Android backports ([Android Common Kernels](https://source.android.com/docs/core/architecture/kernel/android-common)) |
| ANDROID_FRAMEWORK | AMS, lmkd, cgroups.json, task_profiles ([AOSP perf](https://source.android.com/docs/core/perf/cgroups)) |
| VENDOR_HAL | OEM/SoC paths (KGSL, mali, interconnect) |

## Risk classes

| Risk | Daemon policy |
|------|----------------|
| OBSERVE_ONLY | Read / inventory |
| BOUNDED_TUNE | Eligible for CoreFlow allow-list after review |
| VENDOR_VOLATILE | Path exists but OEM may redefine semantics |
| QUARANTINED | No production writes (block queue, drop_caches, …) |
| FRAMEWORK_OWNED | Do not fight AMS / lmkd / PowerManager |

## Primary references

- [Cgroup abstraction layer](https://source.android.com/docs/core/perf/cgroups)
- [lmkd + PSI](https://source.android.com/docs/core/perf/lmkd)
- [Memory Limiter](https://source.android.com/docs/core/perf/memory-limiter)
- [PMGD](https://source.android.com/docs/core/perf/pmgd)
- [Power values / cpufreq stats](https://source.android.com/docs/core/power/values)
- [Linux VM sysctl](https://docs.kernel.org/admin-guide/sysctl/vm.html)
- [cgroup v2](https://docs.kernel.org/admin-guide/cgroup-v2.html)
- [Block queue sysfs](https://docs.kernel.org/block/queue-sysfs.html)

## Rule

**Discovery ≠ permission.** A node in this map or in `expandEcosystemSurface`
only expands situational awareness for Observe / diagnostics / effect context.
