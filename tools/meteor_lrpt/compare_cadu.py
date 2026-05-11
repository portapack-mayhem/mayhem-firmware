#!/usr/bin/env python3
"""
Compare two CADU streams (raw bytes) for Meteor LRPT parity testing.

Usage:
  python compare_cadu.py mayhem.cadu satdump.cadu [--frame 1020]
  python compare_cadu.py a.cadu b.cadu --frame 1024 --normalize-mayhem b

Exit code 0 if byte-identical after optional transforms; 1 on mismatch.
"""
from __future__ import annotations

import argparse
import hashlib
import sys

ASM_BE = bytes([0x1A, 0xCF, 0xFC, 0x1D])


def read_all(path: str) -> bytes:
    with open(path, "rb") as f:
        return f.read()


def normalize_mayhem_1024_to_1020(data: bytes) -> bytes:
    """Strip leading 4-byte ASM from each 1024-byte chunk -> 1020-byte body (SatDump-style per-frame ASM)."""
    if len(data) % 1024 != 0:
        raise ValueError("normalize_mayhem_1024_to_1020: length not multiple of 1024")
    out = bytearray()
    for i in range(0, len(data), 1024):
        chunk = data[i : i + 1024]
        if chunk[0:4] != ASM_BE:
            raise ValueError(f"normalize: missing ASM at frame {i // 1024}")
        out.extend(chunk[4:1024])
    return bytes(out)


def strip_leading_bytes(data: bytes, n: int) -> bytes:
    if n < 0 or n > len(data):
        raise ValueError("strip_leading_bytes: bad n")
    return data[n:]


def main() -> int:
    p = argparse.ArgumentParser(description="Compare CADU binary streams (Meteor LRPT / Mayhem parity)")
    p.add_argument("file_a", help="First CADU stream (e.g. Mayhem)")
    p.add_argument("file_b", help="Second CADU stream (e.g. SatDump)")
    p.add_argument(
        "--frame",
        type=int,
        default=1020,
        help="CADU record length in bytes (Mayhem REC uses 1020; SatDump may use 1024 with ASM inside chunk)",
    )
    p.add_argument(
        "--skip-a",
        type=int,
        default=0,
        help="Skip this many leading bytes of file A before compare",
    )
    p.add_argument(
        "--skip-b",
        type=int,
        default=0,
        help="Skip this many leading bytes of file B before compare",
    )
    p.add_argument(
        "--normalize-mayhem",
        choices=("none", "a", "b", "both"),
        default="none",
        help="Convert 1024-byte ASM+body frames to 1020-byte bodies (strip 4-byte ASM per frame)",
    )
    p.add_argument(
        "--max-frames",
        type=int,
        default=0,
        help="Compare at most this many frames (0 = all)",
    )
    args = p.parse_args()

    a = read_all(args.file_a)
    b = read_all(args.file_b)

    if args.skip_a:
        a = strip_leading_bytes(a, args.skip_a)
    if args.skip_b:
        b = strip_leading_bytes(b, args.skip_b)

    try:
        if args.normalize_mayhem in ("a", "both"):
            a = normalize_mayhem_1024_to_1020(a)
        if args.normalize_mayhem in ("b", "both"):
            b = normalize_mayhem_1024_to_1020(b)
    except ValueError as e:
        print(f"FAIL: normalize: {e}", file=sys.stderr)
        return 1

    ha = hashlib.sha256(a).hexdigest()
    hb = hashlib.sha256(b).hexdigest()
    print(f"A len={len(a)} sha256={ha}")
    print(f"B len={len(b)} sha256={hb}")

    if a == b:
        print("OK: identical files")
        return 0

    fl = args.frame
    if fl <= 0:
        print("FAIL: bad --frame", file=sys.stderr)
        return 1

    if len(a) % fl or len(b) % fl:
        print("FAIL: lengths differ or not multiple of frame size", file=sys.stderr)
        return 1

    nf = min(len(a), len(b)) // fl
    if args.max_frames > 0:
        nf = min(nf, args.max_frames)

    for i in range(nf):
        fa = a[i * fl : (i + 1) * fl]
        fb = b[i * fl : (i + 1) * fl]
        if fa != fb:
            print(f"FAIL: first mismatch at frame {i}")
            for j in range(min(fl, 64)):
                if fa[j] != fb[j]:
                    print(f"  offset {j}: {fa[j]:02x} vs {fb[j]:02x}")
                    break
            return 1

    if args.max_frames > 0:
        print(f"OK: first {nf} frame(s) match ({fl} bytes/frame; partial compare)")
        return 0

    if len(a) != len(b):
        print(f"FAIL: matched {nf} frame(s) but lengths differ ({len(a)} vs {len(b)})", file=sys.stderr)
        return 1

    print(f"OK: all {nf} frame(s) match ({fl} bytes/frame)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
