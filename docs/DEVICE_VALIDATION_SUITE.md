# CoreFlow Device Validation Suite (CF-DVS)

**Status:** Phases 1–3 implemented; physical-device acceptance pending  
**Applies to:** CFAE System Framework v3.1.x and later  
**Implementation base:** PR #7 remediation head `e30174c630c77790dd7fb285e23a70a2035a3c63`

## 1. Purpose

CF-DVS produces a reproducible, evidence-backed validation bundle for a bounded device test session. It reduces reporting gaps between device behavior, CoreFlow telemetry, and engineering review. It is a validation harness, not an always-on monitoring product and not a tuning controller.

The suite must distinguish what was measured, what was unavailable, what was inferred, and what remains unverified. It must never convert missing telemetry into zero or treat a tuner `IMPROVE` verdict as proof that the whole test passed.

## 2. Safety invariants

1. **Read-only observer:** collection must not write sysfs, procfs, cgroup, power HAL, GPU, thermal, or vendor policy nodes. It must not acquire mutation leases or call controller actuation APIs.
2. **No tuning for the test:** do not change governors, frequencies, thermal limits, scheduler policy, UClamp, charging policy, or GPU policy to manufacture a test result.
3. **Session-scoped:** capture begins only after an explicit user action and ends explicitly or on a bounded timeout. No permanent high-frequency polling.
4. **Local-first privacy:** reports remain on-device. No upload, telemetry endpoint, or remote sharing by default. Redact serials, Android IDs, account identifiers, phone numbers, Wi-Fi names, precise location, and arbitrary command output.
5. **Validity is explicit:** every metric carries a value or `null`, a validity state, unit, source, and sample timestamp. Missing, stale, malformed, permission-denied, and unsupported are distinct reasons.
6. **Evidence integrity:** the bundle manifest records SHA-256 and byte length for each payload file. The validator rejects path traversal, symlinks, duplicate paths, and checksum mismatches.
7. **No false PASS:** `PASS` requires every required check to pass and required evidence to be present and valid. Warnings yield `PASS_WITH_WARNINGS`; insufficient evidence yields `INCONCLUSIVE`; failed required checks yield `FAIL`; unresolved rollback/recovery yields `RECOVERY_REQUIRED`.
8. **No ownership interference:** collection must not hold, release, preempt, or otherwise interact with CentralMutationCoordinator leases. It may record read-only snapshots of coordinator state exposed by the daemon.
9. **Bounded resources:** enforce limits for session duration, event count, file count, per-file size, total bundle size, and sampling rate. Disk-full or write failure must terminate capture safely and mark the report incomplete.
10. **No raw secrets:** avoid unrestricted environment dumps, full `/proc/*/cmdline`, full properties dumps, or arbitrary filesystem traversal.

## 3. Architecture

### Phase 1 (this change)

- Versioned JSON report contract and JSON Schema.
- Offline Python standard-library validator for an exported bundle.
- Deterministic tests for verdict precedence, missing evidence, path safety, and integrity checks.
- Operator workflow and acceptance semantics.

### Phase 2 (runtime capture — implemented in this branch)

- C++ daemon session recorder using the existing daemon control tick; no additional sampling timer is installed.
- Bounded JSONL event capture and CSV metrics export with per-session correlation IDs.
- Explicit local daemon commands over a root-only Unix-domain `SOCK_SEQPACKET` socket.
- Read-only telemetry capture; the recorder never acquires mutation leases or calls controller actuation APIs.
- Private session directories (`0700`) and payload files (`0600`), bounded session duration/event volume, SHA-256 manifest, and final directory rename.
- Device acceptance remains `INCONCLUSIVE` in this capture-only phase. Captured data is not a performance pass.

### Phase 3 (operator experience — implemented in this branch)

- Dedicated WebUI tab with all eight profile choices, active-session recovery on view entry, and explicit start/stop controls.
- Bundle export through the existing privileged bridge; only a validated UUID session directory and five fixed filenames are read.
- WebUI verifies the manifest inventory, byte lengths, and SHA-256 digests before constructing a standards-compliant ZIP archive locally.
- The report verdict, required/optional check statuses, and human-readable summary are shown before the operator shares the archive.
- No network upload is performed; the operator uses the normal browser/WebView download flow and manually attaches the ZIP.

CF-DVS Phase 1 and Phase 2 do not modify the production tuning decisions. Phase 2 only adds a read-only sample hook to the existing daemon tick; no extra polling cadence is created.

## 4. Session profiles

| Profile | Intended validation |
|---|---|
| `BOOT_STABILITY` | daemon startup, capability discovery, journal and recovery state after boot |
| `IDLE_EQUILIBRIUM` | idle stability and baseline drift |
| `INTERACTIVE_CPU` | interactive pacing, CPU PSI, scheduler response, and mutation ownership |
| `GAMING_LOAD` | sustained workload, GPU/CPU evidence, thermal response and recovery |
| `MULTITASKING` | memory and I/O pressure response under concurrent workloads |
| `THERMAL_RECOVERY` | cooling trend, headroom validity, and post-load stabilization |
| `OWNERSHIP_CONFLICT` | external writer/OEM arbitration and safe ownership-loss handling |
| `ROLLBACK_RECOVERY` | rollback, journal durability, and recovery admission behavior |

Profiles describe the scenario and required evidence; they do not force controller actions. The initial implementation may report `unsupported` for evidence sources not exposed by the current daemon.

## 5. Bundle layout

```text
cf-dvs-<session-id>.zip
  report.json       # session metadata, metrics, checks, final verdict
  events.jsonl      # ordered, correlated state and mutation lifecycle events
  metrics.csv       # optional flat export for analysis
  summary.txt       # human-readable operator summary
  manifest.json     # payload SHA-256 and byte lengths; excludes itself
```

All timestamps are RFC 3339 UTC with an explicit `Z` suffix. Monotonic nanoseconds should also be recorded for event ordering within one boot. A session ID is a random UUID generated locally; it is not a device identifier.

## 6. Evidence model

Each metric entry has:

- `value`: finite number/string/boolean or `null`;
- `unit`: canonical unit or `null` for dimensionless values;
- `validity`: `valid`, `unavailable`, `unsupported`, `permission_denied`, `stale`, or `invalid`;
- `source`: bounded source label, not an arbitrary path;
- `observed_at`: RFC 3339 UTC timestamp;
- `reason`: optional short explanation, required when value is null.

Never substitute zero for absent PSI, thermal headroom, sensor velocity, GPU frequency, or unsupported UClamp. A metric can be `valid` with a value of zero only when zero was actually measured.

Events should include `session_id`, `sequence`, `timestamp`, `monotonic_ns`, `event_type`, `correlation_id`, and a bounded `data` object. A single correlation ID links candidate planning, observation, mutation transaction, ownership decision, rollback, and recovery outcome. Do not include unbounded raw log lines.

Mutation evidence, when available, records controller, domain, candidate, transaction ID, path token (not arbitrary absolute path), baseline value, requested value, readback value, ownership result, rollback result, and timestamps. Sensitive or device-specific paths must be mapped to stable logical tokens.

## 7. Verdict semantics

Verdict precedence, highest first:

1. `RECOVERY_REQUIRED`: recovery is pending, journal is unresolved, rollback is unverified, or a required safety invariant is violated.
2. `FAIL`: one or more required checks failed.
3. `INCONCLUSIVE`: required evidence is missing/invalid or a required check cannot be evaluated.
4. `PASS_WITH_WARNINGS`: all required checks pass, but one or more optional checks warn or optional evidence is unavailable.
5. `PASS`: all required checks pass and all required evidence is present and valid.

`IMPROVED`, `RETAINED`, `NEUTRAL`, `OWNERSHIP_LOST`, and similar tuner lifecycle verdicts are evidence inputs, not CF-DVS verdicts. `OWNERSHIP_LOST` is not automatically a suite failure: evaluate whether ownership was relinquished safely and whether rollback/recovery state is verified.

## 8. Acceptance checks

A profile defines checks with `required: true|false` and `status: pass|fail|warning|inconclusive|not_applicable`. Required checks must not be silently omitted. The validator enforces the verdict precedence above. Device acceptance should include:

- daemon identity/running state and build commit;
- capability discovery results and unsupported capabilities;
- PSI values and independent validity for CPU, memory, and I/O;
- thermal sensors, aggregate temperature, pressure, headroom validity, and trend validity;
- workload classification and sample freshness;
- tuner lifecycle and candidate correlation;
- coordinator ownership/preemption snapshot when exposed;
- journal/recovery state and rollback readback;
- device/firmware compatibility metadata with privacy-safe identifiers.

## 9. Operator workflow

1. Install a module artifact built from the exact commit being tested.
2. Open the Device Validation tab and select a profile; review the data categories before capture.
3. Start the session, perform the described workload, then stop the session.
4. The UI reads only the five allowlisted files, verifies manifest byte lengths and SHA-256 digests, and blocks ZIP creation on any mismatch.
5. Review the displayed report verdict, acceptance checks, and summary. The current report includes manufacturer/model and Android API metadata; review these fields before sharing.
6. Download the ZIP locally. Attach it to the engineering conversation only when explicitly chosen.

If collection fails, preserve the partial report with `INCONCLUSIVE` or `RECOVERY_REQUIRED`; never report success because the collector exited cleanly.

## 10. Phase 2 runtime capture

The daemon exposes the following local commands while `coreflowd` is running:

```bash
coreflowd --dvs-start INTERACTIVE_CPU
coreflowd --dvs-status
coreflowd --dvs-stop
```

Supported profiles are the eight values listed in Section 4. Only one session may be active. Sessions are stored under `/data/adb/coreflow/cf-dvs/<session-uuid>/`; a `.partial` directory is retained if finalization fails. The control socket is `/data/adb/coreflow/cf-dvs-control.sock`, protected by the private state directory and mode `0600`.

The recorder samples at the daemon's existing control-loop cadence (configured 1–60 seconds), and stops accepting samples after 30 minutes, 20,000 events, or an 8 MiB event log bound. It writes `events.jsonl`, `metrics.csv`, `report.json`, `summary.txt`, and `manifest.json`. No ZIP is created yet; ZIP export belongs to Phase 3. A daemon shutdown attempts to finalize an active session.

The report's `device.acceptance` check is deliberately `inconclusive`. It records evidence for later engineering review and does not claim that the selected workload passed. If no sample was captured, recovery state remains unknown/inconclusive rather than being fabricated as a pending recovery.

## 11. Phase 1 validator

Run from the repository root:

```bash
python3 tools/cf_dvs_validate.py /path/to/cf-dvs-bundle
python3 -m unittest discover -s tests -p 'test_cf_dvs_validator.py' -v
```

The validator uses only Python's standard library. Exit codes: `0` valid bundle, `1` invalid/integrity failure, `2` usage or unreadable input. The validator checks the manifest hashes, report structure, required files, check statuses, and computed verdict. It does not claim that the device behavior itself is correct; it validates report completeness and integrity.

## 12. Phase 3 WebUI export contract\n\nThe WebUI uses the existing KernelSU shell bridge and the daemon's `--dvs-start`, `--dvs-status`, and `--dvs-stop` commands. It does not access the control socket directly. The export path is accepted only when it matches `/data/adb/coreflow/cf-dvs/<UUID>` and the UUID matches the active session. Payload access is limited to `events.jsonl`, `metrics.csv`, `report.json`, `summary.txt`, and `manifest.json`; symlinks and non-regular files are rejected by shell checks.\n\nThe browser-side ZIP writer uses ZIP Store (no compression) to avoid an additional runtime dependency. ZIP entries are created only after validating the native SHA-256 manifest and file sizes. Export has a bounded total-size guard. A WebView without Web Crypto SHA-256 support fails closed and does not create an archive. The WebUI never uploads, posts, or automatically shares the bundle.\n\nThe report's current metadata includes manufacturer, model, Android API level, and build commit; it does not include a serial number, Android ID, account identifier, SSID, or precise location. Operators must review the report before sharing.\n\n## 13. Explicit non-goals

- No cloud telemetry or automatic upload.
- No continuous background monitoring.
- No benchmark score generator or synthetic performance claims.
- No tuning or mutation actuator.
- No replacement for physical-device acceptance, OEM/kernel ABI verification, or maintainer review.
- No automatic release publication or changes to release workflows.
