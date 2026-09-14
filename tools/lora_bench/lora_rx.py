#!/usr/bin/env python3
"""Full golden LoRa receiver (numpy) — decodes a cs8 capture to PHY payload bytes.

Pipeline: load -> burst -> coarse CFO -> preamble run -> grid align (max data
sharpness) -> demod -> gray -> deinterleave -> hamming -> dewhiten -> bytes.

Decode-chain formulas are ported from our firmware (proc_lora.cpp) so a correct
decode also validates the firmware's interleaver/Hamming/gray/whitening.

Validation signature: a broadcast Meshtastic packet de-whitens to
  FF FF FF FF <src LE> ...   (src for Heltec !6983d19c = 9C D1 83 69)
"""
import argparse
import numpy as np


def load_cs8(path):
    raw = np.fromfile(path, dtype=np.int8).astype(np.float32)
    return raw[0::2] + 1j * raw[1::2]


def find_strongest_burst(iq, fs):
    win = max(int(fs * 0.001), 1)
    n = len(iq) // win
    pwr = np.array([np.mean(np.abs(iq[i*win:(i+1)*win])**2) for i in range(n)])
    pdb = 10*np.log10(pwr + 1e-9)
    floor = np.percentile(pdb, 20)
    thr = floor + max(6.0, (pdb.max()-floor)*0.5)
    active = pdb > thr
    best = None; i = 0
    while i < n:
        if active[i]:
            j = i
            while j < n and active[j]:
                j += 1
            if best is None or (j-i) > best[1]-best[0]:
                best = (i, j)
            i = j
        else:
            i += 1
    a = max(0, int((best[0]-3)*win)); b = min(len(iq), int((best[1]+3)*win))
    return a, b


def make_chirps(sf, bw, fs):
    N = 1 << sf
    os = int(round(fs/bw)); sps = N*os
    t = np.arange(sps)/fs; Tsym = sps/fs
    up = np.exp(1j*(np.pi*bw/Tsym*t*t - np.pi*bw*t))
    return N, os, sps, up, np.conj(up)


def demod(block, ref, N, os):
    # dechirp then fold os aliases (= decimate-by-os) -> clean N-bin spectrum
    D = np.fft.fft(block*ref)
    Y = np.abs(D.reshape(os, N).sum(axis=0))
    peak = int(np.argmax(Y))
    return peak, Y[peak], Y[peak]/(np.median(Y)+1e-9)


def gray_decode(v):
    mask = v >> 1
    while mask:
        v ^= mask; mask >>= 1
    return v


def gray_map(v, gd):
    return gray_decode(v) if gd == 0 else (v ^ (v >> 1))


def deinterleave(syms, cpb, cr, var=0):
    """cr symbols of cpb bits -> cpb codewords of cr bits. var selects diagonal variant."""
    cw = [0]*cpb
    for i in range(cpb):
        for j in range(cr):
            if var == 0:
                idx = (i - j + cpb) % cpb
            else:
                idx = (i + j) % cpb
            bit = (syms[j] >> idx) & 1
            cw[i] |= bit << (cr-1-j)
    return cw


def hamming_nibble(cw, cr):
    return (cw >> (cr-4)) & 0xF


def whiten_seq(n):
    """LoRa data-whitening LFSR x^8+x^6+x^5+x^4+1, seed 0xFF (matches firmware)."""
    out = []; s = 0xFF
    for _ in range(n):
        out.append(s)
        fb = ((s>>7) ^ (s>>5) ^ (s>>4) ^ (s>>3)) & 1
        s = ((s<<1) | fb) & 0xFF
    return out


def decode_payload(data_syms, sf, offset, gd, var, cr=5):
    """Decode payload symbols (after the 8 header symbols) to de-whitened bytes."""
    N = 1 << sf
    sf_app = sf  # no LDRO for SF7
    nibbles = []
    for bi in range(0, len(data_syms) - cr + 1, cr):
        blk = [gray_map((s - offset) % N & (N-1), gd) for s in data_syms[bi:bi+cr]]
        cw = deinterleave(blk, sf_app, cr, var)
        for c in cw:
            nibbles.append(hamming_nibble(c, cr))
    raw_bytes = bytes((nibbles[i+1] << 4) | nibbles[i] for i in range(0, len(nibbles)-1, 2))
    w = whiten_seq(len(raw_bytes))
    return bytes(b ^ w[i] for i, b in enumerate(raw_bytes))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("path")
    ap.add_argument("--fs", type=float, default=4e6)
    ap.add_argument("--bw", type=float, default=500e3)
    ap.add_argument("--sf", type=int, default=7)
    ap.add_argument("--foff", type=float, default=-236e3)
    args = ap.parse_args()

    iq = load_cs8(args.path)
    a, b = find_strongest_burst(iq, args.fs)
    seg = iq[a:b]
    N, os, sps, up, down = make_chirps(args.sf, args.bw, args.fs)
    tt = np.arange(len(seg))/args.fs

    # coarse CFO: maximize preamble sharpness (v1 method)
    best = None
    for foff in np.arange(args.foff-60e3, args.foff+60e3, 2e3):
        sh = seg*np.exp(-1j*2*np.pi*foff*tt)
        sc = 0
        for start in range(int(2*sps), int(5*sps), os*4):
            _, _, shp = demod(sh[start:start+sps], down, N, os)
            sc += shp
        if best is None or sc > best[0]:
            best = (sc, foff)
    foff = best[1]
    sh = seg*np.exp(-1j*2*np.pi*foff*tt)
    print(f"CFO offset ~{foff/1e3:.1f} kHz")

    # STO search (v1 method): start offset in [0,sps) maximizing preamble consistency
    best = None
    for o in range(0, sps, max(1, os//2)):
        vals, sharps = [], []
        for k in range(2, 10):
            st = o + k*sps
            if st+sps > len(sh):
                break
            s, _, shp = demod(sh[st:st+sps], down, N, os)
            vals.append(s); sharps.append(shp)
        if len(vals) < 5:
            continue
        score = np.mean(sharps) - 3*np.std(vals)
        if best is None or score > best[0]:
            best = (score, o)
    o0 = best[1]

    # demod the stream, find longest preamble run (equal symbols)
    stream = []
    k = 0
    while o0 + (k+1)*sps <= len(sh):
        s, _, shp = demod(sh[o0+k*sps:o0+(k+1)*sps], down, N, os)
        stream.append((s, shp)); k += 1
    # longest run of (nearly) equal, sharp symbols
    runs = None; i = 0
    while i < len(stream):
        if stream[i][1] > 20:
            j = i
            while j < len(stream) and abs(stream[j][0]-stream[i][0]) <= 1 and stream[j][1] > 20:
                j += 1
            if runs is None or (j-i) > runs[1]-runs[0]:
                runs = (i, j)
            i = max(j, i+1)
        else:
            i += 1
    p0, p1 = runs
    k_up = int(np.round(np.median([stream[x][0] for x in range(p0, p1)])))
    print(f"preamble: symbols [{p0}..{p1-1}] ({p1-p0} syms) at o0={o0}, bin k_up={k_up}")

    # header starts after: preamble + 2 sync + 2.25 SFD (the .25 shifts the grid)
    header_start = o0 + int(round((p1 + 2 + 2.25) * sps))

    # fine align: maximize mean sharpness of ~16 data symbols over ±sps in 4-sample steps
    bestal = None
    for da in range(-sps//2, sps//2+1, 4):
        hs = header_start + da
        if hs < 0 or hs + 20*sps > len(sh):
            continue
        sc = 0
        for kk in range(2, 18):
            st = hs + kk*sps
            _, _, shp = demod(sh[st:st+sps], down, N, os)
            sc += shp
        if bestal is None or sc > bestal[0]:
            bestal = (sc, da)
    header_start += bestal[1]
    print(f"header_start sample={header_start} (fine da={bestal[1]})")

    # demod sync(2 before) + 50 data symbols
    def dm(k):
        st = header_start + k*sps
        if st < 0 or st+sps > len(sh):
            return None
        s, _, _ = demod(sh[st:st+sps], down, N, os)
        return s
    data = [dm(k) for k in range(0, 60) if dm(k) is not None]
    # sync sits 4.25 symbols before header (preamble|2 sync|2.25 SFD|header)
    sync = [dm(-5), dm(-4)]
    print(f"sync symbols (0x2B expect [16,88]): {[(s-k_up) % N for s in sync]}")

    SRC = bytes([0x9C, 0xD1, 0x83, 0x69])
    best_decode = None
    for skip in range(6, 12):
        for gd in (0, 1):
            for var in (0, 1):
                for off in range(N):
                    dew = decode_payload(data[skip:], args.sf, off, gd, var)
                    score = 0
                    if dew[:4] == b'\xff\xff\xff\xff':
                        score += 10
                    if SRC in dew[:24]:
                        score += 20
                    if b'\xff\xff\xff\xff' in dew[:12]:
                        score += 3
                    if best_decode is None or score > best_decode[0]:
                        best_decode = (score, skip, gd, var, off, dew)
    score, skip, gd, var, off, dew = best_decode
    print(f"\nbest: score={score} skip={skip} gray_dir={gd} deint_var={var} offset={off}")
    print(f"de-whitened payload[:32]: {dew[:32].hex(' ')}")
    if score >= 30:
        print("\n*** PHY DECODE VALID: broadcast dest + Heltec src present ***")
    else:
        print("\n(no signature — dumping a few candidates to eyeball)")
        for gd in (0, 1):
            for var in (0, 1):
                d = decode_payload(data[8:], args.sf, k_up, gd, var)
                print(f"  gd={gd} var={var} off=k_up: {d[:20].hex(' ')}")


if __name__ == "__main__":
    main()
