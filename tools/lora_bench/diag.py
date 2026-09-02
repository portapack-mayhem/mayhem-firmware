#!/usr/bin/env python3
"""Key discriminator: with CFO removed + grid locked, does the sync word read [16,88]?
Also calibrate sharpness scale (preamble vs data)."""
import numpy as np
from lora_rx import make_chirps, demod, find_strongest_burst, load_cs8

FS, BW, SF = 4e6, 500e3, 7
iq = load_cs8("captures/cap2.cs8")
a, b = find_strongest_burst(iq, FS)
seg0 = iq[a:b]
N, os, sps, up, down = make_chirps(SF, BW, FS)
tt = np.arange(len(seg0))/FS

best = None
for foff in np.arange(-340e3, -150e3, 1e3):
    sh = seg0*np.exp(-1j*2*np.pi*foff*tt)
    sc = sum(demod(sh[s:s+sps], down, N, os)[2] for s in range(2*sps, 6*sps, os*4))
    if best is None or sc > best[0]:
        best = (sc, foff)
foff = best[1]
seg = seg0*np.exp(-1j*2*np.pi*foff*tt)
print(f"CFO {foff/1e3:.1f} kHz")

# grid via phase search
best = None
for phase in range(0, sps, sps//32):
    bins = []
    k = 0
    while phase+(k+1)*sps <= len(seg):
        bins.append(demod(seg[phase+k*sps:phase+(k+1)*sps], down, N, os)); k += 1
    i = 0
    while i < len(bins):
        if bins[i][2] > 15:
            j = i
            while j < len(bins) and abs(bins[j][0]-bins[i][0]) <= 1 and bins[j][2] > 15:
                j += 1
            if best is None or (j-i) > best[0]:
                best = (j-i, phase, i, j, int(np.median([bins[x][0] for x in range(i, j)])))
            i = max(j, i+1)
        else:
            i += 1
runlen, phase, i0, i1, up_bin = best
print(f"grid phase={phase} preamble[{i0}..{i1-1}] up_bin={up_bin}")

def sym_at(symidx):  # symidx relative to grid phase
    st = phase + symidx*sps
    s, m, shp = demod(seg[st:st+sps], down, N, os)
    su, mu, shpu = demod(seg[st:st+sps], up, N, os)
    return s, shp, shpu

print("\npreamble sharpness (calibration):")
for k in range(i0, i0+4):
    s, shp, shpu = sym_at(k); print(f"  sym{k}: bin={s} sharp_down={shp:.1f}")

print("\nstructure after preamble (sym idx, bin-up_bin, down-sharp, up-sharp):")
for k in range(i1-1, i1+14):
    s, shp, shpu = sym_at(k)
    tag = " <-SFD(down)" if shpu > shp*1.3 else ""
    print(f"  sym{k}: val={(s-up_bin)%N:3d}  d={shp:5.1f} u={shpu:5.1f}{tag}")

# sync = two symbols right after preamble (idx i1, i1+1)
sy = [(sym_at(i1)[0]-up_bin) % N, (sym_at(i1+1)[0]-up_bin) % N]
print(f"\nSYNC (expect 0x2B -> [16,88]): {sy}")

# ---- decode header at quarter-symbol-shifted payload grid ----
def gray_dec(v):
    x = v; m = x >> 1
    while m: x ^= m; m >>= 1
    return x

VAR = [lambda i,j,c:(i-j)%c, lambda i,j,c:(i+j)%c, lambda i,j,c:(i-j-1)%c,
       lambda i,j,c:(j-i)%c, lambda i,j,c:(j-i-1)%c, lambda i,j,c:(i-j+1)%c,
       lambda i,j,c:(j-i+1)%c, lambda i,j,c:(i+j-1)%c, lambda i,j,c:(i+j+1)%c]

def deint(syms, cpb, cr, var):
    f = VAR[var]; cw=[0]*cpb
    for i in range(cpb):
        for j in range(cr):
            cw[i] |= ((syms[j]>>f(i,j,cpb))&1)<<(cr-1-j)
    return cw

# payload/header start sample (with the 0.25 SFD shift)
hstart = phase + int(round((i1 + 2 + 2.25)*sps))
hdr_raw = []
for k in range(8):
    st = hstart + k*sps
    s,_,_ = demod(seg[st:st+sps], down, N, os)
    hdr_raw.append(s)
def hdr_checksum(n):
    n0,n1,n2 = n[0],n[1],n[2]
    c4 = ((n0>>3)&1)^((n0>>2)&1)^((n0>>1)&1)^(n0&1)
    c3 = ((n0>>3)&1)^((n1>>3)&1)^((n1>>2)&1)^((n1>>1)&1)^(n2&1)
    c2 = ((n0>>2)&1)^((n1>>3)&1)^(n1&1)^((n2>>3)&1)^((n2>>1)&1)
    c1 = ((n0>>1)&1)^((n1>>2)&1)^(n1&1)^((n2>>3)&1)^((n2>>2)&1)^((n2>>1)&1)^(n2&1)
    c0 = (n0&1)^((n1>>1)&1)^((n2>>3)&1)^((n2>>2)&1)^((n2>>1)&1)^(n2&1)
    return c4, (c3<<3)|(c2<<2)|(c1<<1)|c0

print(f"\nheader raw bins: {hdr_raw}")
print("header decodes with VALID header-CRC (off,gd,var):")
found = []
for off in range(N):
    for gd in (0,1):
        for hi in (0,1):
            for var in range(9):
                nib = []
                for s in hdr_raw:
                    v = (s-off) % N
                    g = (v>>2) & 0x1F
                    g = gray_dec(g) if gd==0 else (g ^ (g>>1))
                    nib.append(g)
                cw = deint(nib, 5, 8, var)
                n = [((c>>4)&0xF if hi else c&0xF) for c in cw]
                c4, clo = hdr_checksum(n)
                if (n[3]&1)==c4 and n[4]==clo:
                    L=(n[0]<<4)|n[1]; cr=(n[2]>>1)&7; crc=n[2]&1
                    print(f"  off={off} gd={gd} hi={hi} var={var}: len={L} cr={cr} crc={crc} nib={n}")
                    found.append((off,gd,hi,var,L,cr,crc))
print(f"\n{len(found)} checksum-valid header decode(s)")
