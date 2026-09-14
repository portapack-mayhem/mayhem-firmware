/*
 * Copyright (C) 2026 Alexey Verhogladov
 *
 * This file is part of PortaPack.
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Meshtastic AES-256-CTR channel encryption.
 *
 * Nonce layout (128-bit):
 *   [0..3]   packet_id  (LE uint32)
 *   [4..7]   zero padding (high 32-bits of 64-bit packet_id)
 *   [8..11]  from_node  (LE uint32)
 *   [12..15] block_counter (LE uint32, increments per 16-byte block)
 *
 * Channel PSK is the raw AES key (16 or 32 bytes).
 * Default channel PSK (base64 "AQ=="-> expanded):
 *   {0x01, 0x00, 0x00, ..., 0x00}  (16 bytes, AES-128)
 *
 * Reference: meshtastic/firmware mesh/CryptoEngine.cpp
 */

#ifndef __MESH_CRYPTO_H__
#define __MESH_CRYPTO_H__

#include <cstdint>
#include <cstring>
#include <array>

namespace meshtastic {

// Well-known default channel PSK (AES-128, "LongFast" channel).
// Decoded from base64 "1PG7OiApB1nwvP+rz05pAQ==" (Meshtastic default).
static constexpr uint8_t DEFAULT_PSK[16] = {
    0xd4, 0xf1, 0xbb, 0x3a, 0x20, 0x29, 0x07, 0x59,
    0xf0, 0xbc, 0xff, 0xab, 0xcf, 0x4e, 0x69, 0x01,  // well-known "AQ==" default key (last byte 0x01, was 0xbf)
};

// "Admin" channel uses an all-zero key (unencrypted).
static constexpr uint8_t ADMIN_PSK[16] = {0};

class MeshCrypto {
   public:
    // Configure with a 16 or 32 byte PSK.
    void set_key(const uint8_t* key, size_t key_len);

    // In-place encrypt or decrypt (AES-CTR is symmetric).
    // packet_id and from_node are used to build the nonce.
    void crypt(uint8_t* data, size_t len, uint32_t packet_id, uint32_t from_node);

    bool has_key() const { return key_len_ > 0; }

   private:
    uint8_t key_[32]{};
    size_t key_len_{16};

    // Compact software AES-128/256 (encrypt-only, needed for CTR mode).
    static constexpr size_t AES_BLOCK = 16;

    void aes_ecb_encrypt(const uint8_t* in, uint8_t* out) const;
    void key_expand_128(uint32_t* rk) const;
    void key_expand_256(uint32_t* rk) const;

    static uint32_t sub_word(uint32_t w);
    static uint32_t rot_word(uint32_t w);
    static void aes_round(uint32_t* state, const uint32_t* rk);
    static void aes_add_round_key(uint32_t* state, const uint32_t* rk);
    static uint32_t mix_column(uint32_t word);
};

}  // namespace meshtastic

#endif /* __MESH_CRYPTO_H__ */
