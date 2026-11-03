# Xbox 360 disc forensics notes

## Scope and method

The two user-provided images were opened read-only. Work was limited to
directory inventory, hashes, XEX2 execution metadata, public XContent/STFS
headers and file-table entries, and RPF signatures/header bytes. No files were
extracted, images modified, protected executable payloads decrypted, or
archive tables decrypted. The results below distinguish directly observed
bytes from format interpretations and unverified hypotheses.

The documented XEX2 and XContent field layouts were cross-checked against
Xenia's public research implementation:
[XEX2 metadata structures](https://github.com/xenia-project/xenia/blob/master/src/xenia/kernel/util/xex2_info.h)
and
[XContent/STFS structures](https://github.com/xenia-project/xenia/blob/master/src/xenia/vfs/devices/stfs_xbox.h),
with its [STFS directory-table traversal](https://github.com/xenia-project/xenia/blob/master/src/xenia/vfs/devices/stfs_container_device.cc).
These are implementation references, not an official Microsoft format
specification. Xenia is BSD-3-Clause licensed.

For XDVDFS, the
[antangelo/xdvdfs](https://github.com/antangelo/xdvdfs) project documents its
read-only directory traversal and XDVDFS/XISO image support; it is MIT
licensed. It is an independent implementation reference, not proof that every
Xbox 360 XGD3 layout is supported. The scanner in this project is separately
implemented and has been checked against both supplied images.

## Image inventory

Both images are 8,738,846,720 bytes. Their XDVDFS volume descriptors were
observed at image-relative base `0x02080000`; descriptor and file-sector
offsets are interpreted relative to that base. The optimized, read-only
`GameDataCatalog` scan independently reproduced the earlier directory listing.
It hashed each file's bounded data region and byte-compared entries whose
hashes and sizes matched:

| Metric | Install Disc | Play Disc |
|---|---:|---:|
| Files | 158 | 157 |
| Shared paths | 152 | |
| Byte-identical shared paths | 149 | |
| Conflicting shared paths | 3 | |
| Unique combined paths | 163 | |

The verified conflicts are `default.xex`, `xbox360a.rpf`, and
`xbox360b.rpf`. Disc 1 has four additional large files under
`content/0000000000000000/545408A7/00000002/`, plus `disc1.rsn` and
`install_music.wma`. Disc 2 has `audio_rel.rpf` and four additional
`sfx/*.rpf` files. The full catalog pass completed in 475 seconds in an
optimized Windows build. The supplied images retained their original sizes and
timestamps after the scan.

## `default.xex`

Both files are 26,628,096 bytes and begin with `XEX2`.

| Field | Install Disc | Play Disc |
|---|---:|---:|
| SHA-256 | `c6cace59f341edd4579bdac5d2849c5af7b96ddd4aa902ca2a138501d675a3ee` | `f0ca93fbe6b3dfbf50c026e0d77a71e8294b13a5a775af1b6a64f07b0fc10a3d` |
| Module flags | `0x1` | `0x1` |
| XEX header size | `0x5000` | `0x5000` |
| Title ID | `0x545408A7` | `0x545408A7` |
| Media ID | `0x0C48794E` | `0x79E465C5` |
| Disc number / count | `1 / 2` | `2 / 2` |
| Raw version / base version | `0x9 / 0x9` | `0x9 / 0x9` |
| Entry point | `0x83639888` | `0x83639888` |
| Image base | `0x82000000` | `0x82000000` |
| Security-header size | `0x2E24` | `0x2E24` |
| Image size | `0x1DC0000` | `0x1DC0000` |
| Image flags | `0x8` | `0x8` |

The image flag is recorded as raw metadata; the Xenia enum describes `0x8` as
an XGD2-media-only flag, which is not reconciled here with these XGD3-style
images. Both files report the same 20 static-library records:

| Libraries | Version |
|---|---|
| `LIBCMT` | `2.0.11886.0` |
| `XAPILIB`, `MDISC`, `XAPOBA`, `XAUDIO2`, `XHV2`, `XMCORE`, `XMP`, `XNET`, `XONLINE`, `D3D9`, `XGRAPHC`, `XPARTY`, `XSOCPST`, `XHTTP`, `XAUTH`, `XBOXKRNL` | `2.0.21250.0` |
| `LINK` | `10.0.11886.0` |
| `C2`, `C1` | `16.0.11886.0` |

The observed import-library names are `xam.xex` and `xboxkrnl.exe`; the
metadata inspection did not establish a complete runtime dependency/resource
list. The meaningful observed differences are the file hashes and execution
metadata media ID/disc number. These facts distinguish disc-specific builds
but do not establish which executable is the primary launch target. No
protected payload was decrypted.

## Disc 1 PIRS/STFS content packages

The four GTA V package files start with `PIRS`. Public XContent header fields
and metadata identify each as an STFS package with title ID `0x545408A7`,
content type `2`, metadata version `2`, header-size field `0xAD0E`, STFS volume
type, one file-table block, and zero `data_file_count`. In the documented
XContent layout, zero indicates an STFS package (rather than a multi-file
SVOD); it does not mean the package has no file contents.

| Package filename | Bytes | Content ID |
|---|---:|---|
| `545408A700000000` | 2,146,643,968 | `2EFC46BF02E0B126A64A81D0C915514661A2D5CD` |
| `545408A700000001` | 2,147,262,464 | `03C98730D46C8F9CE6A79F6BA75802AC3C6DA21E` |
| `545408A700000002` | 2,146,807,808 | `96CC1A6939BCD6EF4F0522A1E039EEBC294E3544` |
| `545408A700000003` | 1,817,518,080 | `85C21A6E9328BAB01B957E8FBD1C6A3B64E83FBA` |

All four package descriptors use STFS flags `0x01`, file-table block number
zero, and one file-table block. The header size rounds up to `0xB000`; the
public 0x1000-byte directory block is at package-relative offset `0xC000`.
Each table contains exactly two root entries, both with parent index `0xFFFF`:

| Package | Public file-table entries |
|---|---|
| `545408A700000000` | `part0.rpf` (2,133,954,560 bytes; 520,985 allocated blocks); `part0.timestamp` (14 bytes; 1 block) |
| `545408A700000001` | `part1.rpf` (2,134,566,912 bytes; 521,135 allocated blocks); `part1.timestamp` (14 bytes; 1 block) |
| `545408A700000002` | `part2.rpf` (2,134,116,352 bytes; 521,025 allocated blocks); `part2.timestamp` (14 bytes; 1 block) |
| `545408A700000003` | `part3.rpf` (1,806,764,032 bytes; 441,105 allocated blocks); `part3.timestamp` (14 bytes; 1 block) |

The four `.rpf` file starts each contain the four-byte signature `RPF7`.
Their aggregate declared size is 8,209,401,856 bytes. These observations
confirm the package directory entries and signature bytes, but do not validate
the RPF directory tables, establish that the four files form one archive, or
show that they are independent archives. No payload file was extracted or
decrypted.

The names, matching title IDs, ordinal suffixes, file-table entries, and
signature bytes are confirmed observations. Any interpretation of the
`partN.rpf` files' relationship beyond their matching numbering and signatures
remains unverified.

## RPF conflicts

All four conflicting files begin with the four-byte signature `RPF7`. The
following are raw header values under the big-endian RPF7 interpretation used
by PC-oriented readers; these values are not confirmation that the Xbox 360
tables use the PC encoding:

| Disc / file | Bytes | Raw entry-count field | Raw name-length field | Raw encryption tag |
|---|---:|---:|---:|---:|
| Install / `xbox360a.rpf` | 10,014,720 | 17 | `0x800000D0` | `0x0FFFFFF7` |
| Install / `xbox360b.rpf` | 17,874,944 | 56 | `0x80000330` | `0x0FFFFFF7` |
| Play / `xbox360a.rpf` | 711,852,032 | 155 | `0x80000A40` | `0x0FFFFFF7` |
| Play / `xbox360b.rpf` | 1,431,193,600 | 141 | `0x800009A0` | `0x0FFFFFF7` |

The catalog's full-file hashes confirm that each path has two different
content variants. These hashes identify files only; they do not interpret or
validate the archive directory tables:

| Path | Disc | Size | SHA-256 |
|---|---|---:|---|
| `default.xex` | Install | 26,628,096 | `c6cace59f341edd4579bdac5d2849c5af7b96ddd4aa902ca2a138501d675a3ee` |
| `default.xex` | Play | 26,628,096 | `f0ca93fbe6b3dfbf50c026e0d77a71e8294b13a5a775af1b6a64f07b0fc10a3d` |
| `xbox360a.rpf` | Install | 10,014,720 | `dbfcacef7b3faafd638df8c3f2d2b3a6b765f5296ec8303412d3aceec3cbae09` |
| `xbox360a.rpf` | Play | 711,852,032 | `5c7a633870485a4e79738fa6169e5a7937b39ba4474fb6f51a994c8dc6b8e525` |
| `xbox360b.rpf` | Install | 17,874,944 | `a073b0dd36389b24a7cbf2eb91edc4ba95896b18a15e4e2c6ac6d9d206aa6904` |
| `xbox360b.rpf` | Play | 1,431,193,600 | `9d15357ffb79a815020f2bdb66b0faee8ec0873a6499c927569f236bdda41cb1` |

The signature suggests an RPF7-family container, but the high-bit name-length
values and tag are not interpreted as a validated Xbox 360 directory table.
The entry counts above are raw header fields, not validated parsed counts.
Directory tables, entry/resource types, child archives, and per-archive
differences remain unknown. No decryption was attempted.

Open-source leads reviewed:

- [CodeWalker](https://github.com/dexyfex/CodeWalker) is a PC-oriented GTA V
  tooling project; its README lists the PC version of GTA V as a requirement.
  No repository-level open-source license was found in the reviewed tree.
  `Readme_Src.txt` says the source is for educational purposes, while
  `Notice.txt` contains component-specific notices including MIT and GPL
  material. Do not reuse it without a rights/license review. No retail Xbox 360
  RPF support was confirmed.
- [RageKit](https://github.com/RatPrez/ragekit) is MIT licensed and describes
  GTA V/RAGE file and RPF support, with format structures ported from
  CodeWalker. Its support for these Xbox 360 archives is not established.
- [Swage](https://github.com/0x1F9F1/Swage) has an RPF7 implementation and
  identifies its key-search path as `GTA5_PC`; its code does not establish
  support for the retail Xbox 360 variant. No repository-level license file
  was found in the reviewed root, so reuse is not assumed. Its key-search
  functionality was not run or used.
- [Xenia](https://github.com/xenia-project/xenia) has relevant Xbox execution
  and XContent/STFS metadata structures, but is not evidence that a PC GTA V
  RPF reader supports Xbox archive variants.

No third-party implementation is being copied into this repository. Before
adopting any RPF reader, verify its license and demonstrate Xbox 360 support on
synthetic, non-proprietary fixtures or safely validated metadata.

## Catalog design decision

The catalog now accepts multiple extracted-directory and XDVDFS ISO sources,
assigns each a source ID, and retains per-entry disc/path/byte-offset
provenance. It hashes bounded byte ranges, merges content only after a
byte-for-byte duplicate check, preserves conflicting variants, and refuses
conflict resolution unless a caller explicitly selects a source ID. ISO
resource reads are bounded streams over the original image, not extracted
files. A complete optimized scan of both supplied images reproduced all
previously reported file, shared-path, duplicate, conflict, and unique-path
counts. The prior inventory is therefore reconciled for these images and this
scanner version; broader XDVDFS/XGD3 layout coverage is still not established.

The metadata-only STFS reader validates public package/header fields and can
parse a single bounded 0x1000-byte STFS file-table block. Synthetic tests cover
its field decoding, entry bounds, and parent indices. It does not follow
multi-block file-table chains, verify the STFS hash tree, read package payload
files, or decrypt content. XEX imports and RPF directory tables are not parsed.
No executable or conflict winner is chosen by disc precedence.
