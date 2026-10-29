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

Installation enables automatic bounded tuning using explicit compatibility keys and domain grants in config.ini. Upgrades preserve explicit grant_<domain>=no exclusions. There is no Observe/Standby mode selector. Missing, unreadable or malformed configuration blocks new writes; SAFE_MODE and DISABLE remain authoritative. Capability admission, target-bound permits, journals, read-back verification and rollback still apply. Saving WebUI settings never clears an operator stop or recovery marker. Explicit Start may resume DISABLE only while SAFE_MODE is absent.

## SELinux

The module does not use permissive mode or wildcard allow rules. Device-specific AVC denials should be investigated individually before adding a minimal rule.

## Reporting

For a crash, boot loop, corrupted configuration or unexpected kernel behavior, include the CoreFlow version, device/kernel information, relevant `logcat` lines and the contents of `/data/adb/coreflow/config.ini` with any sensitive identifiers removed.

## v2.1.0 capability-driven mutation

Discovery records capabilities, not permission by itself. EffectModel proposes only admitted resources with known semantics, bounded candidate values and the required domain grant. Fatal controls remain excluded. Unknown scalars require appropriate advertised or measured evidence. Neither automatic activation nor historical experience bypasses current permits, write verification, journal recovery or safety holds.

