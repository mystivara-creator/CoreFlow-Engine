# v2.0.0 Baseline / Reference Contract

## Implementation baseline

`CoreFlow-Baseline-Intelligence-v3-(Autonomous-Engine).zip` is the implementation baseline for v2.0.0.

The v2.0.0 tree preserves its environment discovery, resource model, context engine, policy engine, mutation controller, mutation journal, safety hold, thermal predictor and Android module architecture.

## Capability reference

`CoreFlow-Engine-main.zip` (v1.0.0-Rebuild) is a capability reference only.

The reference is used to restore the capability surface that existed in the earlier engine, including dynamic CPU/thermal behavior, VM/I/O tuning, block-device discovery and adaptive runtime behavior. Legacy direct kernel writes are not copied as an authority boundary.

## v2.0.0 rule

A discovered writable resource is eligible for autonomous policy consideration when it is readable, writable and runtime-verified. Policy chooses whether intervention is useful for the current device context. Final mutation remains owned by `MutationPermit`, with durable baseline journaling, read-back verification and rollback.
