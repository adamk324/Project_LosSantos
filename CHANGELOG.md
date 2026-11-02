# Changelog

## 2026-11-02

- Established the initial CMake project structure for PowerPC decoding, ARM64
  emission, runtime memory management, and synthetic tests.
- Published the local foundation alongside the official repository's existing
  MIT license without replacing either history.
- Added native and Android GitHub Actions workflows; native tests pass on
  Windows and Ubuntu.
- Updated Android SDK setup to install `platform-tools` instead of the
  unavailable legacy `tools` package.
- Passed the installed NDK path explicitly into CMake and selected the
  runner-provided Makefiles generator for the Android cross-build.
- Added a read-only two-disc game-data catalog with source attribution,
  duplicate/conflict reporting, resource validation, and synthetic fixtures.
- Documented game-data status, architecture, and follow-up integration stages.
