# Game-data status

## Current status

No GTA V disc images or extracted game files are present in this project. The
exact directory layout, executable location, resource ownership, and dependency
relationships are therefore unknown and intentionally not guessed.

The current generic catalog accepts independent user-selected Install Disc and
Play Disc source directories. An unprovided source or a source with no regular files is reported
as missing data. Each source is scanned without writing into it; the catalog
records relative paths and SHA-256 digests, merges byte-identical duplicates,
and preserves different contents as explicit path conflicts. Lookups refuse to
choose a conflicting version implicitly. The generated JSON manifest records
each content variant and its source disc.

An `.xex` extension or an `XEX2` magic value marks an executable candidate only.
It does not prove which executable starts the game. Dependency validation can
check an explicit resource list, but the list must come from inspection of the
real executable and its format-specific imports before it can be treated as
authoritative.

## Synthetic validation

The native test suite creates temporary, synthetic Install Disc and Play Disc
trees. It checks independent source scanning, identical and conflicting
duplicates, source attribution, lookup across both sources, missing-disc
reporting, required-resource validation, executable candidate detection, and
manifest output. These fixtures are not GTA V data and do not establish that
the game runs.

## Data handling

Keep all user-supplied disc images and extracted files outside Git. The Android
runtime is intended to consume legally supplied extracted data through this
unified read-only catalog; it will not require physical Xbox 360 discs at
runtime. Packaging and installation UX for large game data remains future
work.
