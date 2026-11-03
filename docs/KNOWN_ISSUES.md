# Known issues and validation limits

- The Android app is a diagnostics shell, not a GTA V port or renderer.
- No emulator or physical device is attached to CI. APK build/package checks
  do not prove that the Activity launches on a device.
- The Xiaomi 15T Pro / Immortalis-G925 has been reported working by the user,
  but no independent hardware test is available in this repository.
- The Vulkan probe enumerates system capabilities; it does not create a
  surface, submit GPU work, benchmark the GPU, or validate Xenos translation.
- Native status indicates that JNI and the runtime memory arena initialized.
  It does not establish Xbox kernel or game compatibility.
- The ISO reader's complete inventory now matches the two supplied images'
  independently recorded counts and conflicts. Broader XDVDFS/XGD3 layouts
  and malformed-input cases still need additional coverage.
- Conflicting filenames are preserved and rejected by default. A caller can
  explicitly select a source ID, but no game-specific policy has yet been
  justified for the observed conflicts.
- The two XEX metadata records differ in media ID and disc number, but the
  primary game launch target and complete game dependency set remain unknown.
- The four PIRS files are identified as STFS packages from public header
  metadata. The metadata-only validator has synthetic tests; internal file
  tables and package relationships have not been independently validated.
- The conflicting RPF files have `RPF7` signatures and header-like fields, but
  their directory tables and nested resource entries are not parsed. PC GTA V
  reader compatibility with these Xbox 360 files is unproven; encrypted
  metadata has not been decrypted.
- Game data is not installed or packaged in this project. Disc images and
  extracted game files must remain outside Git and CI artifacts.
- Android import/persistence UX and game-specific dependency validation remain
  future work.
