#!/usr/bin/env python3
import struct
import sys
import zipfile


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: verify_apk.py <apk-path>", file=sys.stderr)
        return 2

    apk_path = sys.argv[1]
    with zipfile.ZipFile(apk_path) as apk:
        names = set(apk.namelist())
        required = {
            "AndroidManifest.xml",
            "classes.dex",
            "lib/arm64-v8a/libnative_lib.so",
        }
        missing = sorted(required - names)
        if missing:
            raise SystemExit(f"APK is missing required entries: {', '.join(missing)}")

        native_libraries = sorted(
            name for name in names if name.startswith("lib/") and name.endswith(".so")
        )
        unexpected_abis = sorted(
            name for name in native_libraries if not name.startswith("lib/arm64-v8a/")
        )
        if unexpected_abis:
            raise SystemExit(f"APK contains unexpected native ABIs: {unexpected_abis}")

        elf = apk.read("lib/arm64-v8a/libnative_lib.so")
        if len(elf) < 20 or elf[:4] != b"\x7fELF" or elf[4] != 2:
            raise SystemExit("Packaged native library is not a 64-bit ELF shared object")
        machine = struct.unpack_from("<H", elf, 18)[0]
        if machine != 183:
            raise SystemExit(f"Packaged library ELF machine is {machine}, expected AArch64 (183)")

    print(f"Verified installable APK: {apk_path}")
    print("Verified launch manifest, classes.dex, and AArch64 native library.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
