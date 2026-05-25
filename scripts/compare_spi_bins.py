#!/usr/bin/env python3
"""Compare two portapack-mayhem-firmware.bin images (layout + checksum).

HRF1 layout note (sanity): the packed image often contains HRF1 twice — a bootloader/stub
near the start and a second HRF1 after the application region. When comparing offsets,
expect the second occurrence higher in the file; a missing or wildly shifted second HRF1
suggests a wrong FLASH_MB_* pack or a truncated image.
"""
import struct
import sys
from pathlib import Path


def _hrf1_offsets(data: bytes):
    needle = b"HRF1"
    out = []
    start = 0
    while True:
        i = data.find(needle, start)
        if i < 0:
            break
        out.append(i)
        start = i + len(needle)
    return out


def analyze(path: Path) -> None:
    data = path.read_bytes()
    print(f"=== {path.name} ({len(data)} bytes) ===")
    s = sum(struct.unpack_from("<I", data, i)[0] for i in range(0, len(data) - 4, 4)) & 0xFFFFFFFF
    stored = struct.unpack_from("<I", data, len(data) - 4)[0]
    print(f"  checksum stored={stored:#010x} expected={((-s) & 0xFFFFFFFF):#010x} ok={stored == ((-s) & 0xFFFFFFFF)}")
    for tag in (b"HRF1", b"PMLR", b"PFUT", b"PADR"):
        idx = data.find(tag)
        print(f"  tag {tag.decode()}: {idx:#x}" if idx >= 0 else f"  tag {tag.decode()}: absent")
    hrf1s = _hrf1_offsets(data)
    if hrf1s:
        print(f"  HRF1 all offsets: {[hex(x) for x in hrf1s]} (expect 2nd after app region on packed 1MiB images)")
    else:
        print("  HRF1 all offsets: none")
    # firmware_info @ 0x400
    if len(data) >= 0x408 and data[0x400:0x408] == b"HACKRFFW":
        ver = data[0x408:0x420].split(b"\x00", 1)[0]
        print(f"  HACKRFFW @0x400 version={ver!r}")
    else:
        print("  HACKRFFW @0x400: absent or too short")


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    off = root / "flashing/firmware/firmware_hackrf.bin"
    cus = root / "share-out/artifacts/portapack-mayhem-firmware.bin"
    for p in (off, cus):
        if p.is_file():
            analyze(p)
        else:
            print(f"missing: {p}")


if __name__ == "__main__":
    main()
