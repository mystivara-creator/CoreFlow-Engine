# Build Status — v1.8.0

The source tree is production-ready for the repository/CI build path.

Validated in this environment:

- C++17 host tests: PASS (3/3)
- strict source syntax checks with `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wformat=2 -Wundef -Werror`: PASS
- module shell syntax checks: PASS
- release layout checks: PASS

The workspace used for this review does not contain the Android NDK toolchain, so an ARM64 Android ELF was not produced locally. The repository workflow is configured to build with NDK 27.2.12479018, verify the aarch64 ELF, package the production module and publish a SHA-256 checksum.

Do not treat the repository source archive as a flashable module. The flashable module is the CI artifact containing the compiled `system/bin/coreflowd`.
