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

The current release only mutates CPU governors and only when `mutation_mode=adaptive` and the selected governor is explicitly advertised by the kernel. Charging, uClamp, cpusets, graphics, ART and other system controls remain capability/telemetry surfaces until their semantics can be proven for the target device.

## SELinux

The module does not use permissive mode or wildcard allow rules. Device-specific AVC denials should be investigated individually before adding a minimal rule.

## Reporting

For a crash, boot loop, corrupted configuration or unexpected kernel behavior, include the CoreFlow version, device/kernel information, relevant `logcat` lines and the contents of `/data/adb/coreflow/config.ini` with any sensitive identifiers removed.
