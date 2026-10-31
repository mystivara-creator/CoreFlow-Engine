# CoreFlow v3.1.0 — Maintenance checkpoint

This is the source release candidate for a development pause. Package identity:
`v3.1.0`, versionCode `3100`, Android API 34+, ARM64. Do not describe a tagged
binary as validated until the CI run and device evidence match its exact commit.

## What survives restart

| State | Location under `/data/adb/coreflow/` | Behavior |
| --- | --- | --- |
| Configuration | `config.ini` | Saved on disk; restart applies settings; upgrade preserves existing exclusions and stop/recovery markers. |
| Verified experience | `experience.db` | Bounded advisory history, scoped to device/kernel; atomic private writes with file and directory fsync; malformed/symlinked/oversized inputs rejected. History influences ranking, never grants permission. |
| Accepted candidates | `retained_cpufreq.db`, `retained_resource.db` | Persisted by the existing retained-state stores; revalidated against capabilities, environment and safety rules on startup. Not blindly written at boot. |
| Recovery obligations | `mutation_journal.txt`, `resource_mutation_journal.txt`, `interactivity.journal` | Recovery runs before capturing untouched baselines. Incomplete restore remains an obligation. Never delete journals to make an error disappear. |
| Lifetime sample identity | `sample_sequence.journal` | A separate diagnostic ID, starting with this checkpoint. IDs from old RAM-only sessions cannot be reconstructed. A clean daemon shutdown checkpoints the next ID; crashes/power loss may skip unused reserved IDs. |
| Display preferences | WebView local storage | System/light/dark and reduced-motion choice; unavailable storage falls back to a session choice. Independent of tuning authority. |
| Short observation windows | RAM only | Fresh after boot or workload changes. Old frame/thermal/load samples are deliberately not resumed as current evidence. |

The lifetime ID does **not** represent the number of successful optimizations or
an exact count of historical measurements. The daemon durably reserves batches
of 256 IDs to avoid per-tick fsync. An abrupt reboot can leave gaps; IDs are not
reused. Session counters remain available separately for correlation with decision
traces. Invalid sequence storage reports unavailable and is not overwritten as a
new zero counter; normal tuning safety does not depend on this diagnostic counter.

Discovery refresh keeps session numbering and same-scope in-memory experience;
it still restores/rechecks owned state and rebuilds observation windows. This
avoids discarding an unflushed outcome just because rediscovery was requested.

## WebUI scope

- Theme choice works offline and applies before initial paint. Dark styling covers
  surfaces, text, borders, warning states, inputs and diagnostic sections.
- Short opacity/transform transitions provide feedback without continuous loops.
  Both the OS accessibility preference and the saved reduce-motion option disable
  them. Native controls remain usable without animation.
- Settings explain purpose, defaults, units and higher/lower tradeoffs. Balanced,
  quieter and faster-observation choices edit only a draft. They do not guarantee
  performance, FPS or temperature. Inline validation and change review precede save;
  discard restores the last loaded configuration. Saving does not clear safety holds.
- Lifetime/session identity and loaded experience are visible separately. CSV keeps
  unavailable readings blank and preserves daemon-session boundaries.
- Journal/logcat/export fixes from the previous checkpoint remain included.

## Verification record

- 47 host core sources compile with C++20 and the project's strict warnings plus
  `-Werror`. All 26 host test executables passed; the persistence suite includes
  orderly restart, crash reservations, exhaustion, corruption, unavailable storage,
  fsync failure, bounded writes, symlink reads and legacy-temp hard-link protection.
- Targeted ASan/UBSan persistence tests pass with leak detection disabled because
  this environment's tracing prevents LeakSanitizer from running.
- Production TypeScript/Vite build, 228 status, 14 render, 70 control and 60
  diagnostics checks pass. Diagnostics exercise actual TS/React handlers and
  sandboxed shell transport, theme persistence and settings draft/validation.
- The Android-only orchestrator passes strict host syntax checking using a logging
  declaration stub. This does not replace Android NDK linking.
- Safety/source release contracts and installer scenarios must be green on the
  final tree. The installer retains state outside the module directory.

Local limitations: CMake/NDK and Chromium are unavailable here. Host compilation
reproduced CMake's source-platform rules with G++; UI checks use a hook harness,
not a full browser render. No phone soak, battery benchmark, visual certification
or universal boot safety is implied by these checks.

## Required release acceptance

1. Push the reviewed source, run all GitHub CI jobs on that SHA and retain the
   checksums/artifact provenance. Publish only a matching `v3.1.0` ARM64 package.
2. Install on the target phone; verify cold start, normal stop/start and reboot.
   Config/experience should persist, lifetime IDs advance, and baseline sampling
   restarts. Exercise idle/active, app changes, unavailable telemetry and recovery.
3. Verify light/dark/system themes, accessibility reduced motion and phone layout;
   verify settings explanations, draft choices, discard, save/restart and that
   Reload does not overwrite an unsaved draft. Open all exported files from Download.
4. Confirm any retained candidate passes current capability/safety gates; a journal
   or persistence error must remain visible. Corrupt recovery journals must block
   mutation rather than being silently cleared.
5. Run representative daily workloads and repeated restarts. Record firmware,
   kernel, commit, start/end temperature, errors and restore outcomes before calling
   the checkpoint device-validated.

## While development is paused

Keep this version fixed during the observation period; prioritize only reproducible
crashes, failed recovery, data loss or broken controls. Save diagnostics before
changing configuration. If behavior is abnormal, use Controls → Stop and allow
graceful restoration. Preserve state/recovery files; do not force-delete journals
or clear SAFE_MODE to hide a failure. Resume development from the exact commit,
CI run and device observations recorded for this checkpoint.
