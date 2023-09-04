/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 *
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

#ifndef __POCSAG_PACKET_H__
#define __POCSAG_PACKET_H__

#include <cstdint>
#include <cstddef>
#include <array>

#include "baseband.hpp"

namespace pocsag {

enum BitRate : uint32_t {
    UNKNOWN,
    FSK512 = 512,
    FSK1200 = 1200,
    FSK2400 = 2400,
    FSK3200 = 3200
};

enum PacketFlag : uint32_t {
    NORMAL,
    TIMED_OUT,
    TOO_LONG
};

/* Number of codewords in a batch. */
constexpr uint8_t batch_size = 16;
using batch_t = std::array<uint32_t, batch_size>;

class POCSAGPacket {
   public:
    void set_timestamp(const Timestamp& value) {
        timestamp_ = value;
    }

    Timestamp timestamp() const {
        return timestamp_;
    }

    void set(size_t index, uint32_t data) {
        if (index < batch_size)
            codewords[index] = data;
    }

    void set(const batch_t& batch) {
        codewords = batch;
    }

    uint32_t operator[](size_t index) const {
        return (index < batch_size) ? codewords[index] : 0;
    }

    void set_bitrate(uint16_t bitrate) {
        bitrate_ = bitrate;
    }

    uint16_t bitrate() const {
        return bitrate_;
    }

    void set_flag(PacketFlag flag) {
        flag_ = flag;
    }

    PacketFlag flag() const {
        return flag_;
    }

    void clear() {
        codewords.fill(0);
        bitrate_ = 0u;
        flag_ = NORMAL;
    }

   private:
    uint16_t bitrate_{0};
    PacketFlag flag_{NORMAL};
    batch_t codewords{};
    Timestamp timestamp_{};
};

} /* namespace pocsag */

#endif /*__POCSAG_PACKET_H__*/
