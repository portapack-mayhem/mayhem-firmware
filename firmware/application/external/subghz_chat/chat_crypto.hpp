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

#ifndef __CHAT_CRYPTO_H__
#define __CHAT_CRYPTO_H__

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

// SHA-256 and AES-256-GCM, small rather than fast: a chat message is a few hundred bytes.
// In the app's namespace, so the linker puts it with the app and not in the firmware.
namespace ui::external_app::subghz_chat::chat_crypto {

constexpr size_t KEY_BYTES = 32;
constexpr size_t IV_BYTES = 12;
constexpr size_t TAG_BYTES = 16;

void sha256(const uint8_t* data, size_t length, uint8_t digest[32]);

class Aes256 {
   public:
    void set_key(const uint8_t key[KEY_BYTES]);
    void encrypt_block(const uint8_t in[16], uint8_t out[16]) const;
    void clear();

   private:
    std::array<uint8_t, 240> round_keys_{};  // 15 round keys of 16 bytes
};

// GCM with a 12-byte IV and a 16-byte tag. in and out may be the same buffer.
class Gcm {
   public:
    void set_key(const uint8_t key[KEY_BYTES]);
    void clear();

    void encrypt(const uint8_t iv[IV_BYTES], const uint8_t* aad, size_t aad_length, const uint8_t* in, uint8_t* out, size_t length, uint8_t tag[TAG_BYTES]) const;
    // false (and out not to be used) when the tag doesn't match
    bool decrypt(const uint8_t iv[IV_BYTES], const uint8_t* aad, size_t aad_length, const uint8_t* in, uint8_t* out, size_t length, const uint8_t tag[TAG_BYTES]) const;

   private:
    Aes256 aes_{};
    std::array<uint8_t, 16> h_{};  // E(K, 0)

    void crypt(const uint8_t iv[IV_BYTES], const uint8_t* in, uint8_t* out, size_t length) const;
    void compute_tag(const uint8_t iv[IV_BYTES], const uint8_t* aad, size_t aad_length, const uint8_t* cipher, size_t length, uint8_t tag[TAG_BYTES]) const;
};

// Overwrites a key or password, not optimised away.
void wipe(void* p, size_t length);

// A chat message as the Flipper's esubghz_chat encrypts it:
// run id (8) | counter (4, big endian) | IV (12) | tag (16) | AES-256-GCM of the text,
// with the run id and counter as additional data. The key is SHA-256 of the password, or 32
// bytes given as hex. A run id and counter that aren't newer than the last ones are a replay;
// every run id seen is kept for as long as the key, and when the table is full new ones are
// refused rather than old ones forgotten.
class ChatCipher {
   public:
    static constexpr size_t RUN_ID_BYTES = 8;
    static constexpr size_t COUNTER_BYTES = 4;
    static constexpr size_t OVERHEAD = RUN_ID_BYTES + COUNTER_BYTES + IV_BYTES + TAG_BYTES;

    bool enabled() const { return enabled_; }
    // seed: random bytes (receiver noise) with anything else that differs per run, for the run id
    void set_key(const uint8_t key[KEY_BYTES], const uint8_t* seed, size_t seed_length);
    void clear();

    // noise: random bytes for the IV, hashed with the run id and counter
    std::string encrypt(const std::string& text, const uint8_t* noise, size_t noise_length);
    // false: not for this key, damaged, a replay, or one sender too many
    bool decrypt(const std::string& message, std::string& text);

   private:
    struct Seen {
        uint8_t run_id[RUN_ID_BYTES];
        uint32_t counter;
    };
    static constexpr size_t MAX_SEEN = 64;

    Gcm gcm_{};
    bool enabled_{false};
    uint8_t run_id_[RUN_ID_BYTES]{};
    uint32_t counter_{1};
    std::array<Seen, MAX_SEEN> seen_{};
    size_t seen_count_{0};

    Seen* find_seen(const uint8_t* run_id);
    bool set_seen(const uint8_t* run_id, uint32_t counter);
};

// 64 hex digits (spaces allowed) to a key
bool parse_hex_key(const std::string& text, uint8_t key[KEY_BYTES]);

}  // namespace ui::external_app::subghz_chat::chat_crypto

#endif /*__CHAT_CRYPTO_H__*/
