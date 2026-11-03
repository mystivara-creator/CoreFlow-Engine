# CoreFlow Modern v2 Security Model

CoreFlow is a privileged Android daemon. It must operate on verified kernel interfaces only.

- The daemon is single-instance and checks operator `DISABLE`, `SAFE_MODE` and the module disable marker.
- Any supported mutation must be journaled before the write and verified after read-back.
- The journal stores the original **observed** value (not a proven factory image) in `/data/adb/coreflow/baseline.journal`. Same-boot restart attempts recovery before further tuning.
- A fatal POSIX signal handler performs no mutex or C++ I/O; it terminates the process. A subsequent daemon restart performs the actual recovery.
- Recovery is best-effort, non-atomic, and can fail if the vendor changes the interface, other root modules race, or hardware is unstable.
- No unknown vendor display nodes are written. Automatic task migration and process OOM policy overrides are disabled.
- A failed mutation triggers rollback and bounded retry; incomplete recovery stops tuning.
- Configuration inputs are bounded, and unrecognized or malformed values use defensive defaults.
- SELinux is not set to permissive; rules are added only for verified device-specific denials.

## Reports

Include `coreflowd --status`, the installed module version, kernel/ROM version,
`/data/adb/coreflow/logs/coreflowd.log` and the outcome of `--rollback-baseline`.
Do not delete `baseline.journal` as a troubleshooting shortcut.
