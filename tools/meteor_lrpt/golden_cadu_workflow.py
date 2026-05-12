#!/usr/bin/env python3
"""
Operator + CI helper for G3 golden CADU comparison (Mayhem vs SatDump).

Steps (manual):
  1. Capture IQ with Mayhem Capture -> passNN.c16 (SPEC 150k/250k, center 137.9 MHz).
  2. Run SatDump meteor LRPT pipeline on the same file -> passNN_satdump.cadu
  3. Record Mayhem Meteor LRPT CADU REC -> passNN_mayhem.cadu (1020-byte frames, ASM inside first 4 bytes of *decoded* CADU field only in stream; see README).

For **M2-x interleaved** SatDump parity CADU, use SatDump on IQ (or `m2x_interleaved_decode.py` / SOFT_FORMAT.md) then compare outputs with this script.

Host-only M2-x FEC smoke (no CADU expected on zeroed soft): see `test_m2x_pipeline_host.cpp` header for the `g++` command.
Host RAM-ring interleaved pipeline: `test_m2x_interleaved_ram_pipeline_host.cpp` (see `run_reader_long_golden_docker.sh`).

This script runs compare_cadu.py as a subprocess and exits with the same code.
Example:
  python golden_cadu_workflow.py mayhem/pass01.cadu satdump/pass01.cadu
  python golden_cadu_workflow.py mayhem/pass01.cadu satdump/pass01.cadu --frame 1020 --normalize-mayhem b
  python golden_cadu_workflow.py --validate-soft-file mayhem/pass01.C8 --soft-block-bytes 8192
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
    p.add_argument("file_a", nargs="?", type=Path, default=None, help="Mayhem or reference CADU")
    p.add_argument("file_b", nargs="?", type=Path, default=None, help="SatDump or second CADU")
    p.add_argument("--frame", type=int, default=1020)
    p.add_argument("--normalize-mayhem", choices=("none", "a", "b", "both"), default="none")
    p.add_argument("--skip-a", type=int, default=0)
    p.add_argument("--skip-b", type=int, default=0)
    p.add_argument("--max-frames", type=int, default=0)
    p.add_argument(
        "--validate-soft-file",
        type=Path,
        default=None,
        metavar="PATH",
        help="Run compare_cadu.py soft alignment check only",
    )
    p.add_argument(
        "--soft-block-bytes",
        type=int,
        default=16384,
        choices=(8192, 16384),
        help="With --validate-soft-file: expected block size",
    )
    p.add_argument(
        "--show-deint-budget",
        action="store_true",
        help="After compare, print deinterleave sector histogram summary (FatFs RMW budgeting).",
    )
    args = p.parse_args()

    if args.validate_soft_file is not None:
        cmd = [
            sys.executable,
            str(compare),
            "--validate-soft-file",
            str(args.validate_soft_file),
            "--soft-block-bytes",
            str(args.soft_block_bytes),
        ]
        print("Running:", " ".join(cmd))
        return int(subprocess.call(cmd))

    if not args.file_a or not args.file_b:
        print("FAIL: provide file_a and file_b, or use --validate-soft-file", file=sys.stderr)
        return 1

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
    rc = int(subprocess.call(cmd))
    if args.show_deint_budget and rc == 0:
        hist = root / "deint_sector_histogram.py"
        print("Running:", hist.name, "(summary)")
        subprocess.call([sys.executable, str(hist), "--summary-only", "--batches", "50"])
    return rc


if __name__ == "__main__":
    sys.exit(main())
