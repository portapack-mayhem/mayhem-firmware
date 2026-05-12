#!/usr/bin/env python3
"""
SatDump-style Meteor M2-x deinterleaver write-index sector histogram.

Mirrors DeinterleaverReader::deinterleave write loop in SatDump deint.cpp
(commit pinned in firmware/baseband/meteor_lrpt/SATDUMP_VENDOR.md):
  INTER_BRANCH_COUNT = 36, INTER_BRANCH_DELAY = 2048,
  INTER_MARKER_INTERSAMPS = 72 (INTER_MARKER_STRIDE - 8).

Used to budget 512-byte FatFs RMW batches for external-memory deint (see SOFT_FORMAT.md / README).
Reports **write** scatter (deinterleave write loop) and **read** pass (sequential ring reads after writes).
"""

from __future__ import annotations

import argparse
import csv
import statistics
import sys

RING_BYTES = 36 * 36 * 2048  # 2654208
SECTOR = 512
INTER_BRANCH_COUNT = 36
INTER_BRANCH_DELAY = 2048
INTER_MARKER_INTERSAMPS = 72
DEFAULT_BATCH = 8192


def deinterleave_read_sectors(offset_start: int, length: int) -> int:
    """Distinct 512-byte sectors touched by meteor_deinterleave sequential read pass (post-write)."""
    read_idx = (offset_start + INTER_BRANCH_COUNT * INTER_BRANCH_DELAY) % RING_BYTES
    sectors: set[int] = set()
    for _ in range(length):
        sectors.add(read_idx // SECTOR)
        read_idx = (read_idx + 1) % RING_BYTES
    return len(sectors)


def deinterleave_write_indices(
    offset: int, cur_branch: int, length: int
) -> tuple[set[int], int, int, int, int, int]:
    """distinct sectors, multi-write cell count, min_write_idx, max_write_idx, new_offset, new_cur_branch."""
    write_idx_list: list[int] = []
    sectors: set[int] = set()
    for _ in range(length):
        if cur_branch == 0:
            pass
        delay = (cur_branch % INTER_BRANCH_COUNT) * INTER_BRANCH_DELAY * INTER_BRANCH_COUNT
        write_idx = (offset - delay + RING_BYTES) % RING_BYTES
        write_idx_list.append(write_idx)
        sectors.add(write_idx // SECTOR)
        offset = (offset + 1) % RING_BYTES
        cur_branch = (cur_branch + 1) % INTER_MARKER_INTERSAMPS
    uniq = set(write_idx_list)
    multi = len(write_idx_list) - len(uniq)
    wmin = min(write_idx_list) if write_idx_list else 0
    wmax = max(write_idx_list) if write_idx_list else 0
    return sectors, multi, wmin, wmax, offset, cur_branch


def run_batches(
    num_batches: int, batch_len: int, offset0: int, branch0: int
) -> list[tuple[int, int, int, int, int]]:
    rows: list[tuple[int, int, int, int, int]] = []
    off, br = offset0, branch0
    for _ in range(num_batches):
        read_sec = deinterleave_read_sectors(off, batch_len)
        sectors, multi, wmin, wmax, off, br = deinterleave_write_indices(off, br, batch_len)
        rows.append((len(sectors), multi, wmin, wmax, read_sec))
    return rows


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--batches", type=int, default=200, help="number of batches to simulate")
    ap.add_argument("--batch-len", type=int, default=DEFAULT_BATCH, help="deinterleave output len per batch")
    ap.add_argument("--offset0", type=int, default=0, help="initial _offset mod ring")
    ap.add_argument("--branch0", type=int, default=0, help="initial _cur_branch")
    ap.add_argument("--csv", type=str, default="", help="write CSV with sector + write_idx stats")
    ap.add_argument("--summary-only", action="store_true", help="print p50/p95/max only")
    args = ap.parse_args()

    rows = run_batches(args.batches, args.batch_len, args.offset0 % RING_BYTES, args.branch0 % INTER_MARKER_INTERSAMPS)
    counts = [r[0] for r in rows]
    multis = [r[1] for r in rows]
    read_counts = [r[4] for r in rows]

    def pct(p: float) -> float:
        if not counts:
            return 0.0
        s = sorted(counts)
        k = min(len(s) - 1, max(0, int(round((p / 100.0) * (len(s) - 1)))))
        return float(s[k])

    print(f"ring_bytes={RING_BYTES} sectors_total={RING_BYTES // SECTOR} batch_len={args.batch_len}")
    print(f"batches={args.batches} distinct_sectors_per_batch: min={min(counts)} max={max(counts)}")
    print(f"  p50={statistics.median(counts):.1f} p95={pct(95):.0f}")
    print(f"multi_write_cells_per_batch: min={min(multis)} max={max(multis)} total={sum(multis)}")
    print(f"write_idx_span_batch0: min={rows[0][2]} max={rows[0][3]}")
    print(
        f"distinct_read_sectors_per_batch (sequential read pass): min={min(read_counts)} max={max(read_counts)} "
        f"p50={statistics.median(read_counts):.1f}"
    )
    comb = [w + r for w, r in zip(counts, read_counts)]
    print(
        f"write+read_distinct_sectors_upper_bound (naive sum; overlap ignored): min={min(comb)} max={max(comb)} "
        f"p50={statistics.median(comb):.1f}"
    )

    if args.csv:
        with open(args.csv, "w", newline="", encoding="utf-8") as f:
            w = csv.writer(f)
            w.writerow(
                [
                    "batch_id",
                    "distinct_write_sectors",
                    "multi_write_cells",
                    "min_write_idx",
                    "max_write_idx",
                    "distinct_read_sectors",
                ]
            )
            for i, row in enumerate(rows):
                w.writerow([i, row[0], row[1], row[2], row[3], row[4]])
        print(f"Wrote {args.csv}")

    if not args.summary_only and args.batches <= 32:
        for i, row in enumerate(rows):
            print(f"  batch {i}: write_sec={row[0]} read_sec={row[4]} multi={row[1]} idx[{row[2]},{row[3]}]")

    return 0


if __name__ == "__main__":
    sys.exit(main())
