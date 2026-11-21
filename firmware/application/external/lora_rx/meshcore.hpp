/*
 * LoRa RX - MeshCore packet parsing and group channel decryption (SHA-256, HMAC, AES-128-ECB).
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#ifndef __LORA_RX_MESHCORE_H__
#define __LORA_RX_MESHCORE_H__

#include <cstddef>
#include <cstdint>

namespace ui::external_app::lora_rx {

struct Channel {
    char name[17];
    uint8_t key[16];
    uint8_t hash;
};

void sha256(const uint8_t* data, size_t len, uint8_t out[32]);
void channel_from_name(const char* name, Channel& ch);  // "#name": key = SHA-256(name)[0..15]
void channel_set_key(Channel& ch);                      // computes ch.hash from ch.key

// Decodes a MeshCore packet into two text lines (ASCII, '\0' terminated):
// line1 = packet kind / channel, line2 = message text (or empty). Returns true if decrypted.
bool meshcore_decode(const uint8_t* pkt, int len, const Channel* chans, int nchans, char* line1, int size1, char* line2, int size2);

}  // namespace ui::external_app::lora_rx

#endif
