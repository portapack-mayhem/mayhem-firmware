#!/usr/bin/env python3
"""
Operator + CI helper for G3 golden CADU comparison (Mayhem vs SatDump).

Steps (manual):
  1. Capture IQ with Mayhem Capture -> passNN.c16 (SPEC 150k/250k, center 137.9 MHz).
  2. Run SatDump meteor LRPT pipeline on the same file -> passNN_satdump.cadu
  3. Record Mayhem Meteor LRPT CADU REC -> passNN_mayhem.cadu (1020-byte frames, ASM inside first 4 bytes of *decoded* CADU field only in stream; see README).

This script runs compare_cadu.py as a subprocess and exits with the same code.
Example:
  python golden_cadu_workflow.py mayhem/pass01.cadu satdump/pass01.cadu
  python golden_cadu_workflow.py mayhem/pass01.cadu satdump/pass01.cadu --frame 1020 --normalize-mayhem b
"""
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parent
    compare = root / "compare_cadu.py"

    p = argparse.ArgumentParser(
        description="Run compare_cadu.py for Mayhem/SatDump golden CADU checks"
    )
    p.add_argument("file_a", type=Path, help="Mayhem or reference CADU")
    p.add_argument("file_b", type=Path, help="SatDump or second CADU")
    p.add_argument("--frame", type=int, default=1020)
    p.add_argument("--normalize-mayhem", choices=("none", "a", "b", "both"), default="none")
    p.add_argument("--skip-a", type=int, default=0)
    p.add_argument("--skip-b", type=int, default=0)
    p.add_argument("--max-frames", type=int, default=0)
    args = p.parse_args()

    cmd = [
        sys.executable,
        str(compare),
        str(args.file_a),
        str(args.file_b),
        "--frame",
        str(args.frame),
    ]
    if args.skip_a:
        cmd.extend(["--skip-a", str(args.skip_a)])
    if args.skip_b:
        cmd.extend(["--skip-b", str(args.skip_b)])
    if args.max_frames:
        cmd.extend(["--max-frames", str(args.max_frames)])
    if args.normalize_mayhem != "none":
        cmd.extend(["--normalize-mayhem", args.normalize_mayhem])

    print("Running:", " ".join(cmd))
    return int(subprocess.call(cmd))


if __name__ == "__main__":
    sys.exit(main())
