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

#ifndef __LRPT_DECODER_H__
#define __LRPT_DECODER_H__

// Meteor-M LRPT, M0 side: CADU (randomized, after the ASM) in, image strips out.
// Derandomizer, RS(255,223) interleaved by 4 (CCSDS, conventional basis), VCDU and M_PDU,
// CCSDS packets, MSU-MR JPEG (Huffman, quantization from the quality factor, IDCT).
// No dependency on the firmware, so the same code runs in a PC test harness.

#include <cstddef>
#include <cstdint>

#include "lrpt_shared.hpp"

namespace ui::external_app::lrpt_rx {

using lrpt::FRAME_BYTES;
using lrpt::PLANE_BYTES;
using lrpt::PLANES;
using lrpt::STRIP_ROWS;
using lrpt::STRIP_WIDTH;

struct DecoderStats {
    uint32_t frames{0};     // CADUs in
    uint32_t rs_fixed{0};   // with errors corrected
    uint32_t rs_failed{0};  // not correctable, dropped
    uint32_t packets{0};    // image packets decoded
    uint32_t strips{0};     // strips out, gaps included
};

class Decoder {
   public:
    // The planes hold a full strip. They are cleared once the handler returns.
    using StripHandler = void (*)(void* context);

    static constexpr size_t MAX_GAP_STRIPS = 20;

    Decoder(StripHandler handler, void* context);

    // PLANES * PLANE_BYTES, or nullptr to decode nothing into them
    void set_planes(uint8_t* planes);
    // APID shown in red, green and blue (64..69)
    void set_apids(uint8_t red, uint8_t green, uint8_t blue);
    // FRAME_BYTES, changed in place
    void frame(uint8_t* data);
    // hands out the strip being decoded, if any of it came
    void flush();
    // forget the strip position, the next packet starts a new image
    void restart();

    const DecoderStats& stats() const { return stats_; }
    // something of the current strip came, it is not handed out yet
    bool strip_pending() const { return strip_has_data_; }

   private:
    static constexpr size_t MAX_PACKET = 2048;
    static constexpr uint32_t PACKETS_PER_STRIP = 43;  // 14 for each of 3 channels and 1 telemetry

    StripHandler handler_;
    void* context_;
    uint8_t* planes_{nullptr};
    uint8_t apids_[PLANES]{64, 65, 66};
    DecoderStats stats_{};

    uint8_t exp_[256]{};
    uint8_t log_[256]{};
    uint8_t pn_[255]{};

    // VCDU continuity
    bool have_counter_{false};
    uint32_t counter_{0};

    // packet assembly
    bool in_sync_{false};
    size_t packet_length_{0};
    size_t packet_total_{0};
    size_t skip_{0};
    uint8_t packet_[MAX_PACKET]{};

    // strip position from the packet sequence count, shared by the APIDs
    bool have_sequence_{false};
    uint16_t last_sequence_{0};
    uint32_t sequence_{0};
    bool have_phase_{false};
    uint16_t phase_apid_{0};
    uint32_t phase_{0};
    bool have_strip_{false};
    uint32_t strip_{0};
    bool strip_has_data_{false};

    // quantization table for the last quality factor
    uint8_t quality_{0};
    uint16_t quant_[64]{};
    int16_t dc_{0};

    // JPEG bit reader
    const uint8_t* bits_{nullptr};
    size_t bit_count_{0};
    size_t bit_pos_{0};

    uint8_t mul(uint8_t a, uint8_t b) const;
    int rs_decode(uint8_t* cw) const;
    void zone(const uint8_t* data, size_t length, uint16_t first_header);
    void packet(const uint8_t* p, size_t length);
    void emit();
    void segment(const uint8_t* data, size_t length, uint8_t mcu, uint8_t apid);
    bool block(uint8_t* pixels);
    int read_bits(uint8_t n);
    int huffman(const uint8_t* counts, const uint8_t* values);
};

}  // namespace ui::external_app::lrpt_rx

#endif /*__LRPT_DECODER_H__*/
