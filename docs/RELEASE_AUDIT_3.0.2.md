# 3.0.2 release audit

Baseline: main at 51756496e64fef52d5064be1370ecb497eae736a.

Reviewed automatic activation, configuration migration, supervisor lifecycle,
continuity handoff, mutation ownership and recovery, asynchronous trace writer,
WebUI contracts, release metadata and CI triggers.

## Findings corrected

- High: service.sh applied global ART, HWUI and compositor properties before
  payload validation and outside daemon authorization, journals and outcome
  measurement. Removed these boot-time writes. Vulkan availability alone did
  not establish that forcing the renderer would improve this device.
- Medium: supervisor forced daemon scheduling, OOM adjustment and cpuset
  placement without workload evidence. Removed those overrides; normal platform
  scheduling now applies.
- Low: installer existence check missed a dangling config symlink. Reject both
  dangling and resolved symlinks; installer regression scenario added.
- Low: lockfile-only changes did not trigger the Android build workflow. Added
  package-lock.json to its push paths.

Release metadata is 3.0.2 / 3020 across CMake, runtime, module, update feed,
frontend and documentation. Historical changelog entries remain unchanged.

## Validation

- Host CTest: 27/27 passed.
- AddressSanitizer + UndefinedBehaviorSanitizer: 27/27 passed. Leak detection
  disabled in this execution environment; this is not a leak-sanitizer result.
- WebUI: 215 status, 70 control and 14 render contract checks passed.
- Installer: 11 scenarios passed, plus real C++ configuration parser validation.
- TypeScript/Vite production build, source-release contract, safety foundation,
  shell syntax and diff whitespace checks passed.
- Service missing-payload probe confirmed no framework property writes.

The user reported Android NDK success for the preceding commit. This modified
3.0.2 tree still needs its own Android CI build and device soak before publication.
Host validation does not establish OEM compatibility, absence of all races,
hardware longevity or performance superiority over Encore. Compare sustained
frame timing, responsiveness, temperature and energy on the same device and
workload before making that claim. No release/tag has been published by this audit.
