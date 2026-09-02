#!/usr/bin/env python3
"""Find LONG_FAST bursts in the 2 MHz cs8 capture, mix +675 kHz -> DC,
decimate, and dump per-burst iq16 slices at OS=2 (500 kHz) and OS=4 (1 MHz)."""
import sys, numpy as np
from scipy.signal import resample_poly

CAP = sys.argv[1] if len(sys.argv) > 1 else "lf_cap.cs8"
FS = 2_000_000
OFFSET = 675_000            # signal is at +675 kHz relative to the 868.4 tune
SYM_T = 2048 / 250_000.0    # SF11/BW250 symbol time = 8.192 ms

raw = np.fromfile(CAP, dtype=np.int8).astype(np.float32)
iq = raw[0::2] + 1j * raw[1::2]
n = len(iq)
print(f"loaded {n} samples = {n/FS:.2f} s")

# mix +675 kHz -> DC
t = np.arange(n, dtype=np.float64)
iq_dc = iq * np.exp(-1j * 2 * np.pi * OFFSET / FS * t).astype(np.complex64)

# coarse energy envelope (decimate-by-40 power for burst finding)
D = 40
pw = np.abs(iq_dc[: (n // D) * D].reshape(-1, D)) ** 2
env = pw.mean(axis=1)                     # power at FS/40 = 50 kHz
env_fs = FS / D
noise = np.median(env)
thr = noise * 6.0
active = env > thr

# find contiguous runs
runs = []
i = 0
while i < len(active):
    if active[i]:
        j = i
        while j < len(active) and active[j]:
            j += 1
        # allow small gaps <2ms merged: (handled loosely) keep run
        runs.append((i, j))
        i = j
    else:
        i += 1

# merge runs separated by < 3 ms
merged = []
for r in runs:
    if merged and (r[0] - merged[-1][1]) < 3e-3 * env_fs:
        merged[-1] = (merged[-1][0], r[1])
    else:
        merged.append(list(r))
merged = [(a, b) for a, b in merged if (b - a) / env_fs > 0.05]   # >50 ms

print(f"noise={noise:.1f} thr={thr:.1f}; {len(merged)} bursts:")
bursts = []
for k, (a, b) in enumerate(merged):
    s0 = a * D
    s1 = b * D
    dur = (s1 - s0) / FS
    snr = env[a:b].mean() / noise
    print(f"  burst {k}: start={s0} ({s0/FS:.3f}s) dur={dur*1000:.0f}ms nsym~{dur/SYM_T:.1f} snr={snr:.1f}x")
    bursts.append((s0, s1))

if len(sys.argv) > 2 and bursts:
    # dump requested burst index (with guard margins) at OS=2 and OS=4
    bi = int(sys.argv[2])
    s0, s1 = bursts[bi]
    pad = int(0.015 * FS)      # 15 ms pad each side (preamble headroom)
    a = max(0, s0 - pad); b = min(n, s1 + pad)
    seg = iq_dc[a:b]
    for dec, tag, sps in [(4, "os2", 4096), (2, "os4", 8192)]:
        d = resample_poly(seg, 1, dec).astype(np.complex64)
        out = np.empty(2 * len(d), dtype=np.int16)
        # scale to a healthy int16 level
        mx = max(1.0, np.max(np.abs(d)))
        g = 6000.0 / mx
        out[0::2] = np.clip(np.round(d.real * g), -32767, 32767)
        out[1::2] = np.clip(np.round(d.imag * g), -32767, 32767)
        fn = f"lf_b{bi}_{tag}.iq16"
        out.tofile(fn)
        print(f"  wrote {fn}: {len(d)} samples @ {FS/dec/1e3:.0f}kHz (sps={sps}, {len(d)/sps:.1f} sym)")
