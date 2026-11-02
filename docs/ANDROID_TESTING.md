# Android bring-up: build, install, and test

## What this build is

The debug APK is an ARM64 Android application with a platform-native Activity
and a C++ JNI library. It initializes the existing runtime memory arena,
queries the installed Vulkan loader for its API version, physical devices, and
device extensions, and displays Android/device/memory diagnostics.

Vulkan is a capability probe only. This build does not render a frame, translate
Xbox 360 Xenos commands, or run GTA V. Both game discs and the game executable
are expected to be unavailable.

## Download the APK

1. Open the repository's [Android build workflow runs](https://github.com/adamk324/Project_LosSantos/actions/workflows/android-build.yml).
2. Select a successful run for the commit being tested.
3. Download the `project-los-santos-android-arm64-debug` artifact.
4. Extract `app-debug.apk` from the artifact.

The APK is unsigned beyond the standard debug signing used by Android Gradle
Plugin and is intended for development testing.

## Install on the Xiaomi 15T Pro

The device must allow installation from the selected file manager/browser, or
you may install with Android platform tools:

```powershell
adb install -r .\app-debug.apk
adb shell monkey -p com.project.los_santos.debug 1
```

The `arm64-v8a` ABI is required. The app does not request access to either disc
or require game data to start.

## Expected checks

- The main screen says the native library is loaded and native/runtime
  initialization succeeded.
- Android API, manufacturer, model, supported ABIs, processor count, memory,
  and process ID are read from the running device.
- Vulkan is reported from a live native Vulkan API query. If the device or
  system loader does not support Vulkan, the report shows the returned error;
  that does not prevent the diagnostics app from starting.
- The Vulkan section lists detected physical devices and the device extensions
  returned by the loader. This is not a GPU rendering or performance test.
- Disc 1, Disc 2, and the game executable show as not supplied/unavailable.
- **Run native C++ check** displays a message produced by a JNI call into the
  loaded C++ library.
- **View application logs** displays the Activity's in-app event log;
  Android Logcat entries use the `ProjectLosSantos` tag.

## Build locally

Install JDK 17, Android SDK platform 35, build-tools 35.0.0, CMake 3.22.1, and
NDK 26.3.11579264. From the repository root, use Gradle 8.9:

```powershell
gradle --no-daemon :app:assembleDebug
python .\scripts\verify_apk.py .\android\app\build\outputs\apk\debug\app-debug.apk
```

The APK verifier checks the Android manifest, DEX bytecode, the packaged
`libnative_lib.so`, and its AArch64 ELF machine/ABI.

## Evidence and limitations

CI validates compilation, APK packaging, and the ARM64 shared-library ELF
metadata. It does not launch the APK in an emulator or on physical hardware.
No Xiaomi 15T Pro is attached to this build environment, so installation,
Activity launch, Vulkan driver behavior, and device-specific compatibility
remain unverified until tested on the target device.
