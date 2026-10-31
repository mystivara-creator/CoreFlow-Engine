# Adaptive rendering measurement checkpoint

Scope: outcome attribution and rendering coverage on top of `a79e3d9`.
This checkpoint establishes source-level behavior; device performance is pending.

## Findings and fixes

1. **Missing rendering could be retained as an improvement.** The evaluator
   previously omitted rendering whenever either window had fewer than two valid
   rendering samples. Lower CPU/load/temperature could then produce Beneficial
   even when presentation telemetry disappeared. If either window contains any
   rendering evidence, both now require at least two valid samples. Otherwise the
   result is invalid/Inconclusive, which the existing loop rolls back and excludes
   from experience learning. When neither window has rendering, ordinary
   non-rendering outcome evaluation remains available.
2. **Interaction history crossed measurement windows.** The rolling 120-frame
   interaction analyzer could carry pre-mutation frames into a post-mutation
   sample at low presentation rates. Runtime outcome samples now use a separate,
   fixed-memory analyzer containing only newly collected frames for that poll.
   Interaction hysteresis retains its existing history.

`EFFICIENCY_OUTCOME` now includes `rendering=<baseline>/<observation>` sample
coverage and `rendering_regression=yes|no`. These are counts of runtime samples
with valid frames, not counts of presented frames. The telemetry remains an
actual-presentation interval proxy, not GPU execution time or app render duration.

## Local verification

- All 45 host core sources compiled with C++20 and the project's strict warning
  flags, including `-Werror`; all 25 existing host test executables passed.
  CMake was unavailable in this environment, so these were built directly with
  G++ using the source platform classification and fixture definitions in CMake.
- Safety foundation, source release, WebUI status/render/control contracts and
  installer harness passed (11 installer scenarios).
- Temporary regression probes reproduced three former Beneficial outcomes:
  rendering disappearing, appearing only after mutation, and falling to one
  observation sample. All now return invalid/Inconclusive. Comparable rendering
  can still produce Beneficial; non-rendering neutral trials remain Neutral;
  changed refresh periods remain Inconclusive.
- Independent frame-batch probe: 120 older missed frames followed by 10 clean
  frames leaves rolling jank above 90%, while the fresh batch correctly reports
  zero misses. Targeted ASan/UBSan probes passed with leak detection disabled
  because LeakSanitizer cannot run under this environment's process tracing.
- The Android-only loop passed strict host syntax checking with an Android log
  declaration stub. This is not an Android NDK build or device validation.

## Device validation gate

1. Build the exact reviewed commit through Android CI before installing it.
2. On the target phone, compare Standby and Adaptive using the same application,
   refresh-rate setting, scene/actions, brightness and charging state. Start each
   pass at a comparable temperature. Repeat at least three paired passes.
3. Verify the rendering layer/availability in status, then correlate
   `EFFICIENCY_OBSERVATION_BEGIN`, `EFFICIENCY_OUTCOME` and the subsequent
   retain/rollback record. Missing coverage must yield Inconclusive and a verified
   rollback. A refresh-rate change must not establish an improvement.
4. After a pass, collect the outcome log in Termux:

   ```sh
   su -c 'logcat -d -v threadtime -s CoreFlowAutonomous:I "*:S" > /sdcard/Download/coreflow-outcome-log.txt'
   ```

5. Record the exact commit, firmware, workload, starting/ending temperatures,
   rendering availability and rollback results. A successful write or lower CPU
   utilization alone does not prove smoother rendering. Capture presentation
   timing separately for comparative performance claims.

Do not proceed to additional rendering actuators until Android build validation
and this device evidence are available. Hardware, firmware, sustained thermal
behavior and collector overhead are not certified by host checks.
