#!/usr/bin/env python3
"""Golden LoRa CSS decoder (numpy) — reference 'oracle' for the bench.

v1 goal: load a cs8 capture, locate the strongest burst, estimate CFO/STO from the
preamble, demodulate the symbol stream, and PRINT it so we can confirm the on-air
format (preamble run, sync word symbols, SFD) before adding the decode chain.

Defaults match the measured Heltec capture: SF7, BW500, fs=4 Msps, sync 0x2B.
"""
import argparse
import numpy as np


def load_cs8(path):
    raw = np.fromfile(path, dtype=np.int8).astype(np.float32)
    return raw[0::2] + 1j * raw[1::2]


def find_strongest_burst(iq, fs, min_ms=5):
    win = max(int(fs * 0.001), 1)
    n = len(iq) // win
    pwr = np.array([np.mean(np.abs(iq[i*win:(i+1)*win])**2) for i in range(n)])
    pdb = 10*np.log10(pwr + 1e-9)
    floor = np.percentile(pdb, 20)
    thr = floor + max(6.0, (pdb.max()-floor)*0.5)
    active = pdb > thr
    best = None
    i = 0
    while i < n:
        if active[i]:
            j = i
            while j < n and active[j]:
                j += 1
            if (j-i)*1.0 >= min_ms and (best is None or (j-i) > best[1]-best[0]):
                best = (i, j)
            i = j
        else:
            i += 1
    if best is None:
        raise RuntimeError("no burst found")
    a = max(0, int((best[0]-3)*win))   # 3 ms pre-margin
    b = min(len(iq), int((best[1]+3)*win))
    return a, b


def make_chirps(sf, bw, fs):
    N = 1 << sf
    os = int(round(fs/bw))
    sps = N*os
    t = np.arange(sps)/fs
    Tsym = sps/fs
    up = np.exp(1j*(np.pi*bw/Tsym*t*t - np.pi*bw*t))
    return N, os, sps, up, np.conj(up)


def demod_sym(block, down, N, os):
    X = np.fft.fft(block*down)
    mag = np.abs(X)
    peak = int(np.argmax(mag))
    sym = int(round(peak/os)) % N
    sharp = mag[peak]/(np.median(mag)+1e-9)
    return sym, mag[peak], sharp


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("path")
    ap.add_argument("--fs", type=float, default=4e6)
    ap.add_argument("--bw", type=float, default=500e3)
    ap.add_argument("--sf", type=int, default=7)
    ap.add_argument("--foff", type=float, default=-236e3, help="signal center offset (Hz)")
    ap.add_argument("--nsym", type=int, default=90)
    args = ap.parse_args()

    iq = load_cs8(args.path)
    a, b = find_strongest_burst(iq, args.fs)
    print(f"burst samples [{a}..{b}] = {(b-a)/args.fs*1000:.1f} ms")
    seg = iq[a:b]

    # coarse CFO removal: scan foff, maximize preamble peak sharpness
    N, os, sps, up, down = make_chirps(args.sf, args.bw, args.fs)
    t = np.arange(len(seg))/args.fs
    best = None
    for foff in np.arange(args.foff-60e3, args.foff+60e3, 4e3):
        sh = seg*np.exp(-1j*2*np.pi*foff*t)
        # try a few symbol starts inside the early preamble region
        sc = 0
        for start in range(int(2.0*sps), int(4.0*sps), os*4):
            s, m, sharp = demod_sym(sh[start:start+sps], down, N, os)
            sc += sharp
        if best is None or sc > best[0]:
            best = (sc, foff)
    foff = best[1]
    print(f"chosen CFO offset ~{foff/1e3:.1f} kHz")
    sh = seg*np.exp(-1j*2*np.pi*foff*t)

    # STO: scan symbol start offset over [0,sps), maximize preamble consistency
    best = None
    for o in range(0, sps, max(1, os//2)):
        vals = []
        sharps = []
        for k in range(2, 8):
            st = o + k*sps
            if st+sps > len(sh):
                break
            s, m, sharp = demod_sym(sh[st:st+sps], down, N, os)
            vals.append(s); sharps.append(sharp)
        if len(vals) < 4:
            continue
        # reward: peaks strong AND symbols consistent (low spread)
        score = np.mean(sharps) - 3*np.std(vals)
        if best is None or score > best[0]:
            best = (score, o, int(np.round(np.median(vals))))
    _, o0, pre_val = best
    print(f"symbol start offset={o0} samples, preamble symbol value≈{pre_val}")

    # dump symbols
    print("\nidx  sym   peak     sharp")
    syms = []
    for k in range(args.nsym):
        st = o0 + k*sps
        if st+sps > len(sh):
            break
        s, m, sharp = demod_sym(sh[st:st+sps], down, N, os)
        # also try downchirp-as-up (detect SFD): demod with up reference
        Xd = np.abs(np.fft.fft(sh[st:st+sps]*up))
        downpeak = Xd.max()/(np.median(Xd)+1e-9)
        tag = ""
        if downpeak > sharp*1.5:
            tag = "  <-DOWN(SFD?)"
        syms.append(s)
        print(f"{k:3d}  {s:3d}  {m:8.0f}  {sharp:6.1f}{tag}")

    # heuristic: find preamble run then show following (sync) symbols
    print("\nlooking for preamble run + sync:")
    for k in range(2, len(syms)-3):
        run = syms[k-2:k+1]
        if max(run)-min(run) <= 1:  # stable preamble
            print(f"  preamble≈{run[-1]} ends ~idx{k}; next syms = {syms[k+1:k+6]}")
            break


if __name__ == "__main__":
    main()
