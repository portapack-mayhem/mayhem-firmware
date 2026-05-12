#!/usr/bin/env python3
"""
Compare two uncompressed 24 bpp BMP files of identical width and height.

Usage:
  python compare_bmp.py a.bmp b.bmp
  python compare_bmp.py a.bmp b.bmp --max-diff 100

Exit code 0 if pixel buffers match within --max-diff differing bytes (default 0).
"""
from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path


def bmp_pixels(path: Path) -> tuple[int, int, bytes]:
    data = path.read_bytes()
    if len(data) < 54 or data[0:2] != b"BM":
        raise ValueError(f"not a BMP or too short: {path}")
    width, height = struct.unpack_from("<ii", data, 18)
    height = abs(height)
    data_offset = struct.unpack_from("<I", data, 10)[0]
    if data_offset >= len(data):
        raise ValueError(f"bad pixel offset in {path}")
    row = (width * 3 + 3) & ~3
    expected = row * height
    pix = data[data_offset:]
    if len(pix) < expected:
        raise ValueError(f"truncated BMP pixel data: {path} have {len(pix)} need {expected}")
    return width, height, pix[:expected]


def main() -> int:
    p = argparse.ArgumentParser(description="Compare two 24 bpp BMP pixel buffers")
    p.add_argument("a", type=Path)
    p.add_argument("b", type=Path)
    p.add_argument("--max-diff", type=int, default=0, help="allow up to this many byte differences")
    args = p.parse_args()
    try:
        wa, ha, pa = bmp_pixels(args.a)
        wb, hb, pb = bmp_pixels(args.b)
    except ValueError as e:
        print(f"FAIL: {e}", file=sys.stderr)
        return 2
    if wa != wb or ha != hb:
        print(f"FAIL: size mismatch {wa}x{ha} vs {wb}x{hb}", file=sys.stderr)
        return 1
    if len(pa) != len(pb):
        print("FAIL: pixel buffer length mismatch", file=sys.stderr)
        return 1
    diff = sum(1 for x, y in zip(pa, pb) if x != y)
    if diff > args.max_diff:
        print(f"FAIL: {diff} byte(s) differ (max allowed {args.max_diff})", file=sys.stderr)
        return 1
    print(f"OK: {wa}x{ha} {len(pa)} bytes, diff={diff}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
