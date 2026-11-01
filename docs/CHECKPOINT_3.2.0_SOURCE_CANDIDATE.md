# CoreFlow v3.2.0 — source checkpoint candidate

**Status: SOURCE CHECKPOINT CANDIDATE ONLY. NOT A STABLE DEVICE RELEASE.**
The installable metadata remains v3.1.2 until release acceptance is complete.

## Exact source baseline and validation evidence

- Baseline was `f51aa617` (v3.1.2 trial retention).
- The prior source hardening and follow-on changes are in `main`.
- CI run [#501](https://github.com/mystivara-creator/CoreFlow-Engine/actions/runs/41299841812)
  completed successfully for `9661c3398843f369423c04bccab779a737cbdd59`:
  host deterministic tests (31/31), ASan/UBSan, and Android 14+ ARM64 production.
- ARM64 module ZIP, native binary, local ONNX Runtime and bundled thermal model
  were packaged. Packaging is **not** proof of successful model inference on-device.

## Source scope

1. **Value admissibility:** Reviewed Linux/Android bounds are applied across
   vendor-derived, kernel-option and numeric-step proposals, with standard
   cgroup utilization clamps interpreted as percentages. Discrete ordered
   hardware values must come from the current advertised option/OPP list.
   Unknown frequencies are deferred, never overclocked.
2. **Live coupled constraints:** Immediately before an attempted resource write,
   `vm.dirty_background_ratio <= vm.dirty_ratio` and
   `cpu.uclamp.min <= cpu.uclamp.max` are checked against readable LIVE
   siblings. Unknown/invalid siblings conservatively defer the write.
3. **Governor-dependent sequencing:** After governor replacement, active
   tunables are rediscovered and an earlier resource selection must be re-scored
   against the new configuration before being applied. Existing trial-history
   holds, exact-target permits, journals, owner handoff, reverse-order rollback,
   whole-cohort attribution and accepted-state retention remain mandatory.
4. **Device-adaptive evidence:** Each CPU policy reports related cores,
   read-only scaling limits and cpuinfo hardware bounds, plus a bounded optional
   kernel-advertised frequency list. Absent or inconsistent OPP tables are
   unavailable, never inferred from SoC branding. Software build provenance,
   vendor source counts and discovery truncation are exposed read-only.
5. **Measured model status:** ONNX session readiness, successful finite model
   inference count, heuristic fallback count and the latest prediction source are
   distinct. The daemon no longer logs a generic `READY` for heuristic-only
   startup. The WebUI exposes these signals, model/selection evidence, trial
   holds, active owned settings and recovery information without mutation authority.
6. **Regression coverage:** Candidate ABI values, UClamp sibling fixtures,
   status JSON, WebUI parsing, and heuristic/model reporting are included in
   host checks; Android NDK and sanitizer jobs pass on the source baseline.

## Invariants and non-goals

- No direct writes to framework-owned CPU frequency limits, voltage/clock
  overclocking, thermal bypass, charging protections or unreviewed kernel ABI.
- No generic governor presets and no invented per-device universal optimum.
- A verified kernel write is NOT automatically a beneficial measured outcome.
- A multi-member cohort is not independent causal credit for its components.
- A lost telemetry signal cannot be treated as a low-risk or safe measurement.
- Flashing and operating a real phone remain outside CI capability.

## Mandatory device acceptance before version bump / stable release

Follow [DEVICE_VALIDATION.md](DEVICE_VALIDATION.md) on the target device:
flash/install the exact CI artifact; verify boot, daemon PID/heartbeat and
telemetry freshness; confirm ONNX session and successful **real** inference or
explicit heuristic fallback; validate repeated unchanged workloads, contextual
A→B progression, governor plus active tunables, thermal/charging transitions,
external ownership handoff, journal rollback, stop/restart/reboot and long soak.

Collect objective jank, CPU/PSI, memory, temperature and power/battery evidence.
If any of these fail or are unavailable, do not promote the source checkpoint to
a stable release. Avoid broad feature additions before this acceptance gate.
