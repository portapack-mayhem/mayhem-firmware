/*
 * LoRa RX - MeshCore packet parsing and group channel decryption.
 *
 * Packet: header (route 2 bits, type 4 bits, version 2 bits), [4 transport bytes],
 * path length (hops 6 bits, hash size-1 2 bits), path, payload.
 * Group text (type 5): channel hash (1), MAC (2: HMAC-SHA256(key || 16 zero bytes, ciphertext)),
 * ciphertext (AES-128-ECB): timestamp (4, LE), flags (1), "name: text".
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

// Keep everything inside this app's namespace (no constprop clones in the firmware image).
#pragma GCC optimize("no-ipa-cp")

#include "meshcore.hpp"

#include <cstring>

namespace ui::external_app::lora_rx {

// ---------------------------------------------------------------- SHA-256
namespace {

const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

// namespace-scope tables (named sections, linked into the app image, not the firmware)
const uint32_t IV[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

inline uint32_t ror(uint32_t x, int n) {
    return (x >> n) | (x << (32 - n));
}

struct Sha {
    uint32_t h[8];
    uint8_t blk[64];
    uint32_t n{0};
    uint64_t total{0};

    Sha() { memcpy(h, IV, sizeof(h)); }
    void block() {
        uint32_t w[64];
        for (int i = 0; i < 16; i++) w[i] = (uint32_t)blk[4 * i] << 24 | blk[4 * i + 1] << 16 | blk[4 * i + 2] << 8 | blk[4 * i + 3];
        for (int i = 16; i < 64; i++) {
            uint32_t s0 = ror(w[i - 15], 7) ^ ror(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = ror(w[i - 2], 17) ^ ror(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a[8];
        memcpy(a, h, sizeof(a));
        for (int i = 0; i < 64; i++) {
            uint32_t t1 = a[7] + (ror(a[4], 6) ^ ror(a[4], 11) ^ ror(a[4], 25)) + ((a[4] & a[5]) ^ (~a[4] & a[6])) + K[i] + w[i];
            uint32_t t2 = (ror(a[0], 2) ^ ror(a[0], 13) ^ ror(a[0], 22)) + ((a[0] & a[1]) ^ (a[0] & a[2]) ^ (a[1] & a[2]));
            for (int j = 7; j > 0; j--) a[j] = a[j - 1];
            a[4] += t1;
            a[0] = t1 + t2;
        }
        for (int i = 0; i < 8; i++) h[i] += a[i];
    }
    void update(const uint8_t* d, size_t len) {
        for (size_t i = 0; i < len; i++) {
            blk[n++] = d[i];
            total++;
            if (n == 64) {
                block();
                n = 0;
            }
        }
    }
    void final(uint8_t out[32]) {
        uint64_t bits = total * 8;
        uint8_t pad = 0x80;
        update(&pad, 1);
        pad = 0;
        while (n != 56) update(&pad, 1);
        for (int i = 7; i >= 0; i--) {
            uint8_t b = bits >> (8 * i);
            update(&b, 1);
        }
        for (int i = 0; i < 8; i++)
            for (int j = 0; j < 4; j++) out[4 * i + j] = h[i] >> (24 - 8 * j);
    }
};

// ---------------------------------------------------------------- AES-128 (decryption only)
uint8_t sbox[256], inv_sbox[256];
bool sbox_ready = false;

inline uint8_t xt(uint8_t x) {
    return (uint8_t)((x << 1) ^ ((x & 0x80) ? 0x1b : 0));
}

uint8_t gmul(uint8_t a, uint8_t b) {
    uint8_t p = 0;
    while (b) {
        if (b & 1) p ^= a;
        a = xt(a);
        b >>= 1;
    }
    return p;
}

void make_sbox() {
    // multiplicative inverse in GF(2^8) then affine transform
    for (int i = 0; i < 256; i++) {
        uint8_t inv = 0;
        if (i)
            for (int j = 1; j < 256; j++)
                if (gmul(i, j) == 1) {
                    inv = j;
                    break;
                }
        uint8_t s = inv;
        for (int k = 1; k < 5; k++) s ^= (uint8_t)((inv << k) | (inv >> (8 - k)));
        s ^= 0x63;
        sbox[i] = s;
        inv_sbox[s] = i;
    }
    sbox_ready = true;
}

void aes128_decrypt_ecb(const uint8_t key[16], uint8_t* data, size_t len) {
    if (!sbox_ready) make_sbox();
    uint8_t rk[176];
    memcpy(rk, key, 16);
    uint8_t rcon = 1;
    for (int i = 16; i < 176; i += 4) {
        uint8_t t[4] = {rk[i - 4], rk[i - 3], rk[i - 2], rk[i - 1]};
        if (i % 16 == 0) {
            uint8_t u = t[0];
            t[0] = sbox[t[1]] ^ rcon;
            t[1] = sbox[t[2]];
            t[2] = sbox[t[3]];
            t[3] = sbox[u];
            rcon = xt(rcon);
        }
        for (int j = 0; j < 4; j++) rk[i + j] = rk[i - 16 + j] ^ t[j];
    }
    for (size_t off = 0; off + 16 <= len; off += 16) {
        uint8_t* s = data + off;
        for (int j = 0; j < 16; j++) s[j] ^= rk[160 + j];
        for (int r = 9; r >= 0; r--) {
            // InvShiftRows (column-major state)
            uint8_t t[16];
            for (int c = 0; c < 4; c++)
                for (int row = 0; row < 4; row++) t[4 * ((c + row) % 4) + row] = s[4 * c + row];
            for (int j = 0; j < 16; j++) s[j] = inv_sbox[t[j]] ^ rk[16 * r + j];
            if (r == 0) break;
            for (int c = 0; c < 4; c++) {
                uint8_t* m = s + 4 * c;
                uint8_t a0 = m[0], a1 = m[1], a2 = m[2], a3 = m[3];
                m[0] = gmul(a0, 14) ^ gmul(a1, 11) ^ gmul(a2, 13) ^ gmul(a3, 9);
                m[1] = gmul(a0, 9) ^ gmul(a1, 14) ^ gmul(a2, 11) ^ gmul(a3, 13);
                m[2] = gmul(a0, 13) ^ gmul(a1, 9) ^ gmul(a2, 14) ^ gmul(a3, 11);
                m[3] = gmul(a0, 11) ^ gmul(a1, 13) ^ gmul(a2, 9) ^ gmul(a3, 14);
            }
        }
    }
}

bool mac_ok(const uint8_t key[16], const uint8_t* ct, size_t len, const uint8_t* mac) {
    // HMAC-SHA256 with the 32-byte secret key || 16 zero bytes. One pad buffer and one SHA
    // context at a time: this runs on the small M0 stack.
    uint8_t pad[64], digest[32];
    for (int i = 0; i < 64; i++) pad[i] = (i < 16 ? key[i] : 0) ^ 0x36;
    {
        Sha a;
        a.update(pad, 64);
        a.update(ct, len);
        a.final(digest);
    }
    for (int i = 0; i < 64; i++) pad[i] = (i < 16 ? key[i] : 0) ^ 0x5c;
    {
        Sha b;
        b.update(pad, 64);
        b.update(digest, 32);
        b.final(digest);
    }
    return digest[0] == mac[0] && digest[1] == mac[1];
}

// Appends UTF-8 text as ASCII (accents dropped, other characters skipped)
void ascii(const uint8_t* s, int len, char* out, int size) {
    int o = 0;
    for (int i = 0; i < len && s[i] && o < size - 1; i++) {
        uint8_t c = s[i];
        if (c < 0x80) {
            if (c >= 32) out[o++] = c;
        } else if (c == 0xC3 && i + 1 < len) {
            static const char map[] = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYPsaaaaaaaceeeeiiiidnooooo/ouuuuypy";
            uint8_t d = s[++i];
            if (d >= 0x80 && d < 0xC0) out[o++] = map[d - 0x80];
        } else if (c >= 0xC0) {
            // skip the continuation bytes of other characters (emoji...)
            while (i + 1 < len && (s[i + 1] & 0xC0) == 0x80) i++;
        }
    }
    while (o > 0 && out[o - 1] == ' ') o--;
    out[o] = 0;
}

void put(char* out, int size, const char* a, const char* b = "", const char* c = "") {
    int o = 0;
    const char* parts[3] = {a, b, c};
    for (int k = 0; k < 3; k++)
        for (const char* p = parts[k]; *p && o < size - 1;) out[o++] = *p++;
    out[o] = 0;
}

// explicit table (a switch would become an anonymous CSWTCH table linked into the firmware)
const char type_names[11][16] = {"request", "response", "direct msg", "ack", "advert",
                                 "group msg", "group data", "anon request", "path", "trace", "other"};

const char* type_name(int t) {
    return type_names[t >= 0 && t < 10 ? t : 10];
}

}  // namespace

void sha256(const uint8_t* data, size_t len, uint8_t out[32]) {
    Sha s;
    s.update(data, len);
    s.final(out);
}

void channel_set_key(Channel& ch) {
    uint8_t h[32];
    sha256(ch.key, 16, h);
    ch.hash = h[0];
}

void channel_from_name(const char* name, Channel& ch) {
    uint8_t h[32];
    sha256((const uint8_t*)name, strlen(name), h);
    memcpy(ch.key, h, 16);
    put(ch.name, sizeof(ch.name), name);
    channel_set_key(ch);
}

bool meshcore_decode(const uint8_t* pkt, int len, const Channel* chans, int nchans, char* line1, int size1, char* line2, int size2) {
    line2[0] = 0;
    if (len < 2) {
        put(line1, size1, "too short");
        return false;
    }
    int route = pkt[0] & 3, type = (pkt[0] >> 2) & 15;
    int i = 1 + ((route == 0 || route == 3) ? 4 : 0);
    if (i >= len) {
        put(line1, size1, "truncated");
        return false;
    }
    int hops = pkt[i] & 63, hsize = (pkt[i] >> 6) + 1;
    i += 1 + hops * hsize;
    const uint8_t* p = pkt + i;
    int plen = len - i;
    char hop_txt[] = "0 hop(s)";
    hop_txt[0] = (char)('0' + (hops > 9 ? 9 : hops));
    put(line1, size1, type_name(type), ", ", hop_txt);
    if (plen < 0) return false;

    if (type == 5 && plen >= 3 + 16 && (plen - 3) % 16 == 0) {
        for (int c = 0; c < nchans; c++) {
            const Channel& ch = chans[c];
            if (ch.hash != p[0] || !mac_ok(ch.key, p + 3, plen - 3, p + 1)) continue;
            uint8_t pt[192];
            int n = plen - 3 > (int)sizeof(pt) ? (int)sizeof(pt) : plen - 3;
            memcpy(pt, p + 3, n);
            aes128_decrypt_ecb(ch.key, pt, n);
            put(line1, size1, ch.name, ", ", hop_txt);
            ascii(pt + 5, n - 5, line2, size2);
            return true;
        }
        put(line1, size1, "group (unknown channel)");
    } else if (type == 4 && plen >= 101) {
        // advert: public key (32), timestamp (4), signature (64), app data: flags, [lat, lon], [f1], [f2], name
        int flags = p[100], o = 101;
        if (flags & 0x10) o += 8;
        if (flags & 0x20) o += 2;
        if (flags & 0x40) o += 2;
        put(line1, size1, "advert ", (flags & 0x0F) == 2 ? "(repeater)" : (flags & 0x0F) == 1 ? "(client)"
                                                                                              : "");
        if ((flags & 0x80) && o < plen) ascii(p + o, plen - o, line2, size2);
    }
    return false;
}

}  // namespace ui::external_app::lora_rx
