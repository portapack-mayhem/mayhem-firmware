/*
 * Copyright (C) 2026 Alexey Verhogladov
 *
 * This file is part of PortaPack.
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Public-key crypto for Meshtastic direct messages: SHA-256 (FIPS 180-4), X25519
 * (RFC 7748, field arithmetic from TweetNaCl, public domain), and AES-256-CCM with an
 * 8-byte tag. Matches the meshtastic CryptoEngine DM path so keys interoperate.
 */

#include "mesh_pki.hpp"
#include <cstring>

namespace meshtastic {
namespace pki {

// ============================================================================
// SHA-256 (FIPS 180-4)
// ============================================================================

namespace {

inline uint32_t ror(uint32_t x, int n) {
    return (x >> n) | (x << (32 - n));
}

const uint32_t K256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

void sha256_block(uint32_t st[8], const uint8_t* p) {
    uint32_t w[64];
    for (int i = 0; i < 16; i++)
        w[i] = (uint32_t(p[4 * i]) << 24) | (uint32_t(p[4 * i + 1]) << 16) |
               (uint32_t(p[4 * i + 2]) << 8) | uint32_t(p[4 * i + 3]);
    for (int i = 16; i < 64; i++) {
        uint32_t s0 = ror(w[i - 15], 7) ^ ror(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = ror(w[i - 2], 17) ^ ror(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    uint32_t a = st[0], b = st[1], c = st[2], d = st[3], e = st[4], f = st[5], g = st[6], h = st[7];
    for (int i = 0; i < 64; i++) {
        uint32_t S1 = ror(e, 6) ^ ror(e, 11) ^ ror(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t t1 = h + S1 + ch + K256[i] + w[i];
        uint32_t S0 = ror(a, 2) ^ ror(a, 13) ^ ror(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2 = S0 + maj;
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    st[0] += a;
    st[1] += b;
    st[2] += c;
    st[3] += d;
    st[4] += e;
    st[5] += f;
    st[6] += g;
    st[7] += h;
}

}  // namespace

void sha256(const uint8_t* msg, size_t len, uint8_t out[32]) {
    uint32_t st[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                      0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    uint8_t block[64];
    size_t i = 0;
    while (len - i >= 64) {
        sha256_block(st, msg + i);
        i += 64;
    }
    size_t rem = len - i;
    memcpy(block, msg + i, rem);
    block[rem] = 0x80;
    if (rem >= 56) {
        memset(block + rem + 1, 0, 64 - rem - 1);
        sha256_block(st, block);
        memset(block, 0, 56);
    } else {
        memset(block + rem + 1, 0, 56 - rem - 1);
    }
    uint64_t bits = uint64_t(len) * 8;
    for (int k = 0; k < 8; k++) block[56 + k] = uint8_t(bits >> (56 - 8 * k));
    sha256_block(st, block);
    for (int k = 0; k < 8; k++) {
        out[4 * k] = uint8_t(st[k] >> 24);
        out[4 * k + 1] = uint8_t(st[k] >> 16);
        out[4 * k + 2] = uint8_t(st[k] >> 8);
        out[4 * k + 3] = uint8_t(st[k]);
    }
}

// ============================================================================
// X25519 (TweetNaCl field arithmetic, public domain)
// ============================================================================

namespace {

typedef int64_t gf[16];
const gf gf_121665 = {0xDB41, 1};

void car25519(gf o) {
    for (int i = 0; i < 16; i++) {
        o[i] += (1LL << 16);
        int64_t c = o[i] >> 16;
        o[(i + 1) * (i < 15)] += c - 1 + 37 * (c - 1) * (i == 15);
        o[i] -= c << 16;
    }
}

void sel25519(gf p, gf q, int b) {
    int64_t c = ~(b - 1);
    for (int i = 0; i < 16; i++) {
        int64_t t = c & (p[i] ^ q[i]);
        p[i] ^= t;
        q[i] ^= t;
    }
}

void pack25519(uint8_t* o, const gf n) {
    // Static, not automatic: see the note above x25519 about the M0 stack.
    static gf m, t;
    for (int i = 0; i < 16; i++) t[i] = n[i];
    car25519(t);
    car25519(t);
    car25519(t);
    for (int j = 0; j < 2; j++) {
        m[0] = t[0] - 0xffed;
        for (int i = 1; i < 15; i++) {
            m[i] = t[i] - 0xffff - ((m[i - 1] >> 16) & 1);
            m[i - 1] &= 0xffff;
        }
        m[15] = t[15] - 0x7fff - ((m[14] >> 16) & 1);
        int b = (m[15] >> 16) & 1;
        m[14] &= 0xffff;
        sel25519(t, m, 1 - b);
    }
    for (int i = 0; i < 16; i++) {
        o[2 * i] = t[i] & 0xff;
        o[2 * i + 1] = t[i] >> 8;
    }
}

void unpack25519(gf o, const uint8_t* n) {
    for (int i = 0; i < 16; i++) o[i] = n[2 * i] + ((int64_t)n[2 * i + 1] << 8);
    o[15] &= 0x7fff;
}

void A(gf o, const gf a, const gf b) {
    for (int i = 0; i < 16; i++) o[i] = a[i] + b[i];
}
void Z(gf o, const gf a, const gf b) {
    for (int i = 0; i < 16; i++) o[i] = a[i] - b[i];
}

void M(gf o, const gf a, const gf b) {
    // Static, not automatic: see the note above x25519 about the M0 stack.
    static int64_t t[31];
    for (int i = 0; i < 31; i++) t[i] = 0;
    for (int i = 0; i < 16; i++)
        for (int j = 0; j < 16; j++) t[i + j] += a[i] * b[j];
    for (int i = 0; i < 15; i++) t[i] += 38 * t[i + 16];
    for (int i = 0; i < 16; i++) o[i] = t[i];
    car25519(o);
    car25519(o);
}

void S(gf o, const gf a) {
    M(o, a, a);
}

void inv25519(gf o, const gf i) {
    // Static, not automatic: see the note above x25519 about the M0 stack.
    static gf c;
    for (int a = 0; a < 16; a++) c[a] = i[a];
    for (int a = 253; a >= 0; a--) {
        S(c, c);
        if (a != 2 && a != 4) M(c, c, i);
    }
    for (int a = 0; a < 16; a++) o[a] = c[a];
}

}  // namespace

// The scratch field elements are static rather than automatic. Seven gf plus the
// temporaries inside M() and inv25519() come to roughly 1.4 KB, and this runs during
// app construction on the M0's 4 KB main stack - which overflowed into a Guru
// Meditation the moment PKC was enabled. Curve25519 here is only ever called from the
// UI thread (key derivation, direct message encrypt/decrypt), never from an interrupt,
// so a single shared scratch area is safe and costs BSS instead of stack.
void x25519(uint8_t out[32], const uint8_t scalar[32], const uint8_t point[32]) {
    uint8_t z[32];
    static gf x, a, b, c, d, e, f;
    for (int i = 0; i < 31; i++) z[i] = scalar[i];
    z[31] = (scalar[31] & 127) | 64;
    z[0] &= 248;
    unpack25519(x, point);
    for (int i = 0; i < 16; i++) {
        b[i] = x[i];
        d[i] = a[i] = c[i] = 0;
    }
    a[0] = d[0] = 1;
    for (int i = 254; i >= 0; --i) {
        int64_t r = (z[i >> 3] >> (i & 7)) & 1;
        sel25519(a, b, r);
        sel25519(c, d, r);
        A(e, a, c);
        Z(a, a, c);
        A(c, b, d);
        Z(b, b, d);
        S(d, e);
        S(f, a);
        M(a, c, a);
        M(c, b, e);
        A(e, a, c);
        Z(a, a, c);
        S(b, a);
        Z(c, d, f);
        M(a, c, gf_121665);
        A(a, a, d);
        M(c, c, a);
        M(a, d, f);
        M(d, b, x);
        S(b, e);
        sel25519(a, b, r);
        sel25519(c, d, r);
    }
    inv25519(c, c);
    M(a, a, c);
    pack25519(out, a);
}

void x25519_base(uint8_t pub[32], const uint8_t priv[32]) {
    uint8_t base[32] = {9};
    x25519(pub, priv, base);
}

void clamp_private(uint8_t priv[32]) {
    priv[0] &= 248;
    priv[31] &= 127;
    priv[31] |= 64;
}

// ============================================================================
// AES-256 ECB (self-contained; used only as the CCM block cipher)
// ============================================================================

namespace {

const uint8_t sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16};

inline uint8_t xtime(uint8_t x) {
    return (x << 1) ^ ((x >> 7) * 0x1b);
}

struct Aes256 {
    uint8_t rk[240];  // 15 round keys

    void set_key(const uint8_t key[32]) {
        memcpy(rk, key, 32);
        uint8_t rcon = 1;
        for (int i = 8; i < 60; i++) {
            uint8_t t[4];
            memcpy(t, rk + 4 * (i - 1), 4);
            if (i % 8 == 0) {
                uint8_t tmp = t[0];
                t[0] = sbox[t[1]] ^ rcon;
                t[1] = sbox[t[2]];
                t[2] = sbox[t[3]];
                t[3] = sbox[tmp];
                rcon = xtime(rcon);
            } else if (i % 8 == 4) {
                for (int k = 0; k < 4; k++) t[k] = sbox[t[k]];
            }
            for (int k = 0; k < 4; k++)
                rk[4 * i + k] = rk[4 * (i - 8) + k] ^ t[k];
        }
    }

    void encrypt(const uint8_t in[16], uint8_t out[16]) const {
        uint8_t s[16];
        memcpy(s, in, 16);
        for (int i = 0; i < 16; i++) s[i] ^= rk[i];
        for (int round = 1; round <= 14; round++) {
            for (int i = 0; i < 16; i++) s[i] = sbox[s[i]];
            uint8_t t[16];
            // ShiftRows
            t[0] = s[0];
            t[4] = s[4];
            t[8] = s[8];
            t[12] = s[12];
            t[1] = s[5];
            t[5] = s[9];
            t[9] = s[13];
            t[13] = s[1];
            t[2] = s[10];
            t[6] = s[14];
            t[10] = s[2];
            t[14] = s[6];
            t[3] = s[15];
            t[7] = s[3];
            t[11] = s[7];
            t[15] = s[11];
            if (round < 14) {
                for (int c = 0; c < 4; c++) {
                    uint8_t* col = t + 4 * c;
                    uint8_t a0 = col[0], a1 = col[1], a2 = col[2], a3 = col[3];
                    uint8_t h = a0 ^ a1 ^ a2 ^ a3;
                    col[0] ^= h ^ xtime(a0 ^ a1);
                    col[1] ^= h ^ xtime(a1 ^ a2);
                    col[2] ^= h ^ xtime(a2 ^ a3);
                    col[3] ^= h ^ xtime(a3 ^ a0);
                }
            }
            for (int i = 0; i < 16; i++) s[i] = t[i] ^ rk[16 * round + i];
        }
        memcpy(out, s, 16);
    }
};

// AES-CCM (RFC 3610) with tag length M = 8. nonce_len N sets L = 15 - N.
void ccm_core(const Aes256& aes, const uint8_t* nonce, size_t N, const uint8_t* in, size_t len, bool encrypt, uint8_t* out, uint8_t tag[8]) {
    const size_t M = 8;
    const size_t L = 15 - N;
    uint8_t blk[16], mac[16], s0[16], ctr[16];

    // --- CBC-MAC over B0 (+ formatted message; no associated data) ---
    memset(blk, 0, 16);
    blk[0] = (uint8_t)((((M - 2) / 2) << 3) | (L - 1));  // flags: Adata=0
    memcpy(blk + 1, nonce, N);
    for (size_t i = 0; i < L; i++)
        blk[15 - i] = (uint8_t)((len >> (8 * i)) & 0xff);
    aes.encrypt(blk, mac);

    // For decryption we need the plaintext to MAC; CTR-decrypt first, then MAC.
    // Encryption MACs the plaintext (in) then CTR-encrypts. Handle both by MACing
    // whichever buffer holds plaintext.
    // Set up the counter block A_i.
    memset(ctr, 0, 16);
    ctr[0] = (uint8_t)(L - 1);
    memcpy(ctr + 1, nonce, N);
    // A0 keystream (for the tag).
    for (size_t i = 0; i < L; i++) ctr[15 - i] = 0;
    aes.encrypt(ctr, s0);

    const uint8_t* pt = encrypt ? in : out;  // plaintext location for MAC
    if (!encrypt) {
        // CTR-decrypt in -> out first so we can MAC the plaintext.
        size_t off = 0;
        uint32_t c = 1;
        while (off < len) {
            for (size_t i = 0; i < L; i++) ctr[15 - i] = (uint8_t)((c >> (8 * i)) & 0xff);
            uint8_t ks[16];
            aes.encrypt(ctr, ks);
            size_t n = (len - off < 16) ? (len - off) : 16;
            for (size_t i = 0; i < n; i++) out[off + i] = in[off + i] ^ ks[i];
            off += n;
            c++;
        }
    }

    // CBC-MAC the plaintext blocks.
    {
        size_t off = 0;
        while (off < len) {
            size_t n = (len - off < 16) ? (len - off) : 16;
            memset(blk, 0, 16);
            memcpy(blk, pt + off, n);
            for (int i = 0; i < 16; i++) mac[i] ^= blk[i];
            aes.encrypt(mac, mac);
            off += n;
        }
    }

    if (encrypt) {
        // CTR-encrypt in -> out.
        size_t off = 0;
        uint32_t c = 1;
        while (off < len) {
            for (size_t i = 0; i < L; i++) ctr[15 - i] = (uint8_t)((c >> (8 * i)) & 0xff);
            uint8_t ks[16];
            aes.encrypt(ctr, ks);
            size_t n = (len - off < 16) ? (len - off) : 16;
            for (size_t i = 0; i < n; i++) out[off + i] = in[off + i] ^ ks[i];
            off += n;
            c++;
        }
    }

    // Tag = (MAC ^ S0) truncated to M bytes.
    for (size_t i = 0; i < M; i++) tag[i] = mac[i] ^ s0[i];
}

}  // namespace

void aes_ccm_encrypt(const uint8_t key[32], const uint8_t* nonce, size_t nonce_len, const uint8_t* in, size_t len, uint8_t* out, uint8_t tag[8]) {
    Aes256 aes;
    aes.set_key(key);
    ccm_core(aes, nonce, nonce_len, in, len, true, out, tag);
}

bool aes_ccm_decrypt(const uint8_t key[32], const uint8_t* nonce, size_t nonce_len, const uint8_t* in, size_t len, const uint8_t tag[8], uint8_t* out) {
    Aes256 aes;
    aes.set_key(key);
    uint8_t want[8];
    ccm_core(aes, nonce, nonce_len, in, len, false, out, want);
    uint8_t diff = 0;
    for (int i = 0; i < 8; i++) diff |= (want[i] ^ tag[i]);
    return diff == 0;
}

}  // namespace pki
}  // namespace meshtastic
