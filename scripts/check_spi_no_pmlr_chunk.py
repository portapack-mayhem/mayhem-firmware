#!/usr/bin/env python3
"""Fail if portapack-mayhem-firmware.bin contains a PMLR baseband SPI chunk.

H4M 1 MiB (MAYHEM_SPI_1MB) packs Meteor M4 as PMLS in meteor-capture.ppma only.
A PMLR chunk header is four ASCII bytes followed by a little-endian uint32 length
(see firmware/tools/make_image_chunk.py). Random 'PMLR' in application rodata is
not a chunk header and is ignored.
"""
from __future__ import annotations

import struct
import sys
from pathlib import Path


def pmlr_chunk_offsets(data: bytes) -> list[int]:
    out: list[int] = []
    start = 0
    needle = b"PMLR"
    while True:
        i = data.find(needle, start)
        if i < 0:
            break
        if i + 8 <= len(data):
            (length,) = struct.unpack_from("<I", data, i + 4)
            # Chunk payload length field; real PMLR chunks are tens of KiB, not garbage.
            if 1024 <= length <= 512 * 1024:
                out.append(i)
        start = i + 1
    return out


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} <portapack-mayhem-firmware.bin>", file=sys.stderr)
        return 2
    path = Path(sys.argv[1])
    if not path.is_file():
        print(f"ERROR: missing {path}", file=sys.stderr)
        return 2
    data = path.read_bytes()
    if len(data) != 1048576:
        print(f"WARNING: {path.name} is {len(data)} bytes (expected 1048576 for 1 MiB H4M)")
    offs = pmlr_chunk_offsets(data)
    if offs:
        for o in offs:
            length = struct.unpack_from("<I", data, o + 4)[0]
            print(f"ERROR: PMLR SPI chunk at {o:#x} (payload length field {length} bytes)")
        print(
            "Rebuild: MAYHEM_CLEAN_BUILD=1 ./scripts/wsl-build-share-out.sh "
            "(spi-prep drops stale baseband.img / PMLR.img)",
            file=sys.stderr,
        )
        return 1
    print(f"OK: no PMLR chunk header in {path.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
