# CoreFlow Autonomous Engine — Final Development Handoff (Historical Snapshot)

> **Historical v2.2.0 handoff.** This is not the current v2.4.0 feature/status inventory; use current source, tests, and `docs/BUILD_STATUS.md` for present behavior.

**Current source identity:** `v2.2.0` / `versionCode 2200`  
**Position:** final development snapshot; pre-production, not device-certified.

## Completed in this source snapshot

- Capability discovery is separate from explicit policy authorization.
- Resource mutations require target-bound permits and retain journal-before-write, bounded writes, read-back verification, and rollback behavior.
- New block-device queue writes are quarantined; restore of old journaled values remains possible.
- Consultation returns Allow/Defer/Deny with explicit reasons and per-namespace user-grant checks.
- Consultation ledger is exposed as read-only runtime telemetry and rendered in the WebUI.
- Expanded domain capabilities are described as evidence, not permission.
- CMake/module/runtime version identity is v2.2.0.

## Deliberately not included in this final snapshot

- WebUI grant/revoke controls: no authenticated, narrowly scoped daemon command interface exists yet. A UI-only toggle would be misleading and unsafe.
- Grant expiry/TTL: config-backed grants remain opt-in and have no expiry semantics.
- Android Settings Database collection: it needs a permission-correct Android-specific implementation and device verification; filesystem guessing is not an acceptable substitute.
- Android NDK/ARM64 link and physical-device validation: unavailable in the local host-only validation environment.

## Next gates (outside this source-only handoff)

1. Build and link ARM64 with the pinned Android NDK/ONNX Runtime toolchain.
2. Install and uninstall on test devices under both supported root managers.
3. Verify Observe mode performs no new kernel writes and old journals recover correctly.
4. Run a documented device-specific soak before considering any Adaptive trial.
5. Only after a reviewed daemon control channel exists, design authenticated grant/revoke operations and explicit grant lifetime semantics.

## Non-negotiable invariants

1. Observe-only defaults remain unchanged.
2. A grant never overrides a denied actuator surface or missing target-bound permit.
3. Journal commit precedes every new write.
4. A write is not success until read-back verification passes.
5. Failed recovery remains visible; unresolved journal state is never silently discarded.
