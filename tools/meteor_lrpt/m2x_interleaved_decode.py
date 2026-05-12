#!/usr/bin/env python3
"""
Host-side helper for Meteor M2-x **interleaved** CADU generation (SatDump parity).

Mayhem on-device interleaved M2-x is not SatDump-equivalent; use this workflow:

1. Capture IQ with Mayhem Capture or record SOFT `.C8` (see SOFT_FORMAT.md).
2. Run **SatDump** (pinned commit in firmware/baseband/meteor_lrpt/SATDUMP_VENDOR.md)
   with pipeline `METEOR M2-x LRPT 72k` and `interleaved: true` in the meteor_lrpt_decoder module.
3. Export CADU from SatDump; compare with Mayhem legacy/non-interleaved CADU using:

     python tools/meteor_lrpt/compare_cadu.py --frame 1020 mayhem.cadu satdump.cadu

This script can print workflow hints (`--dry-run`), optionally run `SATDUMP_CMD`,
or forward to `compare_cadu.py` (`--compare-cadu`).

Environment:
  SATDUMP_CMD   Full command prefix for SatDump, e.g. `C:\\SatDump\\satdump.exe`
  SATDUMP_ARGS  Extra args appended before input/output passthrough (optional).
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parent
    p = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument(
        "--soft",
        type=str,
        default="",
        help="Path to Mayhem SOFT .C8 (int8 I/Q pairs, 16384-byte blocks).",
    )
    p.add_argument(
        "--iq",
        type=str,
        default="",
        help="Path to IQ file for SatDump file source (e.g. .C16).",
    )
    p.add_argument("--out-cadu", type=str, default="", help="Output CADU path (SatDump-specific).")
    p.add_argument(
        "--dry-run",
        action="store_true",
        help="Print recommended SatDump steps only; do not run SATDUMP_CMD.",
    )
    p.add_argument(
        "--compare-cadu",
        nargs=2,
        metavar=("A", "B"),
        type=Path,
        help="Run compare_cadu.py on two CADU files (forwards remaining argv-style flags below).",
    )
    p.add_argument("--frame", type=int, default=1020, help="With --compare-cadu: frame size for compare_cadu.py")
    p.add_argument(
        "--normalize-mayhem",
        choices=("none", "a", "b", "both"),
        default="none",
        help="With --compare-cadu: pass through to compare_cadu.py",
    )
    args, unknown = p.parse_known_args()

    if args.compare_cadu is not None:
        cmd = [
            sys.executable,
            str(root / "compare_cadu.py"),
            str(args.compare_cadu[0]),
            str(args.compare_cadu[1]),
            "--frame",
            str(args.frame),
        ]
        if args.normalize_mayhem != "none":
            cmd.extend(["--normalize-mayhem", args.normalize_mayhem])
        cmd.extend(unknown)
        return int(subprocess.call(cmd))

    cmd = os.environ.get("SATDUMP_CMD", "").strip()
    extra = os.environ.get("SATDUMP_ARGS", "").strip()

    if args.dry_run or not cmd:
        print("Interleaved M2-x CADU: use SatDump meteor_lrpt_decoder with interleaved=true.")
        print("Pin: see firmware/baseband/meteor_lrpt/SATDUMP_VENDOR.md")
        print("Mayhem SOFT format: tools/meteor_lrpt/SOFT_FORMAT.md")
        print("Compare: python tools/meteor_lrpt/compare_cadu.py --frame 1020 a.cadu b.cadu")
        if args.soft:
            print(f"Note: --soft {args.soft!r} — SatDump typically ingests IQ .C16; convert or use IQ path.")
        if args.iq:
            print(f"IQ input: {args.iq!r}")
        if not cmd:
            print("Set SATDUMP_CMD to your satdump executable to enable subprocess mode.")
        return 0

    if not args.out_cadu:
        print("error: --out-cadu required when SATDUMP_CMD is set", file=sys.stderr)
        return 2

    argv = cmd.split()
    if extra:
        argv.extend(extra.split())
    argv.extend(["--help"])
    print("Running:", " ".join(argv))
    try:
        subprocess.run(argv, check=False)
    except OSError as e:
        print("error:", e, file=sys.stderr)
        return 1
    print("Note: wire your local SatDump pipeline JSON for meteor_lrpt_decoder (interleaved).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
