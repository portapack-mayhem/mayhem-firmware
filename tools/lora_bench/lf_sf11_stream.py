#!/usr/bin/env python3
"""STREAMING SF11 decoder — online preamble/SFD detection, one symbol at a time,
no global search (mirrors what the M4 firmware must do).  Validates the streaming
state machine + SFD sync before porting to proc_lora.cpp.  Reads OS=1 iq16 (250kHz)."""
import sys, struct, numpy as np
SF, BW = 11, 250_000
FN = sys.argv[1] if len(sys.argv) > 1 else "lf_b0_os1.iq16"
FS = float(sys.argv[2]) if len(sys.argv) > 2 else 250_000.0
N = 1 << SF; OS = int(round(FS/BW)); SPS = N*OS
raw = np.fromfile(FN, dtype=np.int16).astype(np.float32)
sig = raw[0::2] + 1j*raw[1::2]
n_arr = np.arange(SPS)
# precomputed dechirp phase (per firmware): ph = pi/os*(n^2/sps - n)
BASE_PH = np.pi/OS*(n_arr*n_arr/SPS - n_arr)
def up_fold(x, cfo_ramp=0.0):   # dechirp up (conj-up) + fold -> complex N bins
    ph = BASE_PH + cfo_ramp*n_arr
    d = x*np.exp(-1j*ph)
    return np.fft.fft(d).reshape(OS, N).sum(0)
def up_sharp(x, cfo_ramp=0.0):  # peak^2/total of the folded up-dechirp
    F=up_fold(x,cfo_ramp); m=np.abs(F); return (m.max()**2)/((m*m).sum()+1e-9)
def down_fold(x):               # dechirp down (up-ref) + fold
    d = x*np.exp(1j*BASE_PH)
    return np.fft.fft(d).reshape(OS, N).sum(0)
def pk(F):
    m=np.abs(F); b=int(np.argmax(m)); return b, m[b]/(np.median(m)+1e-9), F[int(np.argmax(m))]

WSEQ=[]; _w=0xFF
for _ in range(256):
    WSEQ.append(_w); _fb=((_w>>7)^(_w>>5)^(_w>>4)^(_w>>3))&1; _w=((_w<<1)|_fb)&0xFF
def i2b(v,n): return [(v>>(n-1-i))&1 for i in range(n)]
def b2i(b):
    x=0
    for k in b: x=(x<<1)|k
    return x
def deint(sy,sfa,cwl):
    it=[i2b(s,sfa) for s in sy]; de=[[0]*cwl for _ in range(sfa)]
    for i in range(cwl):
        for j in range(sfa): de[(i-j-1)%sfa][i]=it[i][j]
    return [b2i(de[c]) for c in range(sfa)]
def hamn(cw,cwl):
    b=i2b(cw,cwl); return b2i([b[3],b[2],b[1],b[0]])
def hdr_ok(h):
    h0,h1,h2=h[0],h[1],h[2]
    c4=((h0>>3)&1)^((h0>>2)&1)^((h0>>1)&1)^(h0&1)
    c3=((h0>>3)&1)^((h1>>3)&1)^((h1>>2)&1)^((h1>>1)&1)^(h2&1)
    c2=((h0>>2)&1)^((h1>>3)&1)^(h1&1)^((h2>>3)&1)^((h2>>1)&1)
    c1=((h0>>1)&1)^((h1>>2)&1)^(h1&1)^((h2>>2)&1)^((h2>>1)&1)^(h2&1)
    c0=(h0&1)^((h1>>1)&1)^((h2>>3)&1)^((h2>>2)&1)^((h2>>1)&1)^(h2&1)
    return (h[3]&1)==c4 and (h[4]&0xF)==((c3<<3)|(c2<<2)|(c1<<1)|c0)

# ---------- streaming state machine ----------
THR_SHARP = 6.0        # up/down sharpness gate
PRE_MIN   = 6          # preamble upchirps before accepting SFD
state = "HUNT"         # HUNT -> HEADER -> PAYLOAD
pre_run = 0; prev_bin = -99; up_bin = 0
pre_peaks = []         # complex peaks for phase-CFO
preamble_confirmed = False
r = 0                  # read index (sample)
cfo_ramp = 0.0; timing_corr = 0
hdr_syms = []; hdr_extra = []; L=0; cr=1
pay_syms = []
result = None

def wrapN(x): return ((x+N//2)%N)-N//2

while r + SPS <= len(sig) and result is None:
    x = sig[r:r+SPS]
    Fu = up_fold(x, cfo_ramp); ub, ush, upk = pk(Fu)
    if state == "HUNT":
        Fd = down_fold(x); db, dsh, _ = pk(Fd)
        if not preamble_confirmed:
            if ush > THR_SHARP and abs(ub-prev_bin) <= 1:
                pre_run += 1; pre_peaks.append(upk); up_bin = ub; pre_pos = r
                if pre_run >= PRE_MIN: preamble_confirmed = True
            elif ush > THR_SHARP:
                pre_run = 1; pre_peaks=[upk]; up_bin = ub; pre_pos = r
            else:
                pre_run = 0; pre_peaks=[]
            prev_bin = ub
            r += SPS
        else:
            # look for the SFD (strong down-chirp) after the preamble/sync
            if dsh > THR_SHARP and dsh > ush:
                down_bin = db
                # SFD found at symbol starting at r. Separate CFO/timing.
                tau = wrapN(up_bin - down_bin)//2       # bins; snap = tau*OS samples
                cfo_bin = (up_bin - tau) % N
                timing_corr = (cfo_bin - 1) % N          # a = (raw - timing_corr - 1) = raw - cfo_bin
                # fractional CFO from preamble phase slope -> cfo_ramp (rad/sample)
                acc = np.sum([pre_peaks[i+1]*np.conj(pre_peaks[i]) for i in range(len(pre_peaks)-1)])
                dphi = np.angle(acc)          # per-symbol phase advance
                cfo_ramp = dphi/ SPS          # rad/sample
                # header starts at SFD_start + 2.25 sym - tau
                h0 = r + int(round(2.25*SPS)) - tau*OS
                # sub-sample fine-align on the PREAMBLE (constant-value chirp → sharpness
                # peaks at the correct sample offset; random payload symbols do NOT).
                # Sum over the last NW preamble windows (all share the sub-sample offset)
                # to average out per-window noise — a single window is unreliable.  The
                # same clock offset applies to the header (0.25-SFD shift = integer sample).
                NW = 8
                best_da, best_sc = 0, -1.0
                for da in range(-2*OS, 2*OS+1):
                    sc = 0.0
                    for w in range(NW):
                        p = pre_pos - w*SPS + da
                        if p < 0 or p+SPS > len(sig): continue
                        sc += up_sharp(sig[p:p+SPS], cfo_ramp)
                    if sc > best_sc: best_sc, best_da = sc, da
                r = h0 + best_da
                print(f"  [SFD] up_bin={up_bin} down_bin={down_bin} tau={tau} cfo_bin={cfo_bin} timing_corr={timing_corr} da={best_da} hdr@{r}")
                hdr_syms = []; hdr_extra=[]; state="HEADER"
            else:
                r += SPS
    elif state == "HEADER":
        a = ((ub - timing_corr - 1) % N) >> 2
        hdr_syms.append(a ^ (a>>1))
        r += SPS
        if len(hdr_syms) == 8:
            cws = deint(hdr_syms, SF-2, 8); nib=[hamn(c,8) for c in cws]
            if hdr_ok(nib):
                L=(nib[0]<<4)|nib[1]; cr=(nib[2]>>1)&7
                hdr_extra = nib[5:SF-2]
                pay_syms=[]; state="PAYLOAD"
            else:
                state="HUNT"; preamble_confirmed=False; pre_run=0; prev_bin=-99; cfo_ramp=0.0
    elif state == "PAYLOAD":
        a = (ub - timing_corr - 1) % N
        pay_syms.append(a ^ (a>>1))
        r += SPS
        cwl = cr+4
        # payload nibbles still needed after the hdr_extra ones, in whole blocks
        need_nib  = (L+2)*2 - len(hdr_extra)
        need_syms = ((need_nib + SF - 1)//SF) * cwl
        if len(pay_syms) >= need_syms:
            nib = list(hdr_extra)
            for bi in range(0, len(pay_syms)-cwl+1, cwl):
                for c in deint(pay_syms[bi:bi+cwl], SF, cwl): nib.append(hamn(c,cwl))
            rb=bytes((nib[i+1]<<4)|nib[i] for i in range(0,len(nib)-1,2))
            dew=bytes(b^WSEQ[i] for i,b in enumerate(rb))[:L]
            result = dew
            break

if result:
    fr=result
    print(f"=== STREAMING DECODE ({FN}) ===")
    print(f"L={L} cr={cr} frame: {fr.hex(' ')}")
    if len(fr)>=14:
        print(f"dest={fr[3]:02x}{fr[2]:02x}{fr[1]:02x}{fr[0]:02x} src={fr[7]:02x}{fr[6]:02x}{fr[5]:02x}{fr[4]:02x} chanHash={fr[13]:02x}")
else:
    print(f"no decode (state={state}, pre_run={pre_run}, confirmed={preamble_confirmed})")
