#!/usr/bin/env python3
#
# Copyright (C) 2026 zxkmm
#
# This file is part of PortaPack.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#
"""Regenerate the receiver-dependent fields of kSimNavConstellation (firmware/baseband/gps_nav_dsp.hpp).

The GPS L1 Rx "Fix -> Test" self-test simulates 5 SVs whose LNAV ephemeris words are fixed in that table.
Where the simulated receiver sits is set only by the per-SV signal state at t_start = 539998.0 s:
init_sf_tow / init_bit / init_ms / init_chip (transmit time = pseudorange) and fd0_hz / fdot_hz_s (Doppler).
This script decodes the ephemeris words, ranges to the requested point with light time + Sagnac (same model
as solve_pvt in firmware/common/gps_pvt.hpp), and prints those fields plus each SV's elevation.

Usage:
    python3 generate_gps_selftest_constellation.py <lat_deg> <lon_deg> <alt_m>
    python3 generate_gps_selftest_constellation.py 37.0 -123.0 0      # current table

Paste the printed initializer heads over the matching lines of kSimNavConstellation (ephemeris words unchanged)
and update the "True receiver location" comment above it. Keep every SV comfortably above the horizon;
the ephemerides are fixed, so stay within a few hundred km of the current point.
If the ephemeris words in gps_nav_dsp.hpp ever change, update TAB below to match.
"""
import math, sys
PI=3.1415926535898; MU=3.986005e14; OE=7.2921151467e-5; C=299792458.0; FREL=-4.442807633e-10
A_=6378137.0; F_=1/298.257223563; E2=F_*(2-F_); FL1=1575.42e6
def sx(v,b): m=1<<(b-1); return (v^m)-m
def i32(v): return v-(1<<32) if v&0x80000000 else v
TAB=[ # prn, sf1_w7, sf1_w9, sf2[8], sf3[8]  (words d[2..9]) -- copied from kSimNavConstellation
(1,0x2A83D6,0x0FBA88,[0x2A0190,0x2DF027,0x1C71C7,0x021902,0x0C49BA,0x0432A1,0x0D3333,0x83D600],[0x0000D6,0x2884D5,0x000027,0x1C71C7,0x1B8000,0x000000,0xFFA881,0x2A0460]),
(7,0x2B83D6,0xF583A4,[0x2B0190,0x2DF018,0xE38E39,0x021902,0xD0E560,0x0432A1,0x0D3333,0x83D600],[0x00000B,0x7DDA2A,0x000027,0x1C71C7,0x1B8000,0x000000,0xFFA881,0x2B0460]),
(13,0x2C83D6,0x068DB8,[0x2C0190,0x2DF03A,0x4FA4FA,0x021901,0x89374C,0x0432A1,0x0D3333,0x83D600],[0x0000E4,0x616863,0x000027,0x1C71C7,0x1B8000,0x000000,0xFFA881,0x2C0460]),
(19,0x2D83D6,0xEC56D4,[0x2D0190,0x2DF018,0xE38E39,0x021903,0x2CA57A,0x0432A1,0x0D3333,0x83D600],[0x0000B9,0xB6BDB8,0x000027,0x1C71C7,0x1B8000,0x000000,0xFFA881,0x2D0460]),
(27,0x2E83D6,0x0BCBE8,[0x2E0190,0x2DF00A,0xAAAAAB,0x021902,0x4DD2F2,0x0432A1,0x0D3333,0x83D600],[0x0000EB,0x7DDA2A,0x000027,0x1C71C7,0x1B8000,0x000000,0xFFA881,0x2E0460]),
]
def eph(t):
    prn,w7,w9,s2,s3=t; w1=[0,0,0x635000,0,0,0,0xF5,w7,0x12,w9]; w2=[0,0]+s2; w3=[0,0]+s3
    e={}
    e['tgd']=sx(w1[6]&0xFF,8)*2**-31; e['toc']=(w1[7]&0xFFFF)*16.0
    e['af2']=sx((w1[8]>>16)&0xFF,8)*2**-55; e['af1']=sx(w1[8]&0xFFFF,16)*2**-43; e['af0']=sx((w1[9]>>2)&0x3FFFFF,22)*2**-31
    e['crs']=sx(w2[2]&0xFFFF,16)*2**-5; e['dn']=sx((w2[3]>>8)&0xFFFF,16)*2**-43*PI
    e['m0']=i32(((w2[3]&0xFF)<<24)|(w2[4]&0xFFFFFF))*2**-31*PI
    e['cuc']=sx((w2[5]>>8)&0xFFFF,16)*2**-29; e['e']=(((w2[5]&0xFF)<<24)|(w2[6]&0xFFFFFF))*2**-33
    e['cus']=sx((w2[7]>>8)&0xFFFF,16)*2**-29; e['sqa']=(((w2[7]&0xFF)<<24)|(w2[8]&0xFFFFFF))*2**-19
    e['toe']=((w2[9]>>8)&0xFFFF)*16.0
    e['cic']=sx((w3[2]>>8)&0xFFFF,16)*2**-29; e['om0']=i32(((w3[2]&0xFF)<<24)|(w3[3]&0xFFFFFF))*2**-31*PI
    e['cis']=sx((w3[4]>>8)&0xFFFF,16)*2**-29; e['i0']=i32(((w3[4]&0xFF)<<24)|(w3[5]&0xFFFFFF))*2**-31*PI
    e['crc']=sx((w3[6]>>8)&0xFFFF,16)*2**-5; e['w']=i32(((w3[6]&0xFF)<<24)|(w3[7]&0xFFFFFF))*2**-31*PI
    e['omd']=sx(w3[8]&0xFFFFFF,24)*2**-43*PI; e['idot']=sx((w3[9]>>2)&0x3FFF,14)*2**-43*PI
    return e
def wrap(d): return d-604800 if d>302400 else (d+604800 if d<-302400 else d)
def sv(e,ttx):  # mirrors compute_sv_state()
    A=e['sqa']**2; n=math.sqrt(MU/A**3)+e['dn']
    doc=wrap(ttx-e['toc']); dclk=e['af0']+e['af1']*doc+e['af2']*doc*doc-e['tgd']
    tk=wrap(ttx-dclk-e['toe']); M=e['m0']+n*tk; E=M
    for _ in range(8): E-=(E-e['e']*math.sin(E)-M)/(1-e['e']*math.cos(E))
    dt=dclk+FREL*e['e']*e['sqa']*math.sin(E)
    tk=wrap(ttx-dt-e['toe']); M=e['m0']+n*tk; E=M
    for _ in range(6): E-=(E-e['e']*math.sin(E)-M)/(1-e['e']*math.cos(E))
    sE,cE=math.sin(E),math.cos(E); om=1-e['e']*cE
    nu=math.atan2(math.sqrt(1-e['e']**2)*sE/om,(cE-e['e'])/om); ph=nu+e['w']
    s2,c2=math.sin(2*ph),math.cos(2*ph)
    u=ph+e['cus']*s2+e['cuc']*c2; r=A*om+e['crs']*s2+e['crc']*c2; i=e['i0']+e['cis']*s2+e['cic']*c2+e['idot']*tk
    xp,yp=r*math.cos(u),r*math.sin(u); O=e['om0']+(e['omd']-OE)*tk-OE*e['toe']
    return (xp*math.cos(O)-yp*math.cos(i)*math.sin(O), xp*math.sin(O)+yp*math.cos(i)*math.cos(O), yp*math.sin(i)), dt
def ecef(lat,lon,h):
    la,lo=math.radians(lat),math.radians(lon); N=A_/math.sqrt(1-E2*math.sin(la)**2)
    return ((N+h)*math.cos(la)*math.cos(lo),(N+h)*math.cos(la)*math.sin(lo),(N*(1-E2)+h)*math.sin(la))
def ttx_of(e,R,trx):  # raw signal transmit time seen at receiver time trx (receiver clock bias 0)
    ttx=trx-0.07
    for _ in range(10):
        S,dt=sv(e,ttx); tau=math.dist(S,R)/C
        a=OE*tau; Sr=(S[0]*math.cos(a)+S[1]*math.sin(a),-S[0]*math.sin(a)+S[1]*math.cos(a),S[2])
        tau=math.dist(Sr,R)/C; ttx=trx+dt-tau
    return ttx, Sr
def elev(R,S,lat,lon):
    la,lo=math.radians(lat),math.radians(lon); d=[S[k]-R[k] for k in range(3)]; rn=math.sqrt(sum(x*x for x in d))
    up=(math.cos(la)*math.cos(lo),math.cos(la)*math.sin(lo),math.sin(la)); return math.degrees(math.asin(sum(d[k]*up[k] for k in range(3))/rn))
def gen(lat,lon,h,t0=539998.0):
    R=ecef(lat,lon,h); out=[]
    for t in TAB:
        e=eph(t); f=lambda tr: ttx_of(e,R,tr)[0]
        tx,S=ttx_of(e,R,t0); h_=5.0
        rate=(f(t0+h_)-f(t0-h_))/(2*h_)   # d t_tx / d t_rx = 1 + fd/FL1
        acc=(f(t0+h_)-2*tx+f(t0-h_))/h_**2
        fd=(rate-1)*FL1; fdot=acc*FL1
        sf=math.floor(tx/6)*6; rem=tx-sf; bit=int(rem/0.02); rem-=bit*0.02; ms=int(rem/0.001); rem-=ms*0.001
        out.append((t[0],int(sf),bit,ms,rem*1.023e6,fd,fdot,elev(R,S,lat,lon)))
    return out
if __name__=='__main__':
    if len(sys.argv)!=4:
        sys.exit(__doc__)
    lat,lon,h=map(float,sys.argv[1:4])
    print("// True receiver location: (%.5f deg N, %.5f deg E, %.1f m)"%(lat,lon,h))
    for prn,sf,bit,ms,chip,fd,fdot,el in gen(lat,lon,h):
        warn="  <-- LOW ELEVATION" if el<10 else ""
        print("    /* PRN %2d */ {%d, %dU, %dU, %dU, %.6ff, %.4ff, %.5ff,   // elev %.1f deg%s"%(prn,prn,sf,bit,ms,chip,fd,fdot,el,warn))
