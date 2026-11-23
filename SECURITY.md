# CFAE System Framework / CoreFlow Engine v6.1.0-R Security Model

CoreFlow Engine (`coreflowd`, C++23 + Rust) is a privileged Android 13+ (API 33+) daemon. It operates strictly on allowlisted, verified kernel interfaces.

- The daemon is single-instance and checks operator `DISABLE`, `SAFE_MODE`, and the module `disable` marker. When any safety marker is active, `service.sh` invokes `coreflowd --recover-only` before exiting so pending same-boot CFJ4 journal entries are replayed even when the daemon does not start.
- Any supported mutation must pass `BaselineStore`'s explicit write allowlist (managed `sysfs`, `vm`/`walt`/`sched` `sysctl`, and ABI-verified `cpu.uclamp.*` / `io.weight` cgroup leaves only — raw `/dev/block/*`, `/sys/firmware/*`, `/sys/power/*`, `/sys/kernel/debug/*`, `/sys/fs/selinux/*`, and `/proc/<pid>/task/<tid>/cgroup` writes are rejected), be journaled (`CFJ4`) before writing, and be verified via `fstat` dev/inode identity and read-back.
- **Captured baseline & reboot volatility assumptions:** The journal stores the original **observed/captured** value (not a proven factory ROM image) in `/data/adb/coreflow/baseline.journal`. Every managed node class (`cpufreq`, `devfreq`, `kgsl`, `block/queue`, `power_supply`, `lru_gen`, `vm/*`, `walt/*`, `sched_*`, `cgroup`) resides on volatile kernel virtual filesystems (`sysfs`/`procfs`/`cgroupfs`) initialized fresh by the kernel on reboot. Therefore, when `boot_id` changes after a reboot, stale previous-boot journals are discarded safely without replaying stale values; same-boot restarts replay pending entries (including restoring `0444` read-only permissions locked by the Sovereign Tamper Guard).
- A fatal POSIX signal handler performs no mutex or C++ I/O; it terminates the process. A subsequent daemon start or `coreflowd --recover-only` performs the actual recovery.
- Recovery is best-effort, non-atomic, and can fail if the vendor changes the interface, other root modules race, or hardware is unstable.
- Bypass charging (`input_suspend` / `charging_enabled` / `battery_charging_enabled`) requires an active `PowerLongevity` lease and enforces a hard **20% lithium-ion SOC floor** (auto-release at ≤ 20%, ≥ 25% resume hysteresis; fail-closed if capacity is unreadable).
- No unknown vendor display nodes are written. Automatic task migration and process OOM policy overrides are disabled.
- Test-only environment overrides (`COREFLOW_TEST_STATE_DIR`, `COREFLOW_TEST_BOOT_ID_PATH`) are compiled exclusively into unit test targets (`COREFLOW_ENABLE_TEST_HOOKS`), never into production `coreflowd` binaries.
- Configuration inputs are bounded (e.g., `stepped_charge_current_ma` clamped to `100..5000 mA`, `battery_step_temp_threshold` clamped to `30.0..45.0°C`), and unrecognized or malformed values use defensive defaults.
- SELinux is never set to permissive; `sepolicy.rule` contains no wildcards and no raw `block_device` access.
- **Boot-loop guard recovery:** If 5 unconfirmed boots trigger `SAFE_MODE` or 8 unconfirmed boots write `disable`, inspect `/data/adb/coreflow/logs/boot_guard.log`, resolve the cause, and run `sh /data/adb/modules/cfae_framework/action.sh clear-safety` to replay any pending journal, clear safety markers, and reset `boot_attempts` to `0`.

## Reports

Include `coreflowd --diag` (or `--status`), the installed module version, kernel/ROM version,
`/data/adb/coreflow/logs/coreflowd.log` and the outcome of `--rollback-baseline`.
Do not delete `baseline.journal` as a troubleshooting shortcut.
