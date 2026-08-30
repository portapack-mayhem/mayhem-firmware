/*
 * Copyright (C) 2026 Alexey Verhogladov
 *
 * This file is part of PortaPack.
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Compact software AES-128/256 for Meshtastic CTR-mode encryption.
 * Based on FIPS-197 AES specification.
 * This runs on the M0 application core, not real-time - one small packet at a time.
 */

#include "mesh_crypto.hpp"
#include <cstring>

namespace meshtastic {

// ---- AES S-Box (forward only, CTR mode only needs encrypt) ---------------

static constexpr uint8_t SBOX[256] = {
    0x63,
    0x7c,
    0x77,
    0x7b,
    0xf2,
    0x6b,
    0x6f,
    0xc5,
    0x30,
    0x01,
    0x67,
    0x2b,
    0xfe,
    0xd7,
    0xab,
    0x76,
    0xca,
    0x82,
    0xc9,
    0x7d,
    0xfa,
    0x59,
    0x47,
    0xf0,
    0xad,
    0xd4,
    0xa2,
    0xaf,
    0x9c,
    0xa4,
    0x72,
    0xc0,
    0xb7,
    0xfd,
    0x93,
    0x26,
    0x36,
    0x3f,
    0xf7,
    0xcc,
    0x34,
    0xa5,
    0xe5,
    0xf1,
    0x71,
    0xd8,
    0x31,
    0x15,
    0x04,
    0xc7,
    0x23,
    0xc3,
    0x18,
    0x96,
    0x05,
    0x9a,
    0x07,
    0x12,
    0x80,
    0xe2,
    0xeb,
    0x27,
    0xb2,
    0x75,
    0x09,
    0x83,
    0x2c,
    0x1a,
    0x1b,
    0x6e,
    0x5a,
    0xa0,
    0x52,
    0x3b,
    0xd6,
    0xb3,
    0x29,
    0xe3,
    0x2f,
    0x84,
    0x53,
    0xd1,
    0x00,
    0xed,
    0x20,
    0xfc,
    0xb1,
    0x5b,
    0x6a,
    0xcb,
    0xbe,
    0x39,
    0x4a,
    0x4c,
    0x58,
    0xcf,
    0xd0,
    0xef,
    0xaa,
    0xfb,
    0x43,
    0x4d,
    0x33,
    0x85,
    0x45,
    0xf9,
    0x02,
    0x7f,
    0x50,
    0x3c,
    0x9f,
    0xa8,
    0x51,
    0xa3,
    0x40,
    0x8f,
    0x92,
    0x9d,
    0x38,
    0xf5,
    0xbc,
    0xb6,
    0xda,
    0x21,
    0x10,
    0xff,
    0xf3,
    0xd2,
    0xcd,
    0x0c,
    0x13,
    0xec,
    0x5f,
    0x97,
    0x44,
    0x17,
    0xc4,
    0xa7,
    0x7e,
    0x3d,
    0x64,
    0x5d,
    0x19,
    0x73,
    0x60,
    0x81,
    0x4f,
    0xdc,
    0x22,
    0x2a,
    0x90,
    0x88,
    0x46,
    0xee,
    0xb8,
    0x14,
    0xde,
    0x5e,
    0x0b,
    0xdb,
    0xe0,
    0x32,
    0x3a,
    0x0a,
    0x49,
    0x06,
    0x24,
    0x5c,
    0xc2,
    0xd3,
    0xac,
    0x62,
    0x91,
    0x95,
    0xe4,
    0x79,
    0xe7,
    0xc8,
    0x37,
    0x6d,
    0x8d,
    0xd5,
    0x4e,
    0xa9,
    0x6c,
    0x56,
    0xf4,
    0xea,
    0x65,
    0x7a,
    0xae,
    0x08,
    0xba,
    0x78,
    0x25,
    0x2e,
    0x1c,
    0xa6,
    0xb4,
    0xc6,
    0xe8,
    0xdd,
    0x74,
    0x1f,
    0x4b,
    0xbd,
    0x8b,
    0x8a,
    0x70,
    0x3e,
    0xb5,
    0x66,
    0x48,
    0x03,
    0xf6,
    0x0e,
    0x61,
    0x35,
    0x57,
    0xb9,
    0x86,
    0xc1,
    0x1d,
    0x9e,
    0xe1,
    0xf8,
    0x98,
    0x11,
    0x69,
    0xd9,
    0x8e,
    0x94,
    0x9b,
    0x1e,
    0x87,
    0xe9,
    0xce,
    0x55,
    0x28,
    0xdf,
    0x8c,
    0xa1,
    0x89,
    0x0d,
    0xbf,
    0xe6,
    0x42,
    0x68,
    0x41,
    0x99,
    0x2d,
    0x0f,
    0xb0,
    0x54,
    0xbb,
    0x16,
};

static constexpr uint8_t RCON[11] = {
    0x00,
    0x01,
    0x02,
    0x04,
    0x08,
    0x10,
    0x20,
    0x40,
    0x80,
    0x1B,
    0x36,
};

// GF(2^8) multiply by 2
static inline uint8_t xtime(uint8_t a) {
    return (a & 0x80) ? ((a << 1) ^ 0x1B) : (a << 1);
}

uint32_t MeshCrypto::sub_word(uint32_t w) {
    return (static_cast<uint32_t>(SBOX[(w >> 24) & 0xFF]) << 24) |
           (static_cast<uint32_t>(SBOX[(w >> 16) & 0xFF]) << 16) |
           (static_cast<uint32_t>(SBOX[(w >> 8) & 0xFF]) << 8) |
           static_cast<uint32_t>(SBOX[(w) & 0xFF]);
}

uint32_t MeshCrypto::rot_word(uint32_t w) {
    return (w << 8) | (w >> 24);
}

// AES-128 key expansion -> 11 round keys (44 words)
void MeshCrypto::key_expand_128(uint32_t* rk) const {
    for (int i = 0; i < 4; i++) {
        rk[i] = (key_[4 * i] << 24) | (key_[4 * i + 1] << 16) | (key_[4 * i + 2] << 8) | key_[4 * i + 3];
    }
    for (int i = 4; i < 44; i++) {
        uint32_t tmp = rk[i - 1];
        if (i % 4 == 0) tmp = sub_word(rot_word(tmp)) ^ (static_cast<uint32_t>(RCON[i / 4]) << 24);
        rk[i] = rk[i - 4] ^ tmp;
    }
}

// AES-256 key expansion -> 15 round keys (60 words)
void MeshCrypto::key_expand_256(uint32_t* rk) const {
    for (int i = 0; i < 8; i++) {
        rk[i] = (key_[4 * i] << 24) | (key_[4 * i + 1] << 16) | (key_[4 * i + 2] << 8) | key_[4 * i + 3];
    }
    for (int i = 8; i < 60; i++) {
        uint32_t tmp = rk[i - 1];
        if (i % 8 == 0)
            tmp = sub_word(rot_word(tmp)) ^ (static_cast<uint32_t>(RCON[i / 8]) << 24);
        else if (i % 8 == 4)
            tmp = sub_word(tmp);
        rk[i] = rk[i - 8] ^ tmp;
    }
}

// One AES round: SubBytes + ShiftRows + MixColumns + AddRoundKey
// We operate on state as 4 uint32_t words (row-major).
void MeshCrypto::aes_add_round_key(uint32_t* state, const uint32_t* rk) {
    state[0] ^= rk[0];
    state[1] ^= rk[1];
    state[2] ^= rk[2];
    state[3] ^= rk[3];
}

uint32_t MeshCrypto::mix_column(uint32_t word) {
    uint8_t s0 = (word >> 24) & 0xFF, s1 = (word >> 16) & 0xFF, s2 = (word >> 8) & 0xFF, s3 = word & 0xFF;
    uint8_t r0 = xtime(s0) ^ xtime(s1) ^ s1 ^ s2 ^ s3;
    uint8_t r1 = s0 ^ xtime(s1) ^ xtime(s2) ^ s2 ^ s3;
    uint8_t r2 = s0 ^ s1 ^ xtime(s2) ^ xtime(s3) ^ s3;
    uint8_t r3 = xtime(s0) ^ s0 ^ s1 ^ s2 ^ xtime(s3);
    return (static_cast<uint32_t>(r0) << 24) | (static_cast<uint32_t>(r1) << 16) |
           (static_cast<uint32_t>(r2) << 8) | r3;
}

void MeshCrypto::aes_ecb_encrypt(const uint8_t* in, uint8_t* out) const {
    // Load input into 4x4 state (column-major per FIPS-197, but we use row-major words)
    uint32_t state[4];
    for (int i = 0; i < 4; i++) {
        state[i] = (static_cast<uint32_t>(in[4 * i]) << 24) |
                   (static_cast<uint32_t>(in[4 * i + 1]) << 16) |
                   (static_cast<uint32_t>(in[4 * i + 2]) << 8) |
                   static_cast<uint32_t>(in[4 * i + 3]);
    }

    const int nr = (key_len_ == 32) ? 14 : 10;
    uint32_t rk[60]{};
    if (key_len_ == 32)
        key_expand_256(rk);
    else
        key_expand_128(rk);

    aes_add_round_key(state, rk);

    // state[c] = column c of the AES state matrix (column-major, per FIPS-197).
    // SubBytes: replace each byte with SBOX[byte] - applies to all 16 bytes.
    // ShiftRows: new_state[c] byte r = old_state[(c-r+4)%4] byte r.
    // MixColumns: each column mixed independently.

    auto sub_bytes = [&]() {
        for (int c = 0; c < 4; c++) {
            state[c] = (static_cast<uint32_t>(SBOX[(state[c] >> 24) & 0xFF]) << 24) |
                       (static_cast<uint32_t>(SBOX[(state[c] >> 16) & 0xFF]) << 16) |
                       (static_cast<uint32_t>(SBOX[(state[c] >> 8) & 0xFF]) << 8) |
                       static_cast<uint32_t>(SBOX[(state[c]) & 0xFF]);
        }
    };

    auto shift_rows = [&]() {
        uint32_t tmp[4];
        for (int c = 0; c < 4; c++) {
            tmp[c] = 0;
            for (int r = 0; r < 4; r++) {
                const int sh = 24 - r * 8;
                const uint8_t b = static_cast<uint8_t>(state[(c + r) % 4] >> sh);
                tmp[c] |= static_cast<uint32_t>(b) << sh;
            }
        }
        for (int c = 0; c < 4; c++) state[c] = tmp[c];
    };

    auto mix_columns = [&]() {
        for (int c = 0; c < 4; c++) {
            const uint8_t s0 = static_cast<uint8_t>(state[c] >> 24);
            const uint8_t s1 = static_cast<uint8_t>(state[c] >> 16);
            const uint8_t s2 = static_cast<uint8_t>(state[c] >> 8);
            const uint8_t s3 = static_cast<uint8_t>(state[c]);
            state[c] = (static_cast<uint32_t>(xtime(s0) ^ xtime(s1) ^ s1 ^ s2 ^ s3) << 24) |
                       (static_cast<uint32_t>(s0 ^ xtime(s1) ^ xtime(s2) ^ s2 ^ s3) << 16) |
                       (static_cast<uint32_t>(s0 ^ s1 ^ xtime(s2) ^ xtime(s3) ^ s3) << 8) |
                       static_cast<uint32_t>(xtime(s0) ^ s0 ^ s1 ^ s2 ^ xtime(s3));
        }
    };

    for (int round = 1; round < nr; round++) {
        sub_bytes();
        shift_rows();
        mix_columns();
        aes_add_round_key(state, rk + 4 * round);
    }
    // Final round: no MixColumns
    sub_bytes();
    shift_rows();
    aes_add_round_key(state, rk + 4 * nr);

    // Store state to output
    for (int i = 0; i < 4; i++) {
        out[4 * i] = static_cast<uint8_t>(state[i] >> 24);
        out[4 * i + 1] = static_cast<uint8_t>(state[i] >> 16);
        out[4 * i + 2] = static_cast<uint8_t>(state[i] >> 8);
        out[4 * i + 3] = static_cast<uint8_t>(state[i]);
    }
}

// ---- Public API ------------------------------------------------------------

void MeshCrypto::set_key(const uint8_t* key, size_t key_len) {
    if (!key || (key_len != 16 && key_len != 32)) {
        key_len_ = 0;  // no/invalid key -> plaintext mode
        return;
    }
    key_len_ = key_len;
    memcpy(key_, key, key_len_);
}

void MeshCrypto::crypt(uint8_t* data, size_t len, uint32_t packet_id, uint32_t from_node) {
    // Meshtastic nonce: packet_id LE[0-3] | zeros[4-7] | from_node LE[8-11] | block_ctr LE[12-15]
    uint8_t nonce[AES_BLOCK]{};
    nonce[0] = packet_id;
    nonce[1] = packet_id >> 8;
    nonce[2] = packet_id >> 16;
    nonce[3] = packet_id >> 24;
    // bytes 4-7 stay 0 (high 32 bits of 64-bit packet_id in Meshtastic nonce spec)
    nonce[8] = from_node;
    nonce[9] = from_node >> 8;
    nonce[10] = from_node >> 16;
    nonce[11] = from_node >> 24;

    // AES-CTR: XOR data with AES_ECB(key, nonce+counter).
    // Meshtastic uses a 4-byte counter in nonce[12..15] incremented BIG-ENDIAN
    // (CryptoEngine: ctr->setCounterSize(4)); nonce[15] is the LSB. Writing it
    // little-endian only agreed on block 0 (counter 0), so every message past
    // the first 16 bytes decrypted to garbage on real nodes.
    uint8_t keystream[AES_BLOCK];
    uint32_t block_counter = 0;
    size_t i = 0;
    while (i < len) {
        nonce[15] = block_counter;
        nonce[14] = block_counter >> 8;
        nonce[13] = block_counter >> 16;
        nonce[12] = block_counter >> 24;
        aes_ecb_encrypt(nonce, keystream);
        for (size_t j = 0; j < AES_BLOCK && i < len; j++, i++) {
            data[i] ^= keystream[j];
        }
        block_counter++;
    }
}

}  // namespace meshtastic
