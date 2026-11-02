# Known issues and validation limits

- The Android app is a development diagnostics shell, not a GTA V port or
  renderer.
- No emulator or physical device is attached to CI. APK build/package checks
  do not prove that the Activity launches on a device.
- The Xiaomi 15T Pro / Immortalis-G925 has not been tested here. Its Vulkan
  driver, supported extensions, and runtime stability need hardware testing.
- The Vulkan check creates an instance and enumerates physical devices and
  device extensions. It does not create a surface, submit GPU work, benchmark
  the GPU, or validate Xenos translation.
- Native status indicates that the JNI library and runtime memory arena
  initialized. It does not establish Xbox kernel or game compatibility.
- Game data is not installed. Disc 1, Disc 2, and the executable are expected
  to be absent; no game-specific dependencies have been inspected.
- Device system information and app memory are sampled on demand and are
  diagnostics, not continuous telemetry.
- In-app logs are kept in Activity memory for the current process. Android
  Logcat provides the platform log stream.
