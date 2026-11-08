/*
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "chat_crypto.hpp"

#include <cstring>

namespace ui::external_app::subghz_chat::chat_crypto {

namespace {

// SHA-256: FIPS 180-4.

constexpr uint32_t sha_k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

inline uint32_t rotr(const uint32_t x, const int n) {
    return (x >> n) | (x << (32 - n));
}

void sha256_block(uint32_t state[8], const uint8_t block[64]) {
    uint32_t w[64];
    for (int i = 0; i < 16; i++)
        w[i] = ((uint32_t)block[4 * i] << 24) | ((uint32_t)block[4 * i + 1] << 16) |
               ((uint32_t)block[4 * i + 2] << 8) | block[4 * i + 3];
    for (int i = 16; i < 64; i++) {
        const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
        const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
    for (int i = 0; i < 64; i++) {
        const uint32_t t1 = h + (rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)) + ((e & f) ^ (~e & g)) + sha_k[i] + w[i];
        const uint32_t t2 = (rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

// AES: FIPS 197. Only the forward cipher: GCM doesn't need the inverse.

constexpr uint8_t sbox[256] = {
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

inline uint8_t xtime(const uint8_t x) {
    return (x << 1) ^ ((x & 0x80) ? 0x1b : 0x00);
}

// GCM: NIST SP 800-38D.

// x = x * y in GF(2^128), bit by bit
void gf_multiply(uint8_t x[16], const uint8_t y[16]) {
    uint8_t z[16]{};
    uint8_t v[16];
    memcpy(v, y, 16);
    for (int i = 0; i < 128; i++) {
        if (x[i >> 3] & (0x80 >> (i & 7)))
            for (int j = 0; j < 16; j++)
                z[j] ^= v[j];
        const bool lsb = v[15] & 1;
        for (int j = 15; j > 0; j--)
            v[j] = (v[j] >> 1) | (v[j - 1] << 7);
        v[0] >>= 1;
        if (lsb)
            v[0] ^= 0xe1;
    }
    memcpy(x, z, 16);
}

void ghash_update(uint8_t y[16], const uint8_t h[16], const uint8_t* data, size_t length) {
    while (length) {
        const size_t n = length < 16 ? length : 16;
        for (size_t i = 0; i < n; i++)
            y[i] ^= data[i];
        gf_multiply(y, h);
        data += n;
        length -= n;
    }
}

void counter_block(const uint8_t iv[IV_BYTES], const uint32_t counter, uint8_t block[16]) {
    memcpy(block, iv, IV_BYTES);
    block[12] = counter >> 24;
    block[13] = counter >> 16;
    block[14] = counter >> 8;
    block[15] = counter;
}

}  // namespace

void sha256(const uint8_t* data, size_t length, uint8_t digest[32]) {
    uint32_t state[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                         0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    const uint64_t bits = (uint64_t)length * 8;
    while (length >= 64) {
        sha256_block(state, data);
        data += 64;
        length -= 64;
    }
    // the rest, 0x80, zeros and the length in bits: one or two blocks
    uint8_t tail[128]{};
    memcpy(tail, data, length);
    tail[length] = 0x80;
    const size_t tail_length = (length < 56) ? 64 : 128;
    for (int i = 0; i < 8; i++)
        tail[tail_length - 1 - i] = bits >> (8 * i);
    sha256_block(state, tail);
    if (tail_length == 128)
        sha256_block(state, tail + 64);
    for (int i = 0; i < 8; i++) {
        digest[4 * i] = state[i] >> 24;
        digest[4 * i + 1] = state[i] >> 16;
        digest[4 * i + 2] = state[i] >> 8;
        digest[4 * i + 3] = state[i];
    }
    wipe(state, sizeof(state));
    wipe(tail, sizeof(tail));
}

void Aes256::set_key(const uint8_t key[KEY_BYTES]) {
    uint8_t* w = round_keys_.data();
    memcpy(w, key, KEY_BYTES);
    uint8_t rcon = 0x01;
    for (size_t i = KEY_BYTES; i < round_keys_.size(); i += 4) {
        uint8_t t[4];
        memcpy(t, w + i - 4, 4);
        if (i % KEY_BYTES == 0) {
            const uint8_t t0 = t[0];
            t[0] = sbox[t[1]] ^ rcon;
            t[1] = sbox[t[2]];
            t[2] = sbox[t[3]];
            t[3] = sbox[t0];
            rcon = xtime(rcon);
        } else if (i % KEY_BYTES == 16) {
            for (auto& b : t)
                b = sbox[b];
        }
        for (int j = 0; j < 4; j++)
            w[i + j] = w[i + j - KEY_BYTES] ^ t[j];
    }
}

void Aes256::encrypt_block(const uint8_t in[16], uint8_t out[16]) const {
    uint8_t s[16];
    for (int i = 0; i < 16; i++)
        s[i] = in[i] ^ round_keys_[i];
    for (int round = 1; round <= 14; round++) {
        // SubBytes and ShiftRows: the state is column by column, row r moves r columns left
        uint8_t t[16];
        for (int c = 0; c < 4; c++)
            for (int r = 0; r < 4; r++)
                t[4 * c + r] = sbox[s[4 * ((c + r) & 3) + r]];
        if (round < 14) {
            // MixColumns
            for (int c = 0; c < 4; c++) {
                uint8_t* col = t + 4 * c;
                const uint8_t a0 = col[0], a1 = col[1], a2 = col[2], a3 = col[3];
                const uint8_t all = a0 ^ a1 ^ a2 ^ a3;
                col[0] ^= all ^ xtime(a0 ^ a1);
                col[1] ^= all ^ xtime(a1 ^ a2);
                col[2] ^= all ^ xtime(a2 ^ a3);
                col[3] ^= all ^ xtime(a3 ^ a0);
            }
        }
        for (int i = 0; i < 16; i++)
            s[i] = t[i] ^ round_keys_[16 * round + i];
    }
    memcpy(out, s, 16);
}

void Aes256::clear() {
    wipe(round_keys_.data(), round_keys_.size());
}

void Gcm::set_key(const uint8_t key[KEY_BYTES]) {
    aes_.set_key(key);
    const uint8_t zero[16]{};
    aes_.encrypt_block(zero, h_.data());
}

void Gcm::clear() {
    aes_.clear();
    wipe(h_.data(), h_.size());
}

void Gcm::crypt(const uint8_t iv[IV_BYTES], const uint8_t* in, uint8_t* out, size_t length) const {
    // counter 1 is for the tag, the data starts at 2
    uint32_t counter = 2;
    uint8_t block[16], stream[16];
    while (length) {
        counter_block(iv, counter++, block);
        aes_.encrypt_block(block, stream);
        const size_t n = length < 16 ? length : 16;
        for (size_t i = 0; i < n; i++)
            out[i] = in[i] ^ stream[i];
        in += n;
        out += n;
        length -= n;
    }
}

void Gcm::compute_tag(const uint8_t iv[IV_BYTES], const uint8_t* aad, size_t aad_length, const uint8_t* cipher, size_t length, uint8_t tag[TAG_BYTES]) const {
    uint8_t y[16]{};
    ghash_update(y, h_.data(), aad, aad_length);
    ghash_update(y, h_.data(), cipher, length);
    uint8_t lengths[16]{};
    const uint64_t aad_bits = (uint64_t)aad_length * 8;
    const uint64_t bits = (uint64_t)length * 8;
    for (int i = 0; i < 8; i++) {
        lengths[7 - i] = aad_bits >> (8 * i);
        lengths[15 - i] = bits >> (8 * i);
    }
    ghash_update(y, h_.data(), lengths, 16);

    uint8_t block[16], first[16];
    counter_block(iv, 1, block);
    aes_.encrypt_block(block, first);
    for (int i = 0; i < 16; i++)
        tag[i] = y[i] ^ first[i];
}

void Gcm::encrypt(const uint8_t iv[IV_BYTES], const uint8_t* aad, size_t aad_length, const uint8_t* in, uint8_t* out, size_t length, uint8_t tag[TAG_BYTES]) const {
    crypt(iv, in, out, length);
    compute_tag(iv, aad, aad_length, out, length, tag);
}

bool Gcm::decrypt(const uint8_t iv[IV_BYTES], const uint8_t* aad, size_t aad_length, const uint8_t* in, uint8_t* out, size_t length, const uint8_t tag[TAG_BYTES]) const {
    uint8_t expected[TAG_BYTES];
    compute_tag(iv, aad, aad_length, in, length, expected);
    uint8_t diff = 0;  // every byte, so the time doesn't tell where it differs
    for (size_t i = 0; i < TAG_BYTES; i++)
        diff |= expected[i] ^ tag[i];
    if (diff)
        return false;
    crypt(iv, in, out, length);
    return true;
}

void ChatCipher::set_key(const uint8_t key[KEY_BYTES], const uint8_t* seed, size_t seed_length) {
    clear();
    gcm_.set_key(key);
    uint8_t digest[32];
    sha256(seed, seed_length, digest);
    memcpy(run_id_, digest, RUN_ID_BYTES);
    enabled_ = true;
}

void ChatCipher::clear() {
    gcm_.clear();
    enabled_ = false;
    counter_ = 1;
    seen_count_ = 0;
}

std::string ChatCipher::encrypt(const std::string& text, const uint8_t* noise, size_t noise_length) {
    std::string message(OVERHEAD + text.size(), '\0');
    uint8_t* m = reinterpret_cast<uint8_t*>(&message[0]);
    memcpy(m, run_id_, RUN_ID_BYTES);
    m[8] = counter_ >> 24;
    m[9] = counter_ >> 16;
    m[10] = counter_ >> 8;
    m[11] = counter_;

    // A random IV, as the Flipper's: GCM needs it never to repeat under one key. The run id and
    // counter make it differ per message even if the noise were to repeat.
    uint8_t seed[64 + 12];
    const size_t n = noise_length < 64 ? noise_length : 64;
    memcpy(seed, noise, n);
    memcpy(seed + n, m, 12);
    uint8_t digest[32];
    sha256(seed, n + 12, digest);
    uint8_t* iv = m + 12;
    memcpy(iv, digest, IV_BYTES);

    gcm_.encrypt(iv, m, RUN_ID_BYTES + COUNTER_BYTES, reinterpret_cast<const uint8_t*>(text.data()),
                 m + OVERHEAD, text.size(), m + 24);
    set_seen(run_id_, counter_);
    counter_++;
    return message;
}

bool ChatCipher::decrypt(const std::string& message, std::string& text) {
    if (!enabled_ || message.size() < OVERHEAD + 1)
        return false;
    const uint8_t* m = reinterpret_cast<const uint8_t*>(message.data());
    const uint32_t counter = ((uint32_t)m[8] << 24) | ((uint32_t)m[9] << 16) | ((uint32_t)m[10] << 8) | m[11];
    const Seen* seen = find_seen(m);
    if (seen ? seen->counter >= counter : seen_count_ == MAX_SEEN)
        return false;

    text.assign(message.size() - OVERHEAD, '\0');
    if (!gcm_.decrypt(m + 12, m, RUN_ID_BYTES + COUNTER_BYTES, m + OVERHEAD,
                      reinterpret_cast<uint8_t*>(&text[0]), text.size(), m + 24)) {
        text.clear();
        return false;
    }
    set_seen(m, counter);
    return true;
}

ChatCipher::Seen* ChatCipher::find_seen(const uint8_t* run_id) {
    for (size_t i = 0; i < seen_count_; i++)
        if (memcmp(seen_[i].run_id, run_id, RUN_ID_BYTES) == 0)
            return &seen_[i];
    return nullptr;
}

bool ChatCipher::set_seen(const uint8_t* run_id, uint32_t counter) {
    Seen* seen = find_seen(run_id);
    if (!seen) {
        if (seen_count_ == MAX_SEEN)
            return false;
        seen = &seen_[seen_count_++];
        memcpy(seen->run_id, run_id, RUN_ID_BYTES);
    }
    seen->counter = counter;
    return true;
}

bool parse_hex_key(const std::string& text, uint8_t key[KEY_BYTES]) {
    size_t digits = 0;
    for (const char c : text) {
        if (c == ' ')
            continue;
        uint8_t v;
        if (c >= '0' && c <= '9')
            v = c - '0';
        else if (c >= 'a' && c <= 'f')
            v = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            v = c - 'A' + 10;
        else
            return false;
        if (digits >= 2 * KEY_BYTES)
            return false;
        if (digits % 2 == 0)
            key[digits / 2] = v << 4;
        else
            key[digits / 2] |= v;
        digits++;
    }
    return digits == 2 * KEY_BYTES;
}

void wipe(void* p, size_t length) {
    volatile uint8_t* b = static_cast<volatile uint8_t*>(p);
    while (length--)
        *b++ = 0;
}

}  // namespace ui::external_app::subghz_chat::chat_crypto
