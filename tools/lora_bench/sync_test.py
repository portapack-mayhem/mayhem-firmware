#!/usr/bin/env python3
"""Focused front-end test: lock preamble grid (no CFO removal), read sync word.

Sync sits on the preamble grid (before the SFD), so offset = up_bin should yield
the two sync symbols directly. Meshtastic 0x2B -> expected [16, 88]."""
import sys
import numpy as np
from lora_rx import make_chirps, demod, find_strongest_burst, load_cs8

path = sys.argv[1] if len(sys.argv) > 1 else "captures/cap2.cs8"
fs, bw, sf = 4e6, 500e3, 7
iq = load_cs8(path)
a, b = find_strongest_burst(iq, fs)
seg = iq[a:b]
N, os, sps, up, down = make_chirps(sf, bw, fs)

# demod sliding by os (1-bin) steps, find longest constant-bin run = preamble
step = os
bins = []
for st in range(0, len(seg)-sps, step):
    s, m, shp = demod(seg[st:st+sps], down, N, os)
    bins.append((st, s, shp))

# longest run where bin stays ~constant and sharp
best = None
i = 0
while i < len(bins):
    if bins[i][2] > 10:
        j = i
        while j < len(bins) and abs(bins[j][1]-bins[i][1]) <= 1 and bins[j][2] > 10:
            j += 1
        if best is None or (j-i) > best[1]-best[0]:
            best = (i, j)
        i = max(j, i+1)
    else:
        i += 1
i0, i1 = best
# the run spans os-steps; the preamble length in symbols ~ (i1-i0)*step/sps
pre_start_sample = bins[i0][0]
pre_len_syms = (i1-i0)*step/sps
up_bin = int(np.round(np.median([bins[k][1] for k in range(i0, i1)])))
print(f"preamble: start~{pre_start_sample}, len~{pre_len_syms:.1f} syms, up_bin={up_bin}")

# grid: preamble symbols at pre_start_sample + k*sps. The run end (sample) marks
# end of preamble; sync = next 2 symbols on the SAME grid.
# Align grid to the LAST clean preamble symbol boundary:
pre_end_sample = bins[i1-1][0] + sps   # end of last detected preamble window
print("\nreading symbols after preamble end (offset=up_bin):")
for k in range(0, 8):
    st = pre_end_sample + k*sps
    if st+sps > len(seg):
        break
    s, m, shp = demod(seg[st:st+sps], down, N, os)
    su, mu, su2 = demod(seg[st:st+sps], up, N, os)   # up-ref to spot SFD downchirps
    tag = "  <-DOWNCHIRP(SFD)" if su2 > shp*1.3 else ""
    print(f"  +{k}: sym(-up_bin)={(s-up_bin)%N:3d}  sharp={shp:5.1f}{tag}")

# Also try grids shifted by ±a few bins to account for run-end rounding
print("\nscan grid offset to find sync [16,88]:")
for goff in range(-os*4, os*4+1, os):
    vals = []
    for k in range(0, 2):
        st = pre_end_sample + goff + k*sps
        s, _, _ = demod(seg[st:st+sps], down, N, os)
        vals.append((s-up_bin) % N)
    if 14 <= vals[0] <= 18 or 86 <= vals[1] <= 90:
        print(f"  goff={goff:+4d}: sync={vals}  <== matches 0x2B!")
    else:
        print(f"  goff={goff:+4d}: sync={vals}")
