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

#include "proc_vor_tx.hpp"

#include "morse.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

void VorTxProcessor::execute(const buffer_c8_t& buffer) {
    for (size_t i = 0; i < buffer.count; i++) {
        if (!configured) {
            buffer.p[i] = {0, 0};
            continue;
        }

        // 30 Hz reference and variable tones (same frequency, offset by the radial).
        const int32_t ref_sine = sine_table_i8[(phase_30 >> 24) & 0xFF];
        const int32_t var_sine = sine_table_i8[((phase_30 + radial_offset) >> 24) & 0xFF];
        phase_30 += delta_30;

        // 9960 Hz subcarrier, FM-modulated +/- 480 Hz by the 30 Hz reference tone.
        const int32_t sub_inc = static_cast<int32_t>(delta_sub) + ((ref_sine * static_cast<int32_t>(delta_480)) >> 7);
        phase_sub += static_cast<uint32_t>(sub_inc);
        const int32_t sub_sine = sine_table_i8[(phase_sub >> 24) & 0xFF];

        // Optional 1020 Hz identification tone.
        int32_t id_sine = 0;
        if (ident_enabled && ident_tone_active()) {
            id_sine = sine_table_i8[(phase_id >> 24) & 0xFF];
            phase_id += delta_1020;
        }

        // DSB-AM envelope (carrier kept at LO -> Q = 0).
        int32_t env = carrier_level + (((var_sine * var_depth) + (sub_sine * sub_depth) + (id_sine * id_depth)) >> 7);
        if (env < 0) env = 0;
        if (env > 127) env = 127;

        buffer.p[i] = {static_cast<int8_t>(env), 0};
    }
}

void VorTxProcessor::vor_tx_config(const VorTxConfigureMessage& message) {
    // The variable tone is offset by +radial_offset relative to the 30 Hz
    // reference. This sign convention was verified by a TX->RX loopback against
    // the VOR RX decoder (proc_vor_rx): a transmitted radial of N degrees is
    // decoded as N degrees (not mirrored), so no negation is required.
    radial_offset = static_cast<uint32_t>(static_cast<uint64_t>(message.radial_deg % 360) * phase_period / 360);
    ident_enabled = message.ident_enabled;

    for (size_t i = 0; i < sizeof(ident_text); ++i)
        ident_text[i] = message.ident_text[i];
    ident_text[sizeof(ident_text) - 1] = '\0';
    build_ident_schedule();

    configured = message.enabled;
}

// Render the ident text into a repeating sequence of keyed-on / keyed-off
// sample runs using the shared ITU Morse table. Timing is derived from the
// standard "dot = 1200 / wpm" relation; letters are separated by a 3-unit gap,
// words by 7 units, and a trailing gap pads each cycle to a fixed 10 s period.
void VorTxProcessor::build_ident_schedule() {
    ident_segment_count = 0;
    ident_index = 0;
    ident_sample_counter = 0;

    const uint32_t dot_samples = static_cast<uint32_t>(1200ULL * baseband_fs / (ident_wpm * 1000ULL));

    uint32_t total_samples = 0;
    auto push_samples = [&](bool on, uint32_t samples) {
        if (ident_segment_count >= max_ident_segments) return;
        ident_segments[ident_segment_count++] = {on, samples};
        total_samples += samples;
    };
    auto push = [&](bool on, uint32_t units) {
        push_samples(on, units * dot_samples);
    };

    bool any = false;
    for (size_t n = 0; n < sizeof(ident_text) && ident_text[n]; ++n) {
        char ch = ident_text[n];
        if (ch >= 'a' && ch <= 'z') ch -= 32;

        uint16_t code = 0;
        if (ch >= '!' && ch <= '_') code = morse::morse_ITU[ch - '!'];

        if (!code) {
            // Space or unsupported character: word gap between symbols.
            if (any) push(false, MORSE_WORD_SPACE);
            continue;
        }

        const uint16_t code_size = code & 7;
        for (uint16_t c = 0; c < code_size; ++c) {
            const bool dash = ((code << c) & 0x8000) != 0;
            push(true, dash ? MORSE_DASH : MORSE_DOT);
            if (c + 1 < code_size)
                push(false, MORSE_SYMBOL_SPACE);
        }
        push(false, MORSE_LETTER_SPACE);
        any = true;
    }

    if (!any) {
        // No sendable characters: disable keying entirely.
        ident_segment_count = 0;
        return;
    }

    // Pad the trailing silence so ident-start to ident-start is a full 10 s.
    // If the keyed sequence is somehow longer than the period, fall back to a
    // one word-space gap so repeats never run back-to-back.
    const uint32_t min_gap = MORSE_WORD_SPACE * dot_samples;
    const uint32_t gap = (ident_period_samples > total_samples + min_gap)
                             ? ident_period_samples - total_samples
                             : min_gap;
    push_samples(false, gap);
}

// Advances the keying schedule by one sample and returns whether the ident
// tone should be sounding right now. Returns false when there is nothing to
// send so the caller leaves the ident tone off.
bool VorTxProcessor::ident_tone_active() {
    if (ident_segment_count == 0) return false;

    if (ident_sample_counter == 0)
        ident_sample_counter = ident_segments[ident_index].length;

    const bool on = ident_segments[ident_index].on;

    if (ident_sample_counter == 0 || --ident_sample_counter == 0) {
        ++ident_index;
        if (ident_index >= ident_segment_count) ident_index = 0;
    }

    return on;
}

void VorTxProcessor::on_message(const Message* const msg) {
    if (msg->id == Message::ID::VorTxConfigure) {
        vor_tx_config(*reinterpret_cast<const VorTxConfigureMessage*>(msg));
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<VorTxProcessor>()};
    event_dispatcher.run();
    return 0;
}
