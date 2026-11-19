# Direct Adaptive Runtime — Production Composition

## Runtime ownership

DirectAdaptiveRuntime is the production composition root for the V5 five-module contract. coreflowd starts it only after CoreFlowContext::Start completes durable baseline recovery. It owns discovered CPU governor, GPU, virtual-memory and block-queue controls. The legacy autonomous tuner and generic frequency controller are kept out of these actuator paths to prevent competing writers. Existing epoll, timerfd and signalfd remain the daemon event reactor.

## Five modules

1. TopologyScanner resolves standard sysfs/procfs capability roots, enumerates CPUFreq policy OPP tables, classifies heterogeneous CPU clusters from related_cpus and per-CPU cpu_capacity evidence (with a conservative two-tier OPP fallback), and discovers ABI-present WALT, schedutil or interactive governor tunables. Ambiguous layouts remain unclassified rather than being guessed as little/big. It also discovers recognized KGSL/devfreq GPU surfaces, VM controls and physical block queue tunables. Device-specific leaf nodes are included only after semantic and ABI checks.
2. Telemetry samples aggregate CPU utilization and per-core deltas from /proc/stat, memory/ZRAM capacity, display/backlight state and optional PSI. The maximum valid per-core utilization exposes a saturated core that an aggregate average can hide. Thermal pressure is accepted from the daemon's OEM thermal fusion path when available. Missing optional evidence stays invalid; it does not become a fabricated zero-valued measurement.
3. SweetSpotEngine evaluates deterministic policy intents from advertised OPPs, supported numeric ranges, RAM/ZRAM class, storage topology and measured refresh rate. KGSL power-level indices are treated as inverse to devfreq OPPs. KGSL mutation requires both writable max_pwrlevel and min_pwrlevel bounds; max_pwrlevel is the faster ceiling (lower index), min_pwrlevel is the slower floor (higher index). default_pwrlevel is not used as a frequency actuator, and the final legacy sentinel index is excluded from target selection.
4. KernelActuator writes a batch intent WAL before the first write, requires an already durable OEM baseline, checks device/inode identity and expected current value immediately before writing, reads the value back, and delegates actual sysfs writes to BaselineStore so CFJ4 remains the restoration authority.
5. EventReactor is the existing daemon epoll/timerfd/signal loop. It invokes direct adaptive evaluation on control ticks; optional PSI/FrameTimeline sources are evidence inputs only, not daemon startup gates.

## Mode policy

The controller resolves SCREEN_OFF, BALANCED_SWEET_SPOT, ACTIVE_LOAD and THERMAL_GUARD. CPU hispeed_freq and GPU frequency targets are selected from discovered operating points; CPU governor tunables are clamped to each node's discovered range. Capacity-ranked little/mid/big domains receive distinct balanced hispeed-load envelopes; unknown domains are not assigned guessed thresholds. Active-load classification considers aggregate CPU utilization, maximum per-core utilization, optional CPU PSI and GPU busy evidence, with hysteresis on entry/exit. Screen-dependent GPU idle time and CPU rate limits are derived from observed refresh rate (normalized through 1000 Hz) and the active frame window. ThermalGuard has priority over screen-off idle, uses the daemon's fused OEM/fallback guard signal, and is retained through temporary loss of thermal evidence; idle throttling is suppressed only after a settled idle policy evaluation. VM swappiness/page-cluster and read-ahead/iostats controls are eligible only when their kernel ABI nodes are present and captured.

These values are policy envelopes, not promises of universal device-specific frequency results. The implementation does not claim to discover an electrical voltage wall where the kernel exposes no corresponding OPP-voltage evidence.

## Mutation and recovery contract

- The initial OEM snapshot is durably persisted before any candidate is admitted.
- The complete eligible intent batch is atomically written and fsynced to /data/adb/coreflow/direct-adaptive.wal.
- BaselineStore records and fsyncs each OEM-relative mutation before the sysfs write. Crash restoration is performed by BaselineStore on the next startup; the secondary intent WAL is retired only after that recovery check.
- A journal fault latches mutation stop for the current daemon lifetime.
- A missing node, denied permission, replaced inode, OEM contention or failed readback isolates the affected actuator. It does not stop independent nodes.
- --status publishes the directAdaptive status object; --journal reports a read-only baseline snapshot; --rollback-baseline stops the daemon and requests an ownership-safe OEM restore.

## Verification

Host CI covers six mutation fault-injection scenarios and deterministic policy tests for CPU OPP selection, capacity-based multi-cluster classification and ambiguous-topology fallback, per-core saturation hysteresis, WALT/interactive governor discovery, high-refresh frame budgets, negative boost normalization, paired KGSL power bounds and index inversion, devfreq direction, GPU idle window, VM behavior, storage read-ahead, and fused screen-off thermal transitions. Host CTest, ASan/UBSan and Android ARM64 production-package jobs are release gates. Physical-device acceptance remains a separate stage.