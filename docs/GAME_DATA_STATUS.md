# Game-data status

## Confirmed inspection results

The user-provided Xbox 360 images were inspected read-only from
`F:\GTA Grand Theft Auto V`. Each image is 8,738,846,720 bytes. The earlier
independent directory inventory counted 158 files on Disc 1, 157 on Disc 2,
152 shared paths, 149 byte-identical shared files, and three conflicting
paths (`default.xex`, `xbox360a.rpf`, and `xbox360b.rpf`), for 163 unique
paths. Disc 1 also has four large packages beneath
`content/0000000000000000/545408A7/00000002/`; Disc 2 has `audio_rel.rpf`
and four additional `sfx/*.rpf` files.

The catalog's optimized full-image scan confirmed exactly 158 Disc 1 file
entries, 157 Disc 2 entries, 149 identical shared paths, three conflicting
shared paths, and 163 unique paths. The result matches the earlier independent
inventory. It took 475 seconds and did not modify either image.

Both `default.xex` files have XEX2 magic and are 26,628,096 bytes. Their
SHA-256 hashes differ:

| Disc | SHA-256 | Media ID | Disc metadata |
|---|---|---|---|
| Install (Disc 1) | `c6cace59f341edd4579bdac5d2849c5af7b96ddd4aa902ca2a138501d675a3ee` | `0x0C48794E` | 1 of 2 |
| Play (Disc 2) | `f0ca93fbe6b3dfbf50c026e0d77a71e8294b13a5a775af1b6a64f07b0fc10a3d` | `0x79E465C5` | 2 of 2 |

Both expose title ID `0x545408A7`, raw version and base-version values `0x9`,
module flags `0x1`, XEX header size `0x5000`, entry point `0x83639888`, image
base `0x82000000`, security-header size `0x2E24`, image size `0x1DC0000`, and
image flags `0x8`. Twenty static-library records match between the files:
`LIBCMT 2.0.11886.0`, `XAPILIB`, `MDISC`, `XAPOBA`, `XAUDIO2`, `XHV2`,
`XMCORE`, `XMP`, `XNET`, `XONLINE`, `D3D9`, `XGRAPHC`, `XPARTY`, `XSOCPST`,
`XHTTP`, `XAUTH`, and `XBOXKRNL` are `2.0.21250.0`; `LINK` is
`10.0.11886.0`; `C2` and `C1` are `16.0.11886.0`. The observed import
libraries are `xam.xex` and `xboxkrnl.exe`.

The metadata establishes distinct disc builds/media identities, not which
executable is the game's primary launch target. No executable has been
selected and no protected payload was decrypted.

The four Disc 1 content files begin with `PIRS`. Their public XContent metadata
identifies STFS packages for title `0x545408A7`, content type `2`, metadata
version `2`, header size `0xAD0E`, STFS volume type, one file-table block, and
zero data-file count. Their 20-byte content IDs are:

| Package | Content ID |
|---|---|
| `545408A700000000` | `2EFC46BF02E0B126A64A81D0C915514661A2D5CD` |
| `545408A700000001` | `03C98730D46C8F9CE6A79F6BA75802AC3C6DA21E` |
| `545408A700000002` | `96CC1A6939BCD6EF4F0522A1E039EEBC294E3544` |
| `545408A700000003` | `85C21A6E9328BAB01B957E8FBD1C6A3B64E83FBA` |

The RPF conflict files begin with `RPF7`. Their full-file SHA-256 hashes,
sizes, big-endian header observations, and the limits of those observations
are recorded in [RESEARCH.md](./RESEARCH.md).
The directory tables, resource entry types, nested archives, and Xbox-specific
table encoding remain unparsed. No encrypted archive metadata was decrypted.

## Catalog implementation and tests

`GameDataCatalog` accepts independently identified extracted directories and
Xbox 360 XDVDFS/XISO images. It hashes each file by streaming a bounded source
region, records source ID/disc/path/byte offset, merges only byte-identical
content, and preserves conflicts. A conflicting path is rejected by default;
callers may explicitly select a source ID. ISO resources are opened as bounded
read-only streams without extracting or rewriting the image.

Synthetic tests cover two ISO sources, identical and conflicting files,
disc-specific files, cross-source reads, explicit source selection, manifest
provenance, invalid volume descriptors, and extracted-directory behavior.
Synthetic STFS header tests also validate recognized package signatures,
bounded header sizes, descriptor length, and truncated input. These tests and
the completed real-image scan validate catalog indexing and byte-range
hashing for these two images; they do not establish game compatibility or
coverage of every XDVDFS/XGD3 layout.

## Data handling and remaining limits

Keep all user-supplied images and extracted game files outside Git and CI.
Metadata observations in the documentation are not game assets. The Android
runtime is intended to consume legally supplied game data through the virtual
resource system; it will not require physical Xbox 360 discs at runtime.
Android import/persistence UX and game-specific dependency validation remain
future work.
