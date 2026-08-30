#!/usr/bin/env python3
"""Build a REAL Meshtastic packet, encode it (validated gr-lora chain), emit cs8 IQ
to transmit to a real Heltec.  Observe with `meshtastic --listen` (a packet from the
fake src node = bidirectional interop proven).
"""
import sys
import numpy as np
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
import lora_encode as E   # int2bool, bool2int, gray_decode, hamming_enc, interleave, hdr_checksum, synth_chip, WSEQ, N, SF, SPS

DEFAULT_KEY = bytes.fromhex("d4f1bb3a20290759f0bcffabcf4e6901")


def crc16(crc, byte):
    for _ in range(8):
        if ((crc & 0x8000) >> 8) ^ (byte & 0x80):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF
        else:
            crc = (crc << 1) & 0xFFFF
        byte = (byte << 1) & 0xFF
    return crc


def meshtastic_crc(payload):
    crc = 0
    for b in payload[:len(payload)-2]:
        crc = crc16(crc, b)
    crc ^= payload[-1] ^ (payload[-2] << 8)
    return crc & 0xFFFF


def build_packet(text, src=0x12ABCDEF, channel_hash=0x0E, hop=3):
    dest = 0xFFFFFFFF
    pid = 0x11223344
    flags = (hop & 7) | ((hop & 7) << 5)        # hop_limit + hop_start
    data = b"\x08\x01" + b"\x12" + bytes([len(text)]) + text.encode()  # portnum=1, payload
    nonce = pid.to_bytes(4, "little") + b"\x00"*4 + src.to_bytes(4, "little") + b"\x00"*4
    enc = (lambda c: c.update(data) + c.finalize())(Cipher(algorithms.AES(DEFAULT_KEY), modes.CTR(nonce)).encryptor())
    hdr = (dest.to_bytes(4, "little") + src.to_bytes(4, "little") + pid.to_bytes(4, "little")
           + bytes([flags, channel_hash, 0x00, src & 0xFF]))
    return hdr + enc          # payload (header + encrypted Data); CRC added in encode


def encode_with_crc(payload, cr=1):
    L = len(payload)
    h = [L >> 4, L & 0xF, (cr << 1) | 1]      # has_crc=1
    c4, clo = E.hdr_checksum(h); h += [c4, clo]
    wpay = bytes(b ^ E.WSEQ[i] for i, b in enumerate(payload))
    nib = h[:]                                # 5 header nibbles
    for b in wpay:
        nib += [b & 0xF, b >> 4]
    crc = meshtastic_crc(payload)             # CRC NOT whitened
    nib += [crc & 0xF, (crc >> 4) & 0xF, (crc >> 8) & 0xF, (crc >> 12) & 0xF]
    # hamming + interleave
    syms = []
    hdr_cws = [E.hamming_enc(nib[i], 4) for i in range(E.SF-2)]
    syms += E.interleave(hdr_cws, E.SF-2, 8, True)
    pl = nib[E.SF-2:]
    cw_len = cr + 4
    for bi in range(0, len(pl), E.SF):
        block = pl[bi:bi+E.SF]
        if len(block) < E.SF:
            block += [0]*(E.SF-len(block))
        cws = [E.hamming_enc(block[i], cr) for i in range(E.SF)]
        syms += E.interleave(cws, E.SF, cw_len, False)
    return syms


def build_iq(text, n_preamble=16):
    payload = build_packet(text)
    g = encode_with_crc(payload)
    chips = [(E.gray_decode(x) + 1) % E.N for x in g]
    parts = [E.synth_chip(0, True) for _ in range(n_preamble)]
    parts += [E.synth_chip(0x2 << (E.SF-4), True), E.synth_chip(0xB << (E.SF-4), True)]
    parts += [E.synth_chip(0, False), E.synth_chip(0, False), E.synth_chip(0, False)[:E.SPS//4]]
    parts += [E.synth_chip(c, True) for c in chips]
    return np.concatenate(parts), payload


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "captures/tx_heltec.cs8"
    text = sys.argv[2] if len(sys.argv) > 2 else "HACKRF-TX"
    frame, payload = build_iq(text)
    reps = 6
    gap = np.zeros(E.SPS*3, dtype=complex)
    sig = np.concatenate([np.concatenate([gap, frame]) for _ in range(reps)] + [gap])
    cs8 = np.empty(sig.size*2, dtype=np.int8)
    cs8[0::2] = np.clip(np.real(sig)*110, -127, 127).astype(np.int8)
    cs8[1::2] = np.clip(np.imag(sig)*110, -127, 127).astype(np.int8)
    cs8.tofile(out)
    print(f"wrote {out}: text={text!r} payload_len={len(payload)} reps={reps}")
    print(f"payload: {payload.hex(' ')}")


if __name__ == "__main__":
    main()
