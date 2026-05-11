#!/usr/bin/env python3
"""Gate G2 helper: crude lag correlation between two int8 soft-symbol dumps (same length).

Usage:
  python corr_soft.py device_soft.bin ref_soft.bin [--max-lag 2000]

Each file is a raw stream of signed 8-bit IQ-ish soft pairs (I,Q,I,Q,...) as emitted
by tooling; alignment is exploratory — prefer comparing against SatDump internal buffers
when available.
"""

from __future__ import annotations

import argparse
import struct
import sys
from typing import List


def read_i8(path: str) -> List[int]:
    with open(path, "rb") as f:
        data = f.read()
    return list(struct.unpack("%db" % len(data), data))


def best_corr(a: List[int], b: List[int], max_lag: int) -> tuple[int, float]:
    n = min(len(a), len(b))
    a = a[:n]
    b = b[:n]
    best_lag = 0
    best = -1e100
    for lag in range(-max_lag, max_lag + 1):
        s = 0
        cnt = 0
        if lag >= 0:
            for i in range(lag, n):
                s += a[i] * b[i - lag]
                cnt += 1
        else:
            L = -lag
            for i in range(L, n):
                s += a[i - L] * b[i]
                cnt += 1
        if cnt == 0:
            continue
        c = s / cnt
        if c > best:
            best = c
            best_lag = lag
    return best_lag, best


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("a")
    ap.add_argument("b")
    ap.add_argument("--max-lag", type=int, default=2000)
    args = ap.parse_args()
    a = read_i8(args.a)
    b = read_i8(args.b)
    lag, score = best_corr(a, b, args.max_lag)
    print("best_lag_samples", lag, "mean_product", score)
    return 0


if __name__ == "__main__":
    sys.exit(main())
