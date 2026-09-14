#!/usr/bin/env python3
"""Streaming SF11 with PARALLEL-DA fine-align that FITS the M4 (buffer demodded
BINS, not samples).  Per symbol: demod at all N_da da-offsets (N_da FFTs, fits the
8ms SF11 budget) + store bins in a tiny N_da x Nsym array.  After the header+dest
region, decode every (da,off) candidate from the buffered bins and pick the one
with a valid-checksum broadcast frame.  Then decode the rest at that (da,off)."""
import sys, numpy as np
SF, BW = 11, 250_000
FN = sys.argv[1] if len(sys.argv) > 1 else "lf_b0_os1.iq16"
FS = float(sys.argv[2]) if len(sys.argv) > 2 else 250_000.0
N = 1 << SF; OS = int(round(FS/BW)); SPS = N*OS
raw = np.fromfile(FN, dtype=np.int16).astype(np.float32); sig = raw[0::2]+1j*raw[1::2]
nn = np.arange(SPS); BASE = np.pi/OS*(nn*nn/SPS-nn)
DA_RANGE = range(-6, 7)          # 13 da candidates
OFF_RANGE = (-1, 0, 1)           # 3 off candidates
FINE_VERIFY = 14                 # symbols (header 8 + ~6 payload) buffered as bins
def fold(x, dn=False, cr=0.0):
    ph = BASE + cr*nn; d = x*(np.exp(1j*ph) if dn else np.exp(-1j*ph)); return np.fft.fft(d).reshape(OS,N).sum(0)
def pk(F): m=np.abs(F); b=int(np.argmax(m)); return b, m[b]/(np.median(m)+1e-9), F[b]
WSEQ=[]; w=0xFF
for _ in range(256): WSEQ.append(w); fb=((w>>7)^(w>>5)^(w>>4)^(w>>3))&1; w=((w<<1)|fb)&0xFF
def i2b(v,n): return [(v>>(n-1-i))&1 for i in range(n)]
def b2i(bb):
    x=0
    for k in bb: x=(x<<1)|k
    return x
def deint(sy,sfa,cwl):
    it=[i2b(s,sfa) for s in sy]; de=[[0]*cwl for _ in range(sfa)]
    for i in range(cwl):
        for j in range(sfa): de[(i-j-1)%sfa][i]=it[i][j]
    return [b2i(de[c]) for c in range(sfa)]
def hamn(cw,cwl): bb=i2b(cw,cwl); return b2i([bb[3],bb[2],bb[1],bb[0]])
def hdr_ok(h):
    h0,h1,h2=h[0],h[1],h[2]
    c4=((h0>>3)&1)^((h0>>2)&1)^((h0>>1)&1)^(h0&1)
    c3=((h0>>3)&1)^((h1>>3)&1)^((h1>>2)&1)^((h1>>1)&1)^(h2&1)
    c2=((h0>>2)&1)^((h1>>3)&1)^(h1&1)^((h2>>3)&1)^((h2>>1)&1)
    c1=((h0>>1)&1)^((h1>>2)&1)^(h1&1)^((h2>>2)&1)^((h2>>1)&1)^(h2&1)
    c0=(h0&1)^((h1>>1)&1)^((h2>>3)&1)^((h2>>2)&1)^((h2>>1)&1)^(h2&1)
    return (h[3]&1)==c4 and (h[4]&0xF)==((c3<<3)|(c2<<2)|(c1<<1)|c0)
def wrapN(x): return ((x+N//2)%N)-N//2
def decode_from_bins(bins, off):
    tc=(off-1)%N
    g=[((s-tc-1)%N)>>2 for s in bins[:8]]
    nib=[hamn(c,8) for c in deint([x^(x>>1) for x in g],SF-2,8)]
    if not hdr_ok(nib): return None
    L=(nib[0]<<4)|nib[1]; cr=(nib[2]>>1)&7; extra=nib[5:SF-2]
    if not (1<=cr<=4 and 8<=L<=64): return None
    psy=[(s-tc-1)%N for s in bins[8:]]; cwl=cr+4; pn=list(extra)
    for bi in range(0,len(psy)-cwl+1,cwl):
        for c in deint([x^(x>>1) for x in psy[bi:bi+cwl]],SF,cwl): pn.append(hamn(c,cwl))
    rb=bytes((pn[i+1]<<4)|pn[i] for i in range(0,len(pn)-1,2))
    return L, bytes(b^WSEQ[i] for i,b in enumerate(rb))

# --- online preamble/SFD (as before) ---
r=0; state="HUNT"; pre_run=0; prev=-99; up_bin=0; pre_peaks=[]; confirmed=False
cfo_ramp=0.0; off0=0; h0=0; da_list=list(DA_RANGE)
binbuf={da:[] for da in da_list}; nsym=0; result=None
while r+SPS<=len(sig) and result is None:
    if state=="HUNT":
        Fu=fold(sig[r:r+SPS]); ub,us,upk=pk(Fu)
        if not confirmed:
            Fd=fold(sig[r:r+SPS],dn=True); db,ds,_=pk(Fd)
            if us>6 and abs(ub-prev)<=1: pre_run+=1; pre_peaks.append(upk); up_bin=ub; pre_pos=r
            elif us>6: pre_run=1; pre_peaks=[upk]; up_bin=ub; pre_pos=r
            else: pre_run=0; pre_peaks=[]
            prev=ub
            if pre_run>=6: confirmed=True
            r+=SPS
        else:
            Fd=fold(sig[r:r+SPS],dn=True); db,ds,_=pk(Fd)
            if ds>6 and ds>us:
                tau=wrapN(up_bin-db)//2
                acc=np.sum([pre_peaks[i+1]*np.conj(pre_peaks[i]) for i in range(len(pre_peaks)-1)])
                cfo_ramp=np.angle(acc)/SPS
                off0=pk(fold(sig[pre_pos-tau*OS:pre_pos-tau*OS+SPS],cr=cfo_ramp))[0]
                h0=r+int(round(2.25*SPS))-tau*OS
                state="FINE"; nsym=0
            else: r+=SPS
    elif state=="FINE":
        # demod symbol nsym at every da; store bins
        for da in da_list:
            st=h0+da+nsym*SPS
            binbuf[da].append(pk(fold(sig[st:st+SPS],cr=cfo_ramp))[0] if st+SPS<=len(sig) else 0)
        nsym+=1
        if nsym>=FINE_VERIFY:
            # decode every (da,off), pick a valid-checksum broadcast frame
            win=None
            for da in sorted(da_list,key=abs):
                for od in OFF_RANGE:
                    res=decode_from_bins(binbuf[da], (off0+od)%N)
                    if res and res[1][:4]==b'\xff\xff\xff\xff':
                        win=(da, (off0+od)%N, res); break
                if win: break
            if not win:
                result=("no broadcast winner",); break
            da,off,(L,dew)=win
            # decode the full frame from this da's bins (continue streaming rest at da)
            need=((( (L+2)*2 - (SF-7) )+SF-1)//SF)*(1+4)   # payload syms after header
            r=h0+da+len(binbuf[da])*SPS
            allbins=list(binbuf[da])
            while len(allbins)-8 < need and r+SPS<=len(sig):
                allbins.append(pk(fold(sig[r:r+SPS],cr=cfo_ramp))[0]); r+=SPS
            fr=decode_from_bins(allbins, off)
            result=(da,off,fr[0],fr[1][:fr[0]])
            break
if result and len(result)>1:
    da,off,L,fr=result
    print(f"=== {FN}: da={da:+d} off={off} L={L} ===")
    print(f"frame: {fr.hex(' ')}")
    print(f"dest={fr[3]:02x}{fr[2]:02x}{fr[1]:02x}{fr[0]:02x} src={fr[7]:02x}{fr[6]:02x}{fr[5]:02x}{fr[4]:02x} chanHash={fr[13]:02x}")
else:
    print(f"{FN}: {result}")
