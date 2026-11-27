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

/* DAB+ audio transport (M4): Reed-Solomon, superframe sync and access
 * units, and the PCM ring the audio interrupt drains. */

#ifndef __DAB_PLUS_H__
#define __DAB_PLUS_H__

#include "dab_common.hpp"

#include <cstddef>
#include <cstdint>

namespace dab {

/* ---- Reed-Solomon ---- */

/* RS(120,110) for DAB+ (TS 102 563): GF(256), primitive polynomial 0x11D,
 * g(x) = product of (x - a^i), i = 0..9; t = 5. Shortened RS(255,245): the 135
 * leading bytes are zero and never transmitted. */

constexpr size_t kRsN = kRsBlockLength;
constexpr size_t kRsK = kRsDataLength;
constexpr size_t kRsParity = kRsN - kRsK;
constexpr size_t kRsT = kRsParity / 2;

class ReedSolomon {
   public:
    ReedSolomon();

    /* In place. Returns errors corrected, or -1 if uncorrectable; the block
     * may then be partly modified. */
    int decode(uint8_t block[kRsN]) const;

    /* Fills block[110..119] from the data bytes. Tests only. */
    void encode(uint8_t block[kRsN]);

   private:
    uint8_t exp_[512];
    uint8_t log_[256];
    uint8_t generator_[kRsParity + 1];

    /* mul(v, a^i) for every v, one table per syndrome (2560 bytes): decode()
     * takes all the syndromes in one pass, a table read per byte each. */
    uint8_t root_mul_[kRsParity][256];

    uint8_t mul(uint8_t a, uint8_t b) const {
        if (a == 0 || b == 0)
            return 0;
        return exp_[log_[a] + log_[b]];
    }

    uint8_t div(uint8_t a, uint8_t b) const {
        if (a == 0)
            return 0;
        return exp_[log_[a] + 255 - log_[b]];
    }

    uint8_t pow_alpha(int e) const {
        e %= 255;
        if (e < 0)
            e += 255;
        return exp_[e];
    }
};

/* ---- DAB+ superframe ---- */

/* DAB+ superframe (TS 102 563): five 24 ms logical frames holding n interleaved
 * RS(120,110) words, byte j of word i at i + j*n. The header (firecode over the
 * next 9 bytes, a config byte, 12-bit unit start addresses) and the audio units,
 * each with its own CRC-16, are read from the corrected bytes. */

constexpr size_t kMaxAudioUnits = 6;

/* Bounded by the payload per CIF, rounded up to 24 so five frames are whole
 * 120-byte RS words. */
constexpr size_t kMaxLogicalFrameBytes = (kMaxPayloadBitsPerCif / 8 + 23) / 24 * 24;
constexpr size_t kMaxSuperframeBytes = kMaxLogicalFrameBytes * kSuperframeLogicalFrames;

constexpr size_t kMaxCorrectedBytes = (kMaxSuperframeBytes / kRsBlockLength) * kRsDataLength;

/* `raw` is superframe_bytes, a non-zero multiple of 120; `out` gets
 * (superframe_bytes / 120) * 110. An uncorrectable word passes through raw for
 * the firecode and unit CRCs to judge. 0 on bad arguments or when `bail` gives
 * up (kRsBailRun in dab_plus.cpp). */
size_t rs_correct_superframe(const uint8_t* raw, size_t superframe_bytes, uint8_t* out, size_t out_capacity, bool bail = true);

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
     * instead of bailing. */
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

/* ---- Audio ring ---- */

/* DAB+ audio hand-over: the codec pulls. The audio DMA's transfer-complete ISR
 * calls render() per free transfer, so the decoder's bursts need only keep two
 * 24 kHz slots (80 ms) from running dry. Single producer (thread), single
 * consumer (ISR), one core: a compiler barrier orders samples before commit. */

class AudioRing {
    static void barrier() { __asm__ volatile("" ::: "memory"); }

   public:
    static constexpr size_t kCore = 960; /* one AAC frame, 40 ms at 24 kHz */
    static constexpr size_t kSlots = 2;
    static constexpr size_t kStorage = kCore * kSlots;

    /* Gain ramp in core samples (2.7 ms): a hole fades out from the last
     * sample and the audio after it fades back in, so neither edge steps. */
    static constexpr int32_t kFadeShift = 6;
    static constexpr int32_t kFade = 1 << kFadeShift;

    void attach(int16_t* storage) { storage_ = storage; }

    /* Baseband thread side. */

    uint32_t queued() const { return written_ - read_; }
    size_t free_slots() const { return kSlots - queued(); }

    int16_t* writable() const {
        if (queued() >= kSlots)
            return nullptr;
        return storage_ + (written_ % kSlots) * kCore;
    }

    /* The slot writable() returned must now hold kCore samples. */
    void commit() {
        barrier();
        written_ = written_ + 1;
    }

    /* Counters the ISR keeps; the thread reads and differences them. */
    uint32_t dry_transfers() const { return dry_transfers_; }
    uint32_t frames_played() const { return read_; }

    /* Audio DMA interrupt side. */

    /* n frames at twice the core rate, same sample on both channels. Even
     * outputs are the core sample, odd ones (-x[-1] + 9 x[0] + 9 x[1] - x[2]) / 16,
     * ~12 dB more image rejection than linear. The run loop is a fast path that
     * must match the per-sample step below. */
    void render(uint32_t* out, size_t n) {
        const uint32_t written = written_;
        uint32_t read = read_;
        uint32_t pos = pos_;
        int32_t gain = gain_, last = last_;
        int32_t d0 = d0_, d1 = d1_, d2 = d2_, d3 = d3_;
        bool dry = false;
        const int16_t* frame = storage_ + (read % kSlots) * kCore;
        uint32_t* const end = out + (n & ~size_t(1));

        while (out != end) {
            if (read != written && gain == kFade) {
                size_t run = size_t(end - out) / 2;
                if (run > kCore - pos)
                    run = kCore - pos;
                const int16_t* src = frame + pos;
                const int16_t* const stop = src + run;
                while (src != stop) {
                    d0 = d1;
                    d1 = d2;
                    d2 = d3;
                    d3 = *src++;
                    out[0] = pack(d1);
                    out[1] = pack(saturate((9 * (d1 + d2) - (d0 + d3) + 8) >> 4));
                    out += 2;
                }
                pos += run;
                last = d3;
                if (pos == kCore) {
                    pos = 0;
                    ++read;
                    frame = storage_ + (read % kSlots) * kCore;
                }
                continue;
            }

            int32_t c;
            if (read != written) {
                const int32_t s = frame[pos];
                if (++pos == kCore) {
                    pos = 0;
                    ++read;
                    frame = storage_ + (read % kSlots) * kCore;
                }
                last = s;
                if (gain < kFade)
                    ++gain;
                c = (s * gain) >> kFadeShift;
            } else {
                dry = true;
                if (gain > 0)
                    --gain;
                c = (last * gain) >> kFadeShift;
            }
            d0 = d1;
            d1 = d2;
            d2 = d3;
            d3 = c;
            out[0] = pack(d1);
            out[1] = pack(saturate((9 * (d1 + d2) - (d0 + d3) + 8) >> 4));
            out += 2;
        }

        read_ = read;
        pos_ = pos;
        gain_ = gain;
        last_ = last;
        d0_ = d0;
        d1_ = d1;
        d2_ = d2;
        d3_ = d3;
        if (dry)
            dry_transfers_ = dry_transfers_ + 1;
    }

    static uint32_t pack(int32_t v) {
        return uint32_t(uint16_t(v)) * 0x00010001u;
    }

    static int32_t saturate(int32_t v) {
#if defined(__ARM_FEATURE_SAT) || defined(__ARM_ARCH_7EM__)
        int32_t r;
        __asm__("ssat %0, #16, %1" : "=r"(r) : "r"(v));
        return r;
#else
        if (v > 32767)
            return 32767;
        if (v < -32768)
            return -32768;
        return v;
#endif
    }

   private:
    int16_t* storage_{nullptr};
    volatile uint32_t written_{0}; /* frames committed, thread-owned */
    volatile uint32_t read_{0};    /* frames consumed, ISR-owned */
    volatile uint32_t dry_transfers_{0};

    /* ISR-only state. */
    uint32_t pos_{0};
    int32_t gain_{0};
    int32_t last_{0};
    int32_t d0_{0}, d1_{0}, d2_{0}, d3_{0};
};

} /* namespace dab */

#endif /*__DAB_PLUS_H__*/
