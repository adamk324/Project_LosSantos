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
- `android/` — Android native library skeleton and app integration points.
- `tests/` — synthetic validation for core infrastructure.
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

Because the Xbox 360 GTA V ISO is not available in this environment, the project validates the following with synthetic fixtures:

- PowerPC instruction decoding
- Control flow and branch handling
- Register state transitions
- Memory arena allocation behavior
- ARM64 instruction emission primitives

These tests are intentionally limited to the infrastructure itself and never claim to validate game behavior.

## Current status

This repository provides the initial foundation for the project and is intended to be expanded incrementally as the architecture matures.
