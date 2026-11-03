# Xbox 360 disc forensics notes

## Scope and method

The two user-provided images were opened read-only. Work was limited to
directory inventory, hashes, XEX2 execution metadata, public XContent/STFS
header fields, and raw RPF header bytes. No files were extracted, images
modified, protected executable payloads decrypted, or archive tables
decrypted. The results below distinguish directly observed bytes from format
interpretations and unverified hypotheses.

The documented XEX2 and XContent field layouts were cross-checked against
Xenia's public research implementation:
[XEX2 metadata structures](https://github.com/xenia-project/xenia/blob/master/src/xenia/kernel/util/xex2_info.h)
and
[XContent/STFS structures](https://github.com/xenia-project/xenia/blob/master/src/xenia/vfs/devices/stfs_xbox.h).
These are implementation references, not an official Microsoft format
specification. Xenia is BSD-3-Clause licensed.

For XDVDFS, the
[antangelo/xdvdfs](https://github.com/antangelo/xdvdfs) project documents its
read-only directory traversal and XDVDFS/XISO image support; it is MIT
licensed. It is an independent implementation reference, not proof that every
Xbox 360 XGD3 layout is supported. The scanner in this project is separately
implemented and remains subject to real-image validation.

## Image inventory

Both images are 8,738,846,720 bytes. Their XDVDFS volume descriptors were
observed at image-relative base `0x02080000`; descriptor and file-sector
offsets are interpreted relative to that base. The earlier independent
read-only listing reported:

| Metric | Install Disc | Play Disc |
|---|---:|---:|
| Files | 158 | 157 |
| Shared paths | 152 | |
| Byte-identical shared paths | 149 | |
| Conflicting shared paths | 3 | |
| Unique combined paths | 163 | |

Conflicts are `default.xex`, `xbox360a.rpf`, and `xbox360b.rpf`. Disc 1 has
four additional large files under
`content/0000000000000000/545408A7/00000002/`, plus `disc1.rsn` and
`install_music.wma`. Disc 2 has `audio_rel.rpf` and four additional
`sfx/*.rpf` files. These counts came from an independent read-only listing and
have not yet been reconciled against this project's hash-producing ISO catalog
scan.

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

The four package files start with `PIRS`. Public XContent header fields and
metadata identify each as an STFS package with title ID `0x545408A7`, content
type `2`, metadata version `2`, header-size field `0xAD0E`, STFS volume type,
one file-table block, and zero data-file count.

| Package filename | Bytes | Content ID |
|---|---:|---|
| `545408A700000000` | 2,146,643,968 | `2EFC46BF02E0B126A64A81D0C915514661A2D5CD` |
| `545408A700000001` | 2,147,262,464 | `03C98730D46C8F9CE6A79F6BA75802AC3C6DA21E` |
| `545408A700000002` | 2,146,807,808 | `96CC1A6939BCD6EF4F0522A1E039EEBC294E3544` |
| `545408A700000003` | 1,817,518,080 | `85C21A6E9328BAB01B957E8FBD1C6A3B64E83FBA` |

The names, matching title IDs, and adjacent ordinal suffixes are confirmed
metadata. Whether the packages are sequential pieces of one install payload,
independent content items, or use another relationship is still a hypothesis;
their internal file tables and protected payloads were not inspected.

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
files.

The metadata-only STFS reader validates the public package signature, header
size, and STFS descriptor and exposes selected public fields; it does not parse
internal STFS file tables. XEX imports and RPF directory tables are not parsed.
These remain separate format investigations; no executable or conflict winner
is chosen by disc precedence.
