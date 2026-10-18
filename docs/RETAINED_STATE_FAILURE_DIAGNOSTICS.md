# Retained-State Failure Diagnostics

**Project:** CoreFlow Autonomous Engine  
**Related commit:** `969f127` — `Add retained-state failure diagnostics`  
**Branch:** `refactor/source-consolidation`  
**Validation environment:** Host test build on Termux  
**Status:** Host validation passed; Android compilation not yet verified.

## 1. Purpose

This document describes the retained-state diagnostic update introduced
to make persistence failures easier to identify and investigate.

Retained state stores settings accepted after a beneficial mutation outcome.
It is distinct from the mutation journal, which records in-flight operations
that may require recovery.

A failed retained-state operation must not be treated as a successful
retention. The candidate remains unaccepted when persistence fails, and the
existing rollback and recovery handling continues to run.

## 2. Changes

The update introduces `RetainedStateFailureStage` in
`include/coreflow/retained_state.hpp`.

Both `MutationController` and `ResourceMutationController` expose
`lastRetainedStateFailureStage()` so the caller can inspect the most recent
retention failure.

The implementation records the stage at which retention failed. When
`RetainedStateStore::upsert()` fails, the controller attempts to commit the
previous retained entries as a best-effort recovery operation.

If that recovery commit also fails, the reported stage becomes
`RecoveryCommitFailed`. The controller does not clear the pending outcome
or accept the candidate in either failure case.

`src/autonomous/autonomous.cpp` adds a
`retained_state_commit_failed` decision-trace event before the existing
rollback handling. The event includes the affected scope and the diagnostic
stage, making the failure more specific in the decision trace.

## 3. Failure-stage reference

| Stage | Meaning |
|---|---|
| `NONE` | No retention failure has been recorded for the latest attempt. |
| `PRECONDITION_FAILED` | Required pending state, store, path, or target is missing. |
| `BASELINE_INVALID` | The baseline is missing, invalid, or equal to the target. |
| `LIVE_READ_FAILED` | The live value could not be read. |
| `LIVE_TARGET_MISMATCH` | The live value does not match the candidate target. |
| `ALLOCATION_FAILED` | Constructing or preparing the updated retained entries failed. |
| `STORE_UPSERT_FAILED` | The candidate could not be persisted, but the recovery commit succeeded. |
| `RECOVERY_COMMIT_FAILED` | The candidate upsert failed and the attempt to restore the previous ledger also failed. |

The stage identifies the observed failure point. It does not, by itself,
prove that the underlying filesystem is in a particular state.

## 4. Failure handling

The retention path follows these rules:

1. Reset the diagnostic stage before evaluating a new retention attempt.
2. Validate the pending outcome, retained store, mutation path, target, and
   baseline.
3. Read the live value and verify that it still matches the candidate target.
4. Prepare the updated retained entries.
5. Persist the candidate through `upsert()`.
6. If persistence fails, attempt to commit the previous retained entries.
7. Preserve the pending outcome when retention fails so the candidate is
   not reported as accepted.
8. Emit the failure diagnostic before the existing rollback handling runs.

A successful recovery commit means the store reported that the previous
ledger was restored. A failed recovery commit means the recovery attempt
could not be confirmed; the diagnostic does not claim otherwise.

## 5. Validation results

The following results were observed during validation of the related
local changes:

| Check | Result |
|---|---|
| `coreflow_mutation_tests` | 359/359 checks passed |
| Host CTest suite | 23/23 test suites passed |
| CTest elapsed time | 30.88 seconds |
| `git diff --check` | Passed |
| Header dependency inspection | `autonomous.hpp` includes the mutation, resource-mutation, and retained-state headers |
| Android NDK environment | `ANDROID_NDK_HOME` and `ANDROID_NDK_ROOT` were unset |
| Android build | Not performed |

Two focused tests were added in `tests/mutation_tests.cpp`:

- `test_retained_commit_failure_keeps_outcome_pending`
- `test_retained_recovery_commit_failure_is_reported`

These tests verify the CPU mutation controller's failure-stage reporting,
pending-outcome preservation, empty fake-store state, and commit-call count.

The resource controller has matching diagnostic implementation, but these
two focused tests do not independently exercise its failure stages.

## 6. Android validation limitation

The host build classifies sources that include Android platform headers,
such as `<android/log.h>`, as Android-only unless an explicit platform tag
overrides that rule.

Consequently, `src/autonomous/autonomous.cpp` is not compiled as part of
the host `coreflow_core` target. Passing host tests therefore does not prove
that the new decision-trace integration compiles in the Android daemon.

The Android target `coreflowd` requires an Android NDK toolchain and a valid
`COREFLOW_ONNX_ROOT` containing the ONNX Runtime headers and library for the
selected ABI. Those prerequisites were not available in the inspected
environment, so Android compilation remains unverified.

No Android daemon execution or device-runtime validation is claimed here.

## 7. Safety and operational boundaries

This update improves failure observability; it does not grant mutation
authority or enable kernel changes by itself.

The existing policy, capability checks, journaling, bounded-write
verification, rollback, and runtime safety controls remain responsible for
mutation safety.

The documentation and host tests do not authorize enabling adaptive mode,
arming mutations, changing kernel tunables, or restarting the Android daemon.

## 8. Reproducible host checks

From the repository root:

```bash
cmake --build build/host --target coreflow_mutation_tests
./build/host/coreflow_mutation_tests
ctest --test-dir build/host --output-on-failure
git diff --check
```

These commands validate the configured host build and working-tree
whitespace. They do not replace an Android build of `coreflowd`.

## 9. Summary

The retained-state update adds explicit failure-stage reporting to the CPU
and resource mutation controllers and emits a decision-trace event when
retention fails. The recorded host test results passed, while Android
compilation and dedicated resource-controller failure-stage tests remain
outstanding validation items.
