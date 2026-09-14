#!/usr/bin/env python3
"""Isolate the demod primitive: synth a known LoRa symbol, demod, check it round-trips."""
import numpy as np
from lora_rx import make_chirps, demod


def synth_symbol(V, N, os, bw, fs):
    sps = N*os
    t = np.arange(sps)/fs
    Tsym = sps/fs
    f0 = -bw/2 + V*bw/N
    f = f0 + (bw/Tsym)*t
    f = ((f + bw/2) % bw) - bw/2          # cyclic wrap into [-bw/2, bw/2)
    phase = 2*np.pi*np.cumsum(f)/fs
    return np.exp(1j*phase)


def main():
    sf, bw, fs = 7, 500e3, 4e6
    N, os, sps, up, down = make_chirps(sf, bw, fs)
    bad = 0
    for V in range(N):
        x = synth_symbol(V, N, os, bw, fs)
        sym, mag, shp = demod(x, down, N, os)
        if sym != V:
            if bad < 12:
                print(f"V={V:3d} -> demod={sym:3d}  (N-V={N-V}, sharp={shp:.0f})")
            bad += 1
    print(f"\nmismatches: {bad}/{N}")
    # also: what does a base upchirp (V=0) and downchirp give?
    print("V=0 demod:", demod(synth_symbol(0, N, os, bw, fs), down, N, os)[0])


if __name__ == "__main__":
    main()
