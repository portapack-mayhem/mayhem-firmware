/*
 * Copyright (C) 2026 Mayhem firmware contributors
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

/* DAB+ superframe (TS 102 563): five 24 ms logical frames holding n interleaved
 * RS(120,110) words, byte j of word i at i + j*n. The header (firecode over the
 * next 9 bytes, a config byte, 12-bit unit start addresses) and the audio units,
 * each with its own CRC-16, are read from the corrected bytes. */

#ifndef __DAB_SUPERFRAME_H__
#define __DAB_SUPERFRAME_H__

#include "dab_constants.hpp"

#include <cstddef>
#include <cstdint>

namespace dab {

constexpr size_t kMaxAudioUnits = 6;

/* Bounded by the payload per CIF, rounded up to 24 so five frames are whole
 * 120-byte RS words. */
constexpr size_t kMaxLogicalFrameBytes = (kMaxPayloadBitsPerCif / 8 + 23) / 24 * 24;
constexpr size_t kMaxSuperframeBytes = kMaxLogicalFrameBytes * kSuperframeLogicalFrames;

constexpr size_t kMaxCorrectedBytes = (kMaxSuperframeBytes / kRsBlockLength) * kRsDataLength;

/* `raw` is superframe_bytes, a non-zero multiple of 120; `out` gets
 * (superframe_bytes / 120) * 110. An uncorrectable word passes through raw for
 * the firecode and unit CRCs to judge. 0 on bad arguments or when `bail` gives
 * up (kRsBailRun in dab_superframe.cpp). */
size_t rs_correct_superframe(const uint8_t* raw, size_t superframe_bytes, uint8_t* out, size_t out_capacity,
                             bool bail = true);

uint8_t rs_last_good_words();

/* Guard that rejected the last call: 1 null argument, 2 not whole code words,
 * 3 output too small, 0 none. */
uint8_t rs_zero_why();
uint8_t rs_last_bad_words();
uint8_t rs_last_total_words();

struct SuperframeHeader {
    uint8_t dac_rate{0};          // 0 = 32 kHz, 1 = 48 kHz
    uint8_t sbr_flag{0};          // spectral band replication
    uint8_t aac_channel_mode{0};  // 0 = mono, 1 = stereo
    uint8_t ps_flag{0};           // parametric stereo
    uint8_t mpeg_surround{0};     // 0 = none
    uint8_t audio_unit_count{0};

    /* dac_rate is the output rate; with SBR the AAC core runs at half. */
    uint32_t output_sample_rate() const {
        return dac_rate ? 48000u : 32000u;
    }
    uint32_t core_sample_rate() const {
        const uint32_t core = dac_rate ? 48000u : 32000u;
        return sbr_flag ? core / 2 : core;
    }
};

struct AudioUnit {
    const uint8_t* data{nullptr};
    size_t length{0};
    bool crc_ok{false};
};

class SuperframeAssembler {
   public:
    /* Payload of one 24 ms logical frame, 1..kMaxLogicalFrameBytes. */
    bool configure(size_t logical_frame_bytes);

    void reset();

    /* True when a firecode-valid superframe has been parsed. Unsynchronised it
     * slides one frame per push, finding the boundary within five frames. */
    bool push_logical_frame(const uint8_t* data);

    const SuperframeHeader& header() const { return header_; }
    const AudioUnit* audio_units() const { return units_; }
    size_t audio_unit_count() const { return unit_count_; }
    bool synchronised() const { return synchronised_; }
    /* The next push completes a superframe and so runs Reed-Solomon over it. */
    bool parse_due() const {
        return logical_frame_bytes_ != 0 && filled_ + logical_frame_bytes_ >= logical_frame_bytes_ * kSuperframeLogicalFrames;
    }

   private:
    bool try_parse(bool full);

    size_t logical_frame_bytes_{0};

    /* rs_fail_: parses with an uncorrectable word (channel quality).
     * firecode_fail_: usually a boundary in the wrong place. */
    uint16_t rs_fail_{0};
    uint16_t firecode_fail_{0};

    /* Firecode passed, header failed a sanity check. */
    uint16_t header_fail_{0};

    uint16_t parse_calls_{0};

    /* RS returned 0 or too few bytes. */
    uint16_t rs_zero_{0};

    /* Boundary tests since the last parse, 0xFF for none. For kSfPhaseMemory
     * superframes after a loss every fifth test (the old phase) runs RS in full
     * instead of bailing (DAB-IMPL 6.105). */
    uint8_t miss_tests_{0xFF};

   public:
    uint16_t rs_failures() const { return rs_fail_; }
    uint16_t firecode_failures() const { return firecode_fail_; }
    uint16_t header_failures() const { return header_fail_; }
    uint16_t parse_calls() const { return parse_calls_; }
    uint16_t rs_zero() const { return rs_zero_; }
    size_t logical_frame_bytes() const { return logical_frame_bytes_; }
    void clear_failures() {
        rs_fail_ = 0;
        firecode_fail_ = 0;
        header_fail_ = 0;
        parse_calls_ = 0;
        rs_zero_ = 0;
    }

   private:
    size_t filled_{0};
    bool synchronised_{false};

    SuperframeHeader header_{};
    AudioUnit units_[kMaxAudioUnits]{};
    size_t unit_count_{0};

    uint8_t buffer_[kMaxSuperframeBytes]{};
    uint8_t corrected_[kMaxCorrectedBytes]{};
};

/* CRC-16-CCITT, inverted, big-endian in the unit's last two bytes. */
bool audio_unit_crc_ok(const uint8_t* unit, size_t length);
void audio_unit_crc_stamp(uint8_t* unit, size_t length);

uint8_t audio_units_for(uint8_t dac_rate, uint8_t sbr_flag);

} /* namespace dab */

#endif /*__DAB_SUPERFRAME_H__*/
