# Project_Los Santos

Project_Los Santos is a long-term engineering effort to build a native Android port of the Xbox 360 version of Grand Theft Auto V, using a codebase grounded in static recompilation, compatibility layers, runtime abstractions, and synthetic validation.

This repository is intentionally structured as a practical foundation rather than a misleading "game-ready" port. The project prioritizes architecture, testability, and reproducible builds over unsupported assumptions.

## Goals

- Study and reuse the relevant parts of the LibertyRecomp and LibertyRecompAndroid architecture.
- Build a generic PowerPC instruction decoding and ELF/PE-like binary-loading foundation.
- Design a native ARM64 recompilation pipeline using synthetic, independently testable fixtures.
- Provide a platform abstraction for Android, SDL3, Vulkan, audio, file I/O, logging, and controller support.
- Keep CI capable of building and testing the non-game-specific source tree.

## Repository layout

- `recomp/` — PowerPC decode and ARM64 code generation experiments.
- `runtime/` — memory, thread, and compatibility abstractions.
- `runtime/gamedata/` — read-only catalog for separate Install Disc and Play
  Disc directory/ISO sources, plus metadata-only Xbox content package
  inspection.
- `android/app/` — installable Android debug app and native JNI entry points.
- `android/src/main/cpp/` — JNI lifecycle and Vulkan capability probe.
- `tests/` — synthetic validation for core infrastructure.
- `docs/` — architecture, game-data status, and development roadmap.
- `.github/workflows/` — native and Android CI configurations.

## Build instructions

```bash
cmake -S . -B build -G "Ninja"
cmake --build build
ctest --test-dir build --output-on-failure
```

For Android-specific validation:

```bash
cmake -S . -B build-android \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-29 \
  -DBUILD_ANDROID=ON \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake"
cmake --build build-android
```

## Synthetic validation

The actual disc images and extracted game files remain outside this repository.
Synthetic fixtures validate the following infrastructure without putting game
data in CI:

- PowerPC instruction decoding
- Control flow and branch handling
- Register state transitions
- Memory arena allocation behavior
- ARM64 instruction emission primitives
- Two-disc directory/ISO catalog, bounded file access, and conflict reporting
- Synthetic XDVDFS images and STFS package headers
- Android ARM64 app, JNI runtime initialization, and Vulkan capability reporting

These tests are intentionally limited to the infrastructure itself and never
claim to validate game behavior.

The catalog does not assume either disc is sufficient and never overwrites
conflicting files. Metadata from both supplied discs has been inspected, but
the primary launch executable, complete dependency set, and Xbox-specific RPF
directory formats remain unverified. See
[`docs/GAME_DATA_STATUS.md`](docs/GAME_DATA_STATUS.md) and
[`docs/RESEARCH.md`](docs/RESEARCH.md).

## Android bring-up

The Android workflow builds an ARM64 debug APK and uploads it as the
`project-los-santos-android-arm64-debug` artifact. See the
[Android installation and testing guide](docs/ANDROID_TESTING.md) for download,
installation, diagnostics, and validation details.

The APK is an independent platform bring-up shell: it starts without game data
and reports actual Android/device information. The Vulkan probe is not a
renderer, and no physical-device compatibility is claimed until tested on
hardware.

## Current status

This repository provides the initial foundation for the project and is intended to be expanded incrementally as the architecture matures.
