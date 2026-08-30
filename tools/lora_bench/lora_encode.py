#!/usr/bin/env python3
"""LoRa encoder = exact inverse of the verified gr-lora_sdr chain (lora_gold.py).
Validate by round-trip: encode -> frame IQ -> lora_gold decode -> original bytes.

TX chain (gr-lora_sdr hamming_enc/interleaver/modulate):
  nibbles -> hamming_enc -> interleave (+reduced-rate parity for header)
  -> g symbol -> chip = (gray_decode(g)+1) mod N (inverse of RX (idx-1)+gray-demap)
  frame = preamble(up id0) + 2 sync + 2.25 downchirp SFD + header(8) + payload syms
"""
import sys
import numpy as np

SF, BW, FS = 7, 500e3, 4e6
N = 1 << SF
OS = int(FS / BW)
SPS = N * OS

WSEQ = [0xFF,0xFE,0xFC,0xF8,0xF0,0xE1,0xC2,0x85,0x0B,0x17,0x2F,0x5E,0xBC,0x78,0xF1,0xE3,
        0xC6,0x8D,0x1A,0x34,0x68,0xD0,0xA0,0x40,0x80,0x01,0x02,0x04,0x08,0x11,0x23,0x47,
        0x8E,0x1C,0x38,0x71,0xE2,0xC4,0x89,0x12,0x25,0x4B,0x97,0x2E,0x5C,0xB8,0x70,0xE0,
        0xC0,0x81,0x03,0x06,0x0C,0x19,0x32,0x64,0xC9,0x92,0x24,0x49,0x93,0x26,0x4D,0x9B]


def int2bool(v, n):  # MSB first
    return [(v >> (n-1-i)) & 1 for i in range(n)]

def bool2int(b):
    x = 0
    for bit in b:
        x = (x << 1) | bit
    return x

def gray_decode(g):   # inverse of g = v ^ (v>>1)
    v = g
    s = g >> 1
    while s:
        v ^= s
        s >>= 1
    return v

def hamming_enc(nibble, cr_app):
    d = int2bool(nibble, 4)          # d[0]=MSB
    if cr_app != 1:
        p0 = d[3]^d[2]^d[1]; p1 = d[2]^d[1]^d[0]; p2 = d[3]^d[2]^d[0]; p3 = d[3]^d[1]^d[0]
        full = (d[3]<<7|d[2]<<6|d[1]<<5|d[0]<<4|p0<<3|p1<<2|p2<<1|p3) >> (4 - cr_app)
        return full
    else:
        p4 = d[0]^d[1]^d[2]^d[3]
        return d[3]<<4|d[2]<<3|d[1]<<2|d[0]<<1|p4

def interleave(codewords, sf_app, cw_len, ldro_block):
    # codewords: cw_len? no -> sf_app codewords of cw_len bits -> cw_len symbols
    cw_bin = [int2bool(codewords[i], cw_len) for i in range(sf_app)]
    syms = []
    nbits = sf_app + (1 if ldro_block else 0)
    for i in range(cw_len):
        inter = [0]*nbits
        for j in range(sf_app):
            inter[j] = cw_bin[(i - j - 1) % sf_app][i]
        if ldro_block:
            inter[sf_app] = sum(inter[:sf_app]) % 2
        # pad to SF bits (MSB-first value); reduced-rate symbols are << (SF-nbits)
        g = bool2int(inter) << (SF - nbits)
        syms.append(g)
    return syms

def hdr_checksum(h):
    c4 = ((h[0]>>3)&1)^((h[0]>>2)&1)^((h[0]>>1)&1)^(h[0]&1)
    c3 = ((h[0]>>3)&1)^((h[1]>>3)&1)^((h[1]>>2)&1)^((h[1]>>1)&1)^(h[2]&1)
    c2 = ((h[0]>>2)&1)^((h[1]>>3)&1)^(h[1]&1)^((h[2]>>3)&1)^((h[2]>>1)&1)
    c1 = ((h[0]>>1)&1)^((h[1]>>2)&1)^(h[1]&1)^((h[2]>>2)&1)^((h[2]>>1)&1)^(h[2]&1)
    c0 = (h[0]&1)^((h[1]>>1)&1)^((h[2]>>3)&1)^((h[2]>>2)&1)^((h[2]>>1)&1)^(h[2]&1)
    return c4, (c3<<3)|(c2<<2)|(c1<<1)|c0

def encode_symbols(payload, cr=1, has_crc=1):
    # header nibbles
    L = len(payload)
    h = [L >> 4, L & 0xF, (cr << 1) | has_crc]
    c4, clo = hdr_checksum(h)
    h += [c4, clo]                       # 5 header nibbles
    # whiten payload, split to nibbles (low first)
    wpay = bytes(b ^ WSEQ[i] for i, b in enumerate(payload))
    pnib = []
    for b in wpay:
        pnib.append(b & 0xF); pnib.append(b >> 4)
    nibbles = h + pnib                    # header reduced-rate, then payload
    # hamming + interleave block by block
    syms = []
    cnt = 0
    idx = 0
    # header block: first sf-2 nibbles (=5) -> cr_app=4, sf_app=sf-2, cw_len=8, ldro
    hdr_cws = [hamming_enc(nibbles[i], 4) for i in range(SF-2)]
    syms += interleave(hdr_cws, SF-2, 8, ldro_block=True)
    idx = SF-2
    # payload blocks: sf_app=SF, cw_len=cr+4
    cw_len = cr + 4
    pl = nibbles[idx:]
    for bi in range(0, len(pl), SF):
        block = pl[bi:bi+SF]
        if len(block) < SF:
            block = block + [0]*(SF-len(block))
        cws = [hamming_enc(block[i], cr) for i in range(SF)]
        syms += interleave(cws, SF, cw_len, ldro_block=False)
    return syms

def synth_chip(c, up=True):
    t = np.arange(SPS)/FS
    Tsym = SPS/FS
    if up:
        f0 = -BW/2 + c*BW/N
        f = f0 + (BW/Tsym)*t
        f = ((f + BW/2) % BW) - BW/2
    else:
        f = BW/2 - (BW/Tsym)*t          # downchirp
    return np.exp(1j*2*np.pi*np.cumsum(f)/FS)

def build_frame(payload, cr=1, has_crc=1, n_preamble=8):
    g_syms = encode_symbols(payload, cr, has_crc)
    chips = [(gray_decode(g) + 1) % N for g in g_syms]
    iq = []
    for _ in range(n_preamble):
        iq.append(synth_chip(0, True))
    iq.append(synth_chip(0x2 << (SF-4), True))     # sync 0x2B
    iq.append(synth_chip(0xB << (SF-4), True))
    iq.append(synth_chip(0, False)); iq.append(synth_chip(0, False))   # 2 downchirps
    iq.append(synth_chip(0, False)[:SPS//4])                            # 0.25 downchirp
    for c in chips:
        iq.append(synth_chip(c, True))
    return np.concatenate(iq)

def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "captures/tx_test.cs8"
    # payload that lora_gold recognizes: broadcast dest + a src
    payload = bytes.fromhex("ffffffff12345678") + bytes(range(0x10, 0x10+22))  # 30 bytes
    frame = build_frame(payload, cr=1, has_crc=1)
    # pad with noise-free silence around, scale to int8
    pad = np.zeros(SPS*4, dtype=complex)
    sig = np.concatenate([pad, frame*0.7, pad])
    cs8 = np.empty(sig.size*2, dtype=np.int8)
    cs8[0::2] = np.clip(np.real(sig)*120, -127, 127).astype(np.int8)
    cs8[1::2] = np.clip(np.imag(sig)*120, -127, 127).astype(np.int8)
    cs8.tofile(out)
    print(f"wrote {out} ({cs8.size} bytes), payload[:8]={payload[:8].hex(' ')}")
    print(f"frame symbols={len(encode_symbols(payload))}")

if __name__ == "__main__":
    main()
