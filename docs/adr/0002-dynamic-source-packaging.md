# 0002. Dynamic source packaging

## Status

Accepted.

## Context

`cmake/CoreFlowSources.cmake` listed every engine source by name, and
`CMakeLists.txt` listed every host test by name. Each new module therefore needed
edits in at least two places, and a forgotten entry compiled silently out of the
daemon. The engine is meant to grow by adding modules, not by wiring them.

## Decision

1. Sources and tests are discovered by glob with `CONFIGURE_DEPENDS`. No file is
   named in the build. Adding `src/**/*.cpp` or `tests/*_tests.cpp` requires no
   CMake edit.
2. Platform membership is decided per file. An explicit tag in the first 40 lines
   (`// @coreflow-platform: any|android`) wins. Without a tag, a file that includes
   an Android platform header (`<android/...>` or `<sys/system_properties.h>`) is
   device-only, and every other file is platform-neutral and belongs to
   `coreflow_core`.
3. Each `tests/<name>_tests.cpp` becomes `coreflow_<name>_tests`, a CTest entry,
   and is compiled with `-Werror`. Test files keep their own `main()`.
4. `tools/verify_safety_foundation.sh` checks structure (the glob covers `src/`,
   the test glob is present, `resource_mutation.cpp` is not device-only) instead
   of the presence of file names. The test-name `grep` checks were removed because
   test names are now derived, not written.

## Verification

- The platform rule was checked against the previous manifest: every file that was
  listed as device-only is still device-only. Three files previously listed as
  device-only (`observer.cpp`, `decision_trace.cpp`, `thermal_predictor.cpp`) have
  no platform headers, compile on the host, and are now part of `coreflow_core`.
- Adding a new source and a new test with no CMake change built and passed.
- `cmake` was not available in the authoring environment. The CMake logic was
  checked by an equivalent script. The first CI run must configure and build
  with real CMake before this ADR is considered closed.

## Consequences

- A file that is platform-neutral by accident (it includes a platform header) must
  be tagged explicitly, or it moves to the device-only set. The tag is the contract.
- Test executable names changed (`coreflow_tests` is now `coreflow_mutation_tests`).
  CTest names follow the file names.
- A file under `src/` that is not meant for the daemon or the host must be placed
  under a directory with a documented role, and tagged, rather than excluded by
  name.
