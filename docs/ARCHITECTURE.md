# Architecture

## Principles

1. Validate infrastructure before game-specific integration.
2. Keep the compatibility layer explicit, testable, and evidence-based.
3. Keep proprietary game data outside Git and CI artifacts.
4. Treat synthetic tests as infrastructure validation, not game compatibility.

## Two-disc game-data catalog

The Xbox 360 Install Disc and Play Disc are represented as separate source
directories. The catalog scans each independently, records each file's source
disc and relative path, and builds a single virtual namespace without changing
the source data. Symlinks are not followed.

Virtual paths are normalized and case-insensitive. Byte-identical duplicates
are represented as one content variant with both origins. Different contents
at the same path remain distinct variants and are reported as a conflict;
lookup and file opening never pick a conflicting version implicitly.

The generated JSON manifest includes paths, file sizes, SHA-256 digests, source
attribution, conflict status, and executable-candidate hints. The catalog
recognizes `.xex` extensions and `XEX2` magic as candidates anywhere in either
source, but does not choose the actual launch executable. Dependency validation
is available for an explicit list; format-aware executable/import parsing must
be added after inspecting the real files.

The Android implementation must mount lawfully supplied extracted data
through this virtual resource system and must not require physical discs at
runtime. Data import and persistence UX remain future work.

## Other foundation components

The repository also contains synthetic PowerPC decoding and ARM64 emission,
memory allocation experiments, and native/Android build workflows. They are
independently tested and are not evidence that GTA V is executable.

## Android application shell

`android/app/` is a minimal Java Android application with a launcher Activity.
It loads `native_lib` over JNI and initializes the existing runtime memory
arena. Native initialization failures are reported in the UI and Android log.
Activity resume/pause events are logged, and the Vulkan instance is destroyed
when the Activity shuts down.

The native Vulkan probe calls the system loader to obtain the instance API
version and enumerate physical devices and their device extensions. Vulkan
unavailability is reported separately from native-library initialization; the
app can still show diagnostics. This milestone does not create a rendering
surface or submit GPU work.

The Gradle module builds only `arm64-v8a`, uses the root CMake project for the
shared library, and packages an installable debug APK independently of game
data. See [ANDROID_TESTING.md](./ANDROID_TESTING.md) for download and device
test instructions and [KNOWN_ISSUES.md](./KNOWN_ISSUES.md) for validation
limits.

See [GAME_DATA_STATUS.md](./GAME_DATA_STATUS.md) and
[ROADMAP.md](./ROADMAP.md) for verified status and the integration plan.
