# CoreFlow v3.1.1 — maintenance release checkpoint

This checkpoint addresses native CPU visibility and measurement/diagnostic defects
reported after v3.1.0. It prepares a reviewable source and Android build; it does
not claim phone soak results that have not been collected.

## Confirmed findings and corrections

| Finding | Correction |
| --- | --- |
| Native WebUI hid the CPU topology section and only listed owned CPU writes. An unchanged governor appeared absent. | A native-only table reads CPU policies from status: live governor, advertised alternatives, OS writability, current/min/max frequency. Empty inventory stays explicitly unavailable; preview values never fill it. |
| CPU utility zero did not explain whether a grant, an equal-scoring governor or a missing alternative blocked selection. | CPU preview retains explicit reasons. The chooser carries separate CPU/resource explanations, including existing dependent ownership; changed no-write reasons enter the trace. |
| Policy NO_ACTION appeared during verified mutation measurement; interaction IDLE looked like the plan's intervention level. | The main badge describes execution activity. Detailed policy recommendation remains distinct; interaction is explicitly labelled. Skip text follows the actual selection. |
| Zero-origin PSI/latency changes scored neutral; tiny relative CPU/load changes could look large. | Compare against finite per-metric scales, with absolute noise and combined standard-error tolerance. A new measured stall from zero can regress. |
| A changed charging context could still influence an outcome from other metrics. | The comparison is inconclusive and cannot establish benefit/regression. Foreground-change cancellation and refresh-period checks remain in force. |
| Exported outcome rows kept a generic regression reason but no measurement evidence. | Native evaluation, status, trace parser, journal and JSON export preserve reason, validity, context, score/confidence, means and counts for seven metrics; exports also retain session, cycle and event sequence. |
| Resource targets such as a vendor WALT operating point were unexplained. | Proposal diagnostics preserve source type and bounded vendor-origin text. A firmware value is not represented as a guessed numeric increment. |
| MHz table conversion could multiply an overflowing signed value. | Strict numeric parsing and a pre-multiplication bound reject invalid advertised entries. |

## Invariants

- No fixed CPU/resource mutation turns and no forced governor switching to make
  the table look active. Equal/worse alternatives do not justify a write.
- Frequency bounds remain read-only; kernel thermal protection remains authoritative.
- Every write retains grants, target-bound authority, journal, read-back and recovery.
- Independently justified domains can share one bounded measured cohort. Recovery
  still unwinds dependents before anchors, preserving failed obligations.
- Sample leases, experience V3 and mutation journal formats remain unchanged.
  Older experience is advisory; the changed scoring does not retrospectively
  prove or disprove an earlier trial. Fresh baseline windows remain mandatory.
- A scored metric needs at least two valid observations on each side; a single
  intermittent sensor reading cannot establish a comparison.
- Outcome comparison uses four baseline and four observation samples. Noise
  handling reduces false sensitivity; it is not proof of application causality.
- Native status schema stays additive version 1. Older snapshots have explicit
  unavailable CPU/evaluation data, never fabricated preview hardware.

## Verification

Local and CI results must be attached to the exact final commit. The required
checks are strict host C++20, all host suites, ASan/UBSan, WebUI build and native
contracts, installer scenarios, safety/source-package checks and Android ARM64
linking/packaging. Leak detection remains enabled in CI; the traced local run
uses detect_leaks=0. The local Android log stub is syntax validation only.

### Local verification record

- Strict C++20 host core compilation and all 28 host suites passed.
- All 28 ASan/UBSan suites passed locally with leak detection disabled in the
  traced environment; CI retains leak detection.
- WebUI: 630 checks passed across status (482), render (14), controls (70) and
  actual React bridge diagnostics (64). Clean source-package checks passed.
- Installer harness: 11/11 scenarios; safety contract and Android orchestrator
  strict syntax (log stub) passed.
- The exact final commit still needs its own Android CI linking/packaging result;
  local syntax checks cannot replace that build or the acceptance below.

## Device acceptance before a stable release designation

1. Install the Android artifact for this exact commit and verify v3.1.1 in status.
2. Verify real governors/advertised alternatives in the native CPU table while
   no CPU mutation is active; compare with kernel files. Missing reads show unavailable.
3. Exercise light use, app switches and a sustained workload. An unchanged
   governor is legitimate when no better admissible proposal exists.
4. Inspect a completed trial's seven before/after metrics and coverage in both
   WebUI and exported trace; verify that the export retains its evaluation object.
5. Exercise thermal transitions, charging changes, stop/restart and reboot.
   Verify journal recovery and preservation of lifetime sample numbering.
6. Record device/kernel, commit, workload, duration, thermal/frame/power evidence
   and failed recoveries. Compare stock and tuned workloads under similar conditions.

A green CI build supports a tested build checkpoint. Field-stable status still
requires the acceptance above; counters or the absence of writes alone do not
prove optimality or stability.
