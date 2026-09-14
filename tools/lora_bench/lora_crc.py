#!/usr/bin/env python3
"""Decisive chain crack via payload CRC-16 (16-bit oracle ~ zero false positives).
Front-end already verified (sync 0x2B). Brute chain on payload, check trailing CRC."""
import sys
import numpy as np
from lora_rx import make_chirps, demod, find_strongest_burst, load_cs8, whiten_seq

CAP = sys.argv[1] if len(sys.argv) > 1 else "captures/cap2.cs8"
FS, BW, SF = 4e6, 500e3, 7
N, OS, SPS, UP, DOWN = make_chirps(SF, BW, FS)
SRC = bytes([0x9C, 0xD1, 0x83, 0x69])

VAR = [lambda i,j,c:(i-j)%c, lambda i,j,c:(i+j)%c, lambda i,j,c:(i-j-1)%c,
       lambda i,j,c:(j-i)%c, lambda i,j,c:(j-i-1)%c, lambda i,j,c:(i-j+1)%c,
       lambda i,j,c:(j-i+1)%c, lambda i,j,c:(i+j-1)%c, lambda i,j,c:(i+j+1)%c]

def gmap(v, gd):
    if gd == 0:
        x=v; m=x>>1
        while m: x^=m; m>>=1
        return x
    return v ^ (v>>1)

def deint(syms, cpb, cr, var):
    f=VAR[var]; cw=[0]*cpb
    for i in range(cpb):
        for j in range(cr):
            cw[i]|=((syms[j]>>f(i,j,cpb))&1)<<(cr-1-j)
    return cw

def crc16(data, init=0x0000):
    crc=init
    for b in data:
        crc ^= b<<8
        for _ in range(8):
            crc = ((crc<<1)^0x1021)&0xFFFF if crc&0x8000 else (crc<<1)&0xFFFF
    return crc

def decode_payload(syms, off, gd, var, hi, norder, wh, cr=5, sf_app=7):
    nib=[]
    for bi in range(0, len(syms)-cr+1, cr):
        blk=[gmap(((s-off)%N)&(N-1), gd) for s in syms[bi:bi+cr]]
        for c in deint(blk, sf_app, cr, var):
            nib.append((c>>(cr-4))&0xF if hi else c&0xF)
    if norder==0:
        rb=bytes((nib[i+1]<<4)|nib[i] for i in range(0,len(nib)-1,2))
    else:
        rb=bytes((nib[i]<<4)|nib[i+1] for i in range(0,len(nib)-1,2))
    if wh:
        w=whiten_seq(len(rb)); rb=bytes(b^w[i] for i,b in enumerate(rb))
    return rb

def main():
    iq=load_cs8(CAP)
    a,b=find_strongest_burst(iq, FS); seg0=iq[a:b]
    tt=np.arange(len(seg0))/FS
    # CFO
    best=None
    for foff in np.arange(-340e3,-150e3,1e3):
        sh=seg0*np.exp(-1j*2*np.pi*foff*tt)
        sc=sum(demod(sh[s:s+SPS],DOWN,N,OS)[2] for s in range(2*SPS,6*SPS,OS*4))
        if best is None or sc>best[0]: best=(sc,foff)
    seg=seg0*np.exp(-1j*2*np.pi*best[1]*tt)
    # grid
    best=None
    for ph in range(0,SPS,SPS//32):
        bins=[]; k=0
        while ph+(k+1)*SPS<=len(seg):
            bins.append(demod(seg[ph+k*SPS:ph+(k+1)*SPS],DOWN,N,OS)); k+=1
        i=0
        while i<len(bins):
            if bins[i][2]>15:
                j=i
                while j<len(bins) and abs(bins[j][0]-bins[i][0])<=1 and bins[j][2]>15: j+=1
                if best is None or (j-i)>best[0]: best=(j-i,ph,i,j)
                i=max(j,i+1)
            else: i+=1
    runlen,ph,i0,i1=best
    hstart=ph+int(round((i1+2+2.25)*SPS))
    syms=[]
    for k in range(0,90):
        st=hstart+k*SPS
        if st+SPS>len(seg): break
        syms.append(demod(seg[st:st+SPS],DOWN,N,OS)[0])
    print(f"grid ph={ph} preamble[{i0}..{i1-1}] hstart={hstart} nsyms={len(syms)}")

    sig=[]; crch=[]
    for hskip in (7,8,9):
        pl=syms[hskip:]
        for off in range(N):
            for gd in (0,1):
                for var in range(9):
                    for hi in (0,1):
                        for norder in (0,1):
                            for wh in (0,1):
                                rb=decode_payload(pl,off,gd,var,hi,norder,wh)
                                if rb[:4]==b'\xff\xff\xff\xff' or SRC in rb[:18]:
                                    sc=(10 if rb[:4]==b'\xff\xff\xff\xff' else 0)+(20 if SRC in rb[:18] else 0)
                                    sig.append((sc,hskip,off,gd,var,hi,norder,wh,rb[:24].hex(' ')))
                                for init in (0x0000,0xFFFF):
                                    for L in range(6,len(rb)-2):
                                        if crc16(rb[:L],init)==(rb[L]<<8|rb[L+1]):
                                            crch.append((hskip,off,gd,var,hi,norder,wh,init,L,rb[:min(L+2,28)].hex(' ')))
    sig.sort(reverse=True)
    print(f"\n*** {len(sig)} SIGNATURE hit(s) (FFFFFFFF dest / Heltec src): ***")
    for h in sig[:15]:
        print("  score=%d hskip=%d off=%d gd=%d var=%d hi=%d norder=%d wh=%d | %s"%h)
    print(f"\n{len(crch)} CRC-only hit(s) (likely noise unless aligned with a signature hit)")
    if not sig:
        print("  no signature — capture/chain still off")

if __name__=="__main__":
    main()
