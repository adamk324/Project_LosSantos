# Architecture

## Principles

1. Validate infrastructure before game-specific integration.
2. Keep the compatibility layer explicit, testable, and evidence-based.
3. Keep proprietary game data outside Git and CI artifacts.
4. Treat synthetic tests as infrastructure validation, not game compatibility.

## Two-disc game-data catalog

The Xbox 360 Install Disc and Play Disc are separate inputs, each of which can
be an extracted directory or a read-only XDVDFS/XISO image. A catalog can
register multiple sources for either disc, each with a stable caller-supplied
or generated source ID. It scans every source independently and builds one
case-insensitive virtual namespace without changing or combining source data.
Directory symlinks are not followed.

Each manifest variant records its size, SHA-256, executable-candidate hint,
and every origin (disc, source ID, source-relative path, and byte offset).
Identical files are merged into one variant only after matching size/hash and
a byte-for-byte comparison. Different contents at the same virtual path remain
separate variants and are always reported as a conflict.

Resolution rejects conflicts by default. Callers must explicitly request
`SelectSource` with the source ID to open one variant. Directory resources
resolve to their original file path; ISO resources resolve to a container path
and bounded byte range and are opened as a read-only stream. The catalog does
not extract or rewrite ISO contents. Manifest schema version 2 records sources,
formats, provenance, byte offsets, and content variants.

XEX2 magic and `.xex` filenames are only executable-candidate hints. The
catalog does not select a launch target or establish dependencies. Required
resource validation checks a caller-provided list and marks both missing and
conflicting paths as issues.

Synthetic tests cover directory and ISO source pairs, duplicate and conflicting
files, explicit source selection, missing resources, invalid ISO metadata,
manifest provenance, bounded reads, and malformed STFS package headers. A
metadata-only STFS reader exposes public header fields; it does not parse
package file tables or protected payloads. These tests do not prove full
XDVDFS compatibility or GTA V execution. See
[GAME_DATA_STATUS.md](./GAME_DATA_STATUS.md) and [RESEARCH.md](./RESEARCH.md)
for observed disc metadata and parser limits.

The Android implementation must mount lawfully supplied data through this
virtual resource system and must not require physical discs at runtime. Data
import and persistence UX remain future work.

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
