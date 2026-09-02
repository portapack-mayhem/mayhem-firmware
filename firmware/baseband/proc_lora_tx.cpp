/*
 * Copyright (C) 2026 Alexey Verhogladov
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
 * LoRa PHY. The format this implements - LoRa's physical layer - is not documented
 * by its vendor; what is public comes from reverse engineering, chiefly Robyns et
 * al. (2016) and the gr-lora / gr-lora_sdr projects. This frame layer follows
 * gr-lora_sdr, and it is worth being precise about how closely, since a reviewer
 * comparing them will see it:
 *
 *   - the diagonal interleaver follows gr-lora_sdr's implementation directly -
 *     same loop structure, same (i - j - 1) mod n index, same appended parity bit
 *     under reduced rate;
 *   - the explicit-header checksum uses the same equations, which are the format
 *     itself, written differently here;
 *   - the whitening is done with the LFSR rather than their lookup table, and the
 *     Hamming layer is a different code word width with different equations.
 *
 * gr-lora_sdr is GPL-3, as is this project, so this is attribution rather than a
 * licensing question.
 *
 * Verified against traffic from a stock Meshtastic node; the recorded frame that
 * pins it down is in tools/lora_bench/test_lora_framing.cpp.
 */

#include "proc_lora_tx.hpp"
#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"
#include "sine_table_int8.hpp"
#include "lora_framing.hpp"
#include "message.hpp"

#include <cmath>
#include <algorithm>
#include <cstring>

// --- Whitening LFSR -----------------------------------------------------------
uint8_t LoRaTXProcessor::whiten_step() {
    return lora::whiten_next(whiten_state_);
}

// --- encode chain (verified bit-exact against a real Heltec V4) ---------------
// The frame layer itself lives in common/lora_framing.hpp, shared with the
// receiver; see the file header for where the format comes from. It used to sit here in full, and a second copy sat in proc_lora.cpp;
// keeping the two in step by hand is what failed when LDRO arrived - this side
// computed the flag and then coded the payload at full rate anyway.
namespace {

// LoRa payload CRC-16 (poly 0x1021, init 0, over len-2 bytes, XOR with last 2 bytes).
inline uint16_t crc16_step(uint16_t crc, uint8_t b) {
    for (int i = 0; i < 8; i++) {
        if (((crc & 0x8000) >> 8) ^ (b & 0x80))
            crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
        else
            crc = static_cast<uint16_t>(crc << 1);
        b = static_cast<uint8_t>(b << 1);
    }
    return crc;
}
inline uint16_t mesh_crc(const uint8_t* p, int len) {
    uint16_t crc = 0;
    for (int i = 0; i < len - 2; i++) crc = crc16_step(crc, p[i]);
    return static_cast<uint16_t>(crc ^ p[len - 1] ^ (p[len - 2] << 8));
}

}  // namespace

// --- Frame builder ------------------------------------------------------------
void LoRaTXProcessor::push_sym(uint16_t chip, bool up, bool quarter) {
    if (frame_len_ < MAX_FRAME_SYMS)
        frame_[frame_len_++] = {chip, up, quarter};
}

// Frame = preamble(16) + sync 0x2B(2) + 2.25 downchirp SFD + header(8) + payload.
// The PHY CRC-16 is appended here; whitening is applied to the payload only.
void LoRaTXProcessor::build_frame(const uint8_t* payload, size_t payload_len) {
    frame_len_ = 0;

    const int sf = static_cast<int>(spreading_factor);
    const int N = static_cast<int>(chips_per_symbol);
    const int cr_code = static_cast<int>(coding_rate) - 4;  // 1=CR4/5 ... 4=CR4/8
    const int cw_len = static_cast<int>(coding_rate);       // 5 for CR4/5
    const int len = static_cast<int>(payload_len);

    // 1. preamble (16 upchirps at chip 0)
    for (int i = 0; i < 16; i++) push_sym(0, true);
    // 2. sync word 0x2B -> the two network-id upchirps sit at FIXED raw symbol
    //    values nibble*8 (16 and 88), the same at every SF - NOT nibble<<(SF-4).
    //    The old SF-scaled form gave 16/88 at SF7 (so ShortTurbo TX worked) but
    //    256/1408 at SF11, so a hardware SX126x (Heltec) rejected our LongFast
    //    packets on the sync-word check -> TX "didn't work" on every non-SF7 preset.
    push_sym(static_cast<uint16_t>(0x2u << 3), true);  // 16
    push_sym(static_cast<uint16_t>(0xBu << 3), true);  // 88
    // 3. SFD: 2.25 downchirps (2 full + one quarter-length)
    push_sym(0, false);
    push_sym(0, false);
    push_sym(0, false, true);

    // -- nibble stream: 5 header nibbles + whitened payload + 4 CRC nibbles --
    static constexpr int MAX_NIB = 560;  // covers a 255-byte payload
    uint8_t nib[MAX_NIB];
    int nn = 0;
    const uint8_t h[3] = {static_cast<uint8_t>(len >> 4), static_cast<uint8_t>(len & 0xF),
                          static_cast<uint8_t>((cr_code << 1) | 1)};  // CRC present
    uint8_t c4, clo;
    lora::header_checksum(h[0], h[1], h[2], &c4, &clo);
    nib[nn++] = h[0];
    nib[nn++] = h[1];
    nib[nn++] = h[2];
    nib[nn++] = c4;
    nib[nn++] = clo;
    whiten_state_ = 0xFF;
    for (int i = 0; i < len && nn + 2 < MAX_NIB; i++) {
        const uint8_t wb = static_cast<uint8_t>(payload[i] ^ whiten_step());
        nib[nn++] = wb & 0xF;
        nib[nn++] = (wb >> 4) & 0xF;
    }
    const uint16_t crc = mesh_crc(payload, len);
    nib[nn++] = crc & 0xF;
    nib[nn++] = (crc >> 4) & 0xF;
    nib[nn++] = (crc >> 8) & 0xF;
    nib[nn++] = (crc >> 12) & 0xF;

    // -- header block: first SF-2 nibbles, cr_app=4, sf_app=SF-2, cw_len=8, LDRO --
    int hcw[12];
    uint16_t chips8[8];
    for (int i = 0; i < sf - 2; i++) hcw[i] = lora::hamming_encode(nib[i], 4);
    lora::interleave(hcw, sf - 2, 8, true, sf, N, chips8);
    for (int j = 0; j < 8; j++) push_sym(chips8[j], true);

    // -- payload blocks: cr_app=cr_code, sf_app=SF (SF-2 under LDRO), cw_len --
    // Above 16 ms per symbol the payload runs at reduced rate exactly like the header
    // block: SF-2 code words plus the parity bit. This was hardwired to full rate, which
    // is right for every BW250 preset and wrong for every BW125 one - the receiver drops
    // the two low chips it expects the transmitter to have left empty, so a hardware node
    // read a different value for every payload symbol we sent and never got a frame.
    const int p_app = use_ldro_ ? (sf - 2) : sf;
    int cw[12];
    uint16_t chipsP[12];
    for (int bi = sf - 2; bi < nn; bi += p_app) {
        for (int i = 0; i < p_app; i++) cw[i] = (bi + i < nn) ? lora::hamming_encode(nib[bi + i], cr_code) : 0;
        lora::interleave(cw, p_app, cw_len, use_ldro_, sf, N, chipsP);
        for (int j = 0; j < cw_len; j++) push_sym(chipsP[j], true);
    }
}

// --- execute (sample generator) ----------------------------------------------
void LoRaTXProcessor::execute(const buffer_c8_t& buffer) {
    if (!transmitting_) {
        for (size_t i = 0; i < buffer.count; i++) buffer.p[i] = {0, 0};
        return;
    }

    for (size_t i = 0; i < buffer.count; i++) {
        if (sym_idx_ >= frame_len_) {
            // Frame finished: notify M0 and stop
            transmitting_ = false;
            TXProgressMessage msg{};
            msg.progress = static_cast<uint8_t>(frame_len_ & 0xFFu);  // DEBUG: report symbol count
            msg.done = true;
            shared_memory.application_queue.push(msg);
            // Zero remaining samples in buffer
            for (; i < buffer.count; i++) buffer.p[i] = {0, 0};
            return;
        }

        const auto& sym = frame_[sym_idx_];

        const int32_t bw_i = static_cast<int32_t>(bw_phase_unit_);
        const int32_t half_i = bw_i >> 1;

        // At each symbol start, seed the smooth-chirp frequency ramp. The
        // instantaneous frequency sweeps CONTINUOUSLY across the symbol (a coarse
        // chip-step staircase shifts demod bins by ~1 and splatters out of band -
        // undecodable by a real SX126x). Phase stays continuous across symbols.
        if (samp_in_sym_ == 0) {
            if (sym.up) {
                // start freq = (chip/N)*BW - BW/2  (N = 2^SF -> shift, no divide)
                const int64_t step0 =
                    ((static_cast<int64_t>(sym.chip) * bw_phase_unit_) >> spreading_factor) - half_i;
                freq_fp_ = step0 << FRAC_BITS;
                dfreq_fp_ = dfreq_mag_fp_;
            } else {
                freq_fp_ = static_cast<int64_t>(half_i) << FRAC_BITS;
                dfreq_fp_ = -dfreq_mag_fp_;
            }
        }

        // Instantaneous frequency (2^32 phase units/sample), wrapped into
        // [-BW/2, +BW/2) so the chirp never radiates outside the LoRa channel.
        // Conditional wrap (not %): a single +/-BW correction always suffices
        // because the ramp spans exactly one BW per symbol - this keeps the
        // per-sample cost low enough for the M4 to sustain 2.5 Msps (no
        // FIFO underrun, which would distort the chirp timing on every frame).
        int32_t freq = static_cast<int32_t>(freq_fp_ >> FRAC_BITS);
        if (freq >= half_i)
            freq -= bw_i;
        else if (freq < -half_i)
            freq += bw_i;
        phase_acc_ += static_cast<uint32_t>(freq);
        freq_fp_ += dfreq_fp_;

        // Output IQ: I = cos(phi), Q = +sin(phi)  - matches the host encoder
        // (np.exp(+j*phi)) that the real Heltec decoded byte-exact (CRC OK).
        // The HackRF TX datapath is identical for host hackrf_transfer and the
        // PortaPack baseband, so the same (non-conjugated) convention applies.
        buffer.p[i] = {
            sine_table_i8[(phase_acc_ + 0x40000000u) >> 24u],  // cos = sin(+90deg)
            sine_table_i8[phase_acc_ >> 24u]                   // sin
        };

        // The 0.25-symbol SFD chirp emits only a quarter of the samples.
        const uint32_t sym_samples = sym.quarter ? (samples_per_symbol_ >> 2u)
                                                 : samples_per_symbol_;
        if (++samp_in_sym_ >= sym_samples) {
            samp_in_sym_ = 0;
            sym_idx_++;
        }
    }
}

// --- on_message --------------------------------------------------------------
void LoRaTXProcessor::on_message(const Message* const message) {
    if (message->id == Message::ID::LoRaConfigure) {
        configure(*reinterpret_cast<const LoRaConfigureMessage*>(message));
    } else if (message->id == Message::ID::LoRaPacket) {
        start_tx(*reinterpret_cast<const LoRaPacketMessage*>(message));
    }
}

void LoRaTXProcessor::configure(const LoRaConfigureMessage& msg) {
    spreading_factor = msg.spreading_factor;
    bandwidth = msg.bandwidth;
    coding_rate = msg.coding_rate;
    chips_per_symbol = 1u << spreading_factor;
    use_ldro_ = lora::ldro_needed(spreading_factor, bandwidth);

    samples_per_chip_ = TX_FS / bandwidth;
    samples_per_symbol_ = chips_per_symbol * samples_per_chip_;

    // 2^32 * bandwidth / TX_FS
    bw_phase_unit_ = static_cast<uint32_t>(
        (static_cast<uint64_t>(bandwidth) << 32u) / static_cast<uint64_t>(TX_FS));

    // Per-sample frequency increment of the smooth chirp (sweeps the full BW
    // over one symbol): bw_phase_unit_ / samples_per_symbol_, in <<FRAC_BITS.
    dfreq_mag_fp_ = samples_per_symbol_
                        ? ((static_cast<int64_t>(bw_phase_unit_) << FRAC_BITS) / samples_per_symbol_)
                        : 0;

    transmitting_ = false;
    phase_acc_ = 0;
    sym_idx_ = 0;
    samp_in_sym_ = 0;
    frame_len_ = 0;
}

void LoRaTXProcessor::start_tx(const LoRaPacketMessage& msg) {
    if (samples_per_symbol_ == 0) return;

    build_frame(msg.data, msg.length);

    sym_idx_ = 0;
    samp_in_sym_ = 0;
    phase_acc_ = 0;
    transmitting_ = true;
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<LoRaTXProcessor>()};
    event_dispatcher.run();
    return 0;
}
