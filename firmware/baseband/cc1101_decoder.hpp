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

#ifndef __CC1101_DECODER_H__
#define __CC1101_DECODER_H__

// CC1101 packets (cc1101_packet.hpp) from FM-demodulated samples.

#include <array>
#include <cstddef>
#include <cstdint>

#include "cc1101_packet.hpp"

namespace cc1101 {

class Decoder {
   public:
    // payload of a packet whose CRC matched
    using PacketHandler = void (*)(void* context, const uint8_t* payload, size_t length);

    Decoder(PacketHandler handler, void* context)
        : handler_{handler}, context_{context} {}

    void configure(const float sampling_rate);
    // sample: instantaneous frequency, any scale, positive for a 1
    void execute(const float sample);

    uint32_t packets() const { return packets_; }

   private:
    enum class State { Search,
                       Length,
                       Data };

    PacketHandler handler_;
    void* context_;

    // A tuning error shows as an offset; it is followed while searching (the alternating
    // preamble averages out) and held while a packet comes in.
    float dc_{0.0f};
    float dc_alpha_{0.01f};
    float bit_phase_{0.0f};
    float bit_step_{0.05f};
    bool last_level_{false};

    State state_{State::Search};
    uint32_t shift_{0};
    uint8_t bit_count_{0};
    uint8_t byte_{0};
    size_t length_{0};
    size_t received_{0};
    std::array<uint8_t, MAX_PAYLOAD + 3> buffer_{};  // length, payload, CRC
    uint32_t packets_{0};

    void bit(const uint8_t b);
};

}  // namespace cc1101

#endif /*__CC1101_DECODER_H__*/
