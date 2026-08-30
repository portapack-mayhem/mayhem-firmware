#!/usr/bin/env python3
"""Crack the LoRa decode chain offline against the known signature.

1) robust symbol grid (2D phase search) -> extract data symbols (verified sharp)
2) wide brute over chain variants (gray dir, interleave variant, hamming bit order,
   nibble order, whitening, offset, header-skip) -> find FF FF FF FF <src>.

A hit pins the exact Semtech chain for BOTH our RX and TX encoder.
"""
import numpy as np
from lora_rx import make_chirps, demod, find_strongest_burst, load_cs8, whiten_seq

FS, BW, SF = 4e6, 500e3, 7
SRC = bytes([0x9C, 0xD1, 0x83, 0x69])   # Heltec !6983d19c, little-endian
BCAST = b'\xff\xff\xff\xff'


def hamming_nibble(cw, cr, hi):
    return (cw >> (cr-4)) & 0xF if hi else (cw & 0xF)


def gray_map(v, gd):
    if gd == 0:
        x = v; m = x >> 1
        while m:
            x ^= m; m >>= 1
        return x
    return v ^ (v >> 1)


_VARIANTS = [
    lambda i, j, c: (i - j) % c,
    lambda i, j, c: (i + j) % c,
    lambda i, j, c: (i - j - 1) % c,
    lambda i, j, c: (j - i) % c,
    lambda i, j, c: (j - i - 1) % c,   # gr-lora_sdr
    lambda i, j, c: (i - j + 1) % c,
    lambda i, j, c: (j - i + 1) % c,
    lambda i, j, c: (i + j - 1) % c,
    lambda i, j, c: (i + j + 1) % c,
]
NVAR = len(_VARIANTS)


def deinter(syms, cpb, cr, var):
    f = _VARIANTS[var]
    cw = [0]*cpb
    for i in range(cpb):
        for j in range(cr):
            cw[i] |= ((syms[j] >> f(i, j, cpb)) & 1) << (cr-1-j)
    return cw


def decode(data, off, gd, var, hi, norder, wh, cr=5, sf_app=SF):
    N = 1 << SF
    nib = []
    for bi in range(0, len(data)-cr+1, cr):
        blk = [gray_map(((s-off) % N) & (N-1), gd) for s in data[bi:bi+cr]]
        for c in deinter(blk, sf_app, cr, var):
            nib.append(hamming_nibble(c, cr, hi))
    if norder == 0:
        rb = bytes((nib[i+1] << 4) | nib[i] for i in range(0, len(nib)-1, 2))
    else:
        rb = bytes((nib[i] << 4) | nib[i+1] for i in range(0, len(nib)-1, 2))
    if wh:
        w = whiten_seq(len(rb))
        rb = bytes(b ^ w[i] for i, b in enumerate(rb))
    return rb


def robust_grid(seg, N, os, sps, down, up):
    best = None
    for phase in range(0, sps, sps//16):
        bins = []
        k = 0
        while phase+(k+1)*sps <= len(seg):
            s, m, shp = demod(seg[phase+k*sps:phase+(k+1)*sps], down, N, os)
            bins.append((s, shp)); k += 1
        # longest run of near-equal sharp symbols
        i = 0
        while i < len(bins):
            if bins[i][1] > 15:
                j = i
                while j < len(bins) and abs(bins[j][0]-bins[i][0]) <= 1 and bins[j][1] > 15:
                    j += 1
                if best is None or (j-i) > best[0]:
                    best = (j-i, phase, i, j, int(np.median([bins[x][0] for x in range(i, j)])))
                i = max(j, i+1)
            else:
                i += 1
    runlen, phase, i0, i1, up_bin = best
    print(f"grid: phase={phase} preamble symbols [{i0}..{i1-1}] ({runlen}) up_bin={up_bin}")
    pre_end = phase + i1*sps
    return pre_end, up_bin


def main():
    iq = load_cs8("captures/cap2.cs8")
    a, b = find_strongest_burst(iq, FS)
    seg = iq[a:b]
    N, os, sps, up, down = make_chirps(SF, BW, FS)

    # remove CFO: fine scan maximizing preamble-region peak sharpness -> crisp symbols
    tt = np.arange(len(seg))/FS
    best = None
    for foff in np.arange(-340e3, -150e3, 1e3):
        shf = seg*np.exp(-1j*2*np.pi*foff*tt)
        sc = sum(demod(shf[s:s+sps], down, N, os)[2] for s in range(2*sps, 6*sps, os*4))
        if best is None or sc > best[0]:
            best = (sc, foff)
    foff = best[1]
    seg = seg*np.exp(-1j*2*np.pi*foff*tt)
    print(f"CFO removed: {foff/1e3:.1f} kHz")
    pre_end, up_bin = robust_grid(seg, N, os, sps, down, up)

    # header starts 2 sync + 2.25 SFD after preamble end
    h0 = pre_end + int(round((2 + 2.25)*sps))
    # fine align on data sharpness
    bestal = None
    for da in range(-sps, sps+1, 8):
        hs = h0+da
        if hs < 0 or hs+24*sps > len(seg): continue
        sc = sum(demod(seg[hs+k*sps:hs+(k+1)*sps], down, N, os)[2] for k in range(2, 20))
        if bestal is None or sc > bestal[0]:
            bestal = (sc, da)
    h0 += bestal[1]
    data = []
    sharps = []
    for k in range(0, 55):
        st = h0+k*sps
        if st+sps > len(seg): break
        s, m, shp = demod(seg[st:st+sps], down, N, os)
        data.append(s); sharps.append(shp)
    print(f"header_start fine da={bestal[1]}; {len(data)} data syms, mean sharp={np.mean(sharps):.1f}")

    hits = []
    for skip in range(6, 11):
        d = data[skip:]
        for off in range(N):
            for gd in (0, 1):
                for var in range(NVAR):
                    for hi in (0, 1):
                        for norder in (0, 1):
                            for wh in (0, 1):
                                rb = decode(d, off, gd, var, hi, norder, wh)
                                if rb[:4] == BCAST or SRC in rb[:20]:
                                    sc = (10 if rb[:4] == BCAST else 0) + (20 if SRC in rb[:20] else 0)
                                    hits.append((sc, skip, off, gd, var, hi, norder, wh, rb[:24].hex(' ')))
    hits.sort(reverse=True)
    print(f"\n{len(hits)} candidate hit(s):")
    for h in hits[:12]:
        print("  score=%d skip=%d off=%d gray=%d var=%d hi=%d norder=%d wh=%d | %s" % h)
    if not hits:
        print("  none — grid/symbols likely off or extra transform needed")


if __name__ == "__main__":
    main()
