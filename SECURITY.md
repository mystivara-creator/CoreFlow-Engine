# Security and Safety

CoreFlow runs as a privileged Android module and can interact with kernel-facing interfaces. Production changes therefore follow a fail-closed policy.

## Mutation rules

1. Discover the interface before use.
2. Capture a baseline before mutation.
3. Require explicit capability checks.
4. Apply bounded changes only.
5. Read back and verify the requested value.
6. Restore the baseline when leaving the protected state or shutting down.
7. Skip unknown or unsupported controls.

The current release can mutate CPU governors and the explicitly allow-listed VM/I/O resources discovered with writable, verifiable interfaces. Charging, uClamp, cpusets, graphics, ART and other system controls remain capability/telemetry surfaces until their semantics can be proven for the target device. Public adaptive mode does not bypass capability, policy, journal, bounded-write, verification, rollback, or safety-hold gates.

## SELinux

The module does not use permissive mode or wildcard allow rules. Device-specific AVC denials should be investigated individually before adding a minimal rule.

## Reporting

For a crash, boot loop, corrupted configuration or unexpected kernel behavior, include the CoreFlow version, device/kernel information, relevant `logcat` lines and the contents of `/data/adb/coreflow/config.ini` with any sensitive identifiers removed.
