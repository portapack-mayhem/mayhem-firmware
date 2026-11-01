/*
 * Copyright (C) 2026 Xmoo26
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

#ifndef __VDL2_DECODER_H__
#define __VDL2_DECODER_H__

// VHF Data Link mode 2 (ICAO Annex 10 vol. III, ETSI EN 301 841-1), receive side:
// D8PSK at 10500 symbols/s, a training sequence, a 25-bit header with its own FEC, the data
// scrambled (x^15 + x + 1), in RS(255,249) blocks interleaved byte by byte, carrying HDLC
// framed AVLC frames with an X.25 FCS.

#include <array>
#include <complex>
#include <cstddef>
#include <cstdint>

namespace vdl2 {

// Longest transmission taken, in bits of AVLC frames; longer ones are almost always a false sync.
constexpr size_t MAX_DATA_BITS = 0x1FFF;
constexpr size_t MAX_DATA_OCTETS = (MAX_DATA_BITS + 7) / 8;
constexpr size_t RS_N = 255;
constexpr size_t RS_K = 249;
constexpr size_t MAX_BLOCKS = (MAX_DATA_OCTETS + RS_K - 1) / RS_K;

// RS(255,249) over GF(256) with x^8 + x^7 + x^2 + x + 1, roots alpha^120..125.
class ReedSolomon {
   public:
    ReedSolomon();
    // Corrects the codeword in place (data[0] is the highest power); erasures are indices.
    // Returns the number of symbols corrected, or -1 when it can't be done.
    int decode(uint8_t* data, const uint8_t* erasures, const size_t erasure_count) const;

   private:
    static constexpr size_t NROOTS = RS_N - RS_K;
    static constexpr uint8_t FCR = 120;
    std::array<uint8_t, 512> exp_{};
    std::array<uint8_t, 256> log_{};

    uint8_t mul(const uint8_t a, const uint8_t b) const;
    uint8_t div(const uint8_t a, const uint8_t b) const;
    uint8_t pow(const int e) const;
};

// Bits after the training sequence in, AVLC frames out.
class BurstDecoder {
   public:
    // frame without its flags and FCS
    using FrameHandler = void (*)(void* context, const uint8_t* frame, size_t length);

    BurstDecoder(FrameHandler handler, void* context);

    void start();
    // false when the burst is over (decoded, or given up on)
    bool bit(const uint8_t b);

    uint32_t frames() const { return frames_; }
    uint32_t bursts() const { return bursts_; }

   private:
    enum class State { Header,
                       Data };

    FrameHandler handler_;
    void* context_;
    ReedSolomon rs_{};
    std::array<uint32_t, 32> header_errors_{};  // syndrome -> the single bit error it means

    State state_{State::Header};
    uint16_t lfsr_{0};
    uint32_t header_{0};
    size_t count_{0};
    size_t data_bits_{0};
    size_t octets_{0};
    size_t blocks_{0};
    size_t last_block_{0};
    size_t fec_octets_{0};
    size_t last_fec_{0};
    std::array<uint8_t, MAX_DATA_OCTETS + MAX_BLOCKS*(RS_N - RS_K)> buffer_{};
    std::array<std::array<uint8_t, RS_N>, MAX_BLOCKS> blocks_table_{};
    std::array<uint8_t, MAX_DATA_OCTETS> frame_{};
    uint32_t frames_{0};
    uint32_t bursts_{0};

    bool header_done();
    void decode_data();
    void deframe(const uint8_t* data, const size_t bits);
};

// 48 kHz complex baseband in (the channel filtered, the station near 0 Hz), bits out.
class Demodulator {
   public:
    explicit Demodulator(BurstDecoder& burst)
        : burst_{burst} {}

    void execute(const std::complex<float> sample);

    // frequency offset seen at the last sync, in Hz
    float last_offset_hz() const { return last_offset_hz_; }
    uint32_t syncs() const { return syncs_; }

   private:
    static constexpr size_t BUFFER = 128;  // a power of two, more than the 16 preamble symbols
    static constexpr size_t PREAMBLE = 16;
    static constexpr float FS = 48000.0f;
    static constexpr float SPS = FS / 10500.0f;  // 32/7
    static constexpr float SYNC_THRESHOLD = 2.0f;

    enum class State { Search,
                       Symbols };

    BurstDecoder& burst_;
    State state_{State::Search};
    uint32_t n_{0};
    std::array<std::complex<float>, BUFFER> iq_{};
    std::array<float, BUFFER> phase_{};
    float power_{0.0f};
    float noise_{1e9f};

    // sync search: the metric of the last samples, to find its minimum
    float metric_[3]{1e9f, 1e9f, 1e9f};
    float slope_[3]{0.0f, 0.0f, 0.0f};

    uint32_t next_n_{0};  // the next symbol is at sample next_n_ + next_frac_
    float next_frac_{0.0f};
    float prev_phase_{0.0f};
    float phase_step_{0.0f};  // carrier offset per symbol
    float last_offset_hz_{0.0f};
    uint32_t syncs_{0};

    float preamble_metric(const uint32_t end, float& slope) const;
    std::complex<float> at(const uint32_t n, const float fraction) const;
    void on_sync(const uint32_t n, const float fraction, const float slope);
    void symbols();
};

}  // namespace vdl2

#endif /*__VDL2_DECODER_H__*/
