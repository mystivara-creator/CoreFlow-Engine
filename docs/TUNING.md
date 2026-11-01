# Tuning Documentation

Document every user-visible or safety-relevant tuning behavior here.

For each tunable, record:

- Purpose and expected effect.
- Valid range and default value.
- Conditions under which CoreFlow may change it.
- Safety limits enforced by `src/conscience`.
- Rollback or recovery behavior.
- Device or chipset-specific caveats.

Keep tuning policies explicit. Avoid undocumented automatic behavior that could reduce stability, battery life, or thermal safety.

## v3.1.0 settings workflow

The WebUI provides per-field purpose, default, units, valid range and tradeoffs in
Indonesian. A settings choice changes a draft only. Review → Save writes config;
Restart applies it. Discard reverts the draft, not the device. The quieter choice
uses an 8-second monitor interval and 70% minimum confidence and disables temporary
interactivity assistance; the main optimizer remains active. Faster observation
uses 2 seconds and 65% confidence, without increasing boost amplitudes. Neither is
an FPS/thermal guarantee. Existing authority, capability, journaling and recovery
gates remain mandatory. See [checkpoint scope](CHECKPOINT_3.1.0.md).


## ThermalGuard and coordinated plans

ThermalGuard withdraws performance boosts while permitting measured,
stabilizing-only resource plans. Existing verified cooling is held through a
confidence dip; no new write bypasses the configured threshold. Kernel options,
live values and reviewed ABI bounds define candidate steps, with a cumulative
factory-relative budget for cgroup capping. There is no fixed scheduler/governor
pairing preset. [Execution and recovery rules](COORDINATED_THERMAL_PLANS.md).


## Condition-driven selection and retention

CPU and resource proposals are compared before writes, without domain rotation.
Independent useful domains can join one measured plan; dependency order only
controls safe execution. No measured demand keeps the current configuration;
measured demand without an admissible gain is a separate diagnostic. Selection
scores are priors, and measured outcomes decide retention/rollback. Accepted
settings survive Idle while an unfinished Idle trial is canceled. See the
[selection details](COORDINATED_THERMAL_PLANS.md).


## v3.1.1 outcome interpretation

The native CPU table reports current kernel values; OS writability is not a
mutation grant. Current/min/max frequency telemetry does not authorize frequency
limit writes. Last evaluation displays raw units in its metric keys (Celsius,
load1, fractions and milliseconds), per-side coverage and normalized scores.
Changes below max(absolute noise, twice combined standard error) score neutral:
thermal 0.3 C, load1 0.10, CPU 0.02, memory 0.01, PSI 0.01, latency 1 ms, jank 0.005.
The relative scale has a finite floor for zero-origin measurements. Existing
rendering-regression and stress recovery rules remain. Short windows cannot
separate every workload change from a tuning effect; inspect evidence before
claiming an improvement. See [acceptance requirements](CHECKPOINT_3.1.1.md).
