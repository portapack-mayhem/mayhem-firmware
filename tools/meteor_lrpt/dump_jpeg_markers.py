#!/usr/bin/env python3
"""Print CCSDS-unrelated JPEG marker lengths (dev helper)."""
import struct
import sys

def main() -> None:
    path = sys.argv[1] if len(sys.argv) > 1 else "tools/meteor_lrpt/data/g4_tiny_baseline.jpg"
    d = open(path, "rb").read()
    if len(d) < 4 or d[:2] != b"\xff\xd8":
        print("not jpeg")
        return
    i = 2
    while i + 4 <= len(d):
        if d[i] != 0xFF:
            print("lost sync", i, hex(d[i]))
            return
        m = d[i + 1]
        ln = struct.unpack(">H", d[i + 2 : i + 4])[0]
        print(f"0x{m:02x} len={ln}")
        if m == 0xDA:
            return
        if m in (0xD8, 0xD9):
            i += 2
            continue
        i += 2 + ln


if __name__ == "__main__":
    main()
