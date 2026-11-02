# Roadmap

## Current: independent infrastructure

- [x] Scan Install Disc and Play Disc as separate directory sources.
- [x] Track each resource's source disc and relative path.
- [x] Merge identical duplicate content without losing provenance.
- [x] Preserve differing duplicates as explicit conflicts; never choose one
  implicitly.
- [x] Generate a deterministic JSON inventory and validate required paths.
- [x] Exercise the catalog with synthetic two-disc fixtures.
- [x] Identify executable candidates without assuming their location.

## Next: inspect actual disc data

- [ ] Inspect each lawfully supplied disc source independently and record its
  observed structure.
- [ ] Determine the real executable candidate(s) and verify the launch target.
- [ ] Parse the executable's format metadata and imports to derive dependencies.
- [ ] Validate required resources and document any unresolved conflicts or
  unsupported formats.
- [ ] Keep all proprietary game files outside the repository and CI artifacts.

## Later: runtime and Android integration

- [ ] Add a format-aware executable loader that resolves reads through the
  unified virtual filesystem.
- [ ] Establish an explicit, documented conflict-resolution policy based on
  verified game behavior, not arbitrary disc precedence.
- [ ] Build a user-facing data import/mount flow for Android that works without
  physical Xbox 360 discs.
- [ ] Exercise loading with user-provided data while keeping synthetic tests
  distinct from game integration results.
