#!/usr/bin/env python3
"""Decisive test: does the FIRMWARE's staircase chirp synthesis (proc_lora_tx.cpp
execute()) decode as cleanly as the validated SMOOTH chirp (lora_encode.synth_chip)?

Both use the SAME chip sequence. We synthesize the full frame at the firmware's
actual rate (FS=2.5 MHz, OS=5) two ways and demod every symbol with the validated
lora_rx.demod. If smooth gives sharp, correct peaks and staircase does not, the
staircase synthesis in execute() is the on-air bug.
"""
import numpy as np
import lora_encode as enc
from lora_rx import make_chirps, demod

SF, BW, FS = 7, 500e3, 2.5e6     # firmware rate
N, OS, SPS, UP, DOWN = make_chirps(SF, BW, FS)
print(f"N={N} OS={OS} SPS={SPS}")

# ---- chip sequence (same as firmware build_frame data symbols) ----
payload = bytes.fromhex("ffffffff12345678") + bytes(range(0x10, 0x10+22))  # 30 bytes
g_syms = enc.encode_symbols(payload, cr=1, has_crc=1)
data_chips = [(enc.gray_decode(g) + 1) % N for g in g_syms]

# full frame chip plan: 16 up @0, sync, 2 down @0 + 0.25 down, then data
sync = [(0x2 << (SF-4)) % N, (0xB << (SF-4)) % N]

# ---------- SMOOTH synthesis (validated convention, reparametrized to FS) ----------
def synth_smooth(c, up=True, frac=1.0):
    n = int(SPS*frac)
    t = np.arange(n)/FS
    Tsym = SPS/FS
    if up:
        f0 = -BW/2 + c*BW/N
        f = f0 + (BW/Tsym)*t
        f = ((f + BW/2) % BW) - BW/2
    else:
        f = BW/2 - (BW/Tsym)*t
    return np.exp(1j*2*np.pi*np.cumsum(f)/FS)

# ---------- STAIRCASE synthesis (exact replica of execute()) ----------
SINE = np.round(127*np.sin(2*np.pi*np.arange(256)/256)).astype(np.int8)
BW_PHASE_UNIT = (int(BW) << 32) // int(FS)     # 858993459
SPC = int(FS // BW)                            # 5
def itrunc(a, b):  # C int64/int64 truncates toward zero
    q = abs(a)//abs(b); return q if (a<0)==(b<0) else -q
def synth_stair(c, up=True, frac=1.0):
    n = int(SPS*frac)
    out = np.empty(n, dtype=complex)
    phase = 0
    for s in range(n):
        chip = (c + s//SPC) % N
        chip_off = chip - (N>>1)
        if not up: chip_off = -chip_off
        step = itrunc(chip_off*BW_PHASE_UNIT, N)
        phase = (phase + step) & 0xFFFFFFFF
        I = SINE[((phase + 0x40000000) & 0xFFFFFFFF) >> 24]
        Q = SINE[phase >> 24]
        out[s] = (int(I) + 1j*int(Q))/127.0
    return out

# ---------- SMOOTH integer synthesis (candidate fix for execute()) ----------
HALF = BW_PHASE_UNIT >> 1
def synth_stair_smooth(c, up=True, frac=1.0):
    n = int(SPS*frac)
    out = np.empty(n, dtype=complex)
    phase = 0
    step0 = (c*BW_PHASE_UNIT)//N - HALF       # start freq (phase units/sample)
    for s in range(n):
        if up:
            fu = step0 + (s*BW_PHASE_UNIT)//SPS
        else:
            fu = HALF - (s*BW_PHASE_UNIT)//SPS
        # wrap into signed half-range [-HALF, +HALF) so the chirp stays in-band
        fu = ((fu + HALF) % BW_PHASE_UNIT) - HALF
        phase = (phase + (fu & 0xFFFFFFFF)) & 0xFFFFFFFF
        I = SINE[((phase + 0x40000000) & 0xFFFFFFFF) >> 24]
        Q = SINE[phase >> 24]
        out[s] = (int(I) + 1j*int(Q))/127.0
    return out

# ---------- ACCUMULATOR synthesis (exact C++ algorithm to port, M4-friendly) ----------
FRAC = 16
BW_I = BW_PHASE_UNIT                      # int32 range OK (8.6e8)
DFREQ_MAG = (BW_PHASE_UNIT << FRAC) // SPS   # per-sample freq increment <<FRAC
def synth_accum(c, up=True, frac=1.0):
    n = int(SPS*frac)
    out = np.empty(n, dtype=complex)
    phase = 0
    if up:
        step0 = (c*BW_PHASE_UNIT) // N - HALF
        dfreq = DFREQ_MAG
    else:
        step0 = HALF
        dfreq = -DFREQ_MAG
    freq_fp = step0 << FRAC
    for s in range(n):
        freq = freq_fp >> FRAC
        freq = ((freq + HALF) % BW_I + BW_I) % BW_I - HALF   # wrap into [-HALF,HALF)
        phase = (phase + (freq & 0xFFFFFFFF)) & 0xFFFFFFFF
        freq_fp += dfreq
        I = SINE[((phase + 0x40000000) & 0xFFFFFFFF) >> 24]
        Q = SINE[phase >> 24]
        out[s] = (int(I) + 1j*int(Q))/127.0
    return out

def build(synth):
    iq = [synth(0, True) for _ in range(16)]
    iq += [synth(sync[0], True), synth(sync[1], True)]
    iq += [synth(0, False), synth(0, False), synth(0, False, frac=0.25)]
    iq += [synth(c, True) for c in data_chips]
    return np.concatenate(iq)

for name, synth in [("SMOOTH", synth_smooth), ("STAIRCASE", synth_stair),
                    ("INT-SMOOTH-FIX", synth_stair_smooth)]:
    frame = build(synth)
    # data symbols start after 16 preamble + 2 sync + 2.25 SFD = 20.25 symbols
    hstart = int(round(20.25*SPS))
    peaks, sharps, ok = [], [], 0
    for k, expect in enumerate(data_chips):
        st = hstart + k*SPS
        if st+SPS > len(frame): break
        pk, mag, sh = demod(frame[st:st+SPS], DOWN, N, OS)
        peaks.append(pk); sharps.append(sh)
        # expected demod bin for an upchirp at chip c: peak ≈ (N - c) % N  (dechirp sign)
        if pk == expect or pk == (N-expect) % N: ok += 1
    print(f"\n{name}: nsym={len(peaks)} mean_sharp={np.mean(sharps):.1f} "
          f"min_sharp={np.min(sharps):.1f}  peak-matches-chip={ok}/{len(peaks)}")
    print(f"  first 12 recovered peaks: {peaks[:12]}")
    print(f"  first 12 expected chips:  {data_chips[:12]}")
