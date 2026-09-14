#!/usr/bin/env python3
"""Offline SF12 receiver, run against real air recorded off a stock node.

The firmware cannot decode SF12 today: its streaming demodulator is gated to
SF7..SF11 because the reference chirp, the FFT and the sample window do not fit
54 KiB at 4096 samples per symbol. Before rearranging that - the riskiest code in
the project - the algorithm gets proven here, where an iteration costs a second
instead of a flash cycle.
"""
import sys, numpy as np

FS_IN, DEC = 1e6, 8
SF, BW = 12, 125e3
N = 1 << SF                      # 4096 chips, and 4096 samples at 125 kHz

def load(path, seconds=None, skip=0.0):
    cnt = -1 if seconds is None else int(2 * FS_IN * seconds)
    off = int(2 * FS_IN * skip)
    d = np.fromfile(path, dtype=np.int8, count=cnt, offset=off)
    iq = (d[0::2].astype(np.float32) + 1j * d[1::2].astype(np.float32)) / 128.0
    # Decimate 1 Msps -> 125 kHz. A boxcar is a poor filter but the neighbours are
    # empty here and it keeps this honest about what the firmware can afford.
    return iq[:len(iq) // DEC * DEC].reshape(-1, DEC).mean(axis=1)

n = np.arange(N)
UP = np.exp(1j * np.pi * (n * n / N - n))
DOWN = np.conj(UP)

def demod(x, i, ref):
    """argmax bin, sharpness and peak of one symbol at sample i."""
    if i < 0 or i + N > len(x):
        return None
    S = np.fft.fft(x[i:i + N] * ref)
    m = np.abs(S)
    b = int(np.argmax(m))
    return b, (m[b] ** 2) / (m ** 2).sum(), S[b]

def find_preamble(x, thr=0.25):
    """A run of up-chirps landing on the same bin - that is the preamble."""
    run, prev, start = 0, -99, 0
    i = 0
    while i + N < len(x):
        r = demod(x, i, np.conj(UP))
        if r and r[1] > thr and abs(r[0] - prev) <= 1:
            run += 1
            if run >= 6:
                return start, r[0]
        elif r and r[1] > thr:
            run, start = 1, i
        else:
            run = 0
        prev = r[0] if r else -99
        i += N
    return None, None

if __name__ == "__main__":
    path = sys.argv[1]
    skip = float(sys.argv[2]) if len(sys.argv) > 2 else 9.0
    x = load(path, seconds=4.0, skip=skip)
    print(f"loaded {len(x)} samples = {len(x)/125e3:.2f} s from {skip:.1f} s")
    start, upbin = find_preamble(x)
    if start is None:
        print("no preamble found"); sys.exit(1)
    print(f"preamble at {start} ({start/125e3+skip:.4f} s), up bin {upbin}")
    # Walk forward: preamble symbols, then the 2.25-symbol SFD of down-chirps.
    for k in range(0, 20):
        i = start + k * N
        u = demod(x, i, np.conj(UP))
        d = demod(x, i, np.conj(DOWN))
        if not u or not d:
            break
        which = "DOWN" if d[1] > u[1] else "up  "
        print(f"  sym {k:2d}  up bin {u[0]:4d} sh {u[1]:.3f} | down bin {d[0]:4d} "
              f"sh {d[1]:.3f}  -> {which}")

# ---------------------------------------------------------------- decoding ----
def ldro_div4(chip, n=N):
    return ((chip + 2) & (n - 1)) >> 2

def gray(v):
    return v ^ (v >> 1)

def deint(syms, words, cw_len):
    cw = [0] * words
    for i in range(cw_len):
        for j in range(words):
            bit = (syms[i] >> (words - 1 - j)) & 1
            cw[(i - j - 1) % words] |= bit << (cw_len - 1 - i)
    return cw

def top4(cw, cw_len):
    b0 = (cw >> (cw_len-1)) & 1; b1 = (cw >> (cw_len-2)) & 1
    b2 = (cw >> (cw_len-3)) & 1; b3 = (cw >> (cw_len-4)) & 1
    return (b3 << 3) | (b2 << 2) | (b1 << 1) | b0

def ham84(cw):
    d0,d1,d2,d3 = (cw>>4)&1,(cw>>5)&1,(cw>>6)&1,(cw>>7)&1
    rp0,rp1,rp2,rp3 = (cw>>3)&1,(cw>>2)&1,(cw>>1)&1,cw&1
    s0=rp0^(d3^d2^d1); s1=rp1^(d2^d1^d0); s2=rp2^(d3^d2^d0); s3=rp3^(d3^d1^d0)
    syn=(s0<<3)|(s1<<2)|(s2<<1)|s3
    if syn==11: d3^=1
    elif syn==14: d2^=1
    elif syn==13: d1^=1
    elif syn==7: d0^=1
    return (d0<<3)|(d1<<2)|(d2<<1)|d3

def hdr_checksum(h0,h1,h2):
    c4=((h0>>3)&1)^((h0>>2)&1)^((h0>>1)&1)^(h0&1)
    c3=((h0>>3)&1)^((h1>>3)&1)^((h1>>2)&1)^((h1>>1)&1)^(h2&1)
    c2=((h0>>2)&1)^((h1>>3)&1)^(h1&1)^((h2>>3)&1)^((h2>>1)&1)
    c1=((h0>>1)&1)^((h1>>2)&1)^(h1&1)^((h2>>2)&1)^((h2>>1)&1)^(h2&1)
    c0=(h0&1)^((h1>>1)&1)^((h2>>3)&1)^((h2>>2)&1)^((h2>>1)&1)^(h2&1)
    return c4, (c3<<3)|(c2<<2)|(c1<<1)|c0

def dewhiten(nibs):
    st, out = 0xFF, []
    for i in range(len(nibs)//2):
        bv = (nibs[2*i+1] << 4) | nibs[2*i]
        fb = ((st>>7)^(st>>5)^(st>>4)^(st>>3)) & 1
        out.append(bv ^ st)
        st = ((st << 1) | fb) & 0xFF
    return out

WORDS = SF - 2                     # reduced rate: 32.768 ms per symbol

def bins_at(x, h0, count, da=0):
    out = []
    for k in range(count):
        r = demod(x, h0 + da + k * N, np.conj(UP))
        out.append(r[0] if r else 0)
    return out

def try_header(bins, off):
    g = [gray(ldro_div4((b - off) % N)) for b in bins[:8]]
    nib = [ham84(c & 0xFF) for c in deint(g, WORDS, 8)]
    c4, clo = hdr_checksum(nib[0], nib[1], nib[2])
    if (nib[3] & 1) != c4 or (nib[4] & 0xF) != clo:
        return None
    L = (nib[0] << 4) | nib[1]
    if not (4 <= L <= 250):
        return None
    return L, 4 + ((nib[2] >> 1) & 7), bool(nib[2] & 1), nib

def decode_frame(x, h0, off, cw_len, nib_hdr, nsym=40):
    """Header extras, then payload blocks of cw_len symbols, dewhitened."""
    pn = list(nib_hdr[5:WORDS])
    bins = bins_at(x, h0, nsym)
    bi = 8
    while bi + cw_len <= len(bins):
        g = [gray(ldro_div4((bins[bi+i] - off) % N)) for i in range(cw_len)]
        pn += [top4(c, cw_len) for c in deint(g, WORDS, cw_len)]
        bi += cw_len
    return dewhiten(pn)

def search(x, start, sfd_syms=(18,), extras=range(-4, 5), want_dest=0xFFFFFFFF):
    hits = []
    for sfd in sfd_syms:
        for extra in extras:
            h0 = start + int(round((sfd + 2.25) * N)) + extra * 8
            bins8 = bins_at(x, h0, 8)
            for off in range(N):
                r = try_header(bins8, off)
                if not r:
                    continue
                L, cw_len, crc, nib = r
                by = decode_frame(x, h0, off, cw_len, nib, nsym=8 + 2 * cw_len)
                if len(by) < 4:
                    continue
                dest = by[0] | (by[1] << 8) | (by[2] << 16) | (by[3] << 24)
                if dest == want_dest:
                    hits.append((sfd, extra, off, L, cw_len, crc, by))
    return hits
