#!/usr/bin/env python3
"""Inspect a cs8 IQ capture: find LoRa bursts, estimate BW/SF, dump a spectrogram PNG.

This is the first look at a fresh capture — confirms center freq offset, occupied
bandwidth (→ BW), and chirp symbol duration (→ SF) before we trust any decoder.

Usage: python3 lora_inspect.py captures/cap1.cs8 --fs 2000000
"""
import argparse
import numpy as np


def load_cs8(path):
    raw = np.fromfile(path, dtype=np.int8).astype(np.float32)
    iq = raw[0::2] + 1j * raw[1::2]
    return iq


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("path")
    ap.add_argument("--fs", type=float, default=2e6)
    ap.add_argument("--png", default=None)
    args = ap.parse_args()

    iq = load_cs8(args.path)
    fs = args.fs
    dur = len(iq) / fs
    print(f"samples={len(iq)}  duration={dur:.3f}s  fs={fs/1e6:.3f} Msps")

    # Power envelope to find bursts
    win = int(fs * 0.001)  # 1 ms windows
    win = max(win, 1)
    n = len(iq) // win
    pwr = np.array([np.mean(np.abs(iq[i*win:(i+1)*win])**2) for i in range(n)])
    if n == 0:
        print("capture too short")
        return
    pdb = 10 * np.log10(pwr + 1e-9)
    floor = np.percentile(pdb, 20)
    peak = pdb.max()
    print(f"noise floor ~{floor:.1f} dB, peak ~{peak:.1f} dB, span {peak-floor:.1f} dB")

    thr = floor + max(6.0, (peak - floor) * 0.5)
    active = pdb > thr
    # group consecutive active windows into bursts
    bursts = []
    i = 0
    while i < n:
        if active[i]:
            j = i
            while j < n and active[j]:
                j += 1
            bursts.append((i*win/fs, j*win/fs))
            i = j
        else:
            i += 1
    print(f"detected {len(bursts)} burst(s) above {thr:.1f} dB:")
    for (a, b) in bursts[:20]:
        print(f"  {a*1000:8.1f} .. {b*1000:8.1f} ms  ({(b-a)*1000:6.1f} ms)")

    # Occupied bandwidth + center offset from the strongest burst
    if bursts:
        a, b = max(bursts, key=lambda x: x[1]-x[0])
        seg = iq[int(a*fs):int(b*fs)]
        if len(seg) > 1024:
            spec = np.abs(np.fft.fftshift(np.fft.fft(seg * np.hanning(len(seg)))))**2
            freqs = np.fft.fftshift(np.fft.fftfreq(len(seg), 1/fs))
            spec_db = 10*np.log10(spec + 1e-9)
            mask = spec_db > spec_db.max() - 20  # -20 dB occupied band
            occ = freqs[mask]
            print(f"strongest burst {a*1000:.1f}-{b*1000:.1f} ms: "
                  f"occupied ~{occ.min()/1e3:.0f}..{occ.max()/1e3:.0f} kHz "
                  f"(BW~{(occ.max()-occ.min())/1e3:.0f} kHz), "
                  f"center offset ~{(occ.min()+occ.max())/2/1e3:.0f} kHz")

    # clip fraction (saturation check)
    raw = np.fromfile(args.path, dtype=np.int8)
    clip = np.mean(np.abs(raw) >= 127)
    print(f"clip fraction (|sample|>=127): {clip*100:.2f}%")

    # Spectrogram of a window around the strongest burst only (full capture is too big)
    if bursts:
        a, b = max(bursts, key=lambda x: x[1]-x[0])
        c0 = max(0.0, a - 0.02)
        c1 = min(dur, b + 0.02)
    else:
        c0, c1 = 0.0, min(dur, 0.3)
    seg = iq[int(c0*fs):int(c1*fs)]
    png = args.png or (args.path + ".png")
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        from scipy import signal
        f, t, Sxx = signal.spectrogram(seg, fs=fs, nperseg=512, noverlap=384,
                                       return_onesided=False)
        f = np.fft.fftshift(f); Sxx = np.fft.fftshift(Sxx, axes=0)
        plt.figure(figsize=(14, 6))
        plt.pcolormesh((t+c0)*1000, f/1e3, 10*np.log10(Sxx + 1e-9), shading="auto")
        plt.ylabel("kHz from center"); plt.xlabel("ms"); plt.colorbar(label="dB")
        plt.title(f"{args.path}  [{c0*1000:.0f}-{c1*1000:.0f} ms]")
        plt.tight_layout(); plt.savefig(png, dpi=110)
        print(f"spectrogram -> {png}")
    except Exception as e:
        print(f"(no spectrogram: {e})")


if __name__ == "__main__":
    main()
