# Source Map

This map describes the current source layout. It is intentionally organized by responsibility, not by individual tunable.

- `src/observe/`: mount-derived discovery, capability registry, tunable inventory, kernel roots, and vendor/ecosystem evidence.
- `src/understand/`: runtime context, effect estimates, thermal features/guards/prediction, experience, and baseline intelligence.
- `src/balance/`: policy, resource state, consultation, control planning, and safety holds.
- `src/mutation/`: generic resource mutation, CPUFreq actuator, write verification, and rollback paths.
- `src/conscience/`: mutation journal, retained state, fatal-surface guard, and decision trace.
- `src/runtime/`: configuration, daemon entry, status snapshots, lifecycle and runtime integration.
- `src/autonomous/`: periodic orchestration that connects observation, reasoning, policy, and authorized actuation.
- `include/coreflow/`: public/internal interfaces and shared data contracts.
- `module/`: KernelSU module scripts, default configuration, ONNX model, and mobile WebUI.
- `tests/`: deterministic host regression tests and source-contract checks.
- `tools/`: build and validation scripts; generated build products do not belong in the source package.

## Authority flow

Environment observation produces evidence. Reasoning can rank candidate effects but cannot authorize a write. Balance/policy and the mutation-permit/safety foundation remain the authority boundary. Mutation code must reject missing or stale authorization and preserve journal, verification, and rollback behavior.
