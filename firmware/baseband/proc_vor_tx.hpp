/*
 * Copyright (C) 2026 PortaPack Mayhem
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
 * along with this program; if not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __PROC_VOR_TX_H__
#define __PROC_VOR_TX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"

// Generates a conventional VOR composite signal as DSB-AM (carrier at LO, Q = 0):
//   - 30 Hz "variable" tone, amplitude-modulated directly (phase = transmitted radial)
//   - 9960 Hz subcarrier, FM-modulated (+/- 480 Hz) by a 30 Hz "reference" tone
//   - optional 1020 Hz identification tone
// The radial encoded is the phase difference between the variable and reference 30 Hz tones.
class VorTxProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const msg) override;

   private:
    static constexpr size_t baseband_fs = 1536000;
    static constexpr uint64_t phase_period = 1ULL << 32;
    // Per-sample phase increments (top byte indexes the 256-entry sine table).
    static constexpr uint32_t delta_30 = static_cast<uint32_t>(30ULL * phase_period / baseband_fs);
    static constexpr uint32_t delta_sub = static_cast<uint32_t>(9960ULL * phase_period / baseband_fs);
    static constexpr uint32_t delta_1020 = static_cast<uint32_t>(1020ULL * phase_period / baseband_fs);
    static constexpr uint32_t delta_480 = static_cast<uint32_t>(480ULL * phase_period / baseband_fs);

    // AM levels (int8 scale). carrier_level + sum of contributions must stay <= 127.
    static constexpr int32_t carrier_level = 60;
    static constexpr int32_t var_depth = 18;  // ~30% of carrier
    static constexpr int32_t sub_depth = 18;  // ~30% of carrier
    static constexpr int32_t id_depth = 6;    // ~10% of carrier

    // CW identifier keying. Real VOR stations key the 1020 Hz ident tone in
    // Morse at ~7 words per minute, so the schedule below is expressed in
    // "dot" time units and rendered to on/off sample runs at config time.
    static constexpr uint32_t ident_wpm = 7;
    static constexpr size_t max_ident_segments = 128;

    // VOR ident is transmitted at least every 10 s; repeat the keyed identifier
    // on a fixed 10 s cycle (ident start to next ident start).
    static constexpr uint32_t ident_period_samples = 10 * baseband_fs;

    struct IdentSegment {
        bool on;
        uint32_t length;  // duration in samples
    };

    uint32_t phase_30{0};
    uint32_t phase_sub{0};
    uint32_t phase_id{0};
    uint32_t radial_offset{0};

    bool ident_enabled{true};
    bool configured{false};

    char ident_text[8]{};
    IdentSegment ident_segments[max_ident_segments]{};
    size_t ident_segment_count{0};
    size_t ident_index{0};
    uint32_t ident_sample_counter{0};

    void vor_tx_config(const VorTxConfigureMessage& message);
    void build_ident_schedule();
    bool ident_tone_active();

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Transmit};
};

#endif /* __PROC_VOR_TX_H__ */
