# P2-A — Deterministic Lifecycle Manager

## Objective

Replace callback-capable, lazily constructed global controller objects with an explicit composition root. The runtime must have one owner for construction, dependency wiring, startup, shutdown, recovery, and destruction.

## Composition root

`CoreFlowContext` owns the following runtime graph:

- `CentralMutationCoordinator`: explicit mutation-ownership and preemption registry.
- `DeviceProfiler`: one immutable device-profile generation for the runtime.
- `ThermalMonitor` and `NeuralPredictor`: runtime-local measurement and advisory inference services.
- `MutationRecoveryMonitor`: shares the context's coordinator and the durable baseline journal.
- `UclampGovernor`, `FrequencyController`, `AutonomousTuner`, `HardwareLongevityManager`, and `VulkanController`: callback-owning controllers injected with the context coordinator.

The callback-capable controllers hold a reference to the coordinator. The context declares the coordinator before these owners and explicitly destroys callback owners in reverse dependency order. Therefore, coordinator lifetime encloses every registration, callback dispatch, acknowledgement, and unregister operation.

## Lifecycle contract

```text
Construct
  |
  v
Start(Mode::ReadOnly)                 Start(Mode::Daemon)
  |                                       |
  v                                       v
Discover CDDP profile                 Recover durable journal
  |                                       |
  v                                       v
Expose passive sensors                Discover CDDP profile
  |                                       |
  v                                       v
Started (no actuators)                Construct callback owners
                                          |
                                          v
                                      Started
                                          |
                                          v
                              Stop: close admission / rollback
                                          |
                                          v
                              verify journal + drain callbacks
                                          |
                                          v
                              destroy owners in reverse order
                                          |
                                          v
                                       Stopped
```

- A repeated `Start` while already started is idempotent only for the same mode.
- A stopped context can be started again; controller identities are unique per instance.
- Daemon startup refuses to construct mutation-capable controllers until durable journal recovery succeeds.
- Shutdown first executes `coordinated_rollback(coordinator, baseline, reason)`. Failed recovery leaves the context in `Failed` with callback owners still alive for another attempt. At process teardown, C++ member destruction still destroys callback owners before the coordinator.
- The event loop remains owned by the daemon stack and is stopped before `CoreFlowContext::Stop()` is called. It is synchronous; no detached worker is introduced by P2-A.

## Read-only CLI contract

`--status` and `--capabilities` create a `CoreFlowContext::Mode::ReadOnly` context. That mode performs CDDP/thermal inspection only. It does not instantiate the tuner, UClamp actuator, frequency controller, longevity controller, Vulkan controller, or mutation recovery monitor. It does not train the online predictor, register preemption callbacks, start a worker, or invoke a mutation API.

The previous `_exit` workaround is removed. Diagnostic commands now return normally after explicit context teardown. Runtime-only tuner/UClamp/GPU fields are marked unavailable or neutral in this independent CLI process rather than implying that the CLI has attached to the daemon's in-memory objects.

## Dependency injection

- `DeviceProfiler`, `ThermalMonitor`, and `NeuralPredictor` are ordinary objects, not process-wide accessors.
- `CentralMutationCoordinator` is explicitly constructed by the context.
- `AutonomousTuner` and all coordinator callback owners receive `CentralMutationCoordinator&` in their constructors.
- `MutationRecoveryMonitor` and global recovery receive the same coordinator explicitly.
- The process-wide `BaselineStore` façade remains temporarily shared because its durable journal is already process-scoped and it owns no callback registrations or worker threads. Its static lifetime does not participate in callback-owner destruction; a future persistence DI pass can make it context-owned without changing this P2-A lifecycle contract.

## Acceptance gates

1. Build host daemon and every native test target with C++23 warnings enabled.
2. Run all CTest targets, including `CoreFlowContextLifecycleTest`.
3. Run `CliOneShotLifecycleContractTest`: no `_exit` workaround; both diagnostic commands use read-only context; status does not train models or create mutation-capable owners.
4. Run daemon lifecycle tests under ASan/UBSan and leak detection.
5. Exercise repeated read-only and daemon context Start/Stop cycles.
6. Verify shutdown during active mutation invokes coordinated rollback before controller destruction; unresolved journal state remains fail-closed.
7. Verify no callback-capable controller singleton remains and no callback owner can outlive its coordinator.
8. Build the Android ARM64 production target; do not publish a release automatically.

## Scope boundary

This milestone does not implement eBPF, workload classification, Xiaomi/HyperOS tier selection, HyperOS-inspired KPIs, or policy tuning changes. Those remain P2-B through P2-E and depend on this lifecycle foundation.
