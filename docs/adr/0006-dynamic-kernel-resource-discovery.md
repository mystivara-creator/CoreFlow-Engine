# ADR 0006: Dynamic Kernel Resource Discovery

- Status: Proposed implementation baseline
- Scope: source consolidation branch, before a separate hardening pass

## Context

The engine is intended to adapt to the kernel interfaces present on the running device. A static list of writable paths or block-device names makes newly exposed interfaces invisible and can cause valid controls to be omitted. In particular, block-device aliases under `/sys/block` are commonly symlinks, while the canonical device tree is exposed under `/sys/devices`.

## Decision

1. Derive traversal roots from the mounted kernel filesystem table.
2. Traverse mounted sysfs and cgroup trees without skipping directories by name; never follow symlinks and never cross the root filesystem boundary.
3. Traverse the procfs sysctl subtree dynamically.
4. Classify block queue resources by structural path segments (`/block/` and `/queue/`), not a fixed device name or mount alias.
5. Admit writable, readable, non-fatal kernel nodes even when their semantic domain is not yet known. Unknown nodes use the generic `Vendor` domain and are counted as lacking a semantic prior rather than being silently discarded.
6. Keep discovery separate from mutation. A discovered node is not itself proof that a proposed value is valid or beneficial; mutation still requires the existing authority, baseline, candidate, actuator, verification, and rollback path.
7. Retain explicit traversal and inventory budgets. If a budget is reached, report truncation rather than claiming the inventory is complete.
8. Build legacy `IoDevice` summaries from the dynamic capability inventory instead of constructing per-device paths.

## Consequences

- Newly exposed writable queue attributes can enter the capability matrix without adding one path per attribute.
- Devices with names other than `sda` through `sdf` are handled through their discovered identity.
- Unknown writable nodes are no longer rejected solely because they are absent from the vendor configuration or a semantic catalog.
- Read-only nodes are not mutation candidates in this pass; broad read-only inventory is a separate capability-registry concern.
- Resource discovery remains bounded. A truncated scan is explicitly incomplete and must not be interpreted as proof that no additional resources exist.
- This is not a claim of real-device validation. Host deterministic tests validate the traversal and classification contract; Android runtime validation remains a separate step.

## Deliberately deferred

- A fully typed semantic model for every kernel attribute.
- Automatic derivation of safe value ranges for arbitrary unknown attributes.
- Removal of every fixed probe used by specialized telemetry collectors (thermal, charging, CPU frequency, and GPU collectors have separate data contracts). Those probes must be migrated independently rather than mechanically replaced with untyped writes.
- A separate safety-hardening pass and real-device mutation trial.
