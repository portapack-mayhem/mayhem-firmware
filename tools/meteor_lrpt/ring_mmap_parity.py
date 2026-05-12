#!/usr/bin/env python3
"""
RAM vs mmap parity for a 2_654_208-byte deinterleaver-sized ring (SatDump deint.h).

Writes a deterministic pattern through byte indices [0, ring), reopens via mmap,
and verifies bytes match. Requires a writable temp path (default: cwd).

Usage:
  python tools/meteor_lrpt/ring_mmap_parity.py
  python tools/meteor_lrpt/ring_mmap_parity.py --path /tmp/deint_ring.bin
"""

from __future__ import annotations

import argparse
import mmap
import os
import sys

RING = 36 * 36 * 2048


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--path", type=str, default="deint_ring_mmap_test.bin", help="temp file path")
    args = ap.parse_args()

    path = args.path
    try:
        with open(path, "wb") as f:
            f.truncate(RING)
    except OSError as e:
        print("error: cannot create ring file:", e, file=sys.stderr)
        return 1

    ref = bytearray(RING)
    for i in range(RING):
        ref[i] = (i * 17 + 41) & 0xFF

    with open(path, "r+b") as f:
        mm = mmap.mmap(f.fileno(), RING, access=mmap.ACCESS_WRITE)
        mm[:] = ref
        mm.flush()
        mm.close()

    with open(path, "rb") as f:
        mm = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
        ok = mm[:] == bytes(ref)
        mm.close()

    try:
        os.remove(path)
    except OSError:
        pass

    if not ok:
        print("mmap parity: FAIL", file=sys.stderr)
        return 2
    print(f"mmap parity: OK ({RING} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
