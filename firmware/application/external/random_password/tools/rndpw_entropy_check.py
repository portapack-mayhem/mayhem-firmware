#!/usr/bin/env python3
#
# Copyright (C) 2026 zxkmm
#
# This file is part of PortaPack.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

"""Assess the raw entropy source behind the PortaPack "Rand Pwd" app.

Feed it /LOGS/RANDOM.TXT from the SD card, written while the app's "save"
checkbox was ticked. Those R lines are the raw ADC least-significant bits the
firmware harvested -- they ARE the seed material, which is why the app warns
before writing them and why the checkbox defaults to off. Treat the log as a
secret, and do not use passwords generated during a logged session.

    ./rndpw_entropy_check.py RANDOM.TXT
    ./rndpw_entropy_check.py RANDOM.TXT --png out

What this can and cannot tell you
---------------------------------
Passing everything here does NOT prove the source is good. Every test below is
a *necessary* condition, never a sufficient one. This was checked rather than
assumed: a 64-bit LCG truncated to one byte, and a 32-bit LCG truncated to one
byte, both pass every test in this script cleanly, including the scatter plot.
Any competent PRNG will. If you want a guarantee that the bytes came from
physics rather than from software, no statistical tool can give it to you --
that guarantee has to come from the architecture, which in this case is that
the bytes are ADC least-significant bits and there is no PRNG anywhere upstream
of them.

What these tests are for is catching the failures that plausibly happen to THIS
hardware, and they do that well:

  - stuck, dead or clipped ADC channel   -> longest-run, monobit, min-entropy
  - DC-offset front end (I and Q each    -> per-channel monobit and the
    constant, interleaving to 0101...)      firmware's own health flags
  - gross bias                           -> monobit, chi-square, MCV
  - sample-to-sample correlation, i.e.   -> serial correlation, bit
    the analogue LPF being too narrow       autocorrelation, and above all
    for the sample rate                     the scatter plot, where it shows
                                            up as an unmistakable diagonal
                                            ridge instead of uniform speckle
  - a source that is looping             -> repeated-chunk test

The correlation case is the one worth taking seriously, because it is the
failure mode the firmware's design most depends on not happening (it sets the
baseband filter wide open specifically to avoid it) and the one the on-device
first-order estimator sees only partially.

Standard library only, no numpy required.
"""

import argparse
import collections
import math
import struct
import sys
import zlib

FLAG_NAMES = [
    (0x01, "rep-I"),
    (0x02, "rep-Q"),
    (0x04, "bias-I"),
    (0x08, "bias-Q"),
    (0x10, "period-I"),
    (0x20, "period-Q"),
]


class Block:
    __slots__ = ("data", "freq_hz", "h_mbits", "flags")

    def __init__(self, data, freq_hz, h_mbits, flags):
        self.data = data
        self.freq_hz = freq_hz
        self.h_mbits = h_mbits
        self.flags = flags


def parse_log(path):
    """Parse the app's log. Format, one record per line:

    # rndpw2 <datetime>
    R <freq_hz> <h_milli_bits> <health_flags> <128 hex chars>
    P <credited_bits> <password>
    """
    blocks = []
    passwords = []
    bad_lines = 0

    with open(path, "r", errors="replace") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue

            parts = line.split()
            if parts[0] == "R" and len(parts) == 5:
                try:
                    blocks.append(
                        Block(
                            bytes.fromhex(parts[4]),
                            int(parts[1]),
                            int(parts[2]),
                            int(parts[3]),
                        )
                    )
                except ValueError:
                    bad_lines += 1
            elif parts[0] == "P" and len(parts) >= 3:
                passwords.append((int(parts[1]), " ".join(parts[2:])))
            else:
                bad_lines += 1

    return blocks, passwords, bad_lines


def bits_of(data):
    """Unpack to a list of 0/1 in the same order the firmware packed them.

    The firmware sets bit (n & 7) of byte (n >> 3) for the nth raw bit, and the
    raw bits alternate I, Q, I, Q, ... so even indices are the I channel and
    odd indices are Q.
    """
    out = []
    for byte in data:
        for i in range(8):
            out.append((byte >> i) & 1)
    return out


def min_entropy_mcv(symbols, alphabet_size):
    """SP 800-90B most-common-value min-entropy estimate, with the 99% upper
    confidence bound on p_max that the standard specifies."""
    n = len(symbols)
    if n == 0:
        return 0.0
    counts = collections.Counter(symbols)
    p_hat = max(counts.values()) / n
    p_upper = min(1.0, p_hat + 2.576 * math.sqrt(p_hat * (1.0 - p_hat) / n))
    if p_upper <= 0:
        return math.log2(alphabet_size)
    return -math.log2(p_upper)


def chi_square_uniform(counts, n, categories):
    expected = n / categories
    if expected <= 0:
        return 0.0
    return sum((c - expected) ** 2 / expected for c in counts)


def chi_square_sf(x, dof):
    """Survival function for chi-square. Only needs to be good enough to tell
    'obviously fine' from 'obviously broken', so use the Wilson-Hilferty
    normal approximation, which is accurate for the dof values used here."""
    if dof <= 0:
        return 1.0
    t = (x / dof) ** (1.0 / 3.0)
    mean = 1.0 - 2.0 / (9.0 * dof)
    sd = math.sqrt(2.0 / (9.0 * dof))
    if sd == 0:
        return 1.0
    z = (t - mean) / sd
    return 0.5 * math.erfc(z / math.sqrt(2.0))


def normal_sf(z):
    return 0.5 * math.erfc(z / math.sqrt(2.0))


def monobit(bits):
    """NIST SP 800-22 frequency (monobit) test."""
    n = len(bits)
    if n == 0:
        return 1.0, 0.0
    s = sum(1 if b else -1 for b in bits)
    s_obs = abs(s) / math.sqrt(n)
    return math.erfc(s_obs / math.sqrt(2.0)), s / n


def runs_test(bits):
    """NIST SP 800-22 runs test. Requires the monobit prerequisite to hold."""
    n = len(bits)
    if n < 100:
        return None, None
    pi = sum(bits) / n
    if abs(pi - 0.5) >= 2.0 / math.sqrt(n):
        return None, pi
    v = 1 + sum(1 for i in range(1, n) if bits[i] != bits[i - 1])
    num = abs(v - 2.0 * n * pi * (1 - pi))
    den = 2.0 * math.sqrt(2.0 * n) * pi * (1 - pi)
    return math.erfc(num / den), pi


def longest_run(bits):
    best = cur = 0
    prev = None
    for b in bits:
        cur = cur + 1 if b == prev else 1
        prev = b
        best = max(best, cur)
    return best


def serial_correlation(values):
    """Lag-1 serial correlation coefficient over byte values."""
    n = len(values)
    if n < 2:
        return 0.0
    a = values[:-1]
    b = values[1:]
    m = n - 1
    mean_a = sum(a) / m
    mean_b = sum(b) / m
    cov = sum((x - mean_a) * (y - mean_b) for x, y in zip(a, b))
    va = sum((x - mean_a) ** 2 for x in a)
    vb = sum((y - mean_b) ** 2 for y in b)
    if va <= 0 or vb <= 0:
        return 0.0
    return cov / math.sqrt(va * vb)


def bit_autocorrelation(bits, max_lag):
    """Fraction of agreements at each lag; 0.5 is ideal. Reported as a z score
    so the numbers are comparable across sample sizes."""
    out = []
    n = len(bits)
    for lag in range(1, max_lag + 1):
        m = n - lag
        if m < 100:
            break
        agree = sum(1 for i in range(m) if bits[i] == bits[i + lag])
        p = agree / m
        z = (p - 0.5) * 2.0 * math.sqrt(m)
        out.append((lag, p, z))
    return out


def write_png(path, width, height, rgb_rows):
    """Minimal RGB PNG writer, so this script needs no image dependencies."""
    raw = b"".join(b"\x00" + bytes(row) for row in rgb_rows)

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    header = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    png = (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", header)
        + chunk(b"IDAT", zlib.compress(raw, 9))
        + chunk(b"IEND", b"")
    )
    with open(path, "wb") as f:
        f.write(png)


def scatter_png(path, values, size=256):
    """Plot value[i] against value[i+1].

    Verified behaviour: a healthy source fills the square with even speckle.
    A source whose consecutive samples are correlated -- an oversampled ADC
    behind too narrow an analogue filter, which is this hardware's real risk --
    collapses onto a diagonal ridge that is impossible to miss.

    It does NOT reveal a byte-truncated LCG; that was tested and the plot looks
    like clean noise. Do not read "the scatter looks fine" as "the source is
    not deterministic"."""
    grid = [[0] * size for _ in range(size)]
    peak = 0
    for x, y in zip(values[:-1], values[1:]):
        gx = x * size // 256
        gy = y * size // 256
        grid[gy][gx] += 1
        peak = max(peak, grid[gy][gx])

    rows = []
    for y in range(size):
        row = bytearray()
        for x in range(size):
            n = grid[y][x]
            if n == 0:
                row += b"\x10\x10\x18"
            else:
                # Normalize against the peak so sparse data stays visible.
                v = int(255 * (math.log1p(n) / math.log1p(peak)))
                row += bytes((v, v, min(255, v + 40)))
        rows.append(row)
    write_png(path, size, size, rows)


def bitmap_png(path, bits, width=512):
    """One pixel per raw bit, in collection order. Visible vertical striping
    means periodic structure locked to the packing period; visible horizontal
    banding means the source changed quality over time."""
    height = min(512, len(bits) // width)
    if height < 1:
        return False
    rows = []
    for y in range(height):
        row = bytearray()
        for x in range(width):
            b = bits[y * width + x]
            row += b"\xf0\xf0\xf0" if b else b"\x14\x14\x1c"
        rows.append(row)
    write_png(path, width, height, rows)
    return True


def verdict(ok):
    return "ok" if ok else "** LOOK AT THIS **"


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("logfile", help="RANDOM.TXT from the PortaPack SD card")
    ap.add_argument("--png", metavar="PREFIX",
                    help="also write PREFIX-scatter.png and PREFIX-bitmap.png")
    ap.add_argument("--max-lag", type=int, default=32,
                    help="highest bit lag for the autocorrelation scan (default 32)")
    args = ap.parse_args()

    blocks, passwords, bad_lines = parse_log(args.logfile)

    if not blocks:
        print("No R (raw block) records found.")
        print("Tick 'save' in the app before generating, then generate a few")
        print("passwords -- ideally with flood mode on, to collect more data.")
        return 1

    print("=" * 68)
    print("source data")
    print("=" * 68)
    print(f"  raw blocks           {len(blocks)}")
    print(f"  passwords logged     {len(passwords)}")
    if bad_lines:
        print(f"  unparsed lines       {bad_lines}")

    freqs = sorted({b.freq_hz for b in blocks})
    print(f"  distinct frequencies {len(freqs)}")
    if freqs:
        print(f"  frequency span       {min(freqs)/1e6:.1f} - {max(freqs)/1e6:.1f} MHz")

    flagged = [b for b in blocks if b.flags]
    print(f"  blocks flagged by on-device health tests   {len(flagged)}")
    if flagged:
        seen = collections.Counter()
        for b in flagged:
            for mask, name in FLAG_NAMES:
                if b.flags & mask:
                    seen[name] += 1
        for name, n in seen.most_common():
            print(f"      {name:<10} {n}")
        print("  NOTE: the firmware credits these zero, but they are still")
        print("        logged. They are included in the analysis below so you")
        print("        can see what a failure actually looks like.")

    h_values = [b.h_mbits / 1000.0 for b in blocks]
    if h_values:
        print(f"  on-device h estimate  min {min(h_values):.3f}  "
              f"mean {sum(h_values)/len(h_values):.3f}  max {max(h_values):.3f} bits/bit")

    data = b"".join(b.data for b in blocks)
    bits = bits_of(data)
    i_bits = bits[0::2]
    q_bits = bits[1::2]

    print()
    print("=" * 68)
    print(f"bit level  ({len(bits)} raw bits: {len(i_bits)} I, {len(q_bits)} Q)")
    print("=" * 68)

    all_ok = True

    for label, stream in (("all", bits), ("I", i_bits), ("Q", q_bits)):
        p_mono, bias = monobit(stream)
        p_runs, _ = runs_test(stream)
        lr = longest_run(stream)
        h_bit = min_entropy_mcv(stream, 2)

        mono_ok = p_mono >= 0.01
        runs_ok = (p_runs is None) or (p_runs >= 0.01)
        # A run this long in a fair stream of this size is essentially
        # impossible; it is the same failure the firmware watches for.
        run_ok = lr < 64
        all_ok &= mono_ok and runs_ok and run_ok

        print(f"  [{label}]")
        print(f"    ones fraction      {0.5 + bias/2:.5f}")
        print(f"    monobit p          {p_mono:.4f}   {verdict(mono_ok)}")
        if p_runs is None:
            print(f"    runs p             skipped (monobit prerequisite failed)")
        else:
            print(f"    runs p             {p_runs:.4f}   {verdict(runs_ok)}")
        print(f"    longest run        {lr}   {verdict(run_ok)}")
        print(f"    min-entropy (MCV)  {h_bit:.4f} bits/bit")

    print()
    print("  bit autocorrelation, |z| > 4 is worth investigating:")
    worst = None
    for lag, p, z in bit_autocorrelation(bits, args.max_lag):
        if worst is None or abs(z) > abs(worst[2]):
            worst = (lag, p, z)
    if worst:
        lag, p, z = worst
        ac_ok = abs(z) <= 4.0
        all_ok &= ac_ok
        print(f"    worst lag {lag:<3} agreement {p:.5f}  z {z:+.2f}   {verdict(ac_ok)}")
        print("    (lag 2 matters most here: raw bits alternate I,Q,I,Q so lag 2")
        print("     is the within-channel sample-to-sample correlation)")

    print()
    print("=" * 68)
    print(f"byte level  ({len(data)} bytes)")
    print("=" * 68)

    counts = [0] * 256
    for byte in data:
        counts[byte] += 1
    chi = chi_square_uniform(counts, len(data), 256)
    p_chi = chi_square_sf(chi, 255)
    chi_ok = p_chi >= 0.01
    all_ok &= chi_ok

    # Repeated chunks. A source that is looping -- a replayed buffer, a stuck
    # DMA ring, a periodic interferer locked to the harvest period -- shows up
    # here and nowhere else. For 8-byte chunks the birthday expectation on a
    # healthy source is ~n^2 / 2^65, which is zero for any realistic log, so
    # any collision at all is a finding.
    chunk_len = 8
    chunks = [data[i:i + chunk_len]
              for i in range(0, len(data) - chunk_len + 1, chunk_len)]
    chunk_counts = collections.Counter(chunks)
    repeats = sum(c - 1 for c in chunk_counts.values() if c > 1)
    repeat_ok = repeats == 0
    all_ok &= repeat_ok

    sc = serial_correlation(list(data))
    sc_z = sc * math.sqrt(max(1, len(data) - 1))
    sc_ok = abs(sc_z) <= 4.0
    all_ok &= sc_ok

    h_byte = min_entropy_mcv(list(data), 256)

    print(f"  chi-square (255 dof) {chi:.1f}   p {p_chi:.4f}   {verdict(chi_ok)}")
    print(f"  serial correlation   {sc:+.5f}  z {sc_z:+.2f}   {verdict(sc_ok)}")
    print(f"  repeated 8B chunks   {repeats} of {len(chunks)}   {verdict(repeat_ok)}")
    print(f"  min-entropy (MCV)    {h_byte:.3f} of 8 bits/byte")
    print(f"  compressed size      {len(zlib.compress(data, 9))} of {len(data)} bytes")
    print("    (a good source will not compress; note zlib adds ~11 bytes of")
    print("     overhead, so tiny logs can appear to 'expand')")

    if len(data) < 4096:
        print()
        print("  WARNING: fewer than 4096 bytes. These statistics are weak at")
        print("           this size. Run flood mode for a minute and re-check.")

    print()
    print("=" * 68)
    print("what the firmware would have credited")
    print("=" * 68)
    credited = sum(b.h_mbits for b in blocks if not b.flags)
    per_block_cap = 512 * 0.125
    print(f"  512 raw bits per block, credited at min(0.125, 0.5*h) bits/bit,")
    print(f"  so at most {per_block_cap:.0f} bits per block.")
    measured_h = h_byte / 8.0
    print(f"  measured whole-log min-entropy   {measured_h:.4f} bits/bit")
    print(f"  the firmware's credit rate        0.1250 bits/bit")
    margin = measured_h / 0.125 if measured_h > 0 else 0.0
    print(f"  margin of the claim               {margin:.1f}x")
    if margin < 1.0:
        print("  ** the measured entropy is BELOW the credited rate. The budget")
        print("     is not conservative for this data. Investigate before trusting")
        print("     anything this app generated. **")
        all_ok = False
    else:
        print("  (the design targets 8x; the MCV estimator is itself conservative,")
        print("   so expect this to land somewhere near 8 on a healthy source)")

    if args.png:
        scatter_path = f"{args.png}-scatter.png"
        scatter_png(scatter_path, list(data))
        print()
        print(f"  wrote {scatter_path}")
        print("    consecutive bytes plotted against each other. Look for ANY")
        print("    structure: lattice planes, diagonal lines, empty regions.")
        print("    Uniform speckle is what you want.")

        bitmap_path = f"{args.png}-bitmap.png"
        if bitmap_png(bitmap_path, bits):
            print(f"  wrote {bitmap_path}")
            print("    one pixel per raw bit in collection order. Vertical stripes")
            print("    mean periodic structure; horizontal bands mean the source")
            print("    quality changed over time.")

    print()
    print("=" * 68)
    if all_ok:
        print("no failures detected.")
        print()
        print("This means the source did not fail any test that could plausibly")
        print("catch this hardware's real failure modes. It is NOT proof of")
        print("randomness -- no statistical test can provide that. Look at the")
        print("scatter image too; the eye catches lattice structure that these")
        print("summary statistics do not.")
    else:
        print("SOMETHING FAILED. See the marked lines above.")
    print("=" * 68)

    return 0 if all_ok else 2


if __name__ == "__main__":
    sys.exit(main())
