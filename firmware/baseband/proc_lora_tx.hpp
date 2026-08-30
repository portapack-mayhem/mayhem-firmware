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
 */

#ifndef __PROC_LORA_TX_H__
#define __PROC_LORA_TX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "message.hpp"

#include <cstdint>
#include <array>

// LoRa CSS transmitter.
// Full encode chain: whiten -> Hamming(CR) -> diagonal interleave -> Gray encode -> CSS chirp.
// Frame: preamble (8 up) + sync word + SFD (2 down) + header (8 syms, CR4/8) + payload symbols.
// Meshtastic sync word = 0x2B: upchirps at chips 0x2 << 3 = 16 and 0xB << 3 = 88.
// The shift is a fixed 3 at every spreading factor, which is what Semtech's radios
// and gr-lora_sdr do. Shifting by (SF-4) instead lands on 16 and 88 only at SF7, so
// a hardware SX126x accepted our SF7 frames and rejected every other preset on the
// sync-word check - while our own receiver locked on regardless, because it
// synchronises on the chirp rather than on the exact word.

class LoRaTXProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    // LoRa parameters (configured via LoRaConfigureMessage)
    uint8_t spreading_factor{7};
    uint32_t bandwidth{125000};
    uint8_t coding_rate{5};          // denominator: 5=CR4/5 ... 8=CR4/8
    uint32_t chips_per_symbol{128};  // 2^SF
    bool use_ldro_{false};           // SF>=11: cpb = SF-2

    // Derived timing
    uint32_t samples_per_chip_{20};   // TX_FS / bandwidth
    uint32_t samples_per_symbol_{0};  // chips_per_symbol * samples_per_chip_
    uint32_t bw_phase_unit_{0};       // 2^32 * bandwidth / TX_FS

    // Smooth-chirp frequency accumulator (2nd-order NCO). The instantaneous
    // chirp frequency ramps CONTINUOUSLY within a symbol - a coarse chip-step
    // staircase spreads energy out of band and shifts demod bins by ~1, which a
    // real SX126x cannot decode. FRAC fractional bits keep the ramp exact while
    // using only 32-bit divides (hardware on Cortex-M4) in the per-sample loop.
    static constexpr int FRAC_BITS{16};
    int64_t dfreq_mag_fp_{0};  // |dfreq| per sample << FRAC_BITS
    int64_t dfreq_fp_{0};      // signed dfreq for the current symbol
    int64_t freq_fp_{0};       // current instantaneous freq << FRAC_BITS

    // --- TX frame (pre-computed symbol list) ---------------------------------
    struct TxSym {
        uint16_t chip;  // raw chip value (0 .. chips_per_symbol-1)
        bool up;        // true=upchirp, false=downchirp
        bool quarter;   // true=emit only samples_per_symbol/4 (the 0.25 SFD chirp)
    };
    static constexpr size_t MAX_FRAME_SYMS{700};
    std::array<TxSym, MAX_FRAME_SYMS> frame_{};
    size_t frame_len_{0};
    size_t sym_idx_{0};
    uint32_t samp_in_sym_{0};

    // --- Phase accumulator ----------------------------------------------------
    uint32_t phase_acc_{0};

    bool transmitting_{false};

    // --- Whitening LFSR -------------------------------------------------------
    uint8_t whiten_state_{0xFF};

    // --- Encode helpers -------------------------------------------------------
    void build_frame(const uint8_t* data, size_t len);
    void push_sym(uint16_t chip, bool up, bool quarter = false);
    uint8_t whiten_step();

    void configure(const LoRaConfigureMessage& msg);
    void start_tx(const LoRaPacketMessage& msg);

    static constexpr uint32_t TX_FS{2500000};

    /* NB: Threads must be the last members. */
    BasebandThread baseband_thread{TX_FS, this, baseband::Direction::Transmit};
};

#endif /* __PROC_LORA_TX_H__ */
