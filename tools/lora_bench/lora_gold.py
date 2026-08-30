#!/usr/bin/env python3
"""Faithful port of the gr-lora_sdr decode chain (the authoritative reference).
Front-end already cracked (sync 0x2B). Only the exact bit-level chain was missing.

Chain (per gr-lora_sdr fft_demod/gray_mapping/deinterleaver/hamming_dec/dewhitening):
  sym = ((idx-1) mod N) / (4 if header/ldro else 1)
  gray = sym ^ (sym>>1)
  int2bool MSB-first; deinter[(i-j-1)%sf_app][i] = bits[i][j]
  hamming nibble = reversed top bits {cw[3],cw[2],cw[1],cw[0]}
  byte = (high<<4)|low (low nibble first); dewhiten ^= whitening_seq  (CRC not whitened)
"""
import sys
import numpy as np
from lora_rx import make_chirps, demod, find_strongest_burst, load_cs8

CAP = sys.argv[1] if len(sys.argv) > 1 else "captures/cap2.cs8"
FS, BW, SF = 4e6, 500e3, 7
N, OS, SPS, UP, DOWN = make_chirps(SF, BW, FS)
SRC = bytes([0x9C, 0xD1, 0x83, 0x69])

# Full whitening sequence, generated from the same LFSR the firmware uses
# (x^8+x^6+x^5+x^4+1, init 0xFF). The old hardcoded table stopped at 64 bytes
# and zero-padded the rest, silently corrupting dewhitening past byte 64.
WSEQ = []
_ws = 0xFF
for _ in range(256):
    WSEQ.append(_ws)
    _fb = ((_ws >> 7) ^ (_ws >> 5) ^ (_ws >> 4) ^ (_ws >> 3)) & 1
    _ws = ((_ws << 1) | _fb) & 0xFF


def int2bool(v, n):           # MSB first: [bit(n-1)...bit0]
    return [(v >> (n-1-i)) & 1 for i in range(n)]

def bool2int(b):
    x = 0
    for bit in b:
        x = (x << 1) | bit
    return x

def sym_proc(idx, off, is_header):
    a = (idx - off - 1) % N         # gr-lora: (get_symbol_val - 1) mod N  (offset folds frame_sync)
    if is_header:
        a //= 4
    return a ^ (a >> 1)             # gray demap

def deinterleave(syms, sf_app, cw_len):
    inter = [int2bool(s, sf_app) for s in syms]      # cw_len entries x sf_app bits
    deinter = [[0]*cw_len for _ in range(sf_app)]
    for i in range(cw_len):
        for j in range(sf_app):
            deinter[(i - j - 1) % sf_app][i] = inter[i][j]
    return [bool2int(deinter[c]) for c in range(sf_app)]   # sf_app codewords of cw_len bits

def ham_nibble(cw, cw_len):
    b = int2bool(cw, cw_len)
    return bool2int([b[3], b[2], b[1], b[0]])

def decode_header(syms, off):     # 8 syms, sf_app=5, cw_len=8 -> 5 nibbles
    g = [sym_proc(s, off, True) for s in syms[:8]]
    cws = deinterleave(g, 5, 8)
    return [ham_nibble(c, 8) for c in cws]

def decode_payload(syms, off, cr=1):   # blocks of cw_len=cr+4 -> sf_app=SF nibbles
    cw_len = cr + 4
    nib = []
    for bi in range(0, len(syms) - cw_len + 1, cw_len):
        g = [sym_proc(s, off, False) for s in syms[bi:bi+cw_len]]
        for c in deinterleave(g, SF, cw_len):
            nib.append(ham_nibble(c, cw_len))
    rb = bytes((nib[i+1] << 4) | nib[i] for i in range(0, len(nib)-1, 2))  # low nibble first
    dew = bytes(b ^ WSEQ[i] for i, b in enumerate(rb))
    return rb, dew

def main():
    iq = load_cs8(CAP); a,b = find_strongest_burst(iq, FS); seg0 = iq[a:b]
    tt = np.arange(len(seg0))/FS
    best=None
    for foff in np.arange(-340e3,-150e3,1e3):
        sh=seg0*np.exp(-1j*2*np.pi*foff*tt)
        sc=sum(demod(sh[s:s+SPS],DOWN,N,OS)[2] for s in range(2*SPS,6*SPS,OS*4))
        if best is None or sc>best[0]: best=(sc,foff)
    seg=seg0*np.exp(-1j*2*np.pi*best[1]*tt)
    best=None
    for ph in range(0,SPS,SPS//32):
        bins=[];k=0
        while ph+(k+1)*SPS<=len(seg):
            bins.append(demod(seg[ph+k*SPS:ph+(k+1)*SPS],DOWN,N,OS));k+=1
        i=0
        while i<len(bins):
            if bins[i][2]>15:
                j=i
                while j<len(bins) and abs(bins[j][0]-bins[i][0])<=1 and bins[j][2]>15: j+=1
                if best is None or (j-i)>best[0]: best=(j-i,ph,i,j)
                i=max(j,i+1)
            else: i+=1
    _,ph,i0,i1=best
    hstart0=ph+int(round((i1+2+2.25)*SPS))
    # fine sample-align the payload grid: maximize symbol sharpness
    bestal=None
    for da in range(-SPS, SPS+1, 4):
        hs=hstart0+da
        if hs<0 or hs+20*SPS>len(seg): continue
        sc=sum(demod(seg[hs+k*SPS:hs+(k+1)*SPS],DOWN,N,OS)[2] for k in range(0,18))
        if bestal is None or sc>bestal[0]: bestal=(sc,da)
    hstart=hstart0+bestal[1]
    syms=[]; sharps=[]
    for k in range(0,90):
        st=hstart+k*SPS
        if st+SPS>len(seg): break
        s,m,sh=demod(seg[st:st+SPS],DOWN,N,OS); syms.append(s); sharps.append(sh)
    print(f"grid ph={ph} preamble[{i0}..{i1-1}] hstart={hstart} (da={bestal[1]}) "
          f"nsyms={len(syms)} mean_sharp={np.mean(sharps):.1f}")

    # diagnostic: any offset giving a structurally-valid header?
    print("valid-looking headers (cr 1..4, 8<=len<=64):")
    for off in range(N):
        hdr=decode_header(syms, off)
        L=(hdr[0]<<4)|hdr[1]; cr=(hdr[2]>>1)&7; crc=hdr[2]&1
        if 1<=cr<=4 and 8<=L<=64:
            print(f"  off={off}: len={L} cr={cr} crc={crc} nib={hdr}")

    hits=[]
    for off in range(N):
        hdr=decode_header(syms, off)
        L=(hdr[0]<<4)|hdr[1]; cr=(hdr[2]>>1)&7; crc=hdr[2]&1
        rb,dew=decode_payload(syms[8:], off, cr if 1<=cr<=4 else 1)
        sc=(10 if dew[:4]==b'\xff\xff\xff\xff' else 0)+(20 if SRC in dew[:18] else 0)
        if sc>0:
            hits.append((sc,off,L,cr,crc,dew[:32].hex(' ')))
    hits.sort(reverse=True)
    if hits:
        print(f"\n*** {len(hits)} HIT(S) ***")
        for sc,off,L,cr,crc,h in hits[:5]:
            print(f"  score={sc} off={off} hdr_len={L} cr={cr} crc={crc}\n   payload: {h}")
        # full decode of the best
        sc,off,L,cr,crc,_=hits[0]
        hdr=decode_header(syms,off); rb,dew=decode_payload(syms[8:],off, cr if 1<=cr<=4 else 1)
        print(f"\nBEST off={off} header_nibbles={hdr} len={L} cr={cr} crc={crc}")
        print(f"full de-whitened ({len(dew)}B): {dew.hex(' ')}")
    else:
        print("\nno hit — printing best-effort header+payload at a few offsets:")
        for off in range(0,6):
            hdr=decode_header(syms,off); rb,dew=decode_payload(syms[8:],off)
            print(f"  off={off} hdr={hdr} dew[:16]={dew[:16].hex(' ')}")

if __name__=="__main__":
    main()
