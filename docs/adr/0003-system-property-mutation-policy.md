# 0003. System property mutation policy

## Status

Proposed. No property is written by the daemon in the current release.
`BuildProp` remains observe-only in `consultation.hpp` until this ADR is accepted
and the boot overlay is implemented.

## Context

The engine should be able to change system configuration that is set at build
time when the device's load and hardware show that the change would help. A
build.prop value is one example. Two platform facts limit how this can be done:

1. `ro.*` properties are set once during init and cannot be changed afterward. The
   property service rejects later writes. A runtime daemon cannot change them.
2. `build.prop` files are read by init at boot. On devices with verified boot and
   dynamic partitions, the system partition is read-only at runtime. A change
   therefore takes effect only after a reboot, and it is applied through the
   module's own boot-time overlay, not by editing the partition.

Consequences of this: a bad value is a boot-time failure. Bootloop risk has to be
handled before the value is written, and recovery has to remove the overlay
without relying on the daemon, which may not start.

## Decision

Property changes are a separate, boot-time class. They are never applied at
runtime, and they are never applied before evidence and a rollback path exist.

1. **Eligibility is derived from evidence, not from a list.** For each property the
   engine reads: whether it is `ro.*` (never eligible), its SELinux owner from the
   device's `property_contexts` files (system, vendor, product, or init-owned), and
   whether init or a HAL reads it, observed from init triggers and service
   definitions on the device. A property owned by init or read at early boot is
   not eligible for an overlay. Only a property with a known consumer set and a
   value range observed on the device can become a candidate.
2. **Effect is predicted from the same evidence, and scored.** A candidate records
   its expected effect (for example a lower latency value under interactive use) and
   the observed load it was based on. A candidate without a predicted effect is not
   written.
3. **Writes go through a staged overlay.** The overlay is part of the module's own
   files, so disabling the module removes it. The daemon writes the overlay only in
   observe mode first, records what it would have written, and in adaptive mode only
   after the candidate has been stable across several boots.
4. **Bootloop protection uses the existing boot-attempt counter.** The counter in
   `post-fs-data.sh` is extended, not replaced. Each overlay generation is tied to a
   boot-attempt marker. If boot does not reach `boot_completed` on a generation, the
   next boot starts without that generation's overlay. At the existing SAFE_MODE
   threshold, every overlay is removed. At the disable threshold, the module is
   disabled.
5. **Fatal properties remain on a denylist.** Properties that control verified boot,
   encryption, the boot chain, or the boot-time service set are denied by name,
   because they are safety invariants rather than tunables. This is the only
   hard-coded rule set, and it is kept deliberately.

## Consequences

- Property writes take effect after a reboot, not during a session. Improvements
  are measured across boots, not within one.
- A candidate that never reaches `boot_completed` is removed automatically. The
  worst case is a boot with the default values, plus the logs.
- The device-validation plan must cover a bootloop test of each overlay generation
  before any property leaves observe mode.
