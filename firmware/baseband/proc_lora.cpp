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

#include "proc_lora.hpp"
#include "lora_framing.hpp"
#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"
#include "message.hpp"
#include "dsp_fir_taps.hpp"
#include "audio_dma.hpp"  // RX-notification beep

#include <cmath>
#include <algorithm>
#include <complex>

// --- Standalone radix-2 in-place FFT (no dsp_fft.hpp size limit) -------------
// n must be a power of two. Operates on complex<float> array of exactly n elements.
static void lora_fft_inplace(std::complex<float>* data, size_t n) {
    // Bit-reversal permutation
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(data[i], data[j]);
    }
    // Cooley-Tukey butterfly stages (register-held twiddle - faster than a table
    // lookup on this memory-bandwidth-bound M4).
    for (size_t len = 2; len <= n; len <<= 1) {
        const float ang = -2.0f * static_cast<float>(M_PI) / static_cast<float>(len);
        const std::complex<float> wlen(cosf(ang), sinf(ang));
        for (size_t i = 0; i < n; i += len) {
            std::complex<float> w(1.0f, 0.0f);
            for (size_t j = 0; j < len / 2; ++j) {
                const auto u = data[i + j];
                const auto v = data[i + j + len / 2] * w;
                data[i + j] = u + v;
                data[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
}

// LoRa CSS demodulation + full decode chain.
// Reference: Robyns et al. (2016), gr-lora project.
//
// The firmware FFT supports max 256 points (K=8 in dsp_fft.hpp).
// For SF>8 we subsample the dechirped signal by (chips_per_symbol/256)
// before the FFT.  For LDRO mode (SF>=11) this introduces ~1-bit aliasing
// on odd LDRO symbols - acceptable for initial testing; proper fix requires
// extending dsp_fft.hpp to K=9 (512 points).
//
// Decode pipeline (M4):
//   FFT peak -> raw chip -> Gray decode -> [LDRO shift] -> sym_block[]
//   -> deinterleave (diagonal block) -> Hamming (CR=4/N) -> nibbles
//   -> byte assembly -> dewhiten (x^8+x^6+x^5+x^4+1 LFSR)
//   -> send_packet() -> M0

// --- Helpers ----------------------------------------------------------------

uint8_t LoRaProcessor::lora_whiten_step() {
    return lora::whiten_next(whiten_state_);
}

void LoRaProcessor::reset_rx() {
    rx_state_ = RxState::HUNT;
    preamble_run_ = 0;
    new_pre_run_ = 0;
    sym_in_block_ = 0;
    payload_sym_count_ = 0;
    nibble_lo_valid_ = false;
    whiten_state_ = 0xFF;
    decoded_len_ = 0;
    last_peak_mag_ = 0.0f;
    ref_peak_mag_ = 0.0f;
    weak_sym_count_ = 0;
    timing_corr_ = 0;
    snapped_ = false;
    skip_samples_ = 0;
    stored_realign_ = 0;
    hdr_count_ = 0;
    payload_len_target_ = 0;
    up_acc_ = 0.0;
    up_cnt_ = 0;
    have_down_ = false;
    best_down_mag_ = 0.0f;
    // NB: read_idx_/write_idx_ are NOT reset - the decimated-sample ring flows
    // continuously across packets; reset_rx only restarts the decode state.
}

// --- Interleaver block decoder -----------------------------------------------
//
// Deinterleave: coding_rate symbols of cpb bits -> cpb codewords of coding_rate bits.
// Diagonal interleaver formula (Robyns 2016 / gr-lora):
//   cw[i] bit j = sym[j] bit ((i-j+cpb) % cpb)
//   packed as: cw[i] |= (bit) << (coding_rate-1-j)
//
// Hamming decode: data nibble = top 4 bits (discard bottom (CR-4) parity bits).
//   nibble = (cw >> (coding_rate-4)) & 0x0F
//
// Byte assembly: first nibble = low half, second = high half (LoRa convention).
// Dewhiten: XOR each assembled byte with LFSR output.

// Assemble a payload nibble into a byte (low nibble first, LoRa order) and, once a
// byte is complete, de-whiten it (LFSR, matches TX) and append to payload_buf.
void LoRaProcessor::feed_nibble(uint8_t nibble) {
    if (!nibble_lo_valid_) {
        nibble_lo_ = nibble;
        nibble_lo_valid_ = true;
    } else {
        const uint8_t byte_val =
            static_cast<uint8_t>(((nibble << 4) | nibble_lo_) ^ lora_whiten_step());
        if (decoded_len_ < MAX_PAYLOAD)
            payload_buf[decoded_len_++] = byte_val;
        nibble_lo_valid_ = false;
    }
}

void LoRaProcessor::process_sym_block() {
    // Number of codewords per block: SF-2 with LDRO (SF>=11), SF without.
    const int cpb = use_ldro_ ? (static_cast<int>(spreading_factor) - 2)
                              : static_cast<int>(spreading_factor);
    const int cr = static_cast<int>(rx_cw_len_);

    uint16_t cw[12] = {};
    lora::deinterleave(sym_block_.data(), cpb, cr, cw);
    for (int i = 0; i < cpb; i++) feed_nibble(lora::hamming_top4(cw[i], cr));
}

// --- Main symbol window processor -------------------------------------------
//
// State machine:
//
// HUNT
//   Count consecutive near-zero upchirps.
//   After PREAMBLE_DETECT (8) -> enter PRE_END.
//
// PRE_END
//   Preamble confirmed.  Wait for end-of-preamble marker:
//     * Non-zero upchirp  -> sync word 1 detected;
//                           skip 9 more (sync2 + ~3 SFD + 5 header) -> SKIP
//     * Near-zero downchirp -> SFD started directly;
//                             skip 8 more (~2 more SFD + 5 header) -> SKIP
//     * Near-zero upchirp -> still in preamble; stay in PRE_END
//     * Safety timeout (64 symbols) -> reset to HUNT
//
// SKIP
//   Count down skip_remain_ symbols without decoding (sync/SFD/header).
//   After skip_remain_ reaches 0 -> enter PAYLOAD (reset decode state).
//
// PAYLOAD
//   Accumulate symbols into interleaver blocks (coding_rate per block).
//   After each full block -> process_sym_block() (deinterleave+Hamming+dewhiten).
//   End conditions:
//     * NEW_PRE_DETECT (8) consecutive near-zero upchirps -> new preamble; send.
//     * PAYLOAD_SYM_LIMIT (220) symbols collected -> send.
//   Minimum 4 decoded bytes required before send.

// Decode the reduced-rate explicit-header block (8 symbols in hdr_sym_) ->
// packet length. gr-lora dims: deinterleave sf_app=SF-2, cw_len=8; nibble =
// bit-reversed top4 of the codeword; length = (nib0<<4)|nib1.
uint8_t LoRaProcessor::decode_header() {
    const int sf_app = static_cast<int>(spreading_factor) - 2;  // 5 for SF7
    const int cw_len = 8;
    uint8_t cw[12] = {};
    for (int i = 0; i < cw_len; i++)
        for (int j = 0; j < sf_app; j++) {
            const int c = ((i - j - 1) % sf_app + sf_app) % sf_app;
            const int bit = (hdr_sym_[i] >> (sf_app - 1 - j)) & 1u;
            cw[c] = static_cast<uint8_t>(cw[c] | (bit << (cw_len - 1 - i)));
        }
    uint8_t nib[12] = {};  // sf_app up to 10 (SF12-2) - MUST exceed 8 or SF>9 overflows
    for (int i = 0; i < sf_app; i++) {
        // Hamming(8,4) SINGLE-ERROR CORRECT - the header block carries FEC; use it.
        nib[i] = lora::hamming_correct84(cw[i]);
    }
    // SF>7 chain fix: the header block's nibbles [5, sf_app) are the FIRST payload
    // nibbles (0 of them at SF7).  Stash them; the PAYLOAD transition feeds them
    // before the first payload block so the payload stays byte-aligned.
    hdr_extra_cnt_ = (sf_app > 5) ? static_cast<uint8_t>(sf_app - 5) : 0u;
    for (uint8_t k = 0; k < hdr_extra_cnt_; ++k) hdr_extra_nib_[k] = nib[5 + k];
    // Semtech header checksum (gr-lora header_impl) over nib[0..2]; nib[3]=c4, nib[4]=c3..c0.
    // Reject the header if it fails - prevents using a corrupt length (over-read / channel spam).
    const uint8_t h0 = nib[0], h1 = nib[1], h2 = nib[2];
    uint8_t c4, clo;
    lora::header_checksum(h0, h1, h2, &c4, &clo);
    header_valid_ = ((nib[3] & 1) == c4 && (nib[4] & 0xF) == clo);
    {  // Block length for the payload that follows, from this header's own CR field.
        const int hdr_cr = lora::header_coding_rate(h2);
        rx_cw_len_ = (hdr_cr >= 1 && hdr_cr <= 4) ? static_cast<uint8_t>(4 + hdr_cr)
                                                  : coding_rate;
    }
    return static_cast<uint8_t>((nib[0] << 4) | nib[1]);  // raw length byte (caller checks header_valid_)
}

void LoRaProcessor::process_one_symbol() {
    // Apply any pending one-shot sample skip (phase scan / SFD realign).
    if (skip_samples_) {
        read_idx_ += skip_samples_;
        skip_samples_ = 0;
    }

    const uint32_t thr = std::max(uint32_t{2}, chips_per_symbol >> 3u);
    const uint32_t win_start = read_idx_;          // ring index of this window
    const int32_t raw_up = dechirp_at(read_idx_);  // fold demod at read_idx_
    read_idx_ += samples_per_symbol;               // advance past this symbol
    const bool near_zero_up = (static_cast<uint32_t>(raw_up) < thr);

    switch (rx_state_) {
        // -- HUNT: phase-scan for the preamble, RECORD + snapshot the most-recent preamble
        //    window each near-zero; on the first non-near-zero (sync word 1), fine-align on
        //    that snapshot right here (see below) so no FFT burst lands just before the header.
        case RxState::HUNT:
            if (near_zero_up) {
                // Record the MOST-RECENT preamble window as the up-bin/tau reference AND
                // snapshot it - this window sits just before the sync/SFD/header, so any
                // dropped samples earlier in the packet are already reflected in it
                // (measuring on the FIRST preamble, ~16 symbols back, went stale under
                // live M4 sample drops).  Linear snapshot also avoids the ring-write race.
                up_bin_sfd_ = static_cast<uint32_t>(last_peak_bin_);
                up_frac_sfd_ = last_peak_frac_;
                up_acc_ = 0.0;
                up_cnt_ = 0;  // use the last window only, no averaging
                preamble_idx_ = win_start;
                pre_buf_base_ = win_start - samples_per_symbol;
                for (uint32_t k = 0; k < 3u * samples_per_symbol; ++k)
                    pre_buf_[k] = ring_[(pre_buf_base_ + k) % RING];
                if (!snapped_) {
                    snapped_ = true;
                    preamble_run_ = 0u;
                } else
                    preamble_run_++;
            } else if (snapped_ && preamble_run_ >= PREAMBLE_DETECT) {
                // sync word 1 (first non-near-zero).  FINE-ALIGN MOVED HERE (was the FFT burst
                // at end-of-SKIP): pre_buf_ still holds the LAST preamble window, so compute
                // dbest NOW - 3 windows (~0.75 ms @BW500) before the header - leaving the M4
                // the sync2 + 2.25-symbol SFD windows to drain any sample backlog before the
                // delicate no-FEC header (the chronic live-RX drop point).  Center the +/-2os
                // sharpness sweep on the preamble BIN-SNAP (no SFD/down-dechirp needed): an
                // aligned preamble peaks at bin 1+cfo, a window off by tau samples peaks at
                // 1+cfo+tau/os => c=-(up_bin-1).os ~ -tau.  Proven bit-identical to the old
                // SFD-tau sweep on the real OTA captures (fw_rx_host: tx1M 11/11, v1M 9/9).
                const int sps = static_cast<int>(samples_per_symbol);
                const int os = static_cast<int>(os_);
                const int c = -((int)up_bin_sfd_ - 1) * os;  // bin-snap center
                const long base = (long)pre_buf_base_;
                const long lim = 3L * sps - sps;  // last valid window start in pre_buf_
                int dbest = c;
                float sbest = -1.0f;
                for (int dd = c - 2 * os; dd <= c + 2 * os; ++dd) {
                    const long idx = (long)preamble_idx_ + dd - base;
                    if (idx < 0 || idx > lim) continue;
                    const float sh = sharpness_buf(&pre_buf_[idx]);
                    if (sh > sbest) {
                        sbest = sh;
                        dbest = dd;
                    }
                }
                // re-measure CFO on the sample-aligned preamble -> timing_corr_ (bin 1+cfo).
                const long ridx = (long)preamble_idx_ + dbest - base;
                if (ridx >= 0 && ridx <= lim) {
                    sharpness_buf(&pre_buf_[ridx]);
                    timing_corr_ = sharp_peak_bin_;
                }
                int realign = sps / 4 + dbest;  // 0.25-SFD grid shift + fine offset
                realign = ((realign % sps) + sps) % sps;
                stored_realign_ = realign;
                skip_remain_ = 2;  // skip sync2 + 2.25-symbol SFD
                rx_state_ = RxState::SKIP;
                // (Energy-triggered store-and-decode: this whole pipeline already runs in the
                //  DECODE phase over the buffered packet, so just continue to SKIP.)
            } else {
                // not the preamble yet: shift the window by sps/8 and keep scanning.
                preamble_run_ = 0u;
                snapped_ = false;
                skip_samples_ = samples_per_symbol >> 3u;
            }
            break;

        // -- SKIP: pure counter now - dbest/timing_corr/realign were computed at
        //    sync-detection (HUNT).  No down-dechirp, no sweep: the M4 does only one cheap
        //    dechirp_at per SKIP window, draining its sample backlog across sync2 + the
        //    2.25-symbol SFD before the no-FEC header (the fix for header-window drops).
        case RxState::SKIP: {
            if (skip_remain_ > 0) {
                skip_remain_--;
                break;
            }
            skip_samples_ = static_cast<uint32_t>(stored_realign_);
            hdr_count_ = 0;
            rx_state_ = RxState::HEADER;
            break;
        }

        // -- HEADER: collect 8 reduced-rate symbols -> explicit-header length ---------
        case RxState::HEADER: {
            const uint32_t chip_corr =
                (static_cast<uint32_t>(raw_up) + chips_per_symbol - timing_corr_) % chips_per_symbol;
            // header symbols are ALWAYS reduced-rate: ((chip-1) mod N) >> 2, then Gray.
            const uint32_t a = lora::ldro_div4((chip_corr + chips_per_symbol - 1u) % chips_per_symbol,
                                               chips_per_symbol);
            if (hdr_count_ < 8) hdr_sym_[hdr_count_++] = static_cast<uint16_t>(a ^ (a >> 1u));
            if (hdr_count_ >= 8) {
                const uint8_t len = decode_header();  // sets header_valid_; len is the raw length
                if (!header_valid_ || len < 4 || len > MAX_PAYLOAD) {
                    // corrupt header -> drop the packet (don't over-read / rebroadcast garbage)
                    reset_rx();
                    return;
                }
                payload_len_target_ = len;
                sym_in_block_ = 0;
                payload_sym_count_ = 0;
                nibble_lo_valid_ = false;
                whiten_state_ = 0xFF;
                decoded_len_ = 0;
                new_pre_run_ = 0;
                // SF>7: feed the header-block's leading payload nibbles first (none at SF7).
                for (uint8_t k = 0; k < hdr_extra_cnt_; ++k) feed_nibble(hdr_extra_nib_[k]);
                rx_state_ = RxState::PAYLOAD;
            }
            break;
        }

        // -- PAYLOAD ---------------------------------------------------------------
        case RxState::PAYLOAD: {
            // End-of-packet: peak magnitude drops >=10 dB for 3 symbols once TX stops.
            if (payload_sym_count_ == 0) {
                ref_peak_mag_ = last_peak_mag_;
                weak_sym_count_ = 0;
            } else {
                if (last_peak_mag_ > ref_peak_mag_) ref_peak_mag_ = last_peak_mag_;
                if (ref_peak_mag_ > 0.0f && last_peak_mag_ < ref_peak_mag_ * 0.10f) {
                    if (++weak_sym_count_ >= 3) {
                        if (decoded_len_ >= 4) send_packet(payload_buf.data(), decoded_len_);
                        reset_rx();
                        return;
                    }
                } else {
                    weak_sym_count_ = 0;
                }
            }

            // New preamble = end of this packet -> send and re-acquire (HUNT snaps again).
            if (near_zero_up) {
                new_pre_run_++;
                if (new_pre_run_ >= NEW_PRE_DETECT) {
                    if (decoded_len_ >= 4) send_packet(payload_buf.data(), decoded_len_);
                    reset_rx();
                    return;
                }
            } else {
                new_pre_run_ = 0;
            }

            // timing_corr_ (preamble bin) corrects the per-symbol bin; gr-lora chain.
            const uint32_t chip_corr =
                (static_cast<uint32_t>(raw_up) + chips_per_symbol - timing_corr_) % chips_per_symbol;
            uint32_t s = (chip_corr + chips_per_symbol - 1u) % chips_per_symbol;
            if (use_ldro_) s = lora::ldro_div4(s, chips_per_symbol);
            const uint16_t sym_val = static_cast<uint16_t>(s ^ (s >> 1u));

            sym_block_[sym_in_block_++] = sym_val;
            if (sym_in_block_ >= rx_cw_len_) {
                process_sym_block();
                sym_in_block_ = 0;
                // Stop at the header-declared length (the rest is CRC + trailing).
                if (payload_len_target_ && decoded_len_ >= payload_len_target_) {
                    send_packet(payload_buf.data(), payload_len_target_);
                    reset_rx();
                    return;
                }
            }

            payload_sym_count_++;
            if (payload_sym_count_ >= PAYLOAD_SYM_LIMIT) {
                if (decoded_len_ >= 4) send_packet(payload_buf.data(), decoded_len_);
                reset_rx();
            }
            break;
        }

        default:
            break;  // PRE_END no longer used
    }
}

// --- Dechirp + FFT peak detection --------------------------------------------
// fft_accum already holds nearest-neighbor resampled chip-domain samples.
// Dechirp phase: pi * chips_per_symbol * i^2 / eff_chips^2 (chip-domain quadratic).
// FFT peak bin = chip value * eff_chips / chips_per_symbol -> chip = bin * chips/eff.
// FOLD demod (matches the validated golden decoder): full-rate dechirp of the
// symbol at ring index `start` (x precomputed conj-chirp), FFT(sps), then SUM
// the os spectral aliases -> chips_per_symbol bins, argmax. Using the
// oversampling (instead of resampling to 1 sample/chip) makes the peak robust
// to sub-sample timing - the resample-to-N approach lost that and mis-decoded.
int32_t LoRaProcessor::dechirp_at(uint32_t start) {
    const uint32_t sps = samples_per_symbol;
    const uint32_t N = chips_per_symbol;
    const uint32_t os = os_;
    for (uint32_t n = 0; n < sps; ++n) {
        const auto& s = ring_[(start + n) % RING];
        fft_buf[n] = std::complex<float>(static_cast<float>(s.real()),
                                         static_cast<float>(s.imag())) *
                     ref_chirp_[n];
    }
    lora_fft_inplace(fft_buf.data(), sps);
    static float mag_row[512];
    float pm = 0.0f;
    uint32_t pb = 0;
    for (uint32_t k = 0; k < N; ++k) {
        std::complex<float> acc{0.0f, 0.0f};
        for (uint32_t m = 0; m < os; ++m) acc += fft_buf[k + m * N];
        const float mag = sqrtf(acc.real() * acc.real() + acc.imag() * acc.imag());
        mag_row[k] = mag;
        if (mag > pm) {
            pm = mag;
            pb = k;
        }
    }
    const float ma = mag_row[(pb + N - 1) % N], mb = mag_row[pb], mc = mag_row[(pb + 1) % N];
    const float den = ma - 2.0f * mb + mc;
    last_peak_frac_ = (den != 0.0f) ? 0.5f * (ma - mc) / den : 0.0f;
    last_peak_mag_ = pm * pm;
    last_peak_bin_ = pb;
    return static_cast<int32_t>(pb);
}

// DOWN-chirp dechirp (multiply by exp(+jphi)) -> fold -> peak bin, for SFD detection.
int32_t LoRaProcessor::dechirp_down(uint32_t start) {
    const uint32_t sps = samples_per_symbol;
    const uint32_t N = chips_per_symbol;
    const uint32_t os = os_;
    const float inv_os = 1.0f / static_cast<float>(os);
    const float sps_f = static_cast<float>(sps);
    for (uint32_t n = 0; n < sps; ++n) {
        const auto& s = ring_[(start + n) % RING];
        const float fn = static_cast<float>(n);
        const float ph = static_cast<float>(M_PI) * inv_os * (fn * fn / sps_f - fn);
        const float cq = cosf(ph), sq = sinf(ph);
        const float sr = static_cast<float>(s.real()), si = static_cast<float>(s.imag());
        fft_buf[n] = std::complex<float>(sr * cq - si * sq, sr * sq + si * cq);  // x.exp(+jphi)
    }
    lora_fft_inplace(fft_buf.data(), sps);
    static float mag_row[512];
    float pm = 0.0f;
    uint32_t pb = 0;
    for (uint32_t k = 0; k < N; ++k) {
        std::complex<float> acc{0.0f, 0.0f};
        for (uint32_t m = 0; m < os; ++m) acc += fft_buf[k + m * N];
        const float mag = sqrtf(acc.real() * acc.real() + acc.imag() * acc.imag());
        mag_row[k] = mag;
        if (mag > pm) {
            pm = mag;
            pb = k;
        }
    }
    const float ma = mag_row[(pb + N - 1) % N], mb = mag_row[pb], mc = mag_row[(pb + 1) % N];
    const float den = ma - 2.0f * mb + mc;
    down_peak_frac_ = (den != 0.0f) ? 0.5f * (ma - mc) / den : 0.0f;
    down_peak_mag_ = pm * pm;
    down_peak_bin_ = pb;
    return static_cast<int32_t>(pb);
}

// Same fold, but returns peak/total energy (sharpness) without touching state -
// used by the brute-force fine-align to pick the best sample offset.
float LoRaProcessor::sharpness_at(uint32_t start) {
    const uint32_t sps = samples_per_symbol;
    const uint32_t N = chips_per_symbol;
    const uint32_t os = os_;
    for (uint32_t n = 0; n < sps; ++n) {
        const auto& s = ring_[(start + n) % RING];
        fft_buf[n] = std::complex<float>(static_cast<float>(s.real()),
                                         static_cast<float>(s.imag())) *
                     ref_chirp_[n];
    }
    lora_fft_inplace(fft_buf.data(), sps);
    float pm = 0.0f, tot = 0.0f;
    uint32_t pb = 0;
    for (uint32_t k = 0; k < N; ++k) {
        std::complex<float> acc{0.0f, 0.0f};
        for (uint32_t m = 0; m < os; ++m) acc += fft_buf[k + m * N];
        const float mag = acc.real() * acc.real() + acc.imag() * acc.imag();
        tot += mag;
        if (mag > pm) {
            pm = mag;
            pb = k;
        }
    }
    sharp_peak_bin_ = pb;
    return tot > 0.0f ? pm / tot : 0.0f;
}

// Same fold as sharpness_at() but over a CONTIGUOUS linear buffer (the preamble
// snapshot), so the fine-align never races the ring writer.  Sets sharp_peak_bin_.
float LoRaProcessor::sharpness_buf(const complex8_t* p) {
    const uint32_t sps = samples_per_symbol;
    const uint32_t N = chips_per_symbol;
    const uint32_t os = os_;
    for (uint32_t n = 0; n < sps; ++n)
        fft_buf[n] = std::complex<float>(static_cast<float>(p[n].real()),
                                         static_cast<float>(p[n].imag())) *
                     ref_chirp_[n];
    lora_fft_inplace(fft_buf.data(), sps);
    float pm = 0.0f, tot = 0.0f;
    uint32_t pb = 0;
    for (uint32_t k = 0; k < N; ++k) {
        std::complex<float> acc{0.0f, 0.0f};
        for (uint32_t m = 0; m < os; ++m) acc += fft_buf[k + m * N];
        const float mag = acc.real() * acc.real() + acc.imag() * acc.imag();
        tot += mag;
        if (mag > pm) {
            pm = mag;
            pb = k;
        }
    }
    sharp_peak_bin_ = pb;
    return tot > 0.0f ? pm / tot : 0.0f;
}

uint16_t LoRaProcessor::gray_decode(uint16_t value) const {
    uint16_t mask = value >> 1;
    while (mask) {
        value ^= mask;
        mask >>= 1;
    }
    return value;
}

// --- execute -----------------------------------------------------------------
// Decimate to 1 MHz, append to the ring, then process whole symbols while there
// is enough look-ahead buffered for the fine-align (3.sps + os).
void LoRaProcessor::execute(const buffer_c8_t& buffer) {
    if (!rx_active) return;
    if (sf11_mode_) {
        execute_sf11(buffer);
        return;
    }  // LONG_FAST streaming path
    if (samples_per_symbol == 0 || samples_per_symbol > MAX_SPS) return;

    const auto result = decim_0.execute(buffer, decim_buffer);

    // -- DECODE: the buffered packet is frozen in the ring.  Do NOT store the freshly-
    //    decimated (idle) samples.  Run the FULL pipeline (HUNT scan -> sync -> fine-align
    //    -> SKIP/HEADER/PAYLOAD) over the buffer, a bounded chunk of symbols per call.
    //    process_one_symbol() self-terminates a packet via reset_rx() (->HUNT) and then
    //    re-scans any remaining buffer.  When the buffered span is consumed -> ACQUIRE.
    if (phase_ == RxPhase::DECODE) {
        uint32_t budget = 8;                                 // <=8 FFTs/call keeps each execute() well under the deadline
        const uint32_t lookahead = 4u * samples_per_symbol;  // HUNT fine-align reach
        while ((write_idx_ - read_idx_) >= lookahead && budget--) {
            process_one_symbol();
        }
        if ((write_idx_ - read_idx_) < lookahead) {  // buffer consumed -> resume acquiring
            // A packet longer than the ring (NodeInfo / long text ~ 110-123 sym) is
            // truncated here mid-PAYLOAD: emit what we decoded instead of dropping it.
            // The peer NAME sits early in the NodeInfo payload, and both AES-CTR (a
            // stream cipher) and protobuf tolerate a cut tail, so the M0 still recovers
            // the name from the partial.  Short packets finish via the length/weak-sym
            // paths (state already HUNT here), so this never double-sends them.
            if (rx_state_ == RxState::PAYLOAD && decoded_len_ >= 20)
                send_packet(payload_buf.data(), decoded_len_);
            reset_rx();
            phase_ = RxPhase::ACQUIRE;
            read_idx_ = write_idx_;  // discard stale; re-acquire fresh
        }
        return;
    }

    // -- ACQUIRE: store decimated samples as int8 (>>3 undoes the CIC3/2 gain of 8,
    //    recovering the ADC's native 8-bit resolution) AND accumulate energy.  This is
    //    the ONLY real-time work now - no FFT - so it can't be starved by M0 contention.
    float energy = 0.0f;
    for (size_t i = 0; i < result.count; ++i) {
        int16_t si = static_cast<int16_t>(result.p[i].real() >> 3);
        int16_t sq = static_cast<int16_t>(result.p[i].imag() >> 3);
        if (si > 127)
            si = 127;
        else if (si < -127)
            si = -127;
        if (sq > 127)
            sq = 127;
        else if (sq < -127)
            sq = -127;
        ring_[write_idx_ % RING] = complex8_t{static_cast<int8_t>(si), static_cast<int8_t>(sq)};
        ++write_idx_;  // % RING everywhere -> read/write always agree
        energy += static_cast<float>(si * si + sq * sq);
    }

    // Energy-triggered packet segmentation (real OTA measured: packet ~ 77x the noise
    // floor; a 4x threshold cleanly isolates each packet).  No FFT - argmax/sharpness
    // are scale-invariant, so int8 energy ratios are all that's needed here.
    if (noise_ema_ <= 0.0f) noise_ema_ = energy;  // init on first batch
    const float thr_hi = 4.0f * noise_ema_;
    const float thr_lo = 3.0f * noise_ema_;

    if (!in_packet_) {
        if (energy > thr_hi) {
            in_packet_ = true;
            low_run_ = 0;
            hi_run_ = 1;
            // Start at this (energy-rise) batch - it sits at the preamble onset, and the
            // remaining ~14 preamble symbols are plenty for HUNT.  No pre-roll, to keep the
            // captured span within the (RAM-limited) ring for a full-length text packet.
            pkt_start_ = write_idx_ - static_cast<uint32_t>(result.count);
        } else {
            noise_ema_ += (energy - noise_ema_) * (1.0f / 64.0f);  // track the noise floor
        }
    } else {
        bool decode_now = false;
        if (energy < thr_lo) {
            // packet ended: decode only if it was long enough to be a real packet.
            if (++low_run_ >= 2u) {
                if (hi_run_ >= MIN_PKT_BATCHES)
                    decode_now = true;
                else {
                    in_packet_ = false;
                }  // brief noise spike -> discard, keep hunting
            }
        } else {
            low_run_ = 0;
            ++hi_run_;
        }
        // Safety: packet longer than the ring -> decode what we have (caps packet length).
        if (in_packet_ && (write_idx_ - pkt_start_) >= (RING - 2u * samples_per_symbol))
            decode_now = (hi_run_ >= MIN_PKT_BATCHES);

        if (decode_now) {
            read_idx_ = pkt_start_;
            rx_state_ = RxState::HUNT;  // run the full pipeline over the buffer
            snapped_ = false;
            preamble_run_ = 0;
            in_packet_ = false;
            phase_ = RxPhase::DECODE;
        }
    }
}

// --- send_packet -------------------------------------------------------------

void LoRaProcessor::set_rx_busy(bool busy) {
    if (rx_busy_ == busy) return;
    rx_busy_ = busy;
    LoRaRxStatusMessage msg{busy};
    shared_memory.application_queue.push(msg);
}

void LoRaProcessor::send_packet(const uint8_t* data, size_t len) {
    LoRaPacketMessage msg{};
    msg.length = static_cast<uint8_t>(std::min(len, sizeof(msg.data)));
    for (size_t i = 0; i < msg.length; i++) msg.data[i] = data[i];
    // Approximate RSSI (dBm) from the packet's peak dechirp power. ref_peak_mag_
    // is |peak FFT bin|^2, so 10*log10 gives dB. log10 via a cheap IEEE-754
    // bit-trick log2 (no libm: log10f drags in ~1.7 KB and the flash is full);
    // exponent + linearised mantissa is accurate to <0.3 dB, plenty for an
    // uncalibrated signal indicator.
    //
    // The peak scales with the symbol length (coherent gain over sps samples), so the
    // raw figure has to be divided by sps^2 before it means anything. Without that the
    // estimate ran off the top of the scale at the longer spreading factors, was
    // clamped to 0, and 0 is what the node list reads as "no measurement" - which is
    // why every node showed "--" for signal.
    if (ref_peak_mag_ > 1.0f) {
        uint32_t bits;
        __builtin_memcpy(&bits, &ref_peak_mag_, sizeof(bits));
        const int exp = static_cast<int>((bits >> 23) & 0xFF) - 127;
        uint32_t mbits = (bits & 0x7FFFFF) | 0x3F800000;  // mantissa in [1,2)
        float m;
        __builtin_memcpy(&m, &mbits, sizeof(m));
        const uint32_t sps = sf11_mode_ ? sf11_sps_ : samples_per_symbol;
        const int log2sps = sps ? (31 - __builtin_clz(sps)) : 0;  // sps is a power of two
        // Undo both the coherent gain of the symbol and whatever the sample store
        // scaled the signal by, or the reading would move with the automatic gain.
        const float log2v = static_cast<float>(exp) + (m - 1.0f) -
                            2.0f * static_cast<float>(log2sps) +
                            2.0f * static_cast<float>(store_shift_);
        // Samples reach the FFT clipped to +-127, so the usable span is ~42 dB; the
        // offset puts a saturating local signal near -10 dBm and the noise floor near
        // -52. Uncalibrated, but monotonic and comparable between nodes.
        float dbm = 3.0103f * log2v - 52.0f;  // 10/log2(10)*log2 = 10*log10
        if (dbm > -1.0f) dbm = -1.0f;         // 0 is reserved for "unknown"
        if (dbm < -120.0f) dbm = -120.0f;
        msg.rssi = static_cast<int8_t>(dbm);
    }
    if (ref_sharp_n_) {
        float s = ref_sharp_acc_ / static_cast<float>(ref_sharp_n_);
        if (s > 0.999f) s = 0.999f;
        if (s < 0.0005f) s = 0.0005f;
        // 10*log10(s / (1-s)) with the same bit-trick logarithm used above.
        const float ratio = s / (1.0f - s);
        uint32_t rb;
        __builtin_memcpy(&rb, &ratio, sizeof(rb));
        const int rexp = static_cast<int>((rb >> 23) & 0xFF) - 127;
        uint32_t rm = (rb & 0x7FFFFF) | 0x3F800000;
        float rmf;
        __builtin_memcpy(&rmf, &rm, sizeof(rmf));
        const float snr = 3.0103f * (static_cast<float>(rexp) + (rmf - 1.0f));
        msg.snr_tenths = static_cast<int16_t>(snr * 10.0f);
    }
    shared_memory.application_queue.push(msg);
}

// ===========================================================================
//  LONG_FAST (SF11/BW250) STREAMING PATH
//  Store-and-decode can't hold a 58-symbol SF11 packet, so decode one symbol at a
//  time (affordable: 8 ms/symbol).  Sub-sample lock is solved WITHOUT a whole-packet
//  buffer: in FINE, demod each symbol at all N_DA da offsets (FFTs spread across the
//  symbol's execute() calls) and buffer only the demodded BINS; then decode every
//  (da,off) candidate from the bins and keep the valid-checksum broadcast frame.
//  Mirrors tools/lora_bench/lf_sf11_stream_finealign.py (8/8 real bursts byte-exact).
// ===========================================================================

#define SF11_DEBUG 0
// NOFFT: skip ALL demod (pure decim+store, zero FFT load) and probe periodicity by
// energy alone.  corr ~1000 here but ~8 with the FFT running == the stream is being
// starved (dropped buffers), not corrupted by the data path.
#define SF11_NOFFT 0
#ifndef SF11_NOFFT_GUARD
#define SF11_NOFFT_GUARD
#endif
#if SF11_DEBUG
static float g_dbg_sharp = 0.0f;  // max fold-sharpness seen since the last marker
static int g_dbg_bin = 0;
static int g_dbg_bins[6] = {0, 0, 0, 0, 0, 0};  // first 6 sharp bins from a preamble onset
static int g_dbg_capi = 99;                     // capture index (99 = idle)
// Pipeline-progress probe: how far past the preamble each packet actually gets.
static int g_dbg_stage = 0;  // 1=SFD found, 2=FINE ran try_decode and failed
static int g_dbg_up = 0, g_dbg_dn = 0, g_dbg_tau = 0;
static uint8_t g_dbg_L = 0, g_dbg_f[4] = {0, 0, 0, 0};  // best-aligned off's frame
static int g_dbg_lead = -1, g_dbg_bestod = 0;           // leading-FF count + which od won it
static float g_dbg_frac[16] = {0};                      // parabolic sub-bin per FINE symbol
static bool g_dbg_stage_ready = false;
static bool g_dbg_ready = false;  // 6 bins captured, ready to send
static int g_dbg_period = 999;    // per-mille |win[n]-win[n-SPS]| / |win[n]|
#endif

void LoRaProcessor::sf11_reset() {
    ref_peak_mag_ = 0.0f;  // per-packet RSSI accumulator (send_packet ran already)
    ref_sharp_acc_ = 0.0f;
    ref_sharp_n_ = 0;
    // Reception is over (delivered or abandoned): let the application transmit again.
    set_rx_busy(false);
    sf11_state_ = Sf11State::HUNT;
    sf11_pre_run_ = 0;
    sf11_prev_bin_ = -99;
    sf11_confirmed_ = false;
    sf11_sfd_step_ = 0;
    sf11_cfo_acc_ = std::complex<float>(0.0f, 0.0f);
    sf11_prev_pk_ = std::complex<float>(0.0f, 0.0f);
    sf11_fine_sym_ = 0;
    sf11_fine_da_ = 0;
    sf11_pay_have_ = 0;
    sf11_in_sfd_ = false;
    // sf11_read_ / win_w_ flow continuously - BUT FINE/RESOLVE demod h0-based windows and
    // never advance sf11_read_, so after a ~20-symbol packet read is far behind win_w. If
    // that gap exceeds the staleness window, HUNT's own stale-guard would reset every call
    // WITHOUT advancing read -> a permanent freeze (RX stuck, no new packets). Snap read up to
    // the latest symbol so HUNT always restarts on live samples.
    if (win_w_ - sf11_read_ > 3u * sf11_sps_) sf11_read_ = win_w_ - sf11_sps_;
}

// ref11[n] = exp(-j(base(n) + cfo.n)), base(n) = pi(n^2/sps - n) at OS=1.
// ref[n] = exp(-j*ph(n)), ph(n) = pi*(n^2/sps - n) + cfo_ramp*n = A*n^2 + B*n.
//
// Built by double rotation rather than 2048 cosf/sinf pairs: this is called from the
// SFD decision, i.e. inside one execute() call, and the trig version cost ~3 ms against
// a 1.024 ms deadline - dropping 2-3 buffers exactly on the header symbols that follow
// the SFD.  ph is quadratic, so its first difference is linear and its second difference
// is the constant 2A: step z by d, and d by the fixed D, giving two complex multiplies
// per sample and only three trig calls total (~0.1 ms).  Renormalise periodically so the
// rotators can't drift off the unit circle.
void LoRaProcessor::sf11_recompute_ref(float cfo_ramp) {
    sf11_ref_ramp_ = cfo_ramp;
    if (sf11_gen_ref_) return;  // generated per chunk instead - nothing to fill
    const float A = static_cast<float>(M_PI) / static_cast<float>(sf11_sps_);
    const float B = -static_cast<float>(M_PI) + cfo_ramp;
    float zr = 1.0f, zi = 0.0f;                             // z = exp(-j*ph(n)), ph(0) = 0
    float dr = cosf(A + B), di = -sinf(A + B);              // d = exp(-j*(ph(1)-ph(0)))
    const float Dr = cosf(2.0f * A), Di = -sinf(2.0f * A);  // d's own constant step
    float* const ref = reinterpret_cast<float*>(sf11_ref());
    for (uint32_t n = 0; n < sf11_sps_; ++n) {
        ref[2 * n] = zr;
        ref[2 * n + 1] = zi;
        const float nzr = zr * dr - zi * di;  // z *= d
        zi = zr * di + zi * dr;
        zr = nzr;
        const float ndr = dr * Dr - di * Di;  // d *= D
        di = dr * Di + di * Dr;
        dr = ndr;
        if ((n & 0xFF) == 0xFF) {  // pull both back onto the unit circle
            const float zs = 1.0f / sqrtf(zr * zr + zi * zi);
            zr *= zs;
            zi *= zs;
            const float ds = 1.0f / sqrtf(dr * dr + di * di);
            dr *= ds;
            di *= ds;
        }
    }
}

// Fold demod (OS=1 -> a plain sps-pt FFT) of the symbol at absolute win index win_start.
// is_up: x ref11 (up-chirp).  !is_up: x conj(ref11) = up-ref (for the SFD down-chirp).
// Bit-reversal permutation, split out of lora_fft_inplace so the butterfly stages can
// run separately (see sf11_demod_step).
static void lora_fft_bitrev(std::complex<float>* data, size_t n) {
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(data[i], data[j]);
    }
}

// One Cooley-Tukey butterfly stage (len = 2,4,...,n) over pre-bit-reversed data.
//
// The arithmetic is written out on raw floats instead of std::complex<float> on
// purpose: operator* on complex compiles to a __mulsc3 libcall (it must honour the
// Inf/NaN rules of Annex G), which costs ~160 cycles per butterfly instead of a
// handful of FPU ops.  That made one stage ~0.78 ms, so a whole 2048-pt demod ran
// ~9 ms against a 1.024 ms buffer deadline.  Nothing here needs those semantics -
// the data is bounded int8 - so do the multiply by hand.  Measured: 424 -> 69 -> 0
// dropped buffers/s (shared_memory.m4_buffer_missed, `sysinfo` on the console).
static void lora_fft_stage(std::complex<float>* data, size_t n, size_t len) {
    const float ang = -2.0f * static_cast<float>(M_PI) / static_cast<float>(len);
    const float wr = cosf(ang), wi = sinf(ang);
    float* const d = reinterpret_cast<float*>(data);
    const size_t half = len / 2;
    for (size_t i = 0; i < n; i += len) {
        float cr = 1.0f, ci = 0.0f;  // running twiddle
        for (size_t j = 0; j < half; ++j) {
            float* const a = d + 2 * (i + j);
            float* const b = a + 2 * half;
            const float vr = b[0] * cr - b[1] * ci;  // v = b * w
            const float vi = b[0] * ci + b[1] * cr;
            b[0] = a[0] - vr;
            b[1] = a[1] - vi;
            a[0] = a[0] + vr;
            a[1] = a[1] + vi;
            const float ncr = cr * wr - ci * wi;  // w *= wlen
            ci = cr * wi + ci * wr;
            cr = ncr;
        }
    }
}

void LoRaProcessor::sf11_demod_start(uint32_t win_start, bool is_up) {
    sf11_job_start_ = win_start;
    sf11_job_up_ = is_up;
    sf11_job_step_ = 0;
    sf11_job_active_ = true;
}

// Runs ONE bounded chunk of the in-flight demod; returns true when the result is ready.
// A whole demod is ~4.5 ms against a 1.024 ms buffer deadline, and an overrun SKIPS
// buffers (wait_for_buffer doesn't queue) - see the SF11_STEPS notes in the header.
// Dechirp the window at sf11_job_start_ into sf11_fft() (xconj-ref, hand-rolled complex
// multiply - see lora_fft_stage for the __mulsc3 reason).  Shared by the atomic and staged
// demod paths; keeping ONE copy matters - the % SF11_WIN (non-power-of-2) is code-heavy.
void LoRaProcessor::sf11_dechirp() {
    sf11_dechirp_chunk(0, 1);
}

// One slice of the dechirp, samples [part*sps/parts, (part+1)*sps/parts).
//
// Cut into pieces because at 4096 samples a whole one does not fit a buffer's worth of
// time, and an overrun does not queue - it SKIPS the next buffer, tearing the very
// stream being decoded.
//
// The window index is advanced by hand rather than recomputed with a modulo per sample:
// the window length is not a power of two, so `%` is a division, and this loop is the
// hottest in the receiver.
void LoRaProcessor::sf11_dechirp_chunk(uint32_t part, uint32_t parts) {
    const uint32_t n0 = part * sf11_sps_ / parts;
    const uint32_t n1 = (part + 1u) * sf11_sps_ / parts;
    float* const out = reinterpret_cast<float*>(sf11_fft());
    const float cs = sf11_job_up_ ? 1.0f : -1.0f;  // conj(ref) for the down-chirp ref
    uint32_t idx = (sf11_job_start_ + n0) % SF11_WIN;

    if (!sf11_gen_ref_) {
        const float* const ref = reinterpret_cast<const float*>(sf11_ref());
        for (uint32_t n = n0; n < n1; ++n) {
            const auto& c = sf11_win()[idx];
            if (++idx == SF11_WIN) idx = 0;
            const float xr = static_cast<float>(c.real()), xi = static_cast<float>(c.imag());
            const float rr = ref[2 * n], ri = cs * ref[2 * n + 1];
            out[2 * n] = xr * rr - xi * ri;
            out[2 * n + 1] = xr * ri + xi * rr;
        }
        return;
    }

    // Generated reference: exp(-j*ph(n)), ph(n) = A*n^2 + B*n, advanced by two
    // multiplies a sample. Chunk 0 seeds it; the rest continue where the last left off,
    // which is why the chunks must run in order - they do, one per step.
    if (part == 0) {
        const float A = static_cast<float>(M_PI) / static_cast<float>(sf11_sps_);
        const float B = -static_cast<float>(M_PI) + sf11_ref_ramp_;
        sf11_gz_r_ = 1.0f;
        sf11_gz_i_ = 0.0f;
        sf11_gd_r_ = cosf(A + B);
        sf11_gd_i_ = -sinf(A + B);
    }
    const float A2 = 2.0f * static_cast<float>(M_PI) / static_cast<float>(sf11_sps_);
    const float Dr = cosf(A2), Di = -sinf(A2);
    float zr = sf11_gz_r_, zi = sf11_gz_i_, dr = sf11_gd_r_, di = sf11_gd_i_;
    for (uint32_t n = n0; n < n1; ++n) {
        const auto& c = sf11_win()[idx];
        if (++idx == SF11_WIN) idx = 0;
        const float xr = static_cast<float>(c.real()), xi = static_cast<float>(c.imag());
        const float rr = zr, ri = cs * zi;
        out[2 * n] = xr * rr - xi * ri;
        out[2 * n + 1] = xr * ri + xi * rr;
        const float nzr = zr * dr - zi * di;  // z *= d
        zi = zr * di + zi * dr;
        zr = nzr;
        const float ndr = dr * Dr - di * Di;  // d *= D
        di = dr * Di + di * Dr;
        dr = ndr;
        if ((n & 0xFF) == 0xFF) {  // back onto the unit circle
            const float zs = 1.0f / sqrtf(zr * zr + zi * zi);
            zr *= zs;
            zi *= zs;
            const float ds = 1.0f / sqrtf(dr * dr + di * di);
            dr *= ds;
            di *= ds;
        }
    }
    sf11_gz_r_ = zr;
    sf11_gz_i_ = zi;
    sf11_gd_r_ = dr;
    sf11_gd_i_ = di;
}

bool LoRaProcessor::sf11_demod_step() {
    // Adaptive staged demod: the log2(sps) butterfly stages are split across sf11_nsteps_
    // (= sps/256 = buffers/symbol) bounded steps, one per M4 buffer, so a symbol's demod
    // finishes within the symbol's own buffers for EVERY BW250 SF - SF8 nsteps=1 (atomic:
    // dechirp+FFT+argmax in one step, ~0.5 ms), SF9 2, SF10 4, SF11 8 - and no single step
    // approaches the 1.024 ms deadline.  All hand-rolled (lora_fft_stage): std::complex
    // operator* (lora_fft_inplace) is a __mulsc3 libcall ~10x slower and starves the M4 -
    // a single 1024-pt inplace FFT ran ~2.25 ms and dropped 30k buffers on SF10.
    const uint32_t s = sf11_job_step_++;
    // SF12 takes its own route. A 4096-sample dechirp cannot be one step, so it is cut
    // into sf11_dechirp_steps_ of them and the FFT stages share what is left. The
    // SF7..SF11 schedule below is untouched on purpose: it was tuned against the 1.024 ms
    // deadline a buffer at a time, and every preset in the sweep depends on it.
    if (sf11_dechirp_steps_ > 1u) {
        const uint32_t D = sf11_dechirp_steps_;
        if (s < D) {
            sf11_dechirp_chunk(s, D);
            if (s + 1u == D) lora_fft_bitrev(sf11_fft(), sf11_sps_);
            return false;
        }
        const uint32_t s2 = s - D, n2 = sf11_nsteps_ - D;  // steps left for the FFT
        const uint32_t W2 = sf11_stages_ + 1u;             // stages + argmax
        const uint32_t a0 = (s2 * W2 + n2 - 1u) / n2;
        const uint32_t a1 = ((s2 + 1u) * W2 + n2 - 1u) / n2;
        const uint32_t k0 = (a0 > sf11_stages_) ? sf11_stages_ : a0;
        const uint32_t k1 = (a1 > sf11_stages_) ? sf11_stages_ : a1;
        for (uint32_t k = k0; k < k1; ++k)
            lora_fft_stage(sf11_fft(), sf11_sps_, static_cast<size_t>(2u) << k);
        if (s + 1u < sf11_nsteps_)
            return false;
    } else if (s == 0) {  // dechirp + bit-reversal (once)
        sf11_dechirp();
        lora_fft_bitrev(sf11_fft(), sf11_sps_);
    }
    // Distribute the log2(sps) FFT stages across the nsteps steps, treating dechirp+bitrev as 2
    // and argmax as 1 leading/trailing "virtual stages" - so the heavy step 0 (dechirp) and the
    // last step (argmax) carry FEWER real stages.  Critical for SF11: dechirp is 2048 samples
    // (~0.6 ms), so a dechirp+stage or a stage+argmax step overruns the 1.024 ms deadline (~1
    // drop/symbol - measured).  This reproduces the proven SF11 layout (step 0 dechirp-only,
    // argmax-only last step) while packing SF8-10's cheaper stages tighter.
    if (sf11_dechirp_steps_ == 1u) {
        const uint32_t W = sf11_stages_ + 3u;                                   // real stages + dechirp(2) + argmax(1)
        const uint32_t c0 = (s * W + sf11_nsteps_ - 1u) / sf11_nsteps_;         // ceil(s*W/nsteps)
        const uint32_t c1 = ((s + 1u) * W + sf11_nsteps_ - 1u) / sf11_nsteps_;  // ceil((s+1)*W/nsteps)
        const uint32_t j0 = (c0 < 2u) ? 0u : ((c0 - 2u > sf11_stages_) ? sf11_stages_ : c0 - 2u);
        const uint32_t j1 = (c1 < 2u) ? 0u : ((c1 - 2u > sf11_stages_) ? sf11_stages_ : c1 - 2u);
        for (uint32_t k = j0; k < j1; ++k)  // this step's slice of the stages
            lora_fft_stage(sf11_fft(), sf11_sps_, static_cast<size_t>(2u) << k);
        if (s + 1u < sf11_nsteps_)
            return false;  // more steps to run
    }

    // -- final step: magnitude argmax + sharpness + parabolic sub-bin frac --
    float pm = 0.0f, tot = 0.0f;
    uint32_t pb = 0;
    for (uint32_t k = 0; k < sf11_sps_; ++k) {
        const float m = sf11_fft()[k].real() * sf11_fft()[k].real() +
                        sf11_fft()[k].imag() * sf11_fft()[k].imag();
        tot += m;
        if (m > pm) {
            pm = m;
            pb = k;
        }
    }
    sf11_last_sharp_ = (tot > 0.0f) ? pm / tot : 0.0f;
    sf11_last_pk_ = sf11_fft()[pb];
    last_peak_mag_ = pm;
    // Packet-level peak for the RSSI estimate. Only the legacy store-and-decode path
    // used to fill this, so every packet from the streaming path (which is what all
    // the BW250 presets use) reported 0 dBm. Peaks found while hunting are noise, so
    // only accumulate once the state machine is locked on to a packet.
    if (sf11_state_ != Sf11State::HUNT) {
        if (pm > ref_peak_mag_) ref_peak_mag_ = pm;
        // Sharpness (peak power over total power) averaged across the packet is a
        // usable signal-to-noise estimate: the rest of the spectrum is noise.
        ref_sharp_acc_ += sf11_last_sharp_;
        ++ref_sharp_n_;
    }
    last_peak_bin_ = pb;
    // Parabolic sub-bin position of the peak (near-integer when the timing is right, whatever
    // the data bin) - used for the fractional-CFO tail correction.  km/kp are just pb-/+1 with
    // wrap, done by conditional (sf11_sps_ is a runtime value -> avoid a udiv per symbol).
    {
        const uint32_t km = (pb == 0) ? sf11_sps_ - 1 : pb - 1;
        const uint32_t kp = (pb + 1 == sf11_sps_) ? 0 : pb + 1;
        const float* const f = reinterpret_cast<const float*>(sf11_fft());
        const float ma = sqrtf(f[2 * km] * f[2 * km] + f[2 * km + 1] * f[2 * km + 1]);
        const float mb = sqrtf(f[2 * pb] * f[2 * pb] + f[2 * pb + 1] * f[2 * pb + 1]);
        const float mc = sqrtf(f[2 * kp] * f[2 * kp] + f[2 * kp + 1] * f[2 * kp + 1]);
        const float den = ma - 2.0f * mb + mc;
        sf11_last_frac_ = (den != 0.0f) ? 0.5f * (ma - mc) / den : 0.0f;
    }
#if SF11_DEBUG
    if (sf11_job_up_ && sf11_last_sharp_ > g_dbg_sharp) {
        g_dbg_sharp = sf11_last_sharp_;
        g_dbg_bin = (int)pb;
    }
#endif
    return true;
}

// gr-lora deinterleave/hamming/dewhiten from a da's buffered bins, at a given `off`.
// Fills out[] with de-whitened bytes; returns length L (or 0 on bad header).
// (Self-contained locals - no member side effects, so many candidates can be tried.)

// Decode the frame from a da's buffered bins at `off`.  Returns L (0 = bad header).
// Only decodes what the buffered SF11_FINE_SYMS bins allow (header + first payload
// bytes - enough to check the broadcast dest); the winner re-decodes the full payload.
uint8_t LoRaProcessor::sf11_decode_bins(const int16_t* bins, uint8_t nbins, uint32_t off, uint8_t* out, uint8_t out_cap) {
    const int N = static_cast<int>(chips_per_symbol);    // 2048
    const int SFv = static_cast<int>(spreading_factor);  // 11
    // header block: 8 reduced-rate symbols -> SF-2 nibbles (Hamming-correct)
    uint16_t g[8];
    for (int i = 0; i < 8; i++) {
        int a = ((static_cast<int>(bins[i]) - static_cast<int>(off)) % N + N) % N;  // (raw - off)
        a = static_cast<int>(lora::ldro_div4(static_cast<uint32_t>(a), static_cast<uint32_t>(N)));
        g[i] = static_cast<uint16_t>(a ^ (a >> 1));
    }
    uint16_t cw[16];
    lora::deinterleave(g, SFv - 2, 8, cw);
    uint8_t nib[12] = {};
    for (int i = 0; i < SFv - 2; i++) {
        int d0 = (cw[i] >> 4) & 1, d1 = (cw[i] >> 5) & 1, d2 = (cw[i] >> 6) & 1, d3 = (cw[i] >> 7) & 1;
        int rp0 = (cw[i] >> 3) & 1, rp1 = (cw[i] >> 2) & 1, rp2 = (cw[i] >> 1) & 1, rp3 = cw[i] & 1;
        int s0 = rp0 ^ (d3 ^ d2 ^ d1), s1 = rp1 ^ (d2 ^ d1 ^ d0), s2 = rp2 ^ (d3 ^ d2 ^ d0), s3 = rp3 ^ (d3 ^ d1 ^ d0);
        int syn = (s0 << 3) | (s1 << 2) | (s2 << 1) | s3;
        if (syn == 11)
            d3 ^= 1;
        else if (syn == 14)
            d2 ^= 1;
        else if (syn == 13)
            d1 ^= 1;
        else if (syn == 7)
            d0 ^= 1;
        nib[i] = static_cast<uint8_t>((d0 << 3) | (d1 << 2) | (d2 << 1) | d3);
    }
    uint8_t c4, clo;
    lora::header_checksum(nib[0], nib[1], nib[2], &c4, &clo);
    if ((nib[3] & 1) != c4 || (nib[4] & 0xF) != clo) return 0;
    const uint8_t L = static_cast<uint8_t>((nib[0] << 4) | nib[1]);
    if (L < 4 || L > MAX_PAYLOAD) return 0;
    // payload nibbles: header-block extras nib[5..SF-2) first, then payload blocks
    uint8_t pnib[64];
    int pc = 0;
    for (int c = 5; c < SFv - 2 && pc < (int)sizeof(pnib); c++) pnib[pc++] = nib[c];
    // Block length from THIS header's CR field, not from the preset table: nib[2] is
    // (cr << 1) | has_crc, so 4 + cr is the symbols-per-block the transmitter used.
    const int hdr_cr = (nib[2] >> 1) & 7;
    if (hdr_cr < 1 || hdr_cr > 4) return 0;
    const int cw_len = 4 + hdr_cr;
    // Under LDRO a symbol carries SF-2 bits: the two low chips are dropped and the block
    // deinterleaves to SF-2 code words, not SF. This loop assumed SF unconditionally,
    // which is right at BW250 (never LDRO) and wrong for every BW125 preset above SF10 -
    // the candidate frame came out with two of the four broadcast bytes right.
    const int ppm = use_ldro_ ? (SFv - 2) : SFv;
    for (int bi = 8; bi + cw_len <= nbins && pc + ppm <= (int)sizeof(pnib); bi += cw_len) {
        uint16_t pg[16];
        for (int i = 0; i < cw_len; i++) {
            int a = ((static_cast<int>(bins[bi + i]) - static_cast<int>(off)) % N + N) % N;
            if (use_ldro_) a = static_cast<int>(lora::ldro_div4(static_cast<uint32_t>(a), static_cast<uint32_t>(N)));
            pg[i] = static_cast<uint16_t>(a ^ (a >> 1));
        }
        uint16_t pcw[16];
        lora::deinterleave(pg, ppm, cw_len, pcw);
        for (int c = 0; c < ppm; c++) pnib[pc++] = lora::hamming_top4(pcw[c], cw_len);
    }
    uint8_t st = 0xFF;
    int nbytes = pc / 2;
    for (int i = 0; i < nbytes && i < out_cap; i++) {
        uint8_t bv = static_cast<uint8_t>((pnib[2 * i + 1] << 4) | pnib[2 * i]);
        // dewhiten (same LFSR as lora_whiten_step, run inline so no member state touched)
        const uint8_t wb = st;
        const uint8_t fb = ((st >> 7) ^ (st >> 5) ^ (st >> 4) ^ (st >> 3)) & 1u;
        st = static_cast<uint8_t>((st << 1) | fb);
        out[i] = static_cast<uint8_t>(bv ^ wb);
    }
    return L;
}

// Decode every buffered (da, off) candidate; on the first valid-checksum BROADCAST
// frame, set up the payload chain to continue streaming and return true.
bool LoRaProcessor::sf11_try_decode() {
    const int N = static_cast<int>(chips_per_symbol);
#if SF11_DEBUG
    g_dbg_lead = -1;
    g_dbg_bestod = 0;
#endif
    for (int rad = 0; rad <= SF11_DA; ++rad) {
        for (int s = 0; s < (rad == 0 ? 1 : 2); ++s) {
            const int da = (s == 0) ? -rad : rad;  // 0, -1,+1, -2,+2, ...
            const int da_idx = da + SF11_DA;
            // Multi-start header hypothesis.  The down-only SFD search fires on the FIRST
            // sharp downchirp, but the 2.25-symbol SFD has two, so depending on the packet it
            // locks on the correct one (h0) or one symbol early (so the real header begins at
            // h0+SPS = buffered symbol 1).  The up+down reference search disambiguated by
            // "down sharper than up", which costs a 2nd demod/symbol we can't afford in real
            // time - so instead try both header starts here, in the bin domain (free).  On 5
            // fresh strong captures this recovered 5/5 (down-only alone got 2/5); hs=0 won for
            // some packets, hs=1 for others.
            for (int hs = 0; hs <= SF11_HS_MAX; ++hs) {
                const int16_t* const B = &sf11_bins_[da_idx][hs];  // header starts at buffered symbol hs
                const uint8_t nb = static_cast<uint8_t>(sf11_fine_syms_ - hs);
                // off search radius.  On a clean offline capture off0 lands within +/-2, but on-air
                // it is noisier (a real crystal offset), and a symbol-timing error of d samples
                // shifts every dechirped bin by d (chirp coupling), a constant shift this loop
                // absorbs.  Free: bin-domain (deinterleave+hamming), no FFT.
                for (int od = -SF11_OFF_RAD; od <= SF11_OFF_RAD; ++od) {
                    const uint32_t off = static_cast<uint32_t>(((int)sf11_off0_ + od) % N + N) % N;
                    uint8_t frame[12];  // enough to reach the channel-hash byte
                    const uint8_t L = sf11_decode_bins(B, nb, off, frame, sizeof(frame));
#if SF11_DEBUG
                    if (rad == 0) {  // track the (hs,od) with the most leading 0xFF bytes
                        int lead = 0;
                        while (lead < 4 && L > (uint8_t)lead && frame[lead] == 0xFF) ++lead;
                        if (lead > g_dbg_lead) {
                            g_dbg_lead = lead;
                            g_dbg_bestod = od;
                            g_dbg_L = L;
                            for (int q = 0; q < 4; ++q) g_dbg_f[q] = (L > (uint8_t)q) ? frame[q] : 0;
                        }
                    }
#endif
                    // Header validity gate: the first 4 bytes are the Meshtastic destination.
                    // Accept a broadcast, or a unicast addressed to us - the old broadcast-only
                    // test made every direct message (and every unicast ACK) invisible.
                    if (L < 4) continue;
                    const uint32_t dest = static_cast<uint32_t>(frame[0]) |
                                          (static_cast<uint32_t>(frame[1]) << 8) |
                                          (static_cast<uint32_t>(frame[2]) << 16) |
                                          (static_cast<uint32_t>(frame[3]) << 24);
                    if (dest != 0xFFFFFFFFu && !(local_node_id_ && dest == local_node_id_))
                        continue;
                    // -- WINNER -- set up the member chain, then keep streaming the payload.
                    sf11_win_da_ = da;
                    sf11_win_off_ = off;
                    for (int i = 0; i < 8; i++) {
                        int a = (((int)B[i] - (int)off) % N + N) % N;
                        a = static_cast<int>(lora::ldro_div4(static_cast<uint32_t>(a), static_cast<uint32_t>(N)));
                        hdr_sym_[i] = static_cast<uint16_t>(a ^ (a >> 1));
                    }
                    const uint8_t len = decode_header();  // sets header_valid_ + hdr_extra_*
                    if (!header_valid_ || len < 4 || len > MAX_PAYLOAD) continue;
#if SF11_DEBUG
                    {  // WINNER: broadcast dest + valid header. Show src (frame[4..7]) + len.
                        uint8_t wm[8] = {0xEA, len, frame[4], frame[5], frame[6], frame[7],
                                         (uint8_t)hs, (uint8_t)(int8_t)0};
                        send_packet(wm, 8);
                    }
#endif
                    payload_len_target_ = len;
                    sym_in_block_ = 0;
                    payload_sym_count_ = 0;
                    nibble_lo_valid_ = false;
                    whiten_state_ = 0xFF;
                    decoded_len_ = 0;
                    for (uint8_t k = 0; k < hdr_extra_cnt_; ++k) feed_nibble(hdr_extra_nib_[k]);
                    for (uint8_t i = 8; i < nb; i++) {
                        uint32_t sv = static_cast<uint32_t>(((int)B[i] - (int)off) % N + N) % N;
                        // The third place LDRO had to be applied: these are the payload
                        // symbols already buffered during FINE, replayed into the streaming
                        // block machinery. Without the shift the first block or two of every
                        // BW125 packet decoded from unreduced symbol values - the frame's
                        // first four bytes came out right (the header search checks those)
                        // and the channel-hash byte after them did not, so the M0 dropped
                        // the packet at its channel filter without a trace.
                        if (use_ldro_) sv = lora::ldro_div4(sv, static_cast<uint32_t>(N));
                        sym_block_[sym_in_block_++] = static_cast<uint16_t>(sv ^ (sv >> 1));
                        if (sym_in_block_ >= rx_cw_len_) {
                            process_sym_block();
                            sym_in_block_ = 0;
                            if (payload_len_target_ && decoded_len_ >= payload_len_target_) {
#if SF11_DEBUG
                                {  // dump first 6 payload bytes + drop flag (miss delta since FINE start)
                                    uint16_t dmiss = (uint16_t)(shared_memory.m4_buffer_missed - sf11_miss0_);
                                    uint8_t pm[8] = {0xE9, (uint8_t)(dmiss > 255 ? 255 : dmiss),
                                                     payload_buf[4], payload_buf[5], payload_buf[6],
                                                     payload_buf[7], payload_buf[13], (uint8_t)decoded_len_};
                                    send_packet(pm, 8);
                                }
#endif
                                send_packet(payload_buf.data(), payload_len_target_);
                                sf11_reset();
                                return true;
                            }
                        }
                    }
                    // The buffer always ends at h0+(FINE_SYMS-1)*SPS, so the next live payload
                    // symbol is at h0+FINE_SYMS*SPS regardless of hs; only the count already
                    // consumed from the buffer changes.
                    sf11_read_ = static_cast<uint32_t>((long)sf11_h0_ + da + (long)sf11_fine_syms_ * sf11_sps_);
                    sf11_pay_have_ = static_cast<uint16_t>(sf11_fine_syms_ - hs - 8);
                    sf11_state_ = Sf11State::PAYLOAD;
                    // Header locked, payload streaming: tell the application to hold its
                    // transmitter. A long packet takes ~0.8 s on air at LongFast, far more
                    // than the TX countdown, and switching to the TX baseband unloads this
                    // demodulator - which silently killed every long reception.
                    set_rx_busy(true);
                    return true;
                }
            }
        }
    }
    return false;
}

// SF11 streaming: decimate /8 -> 250 kHz, append to win11, run the state machine with a
// per-call FFT budget (spread across the 8192-sample ~4 ms deadline).
void LoRaProcessor::execute_sf11(const buffer_c8_t& buffer) {
    // buffer_t has const members, so the second stage cannot be assigned over the first.
    const auto d8 = decim8_.execute(buffer, decim_buffer);
    const auto result = bw125_ ? decim2_.execute(d8, decim_buffer2) : d8;
#if SF11_DEBUG
    static uint32_t dbg_calls = 0;
    static int dbg_out_peak = 0;
    static int dbg_win_peak = 0;
    static uint8_t dbg_maxpre = 0;
#endif
    // Sensitivity depends on how much of the eight-bit sample store a weak signal
    // actually uses. With a fixed shift a distant node arrived as ones and zeroes -
    // everything below the shift was thrown away before the demodulator saw it. Track
    // the level and pick the shift that puts the peak near full scale instead.
    int buf_peak = 0;
    for (size_t i = 0; i < result.count; ++i) {
        int ar = result.p[i].real();
        if (ar < 0) ar = -ar;
        int aq = result.p[i].imag();
        if (aq < 0) aq = -aq;
        if (ar > buf_peak) buf_peak = ar;
        if (aq > buf_peak) buf_peak = aq;
    }
    // Decay slowly so a gap between packets does not wind the gain straight back up.
    agc_peak_ = (buf_peak > agc_peak_) ? buf_peak : (agc_peak_ - (agc_peak_ >> 6));
    if (sf11_state_ == Sf11State::HUNT) {
        // Only between packets: changing the scale mid-frame would move the magnitudes
        // the state machine is comparing against each other.
        int want = 0;
        while (want < 8 && (agc_peak_ >> want) > 110) ++want;
        if (want != store_shift_) store_shift_ = want;
    }

    for (size_t i = 0; i < result.count; ++i) {
#if SF11_DEBUG
        {
            int ar = result.p[i].real();
            if (ar < 0) ar = -ar;
            int aq = result.p[i].imag();
            if (aq < 0) aq = -aq;
            if (ar > dbg_out_peak) dbg_out_peak = ar;
            if (aq > dbg_out_peak) dbg_out_peak = aq;
        }
#endif
        int16_t si = static_cast<int16_t>(result.p[i].real() >> store_shift_);
        int16_t sq = static_cast<int16_t>(result.p[i].imag() >> store_shift_);
        if (si > 127)
            si = 127;
        else if (si < -127)
            si = -127;
        if (sq > 127)
            sq = 127;
        else if (sq < -127)
            sq = -127;
        sf11_win()[win_w_ % SF11_WIN] = complex8_t{static_cast<int8_t>(si), static_cast<int8_t>(sq)};
        ++win_w_;
#if SF11_DEBUG
        {
            int a = si < 0 ? -si : si, b = sq < 0 ? -sq : sq;
            if (a > dbg_win_peak) dbg_win_peak = a;
            if (b > dbg_win_peak) dbg_win_peak = b;
        }
#endif
    }
#if SF11_DEBUG && SF11_NOFFT
    // ---- FFT-FREE PERIODICITY PROBE -------------------------------------------------
    // No demod runs at all in this build, so the M4 does only decim8 + store: if the
    // stream were being starved by the per-buffer FFT, it is not starved now.  Arm on
    // ENERGY (no sharpness available without a demod): the first loud symbol after idle
    // is the preamble onset; on the 3rd, correlate the last two whole symbols.
    {
        static uint32_t dbg_e = 0;
        for (size_t i = 0; i < result.count; ++i) {
            const auto& s = sf11_win()[(win_w_ - result.count + i) % SF11_WIN];
            int a = s.real();
            if (a < 0) a = -a;
            int b = s.imag();
            if (b < 0) b = -b;
            dbg_e += (uint32_t)(a + b);
        }
        if ((win_w_ & (sf11_sps_ - 1u)) == 0 && win_w_ >= 3u * sf11_sps_) {  // sps is 2^SF -> mask
            const bool loud = dbg_e > (uint32_t)sf11_sps_ * 20u;
            if (!loud) {
                g_dbg_capi = 99;  // idle -> re-arm
            } else if (g_dbg_capi == 99) {
                g_dbg_capi = 0;  // first loud symbol = preamble onset
            } else if (g_dbg_capi < 3) {
                if (++g_dbg_capi == 2) {
                    float cre = 0.0f, cim = 0.0f, pw = 0.0f;
                    for (uint32_t i = 0; i < sf11_sps_; ++i) {
                        const auto& a = sf11_win()[(win_w_ - sf11_sps_ + i) % SF11_WIN];
                        const auto& b = sf11_win()[(win_w_ - 2u * sf11_sps_ + i) % SF11_WIN];
                        const float ar = a.real(), ai = a.imag(), br = b.real(), bi = b.imag();
                        cre += ar * br + ai * bi;
                        cim += ai * br - ar * bi;
                        pw += ar * ar + ai * ai;
                    }
                    const float mag = sqrtf(cre * cre + cim * cim);
                    g_dbg_period = (pw > 0.0f) ? (int)(1000.0f * mag / pw) : 0;
                    g_dbg_bins[0] = (int16_t)(dbg_e >> 8);  // report energy too
                    g_dbg_bins[1] = g_dbg_bins[2] = g_dbg_bins[3] = 0;
                    g_dbg_ready = true;
                }
            }
            dbg_e = 0;
        }
    }
#endif
#if SF11_DEBUG
    (void)dbg_calls;
    (void)dbg_out_peak;
    (void)dbg_win_peak;
    (void)dbg_maxpre;
    if (g_dbg_stage_ready) {
        uint8_t m[8];
        if (g_dbg_stage == 2) {  // decode failed -> show what the chain actually produced
            m[0] = 0xED;
            m[1] = 2;
            m[2] = g_dbg_L;
            m[3] = g_dbg_f[0];
            m[4] = g_dbg_f[1];
            m[5] = g_dbg_f[2];
            m[6] = g_dbg_f[3];
            m[7] = (uint8_t)(int8_t)g_dbg_bestod;
        } else {
            m[0] = 0xED;
            m[1] = (uint8_t)g_dbg_stage;
            m[2] = (uint8_t)(g_dbg_up >> 8);
            m[3] = (uint8_t)(g_dbg_up & 0xFF);
            m[4] = (uint8_t)(g_dbg_dn >> 8);
            m[5] = (uint8_t)(g_dbg_dn & 0xFF);
            m[6] = (uint8_t)((g_dbg_tau >> 8) & 0xFF);
            m[7] = (uint8_t)(g_dbg_tau & 0xFF);
        }
        send_packet(m, 8);
        g_dbg_stage_ready = false;
    }
    if (g_dbg_ready) {
        // [0xEE, bin0(hi,lo), d1,d2,d3,d4,d5] - 6 preamble-onset bins as base + 5 signed deltas
        auto d8 = [](int a, int b) { int v = a - b; if (v > 127) v = 127; if (v < -128) v = -128; return (uint8_t)(int8_t)v; };
        uint8_t m[8] = {0xEE, (uint8_t)(g_dbg_bins[0] >> 8), (uint8_t)(g_dbg_bins[0] & 0xFF),
                        d8(g_dbg_bins[1], g_dbg_bins[0]), d8(g_dbg_bins[2], g_dbg_bins[1]),
                        d8(g_dbg_bins[3], g_dbg_bins[2]),
                        (uint8_t)((g_dbg_period >> 8) & 0xFF), (uint8_t)(g_dbg_period & 0xFF)};
        send_packet(m, 8);
        g_dbg_ready = false;
        g_dbg_capi = 100;  // done: re-arm only after idle
    }
#endif
#if SF11_DEBUG && SF11_NOFFT
    return;  // probe build: decim+store only, no demod
#endif
    // ONE bounded demod step per buffer.  A whole demod is ~4.5 ms against the 1.024 ms
    // deadline, and an overrun SKIPS buffers rather than queueing them, which shreds the
    // stream (42% of buffers lost, preamble reads as non-periodic).  See the SF11_STEPS
    // budget notes in the header.
    for (uint32_t k = 0; k < sf11_steps_per_call_; ++k) {
        if (!sf11_job_active_ && !sf11_start_job()) return;  // nothing ready to demod
        if (!sf11_demod_step()) continue;                    // still working on this one
        sf11_job_active_ = false;
        sf11_consume();
    }
}

// True when the window at `start` has already been overwritten by newer samples, i.e. the
// state machine has fallen behind real time.  win11 only holds SF11_WIN samples, so a
// stage that needs more demods per symbol than the step budget delivers silently reads
// stale data instead of the signal.  Bail to HUNT rather than decode garbage.
bool LoRaProcessor::sf11_win_stale(uint32_t start) const {
    return (win_w_ - start) > (uint32_t)(SF11_WIN - sf11_sps_);
}

// Picks the next window to demod for the current state.  false = samples not ready yet.
bool LoRaProcessor::sf11_start_job() {
    switch (sf11_state_) {
        case Sf11State::HUNT: {
            if (win_w_ - sf11_read_ < sf11_sps_) return false;
            if (sf11_win_stale(sf11_read_)) {
                sf11_reset();
                return false;
            }
            // Up-ref for the preamble hunt; down-ref once confirmed (down-only SFD search).
            sf11_demod_start(sf11_read_, !sf11_confirmed_);
            return true;
        }
        case Sf11State::RESOLVE: {
            // demod the first header symbol at candidate h0 + (idx-1)*N/2
            const long ws = (long)sf11_h0_ + ((int)sf11_res_idx_ - 1) * (sf11_sps_ / 2);
            if (ws < 0 || (long)win_w_ - ws < (long)sf11_sps_) return false;
            if (sf11_win_stale((uint32_t)ws)) {
                sf11_reset();
                return false;
            }
            sf11_demod_start(static_cast<uint32_t>(ws), true);
            return true;
        }
        case Sf11State::FINE: {
            const int da = static_cast<int>(sf11_fine_da_) - SF11_DA;
            const long ws = (long)sf11_h0_ + da + (long)sf11_fine_sym_ * sf11_sps_;
            if (ws < 0 || (long)win_w_ - ws < (long)sf11_sps_) return false;
            if (sf11_win_stale((uint32_t)ws)) {
                sf11_reset();
                return false;
            }
            sf11_demod_start(static_cast<uint32_t>(ws), true);
            return true;
        }
        default:  // PAYLOAD
            if (win_w_ - sf11_read_ < sf11_sps_) return false;
            if (sf11_win_stale(sf11_read_)) {
                sf11_reset();
                return false;
            }
            sf11_demod_start(sf11_read_, true);
            return true;
    }
}

// Acts on a finished demod (result in last_peak_bin_ / sf11_last_sharp_ / sf11_last_pk_).
void LoRaProcessor::sf11_consume() {
    const int N = static_cast<int>(chips_per_symbol);
    const int32_t bin = static_cast<int32_t>(last_peak_bin_);
    const float sh = sf11_last_sharp_;

    if (sf11_state_ == Sf11State::HUNT) {
        if (!sf11_confirmed_) {
            // Preamble hunt: one up-demod per symbol.  This mirrors the reference decoder
            // (tools/lora_bench/fw_sf11_stream_host.cpp) exactly - consecutive preamble
            // symbols must land within ONE bin of each other, and the window advances by a
            // fixed sf11_sps_.  An earlier version compared against the run's first bin with
            // a +/-48 tolerance and re-centred the grid by the residual; that was written to
            // absorb bin drift which turned out to be dropped buffers (fixed in 0453edc6),
            // and it perturbed sf11_read_ - which h0 is derived from at the SFD.
            const std::complex<float> upk = sf11_last_pk_;
            const int bd = (bin > sf11_prev_bin_) ? (bin - sf11_prev_bin_) : (sf11_prev_bin_ - bin);
            if (sh > sf11_sharp_thr_ && sf11_prev_bin_ >= 0 && bd <= 1) {
                sf11_pre_run_++;
                sf11_up_bin_ = static_cast<uint32_t>(bin);
                sf11_pre_pos_ = sf11_read_;
                sf11_cfo_acc_ += upk * std::conj(sf11_prev_pk_);
            } else if (sh > sf11_sharp_thr_) {  // first sharp symbol (or a break)
                sf11_pre_run_ = 1;
                sf11_up_bin_ = static_cast<uint32_t>(bin);
                sf11_pre_pos_ = sf11_read_;
                sf11_cfo_acc_ = std::complex<float>(0.0f, 0.0f);
            } else {
                sf11_pre_run_ = 0;
            }
            sf11_prev_bin_ = bin;
            sf11_prev_pk_ = upk;
#if SF11_DEBUG
            // Capture the first bins of a SHARP RUN (= the packet's preamble onset) plus a
            // CFO-invariant periodicity correlation |S a.conj(b)| / S|a|^2 over the previous
            // symbol: ~1000 permille = a truly periodic preamble, ~0 = a shredded stream.
            // Reads ~950 with the demod staged, ~8 when a whole demod ran in one call.
            if (sh > sf11_sharp_thr_) {
                if (g_dbg_capi == 99) g_dbg_capi = 0;  // start of a sharp run
                if (g_dbg_capi == 2 && sf11_read_ > (uint32_t)sf11_sps_) {
                    float cre = 0.0f, cim = 0.0f, pw = 0.0f;
                    for (uint32_t i = 0; i < sf11_sps_; ++i) {
                        const auto& a = sf11_win()[(sf11_read_ + i) % SF11_WIN];
                        const auto& b = sf11_win()[(sf11_read_ - sf11_sps_ + i) % SF11_WIN];
                        const float ar = a.real(), ai = a.imag(), br = b.real(), bi = b.imag();
                        cre += ar * br + ai * bi;
                        cim += ai * br - ar * bi;
                        pw += ar * ar + ai * ai;
                    }
                    const float mag = sqrtf(cre * cre + cim * cim);
                    g_dbg_period = (pw > 0.0f) ? (int)(1000.0f * mag / pw) : 0;
                }
                if (g_dbg_capi < 6) {
                    g_dbg_bins[g_dbg_capi++] = bin;
                    if (g_dbg_capi == 6) g_dbg_ready = true;
                }
            } else
                g_dbg_capi = 99;
#endif
            if (sf11_pre_run_ >= SF11_PRE_MIN) {
                sf11_confirmed_ = true;
                sf11_sfd_step_ = 0;
                sf11_in_sfd_ = false;
                sf11_pre_frac_ = sf11_last_frac_;
            }
            sf11_read_ += sf11_sps_;
        } else {
            // SFD search (down-ref demod each symbol).  Fire only on a SHARP downchirp
            // (SF11_SFD_THR, not the 0.02 preamble gate): the SFD is only ~2.25 downchirps
            // with sync/header upchirps on either side, so a window at the SFD edge straddles
            // into them and reads a weak, mis-placed peak.  Gating on a strong peak fires in
            // the pure-downchirp middle -> an accurate down_bin -> accurate tau, at any arrival
            // phase.  (Validated offline: a full arrival-phase sweep decodes 16/16.)
            if (sh > SF11_SFD_THR) {  // SFD downchirp
                int diff = ((int)sf11_up_bin_ - bin) % N;
                if (diff < 0) diff += N;
                if (diff >= N / 2) diff -= N;  // wrap to (-N/2, N/2]
                const int tau = diff / 2;      // OS=1 -> tau samples
                sf11_cfo_ramp_ = std::arg(sf11_cfo_acc_) / static_cast<float>(sf11_sps_);
                sf11_recompute_ref(sf11_cfo_ramp_);
                sf11_off0_ = static_cast<uint32_t>(((int)sf11_up_bin_ - tau) % N + N) % N;
                sf11_h0_ = static_cast<uint32_t>((long)sf11_read_ +
                                                 (long)llroundf(2.25f * sf11_sps_) - tau);
                // Resolve the tau mod-N/2 ambiguity before FINE (see RESOLVE state).
                sf11_state_ = Sf11State::RESOLVE;
                sf11_res_idx_ = 0;
                sf11_res_bestsh_ = -1.0f;
#if SF11_DEBUG
                g_dbg_up = (int)sf11_up_bin_;
                g_dbg_dn = (int)bin;
                g_dbg_tau = tau;
                g_dbg_stage = 1;
                g_dbg_stage_ready = true;
#endif
            } else {
                sf11_read_ += sf11_sps_;
                if (sf11_read_ - sf11_pre_pos_ > 24u * sf11_sps_) sf11_reset();  // no SFD -> bail
            }
        }
    } else if (sf11_state_ == Sf11State::RESOLVE) {
        // Demod the first header symbol at h0-N/2, h0, h0+N/2 (res_idx 0,1,2); keep the h0
        // whose peak is SHARPEST (the aligned one - a half-symbol-off window straddles two
        // header symbols and dulls).  A d-sample h0 shift is a -d-bin off shift (chirp
        // coupling), so correct off0 by the winning shift.
        if (sh > sf11_res_bestsh_) {
            sf11_res_bestsh_ = sh;
            sf11_res_besth_ = (long)sf11_h0_ + ((int)sf11_res_idx_ - 1) * (sf11_sps_ / 2);
            sf11_res_bestbin_ = bin;               // this candidate's demod IS FINE symbol 0 - reuse it
            sf11_res_bestfrac_ = sf11_last_frac_;  // its sub-bin residual (fractional CFO)
        }
        if (++sf11_res_idx_ >= 3) {
            const long dh = sf11_res_besth_ - (long)sf11_h0_;
            sf11_off0_ = static_cast<uint32_t>(((int)sf11_off0_ - (int)dh) % N + N) % N;
            sf11_h0_ = static_cast<uint32_t>(sf11_res_besth_);
            // Fold the winner's sub-bin residual into the ref as a fractional CFO so the
            // FULL-RATE payload symbols land on integer bins (the integer off search can't
            // remove a fractional offset; the /4 header tolerates the mixed ref on sym 0).
            // ONLY at SF11: this is a single-symbol parabolic estimate (noisy), and at lower SF
            // the coarser bins (SF9 488 Hz vs SF11 122 Hz) make the preamble-averaged cfo_ramp
            // sufficient - folding the noisy residual there OVER-corrects and shreds the payload.
            // Offline on real captures: SF9 8/9->9/9, SF10 6/8->8/8 without it; SF11 9/9 either way.
            const float frac_term = (sf11_sps_ == SF11_SPS)
                                        ? sf11_res_bestfrac_ * 2.0f * static_cast<float>(M_PI) / static_cast<float>(sf11_sps_)
                                        : 0.0f;
            sf11_recompute_ref(sf11_cfo_ramp_ + frac_term);
            // Reuse the winning RESOLVE demod as FINE symbol 0 (already demodded at besth):
            // saves a symbol-time of latency AND captures symbol 0 before it can age out.
            sf11_bins_[SF11_DA][0] = static_cast<int16_t>(sf11_res_bestbin_);
            sf11_miss0_ = shared_memory.m4_buffer_missed;  // baseline for the drop flag
            sf11_state_ = Sf11State::FINE;
            sf11_fine_sym_ = 1;
            sf11_fine_da_ = 0;
#if SF11_DEBUG
            g_dbg_up = (int)sf11_off0_;
            g_dbg_dn = (int)dh;
            g_dbg_tau = 8888;  // 8888 = RESOLVE->FINE
            g_dbg_stage = 1;
            g_dbg_stage_ready = true;
#endif
        }
    } else if (sf11_state_ == Sf11State::FINE) {
        sf11_bins_[sf11_fine_da_][sf11_fine_sym_] = static_cast<int16_t>(bin);
#if SF11_DEBUG
        if (sf11_fine_sym_ < 16) g_dbg_frac[sf11_fine_sym_] = sf11_last_frac_;
#endif
        if (++sf11_fine_da_ >= SF11_NDA) {
            sf11_fine_da_ = 0;
            if (++sf11_fine_sym_ >= sf11_fine_syms_) {
                if (!sf11_try_decode()) {
#if SF11_DEBUG
                    g_dbg_stage = 2;
                    g_dbg_stage_ready = true;  // DEC marker shows the best-aligned bytes
#endif
                    sf11_reset();  // no broadcast winner -> hunt again
                }
            }
        }
    } else {  // PAYLOAD - stream the rest at the winning (da, off)
        // A mid-packet refinement of the reference used to sit here and it moved the
        // ground under a block that was already being decoded. It fired six streamed
        // symbols in, and a block is one buffered symbol plus seven streamed ones - so
        // the last symbol of that block met a different reference from the first seven.
        // The header dump put the corruption at exactly that block, every time, while
        // the block before it - replayed from the buffer, never streamed - was perfect.
        // It also shared the tracking loop's mistaken premise: a residual that stays at
        // +0.3 is a peak parked off centre, not one that is drifting.
        sf11_read_ += sf11_sps_;
        uint32_t sv = static_cast<uint32_t>((bin - (int)sf11_win_off_) % N + N) % N;
        // Deliberately the SAME call the buffered replay makes, with no extra cleverness.
        // The replay decodes its block perfectly on every packet and the streamed blocks
        // do not, so every difference between the two paths is a suspect until proven
        // otherwise - and folding the residual into the group decision was the last one
        // left. If the boundary survives this, the fault is older than anything I added.
        if (use_ldro_) sv = lora::ldro_div4(sv, static_cast<uint32_t>(N));
        // A tracking loop used to sit here and it was wrong in its premise. The residual
        // is where the peak sits INSIDE its bin, not evidence that the peak is moving:
        // a signal parked at b+0.3 hands over +0.3 every symbol, and integrating that
        // steps the offset by a whole bin after seven of them, corrupting everything
        // downstream. The header dump showed exactly that boundary - the first payload
        // block, replayed from the buffer and untouched by the loop, decoded perfectly,
        // and the second, the first to come through here, was wrong from its opening
        // nibble. Tracking real drift needs the CHANGE in residual, not its value.
        sym_block_[sym_in_block_++] = static_cast<uint16_t>(sv ^ (sv >> 1));
        if (sym_in_block_ >= rx_cw_len_) {
            process_sym_block();
            sym_in_block_ = 0;
            if (payload_len_target_ && decoded_len_ >= payload_len_target_) {
#if SF11_DEBUG
                {  // streamed-payload dump: drop flag + first 6 bytes
                    uint16_t dmiss = (uint16_t)(shared_memory.m4_buffer_missed - sf11_miss0_);
                    uint8_t pm[8] = {0xE9, (uint8_t)(dmiss > 255 ? 255 : dmiss),
                                     payload_buf[4], payload_buf[5], payload_buf[6],
                                     payload_buf[7], payload_buf[13], (uint8_t)decoded_len_};
                    send_packet(pm, 8);
                }
#endif
                send_packet(payload_buf.data(), payload_len_target_);
                sf11_reset();
                return;
            }
        }
        if (++sf11_pay_have_ >= PAYLOAD_SYM_LIMIT) {
            if (decoded_len_ >= 4) send_packet(payload_buf.data(), decoded_len_);
            sf11_reset();
        }
    }
}

void LoRaProcessor::on_message(const Message* const message) {
    if (message->id == Message::ID::LoRaConfigure)
        configure(*reinterpret_cast<const LoRaConfigureMessage*>(message));
    else if (message->id == Message::ID::AudioBeep) {
        const auto& b = *reinterpret_cast<const AudioBeepMessage*>(message);
        audio::dma::beep_start(b.freq, b.sample_rate, b.duration_ms);
    }
}

void LoRaProcessor::configure(const LoRaConfigureMessage& msg) {
    spreading_factor = msg.spreading_factor;
    bandwidth = msg.bandwidth;
    coding_rate = msg.coding_rate;
    local_node_id_ = msg.local_node_id;
    chips_per_symbol = 1u << spreading_factor;
    // -- BW250 STREAMING path (SF7..SF11 - payloads won't fit the ring): decim8 -> 250 kHz,
    // per-symbol dechirp+FFT, staged across sps/256 buffers/symbol (SF8=1 atomic ... SF11=8);
    // SF7 goes the other way - 2 whole demods per buffer, see sf11_steps_per_call_. --
    // BW125 joins the streaming path through a second /2 decimation stage, and SF12
    // through generating its reference rather than storing it - 4096 samples per symbol
    // do not fit the ref/fft/window layout otherwise.
    bw125_ = (bandwidth == 125000 && spreading_factor >= 7 && spreading_factor <= 11);
    // SF12 is BUILT but not yet ENABLED, and the reason is a timing budget, not the
    // memory one that is now solved. At 4096 samples a demod takes exactly one symbol
    // of wall time, and RESOLVE runs three of them to settle the half-symbol ambiguity.
    // Counting from the SFD: h0 lands 1.25 symbols ahead of the write pointer, the three
    // demods finish 3.5 symbols later, and the window holds 2.75 - stale threshold 1.75.
    // FINE would bail before reading its first symbol, every time.
    //
    // Shortening those three probes does not fix it: they tell the candidates apart by
    // sharpness, and a mis-aligned window is unsharp because it STRADDLES two symbols.
    // A quarter-symbol probe sits inside one symbol either way and looks equally sharp.
    // What is needed is a cheaper way to resolve tau mod N/2 - most likely off the SFD's
    // own down-chirp, which is demodulated already - or an FFT in fixed point, which
    // would halve its 32 KiB and buy the window the symbols it needs.
    //
    // Everything else for SF12 is in place and tested: the generated reference
    // (test_lora_ref.cpp), the chunked dechirp, the layout, and an offline receiver that
    // decodes real air (tools/lora_bench/sf12_off.py).
    sf11_mode_ = (spreading_factor >= 7 && spreading_factor <= 11 &&
                  (bandwidth == 250000 || bandwidth == 125000));
    if (sf11_mode_) {
        sf11_out_per_buf_ = bw125_ ? 128u : 256u;
        sf11_sps_ = 1u << spreading_factor;            // 256/512/1024/2048
        sf11_stages_ = spreading_factor;               // log2(sps) FFT butterfly stages
        sf11_nsteps_ = sf11_sps_ / sf11_out_per_buf_;  // demod steps = buffers/symbol
        if (sf11_nsteps_ < 1u) sf11_nsteps_ = 1u;
        // The other direction: at SF7 a symbol is shorter than a buffer, so a buffer
        // carries 256/sps of them and each needs its own whole demod.
        sf11_steps_per_call_ = (sf11_sps_ >= sf11_out_per_buf_)
                                   ? 1u
                                   : (sf11_out_per_buf_ / sf11_sps_);
        // Symbols to buffer before the header search: enough payload blocks to reach the
        // eight nibbles the four destination bytes need, given what the header block
        // already hands over.
        {
            const int sfv = static_cast<int>(spreading_factor);
            const int extras = (sfv - 2 > 5) ? (sfv - 2 - 5) : 0;
            const int want = (8 - extras > 0) ? (8 - extras) : 0;
            int blocks = (want + sfv - 1) / sfv;  // ceil(want / nibbles-per-block)
            if (blocks < 1) blocks = 1;
            int syms = 8 + blocks * static_cast<int>(coding_rate) + 1;  // preset CR: an estimate, the header decides the real cut
            if (syms > SF11_FINE_SYMS) syms = SF11_FINE_SYMS;
            sf11_fine_syms_ = static_cast<uint8_t>(syms);
        }
        // Keep the margin over the noise floor that SF11 has, rather than the number.
        sf11_sharp_thr_ = SF11_SHARP_THR;
        if (sf11_sps_ < sf11_out_per_buf_) {
            const float noise_here = logf(static_cast<float>(sf11_sps_)) /
                                     static_cast<float>(sf11_sps_);
            const float noise_sf11 = logf(2048.0f) / 2048.0f;
            sf11_sharp_thr_ = SF11_SHARP_THR * (noise_here / noise_sf11);
        }
        // Above the stored reference's size the reference is generated on the fly and the
        // FFT moves to the front of the ring; the dechirp is cut into chunks small enough
        // that one of them fits a buffer, leaving the FFT stages the rest of the steps.
        sf11_gen_ref_ = (sf11_sps_ > SF11_SPS);
        sf11_dechirp_steps_ = sf11_gen_ref_ ? 8u : 1u;
        if (sf11_dechirp_steps_ >= sf11_nsteps_)  // never eat every step
            sf11_dechirp_steps_ = (sf11_nsteps_ > 2u) ? sf11_nsteps_ / 2u : 1u;
        chips_per_symbol = sf11_sps_;    // N = 2^SF
        samples_per_symbol = sf11_sps_;  // OS = 1
        os_ = 1;
        // LDRO is required once a symbol lasts longer than 16 ms. At BW250 the longest
        // (SF11) is 8.192 ms, so this used to be hardwired off - but SF11 at BW125 is
        // 16.384 ms and SF12 is 32.8, and with LDRO the symbol carries SF-2 bits, not SF.
        use_ldro_ = lora::ldro_needed(spreading_factor, bandwidth);
        // 2 MHz /8 -> 250 kHz + fs/4 translate to cancel the receiver's WidebandFMAudio
        // -fs/4 offset (signal -> DC).  Shift::Down is the correct rotation: the exact
        // tap-rotation emulation gives preamble sharpness 0.45 for Down vs 0.005 for Up
        // (Up matched the dead on-air sharpness of 0.004).  scale = default.
        decim8_.configure(taps_200k_decim_0.taps, dsp::decimate::c8_to_c32_sat_scalar,
                          dsp::decimate::FIRC8xR16x24FS4Decim8::Shift::Down);
        if (bw125_) decim2_.configure(taps_125k_decim_2.taps);
        sf11_recompute_ref(0.0f);  // base dechirp for HUNT (recomputed w/ CFO at SFD)
        win_w_ = 0;
        sf11_read_ = 0;
        sf11_reset();
        rx_active = true;
        return;
    }

    // LDRO required when symbol time > 16 ms: (1<<SF)/BW > 0.016
    use_ldro_ = lora::ldro_needed(spreading_factor, bandwidth);

    const uint32_t dec_fs = static_cast<uint32_t>(baseband_fs) / DECIM;  // 1 MHz
    samples_per_symbol = static_cast<uint32_t>(
        ((uint64_t)chips_per_symbol * dec_fs + bandwidth / 2u) / bandwidth);
    os_ = (chips_per_symbol > 0) ? (samples_per_symbol / chips_per_symbol) : 1u;

    // Precompute the conjugate reference chirp for one symbol (sps samples):
    //   ref[n] = exp(-j * pi/os * (n^2/sps - n))
    if (samples_per_symbol >= chips_per_symbol && samples_per_symbol <= MAX_SPS && os_ >= 1) {
        const float inv_os = 1.0f / static_cast<float>(os_);
        const float sps_f = static_cast<float>(samples_per_symbol);
        for (uint32_t n = 0; n < samples_per_symbol; ++n) {
            const float fn = static_cast<float>(n);
            const float ph = static_cast<float>(M_PI) * inv_os * (fn * fn / sps_f - fn);
            ref_chirp_[n] = std::complex<float>(cosf(ph), -sinf(ph));
        }
    }

    // CIC3 /2 decimator has no configurable taps (fixed sinc^3 response; ~-2.7 dB
    // droop at the BW500 +/-250 kHz edge - tolerable for the fold demod).
    write_idx_ = 0;
    read_idx_ = 0;
    rx_active = true;
    phase_ = RxPhase::ACQUIRE;
    noise_ema_ = 0.0f;
    in_packet_ = false;
    low_run_ = 0;
    reset_rx();
}

int main() {
    audio::dma::init_audio_out();  // audio path for RX beeps
    EventDispatcher event_dispatcher{std::make_unique<LoRaProcessor>()};
    event_dispatcher.run();
    return 0;
}
