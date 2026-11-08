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

#ifndef __CC1101_PACKET_H__
#define __CC1101_PACKET_H__

// Packets as a TI CC1101 sends them in the Flipper Zero's Sub-GHz chat (GFSK 9.99 kbaud,
// 19 kHz deviation): preamble 0xAA x 4, sync word 0x464C, a length byte, up to 60 bytes, a
// CRC-16 over length and data. No whitening, bits most significant first.

#include <cstddef>
#include <cstdint>

// Always inline: a copy out of line would be named cc1101::..., which an external app's linker
// section doesn't take, and would land in the firmware instead of the app.
#define CC1101_INLINE inline __attribute__((always_inline))

namespace cc1101 {

constexpr uint16_t SYNC_WORD = 0x464C;
constexpr size_t PREAMBLE_BYTES = 4;
constexpr size_t MAX_PAYLOAD = 60;
constexpr float BAUD = 9992.6f;
constexpr uint32_t DEVIATION_HZ = 19043;

// CRC-16 of the CC1101: x^16 + x^15 + x^2 + 1, starting at 0xFFFF, most significant bit first.
CC1101_INLINE uint16_t crc16(const uint8_t* data, const size_t length, uint16_t crc = 0xFFFF) {
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (size_t b = 0; b < 8; b++)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x8005 : crc << 1;
    }
    return crc;
}

// Writes bits into a buffer, most significant first, as the FSK transmitter reads them.
class BitWriter {
   public:
    BitWriter(uint8_t* buffer, const size_t capacity_bits)
        : buffer_{buffer}, capacity_{capacity_bits} {}

    CC1101_INLINE bool byte(const uint8_t v) {
        if (bits_ + 8 > capacity_)
            return false;
        for (int i = 7; i >= 0; i--) {
            const size_t n = bits_++;
            if ((v >> i) & 1)
                buffer_[n >> 3] |= 0x80 >> (n & 7);
            else
                buffer_[n >> 3] &= ~(0x80 >> (n & 7));
        }
        return true;
    }

    // One packet: preamble, sync word, length, payload, CRC.
    CC1101_INLINE bool packet(const uint8_t* payload, const size_t length) {
        if (length == 0 || length > MAX_PAYLOAD)
            return false;
        for (size_t i = 0; i < PREAMBLE_BYTES; i++)
            if (!byte(0xAA)) return false;
        if (!byte(SYNC_WORD >> 8) || !byte(SYNC_WORD & 0xFF))
            return false;
        const uint8_t len = length;
        uint16_t crc = crc16(&len, 1);
        crc = crc16(payload, length, crc);
        if (!byte(len))
            return false;
        for (size_t i = 0; i < length; i++)
            if (!byte(payload[i])) return false;
        return byte(crc >> 8) && byte(crc & 0xFF);
    }

    size_t bits() const { return bits_; }

   private:
    uint8_t* buffer_;
    size_t capacity_;
    size_t bits_{0};
};

}  // namespace cc1101

#endif /*__CC1101_PACKET_H__*/
