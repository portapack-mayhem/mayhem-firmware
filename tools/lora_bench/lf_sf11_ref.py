# Pure SFD-based sync (no brute search): measure up_bin (preamble) & down_bin (SFD),
# separate CFO/timing, snap, decode. This is the exact recipe to port to C++.
import numpy as np, sys, struct
exec(open("lf_fine.py").read().split("phs=[peakc")[0])
FN=sys.argv[1]; FS=float(sys.argv[2])
raw=np.fromfile(FN,dtype=np.int16).astype(np.float32); base=raw[0::2]+1j*raw[1::2]
def up_demod(b):
    Y=np.abs(np.fft.fft(b*DOWN).reshape(OS,N).sum(0)); return int(np.argmax(Y)), Y.max()/(np.median(Y)+1e-9)
def down_demod(b):
    Y=np.abs(np.fft.fft(b*UP).reshape(OS,N).sum(0)); return int(np.argmax(Y)), Y.max()/(np.median(Y)+1e-9)
def peakc(b):
    F=np.fft.fft(b*DOWN).reshape(OS,N).sum(0); return F[int(np.argmax(np.abs(F)))]

# 1) fractional CFO from preamble phase slope, remove it
phs=[peakc(base[k*SPS:(k+1)*SPS]) for k in range(3,16)]
f_frac=np.angle(np.sum([phs[i+1]*np.conj(phs[i]) for i in range(len(phs)-1)]))/(2*np.pi*Tsym)
tt=np.arange(len(base))/FS
seg=base*np.exp(-1j*2*np.pi*f_frac*tt)

# 2) find preamble run + up_bin (ph=0 grid), and locate SFD (down-ref strong)
bins=[up_demod(seg[k*SPS:(k+1)*SPS]) for k in range((len(seg)//SPS))]
# preamble = longest run of equal up-bins with good sharpness
i=0; run=None
while i<len(bins):
    if bins[i][1]>8:
        j=i
        while j<len(bins) and abs(bins[j][0]-bins[i][0])<=1 and bins[j][1]>8: j+=1
        if run is None or j-i>run[1]-run[0]: run=(i,j)
        i=max(j,i+1)
    else: i+=1
p0,p1=run
up_bin=int(np.median([bins[k][0] for k in range(p0,p1)]))
# SFD: scan windows after preamble for the strongest down-ref peak
sfd=None
for k in range(p1, min(p1+6, len(bins))):
    db,ds=down_demod(seg[k*SPS:(k+1)*SPS])
    us=bins[k][1]
    if ds>8 and ds>us:
        if sfd is None or ds>sfd[2]: sfd=(k,db,ds)
sfd_k,down_bin,_=sfd
print(f"preamble sym[{p0}..{p1-1}] up_bin={up_bin}; SFD at sym {sfd_k} down_bin={down_bin}")

# 3) separate CFO(integer) and timing (in bins), resolve /2 ambiguity to small magnitude
def wrap(x): return ((x+N//2)%N)-N//2
sum_b=(up_bin+down_bin)          # 2*cfo (mod N)
dif_b=(up_bin-down_bin)          # 2*timing (mod N)
# timing in bins -> pick candidate with |tau|<N/4
tau=wrap(dif_b)/2.0
cfo_bin=(up_bin - tau)           # cfo = up - tau (bins), integer-ish
tau_samp=int(round(tau*OS))
print(f"tau={tau:.1f} bins ({tau_samp} samp)  cfo_bin={cfo_bin:.1f}")

# 4) header starts at sym (sfd_k + 2.25) in ph=0 grid; snap by timing
HDR=int(round((p1+4.25)*SPS)) - tau_samp
off=int(round(cfo_bin))-1        # value=(raw-off-1); preamble value 0 => off=cfo_bin-1
# wait: preamble raw = up_bin = cfo+tau; after snapping tau out, preamble raw=cfo => off=cfo-1
# but header grid is snapped, so use off = round(cfo_bin)-1... verify below by trying a few
def up_at(st):
    Y=np.abs(np.fft.fft(seg[st:st+SPS]*DOWN).reshape(OS,N).sum(0)); return int(np.argmax(Y))
def dec_header_nibs(sy,o):
    gg=[symp(s,o,True) for s in sy[:8]]; return [hamn(c,8) for c in deint(gg,SF-2,8)]
def dec_payload_nibs(sy,o,cr):
    cwl=cr+4; nib=[]
    for bi in range(0,len(sy)-cwl+1,cwl):
        gg=[symp(s,o,False) for s in sy[bi:bi+cwl]]
        for c in deint(gg,SF,cwl): nib.append(hamn(c,cwl))
    return nib
def full(sy,o,cr,L):
    h=dec_header_nibs(sy,o); pn=h[5:]+dec_payload_nibs(sy[8:],o,cr)
    rb=bytes((pn[i+1]<<4)|pn[i] for i in range(0,len(pn)-1,2))
    return bytes(b^WSEQ[i] for i,b in enumerate(rb))[:L]
# measure off directly from a snapped preamble window (robust)
pre_snapped=up_at(p0*SPS - tau_samp + 3*SPS)
print(f"snapped preamble bin={pre_snapped} -> off={pre_snapped-1}")
for da in range(-6,7):
    hs=HDR+da; 
    if hs<0 or hs+34*SPS>len(seg): continue
    sy=[up_at(hs+k*SPS) for k in range(min(34,(len(seg)-hs)//SPS))]
    for o in (pre_snapped-1,):
        h=dec_header_nibs(sy,o); L=(h[0]<<4)|h[1]; cr=(h[2]>>1)&7
        if 1<=cr<=4 and 8<=L<=64 and hdr_ok(h):
            fr=full(sy,o,cr,L)
            print(f"  da={da} off={o} L={L} cr={cr} dest={fr[:4].hex()} src={fr[4:8].hex()} frame={fr[:16].hex(' ')}")
