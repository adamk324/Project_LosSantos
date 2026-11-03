# Roadmap

## Completed foundation

- [x] Build an installable ARM64 Android diagnostics app with JNI native
  initialization and a Vulkan capability probe.
- [x] Upload the debug APK as a GitHub Actions artifact and verify its ABI.
- [x] Scan Install Disc and Play Disc independently as directories or XDVDFS
  ISO sources.
- [x] Track source identity, disc, virtual path, and byte-range provenance.
- [x] Hash resources; merge byte-identical duplicates only after byte
  comparison; preserve all differing variants as explicit conflicts.
- [x] Reject ambiguous resolution by default and support caller-selected
  source IDs.
- [x] Generate a deterministic schema-version-2 manifest.
- [x] Add synthetic tests for directory and ISO catalog sources, resource
  lookup, conflicts, missing resources, invalid volume descriptors, and source
  selection.
- [x] Add a metadata-only STFS public-header reader with synthetic valid and
  malformed-header tests.
- [x] Inspect metadata for both `default.xex` files, four Disc 1 PIRS/STFS
  packages, and the four conflicting `xbox360a.rpf`/`xbox360b.rpf` headers.
- [x] Record evidence and uncertainty in [GAME_DATA_STATUS.md](./GAME_DATA_STATUS.md)
  and [RESEARCH.md](./RESEARCH.md).

## Next: validate real-source scanner and formats

- [ ] Run the hash-producing catalog scan over both supplied images and
  reconcile its file counts, duplicate hashes, and conflicts with the earlier
  independent inventory.
- [ ] Validate the XDVDFS scanner on additional synthetic structures and
  unusual but valid metadata before treating it as production-ready.
- [ ] Determine the launch behavior of the two XEX builds using additional
  non-decrypting evidence; do not select a primary executable based only on
  disc metadata or filename.
- [ ] Investigate the four STFS packages' public file-table metadata and
  relationship without decrypting or extracting protected payloads.
- [ ] Find or implement a GTA V Xbox 360-specific RPF directory-table reader;
  validate archive structure before using any counts or entry types.
- [ ] Derive required game resources from validated executable metadata and
  check them through the virtual catalog.

## Later: runtime and Android integration

- [ ] Add Android game-data selection/import UI and connect it to the unified
  catalog without requiring physical discs at runtime.
- [ ] Add a format-aware executable loader that resolves all reads through the
  unified virtual filesystem.
- [ ] Add real Vulkan surface/swapchain rendering and device-specific graphics
  validation before claiming renderer support.
- [ ] Exercise loading with lawfully supplied data while keeping proprietary
  files outside Git and CI artifacts.
