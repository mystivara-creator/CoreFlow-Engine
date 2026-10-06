# Changelog

## v1.1.0-A — Baseline Intelligence (Development)

- added `BaselineIntelligence` as a separate efficiency-evaluation layer
- preserved `MutationController` restore baseline as the safety baseline
- added five-sample factory/runtime baseline capture before mutation is allowed
- added five-sample post-mutation observation windows
- added Beneficial / Neutral / Regression / Inconclusive outcomes
- regression outcomes restore the MutationController baseline
- added deterministic Baseline Intelligence host tests
- integrated the feature into `AutonomousEngine::tick()`
- reset efficiency state on startup and runtime rediscovery


## v1.8.0 — Production Foundation

- normalized project/module/runtime versioning to 1.8.0
- added deterministic host-side tests for policy, configuration and mutation
- added native instance locking to prevent duplicate daemons
- added safe SIGUSR1 runtime rediscovery with signal-only flag handling
- added persistent runtime configuration under `/data/adb/coreflow/config.ini`
- improved CPU policy discovery with governor lists and current limits
- added I/O, VM, uClamp and cpuset capability inventory
- added CPU utilization measurement from `/proc/stat`
- added bounded, verified CPU-governor mutation with baseline restoration
- kept charging mutation disabled until semantic validation is proven
- hardened Android build flags and CI checks
- removed the fake one-byte daemon from the source repository; CI must provide the real ARM64 ELF
