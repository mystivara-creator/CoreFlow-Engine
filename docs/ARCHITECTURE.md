# Architecture and Authority Boundaries

## Control pipeline

1. **Observe:** discover mounted kernel interfaces and collect telemetry.
2. **Understand:** construct context, baselines, candidate effects, confidence, and expected impact.
3. **Balance:** evaluate policy, consultation, current constraints, and safety holds.
4. **Adapt:** only an explicitly armed and authorized mutation path may attempt a bounded change.
5. **Verify:** journal before mutation, read the value back, evaluate the result, and roll back when verification or safety checks fail.

## Non-negotiable invariants

- Default configuration is observe-only and mutation is disarmed.
- Discovery/readability/writability is evidence, not permission.
- Inference, confidence, experience memory, and UI selections cannot bypass the final mutation authority.
- CPUFreq governor changes require the separate `allow_cpu_governor` opt-in.
- A write must be bounded, journaled, read-back verified, and recoverable.
- Unknown or insufficiently described interfaces should remain observation-only rather than receive guessed values.
- A missing sensor or invalid telemetry value must not be interpreted as a safe reading.

## Epoch and freshness

A plan should be based on current telemetry and the current discovered capability state. If the environment or relevant capability state changes, stale candidates should be reevaluated rather than blindly replayed. Persistent experience is advisory and scoped; it cannot replace fresh measurements or authorization.

## Release boundary

Passing host tests does not establish Android ABI/link correctness, vendor-kernel compatibility, or device-level safety. Those require a successful Android ARM64 build and staged device validation, including recovery behavior.
