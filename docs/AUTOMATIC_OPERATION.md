# Automatic CoreFlow operation

The module starts bounded optimization after installation and reboot. There is no
Observe/Standby selection in the installer or WebUI. KernelSU WebUI is a control
surface, not the owner of the daemon lifetime.

## What was taken from the Encore reference

Reference reviewed: Rem01Gaming/encore at
`529cd0cced6fd83a244ac5a8cb6f074df25d9b5d`.

- Its service script launches the daemon after boot.
- Its daemon evaluates an initial profile and reevaluates when foreground game,
  display state or battery saver changes.
- Its profile layer skips reapplication when the requested profile is already active.
- Its default preferences enable tweaks without a separate manual arming step.

CoreFlow adopts the automatic activation and simple controls. It keeps its own
capability-driven candidate generation, measured outcomes, journal and rollback.
No Encore source or per-vendor tuning table is copied. The comparison is about
control flow, not proof that either implementation has better performance.

References:
- https://encore.rem01gaming.dev/guide/faq
- https://github.com/Rem01Gaming/encore/blob/529cd0cced6fd83a244ac5a8cb6f074df25d9b5d/module/service.sh
- https://github.com/Rem01Gaming/encore/blob/529cd0cced6fd83a244ac5a8cb6f074df25d9b5d/jni/Main.cpp
- https://github.com/Rem01Gaming/encore/blob/529cd0cced6fd83a244ac5a8cb6f074df25d9b5d/jni/EncoreConfigStore.cpp

Encore's Binder callback infrastructure is not transplanted in this patch.
CoreFlow continues to sample using its configured cadence and existing signals;
this package does not claim a new event-driven Android framework backend.

## Installed configuration and migration

The shipped configuration explicitly enables bounded mutation, per-domain grants
and interactivity assistance. The default sample cadence is 5 seconds. Legacy
`mutation_mode`, `mutation_armed` and `interactivity_mode` tokens remain accepted
internally for protocol/config compatibility, not as user-facing operating modes.
Uninitialized or invalid C++ configuration still grants no authority.

The installer atomically merges existing configuration with packaged defaults:
- preserves monitor interval, numeric tuning, unknown keys and explicit domain denials;
- fills missing keys from the packaged configuration;
- collapses legacy mode/arming keys to one canonical active configuration;
- enables interactivity assistance during installation; its separate feature switch
  can still be turned off afterward without stopping the optimization engine;
- never clears SAFE_MODE or DISABLE, and rejects a symlinked config.ini.

Installation has no volume-key mode prompt. Existing operator stop markers are
reported instead of silently cleared. Use Start to resume DISABLE explicitly.
Recovery holds still require the appropriate recovery procedure.

## WebUI behavior

Engine Settings contains runtime and interactivity parameters, with no mode cards
or arm/observe confirmation banner. Saving writes active compatibility fields but
never changes stop/recovery markers. Explicit Start may remove DISABLE only when
SAFE_MODE is absent. Stop and Restart retain their existing daemon lifecycle behavior.
Raw legacy status codes remain parseable; presentation uses Active, Held, or an
explanation of an invalid/incomplete configuration.

Waiting for a baseline, measuring a change, cooldown, preserving a successful
setting, or finding no justified candidate are normal parts of active operation.
Automatic operation is not a demand to write a tunable every sample.

## Conflict regression included

The continuity fix is part of this package. Externally replaced targets are
released durably and excluded across actuator classes until rediscovery/restart.
Other admitted targets remain eligible and configured operation is preserved.
Persistence failures pause new changes until recovery finishes. This is not an
atomic lock shared with vendor processes and cannot eliminate every external race.

## Build and test scope

The offline WebUI is rebuilt from source. package-lock.json and npm ci pin the
resolved frontend dependency graph in the workflow. Host tests verify active
packaged defaults, fail-closed invalid configuration, migration of legacy config,
explicit exclusions, marker preservation, lifecycle controls, and scoped handoff.

Actual NDK linking and device validation are still required before treating this
new source package as a verified Android release. No comparative device benchmark
or hardware lifetime improvement is claimed.
