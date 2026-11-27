/*
 * Copyright (C) 2026 Belousov Oleg aka R1CBU
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

#include "ui_dmr_voice.hpp"
#include "baseband_api.hpp"
#include "memory_map.hpp"
#include "string_format.hpp"
#include "audio.hpp"

using namespace portapack;

namespace ui::external_app::dmr_voice {
DmrVoiceView::DmrVoiceView(NavigationView& nav)
    : nav_{nav} {

    add_children({&field_frequency, &field_rf_amp, &field_lna, &field_vga,
                  &volume, &status, &color, &source, &destination});
    baseband::run_dmr_voice_image();
    field_frequency.set_value(frequency);
    receiver_model.set_target_frequency(frequency);

    field_frequency.updated = [this](rf::Frequency value) {
        frequency = value;
        receiver_model.set_target_frequency(value);
        baseband::configure_dmr_voice();
        status.set("Searching...");
        color.set("");
        source.set("SRC --");
        destination.set("DST --");
        audio::output::mute();
        output_enabled = false;
    };

    receiver_model.set_sampling_rate(2457600);
    receiver_model.set_baseband_bandwidth(1750000);
    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();
    audio::output::mute();
    receiver_model.enable();
    baseband::configure_dmr_voice();
}

DmrVoiceView::~DmrVoiceView() {
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

void DmrVoiceView::on_stats(const DmrVoiceStatusMessage& r) {
    if (r.audio_active != output_enabled) {
        output_enabled = r.audio_active;
        if (output_enabled)
            audio::output::unmute();
        else
            audio::output::mute();
    }

    const char* kind = r.kind == 0 ? "BS voice" : r.kind == 1 ? "MS voice"
                                              : r.kind == 2   ? "BS data"
                                                              : "MS data";
    status.set(!r.ready ? "RX allocation failed" : r.recent ? kind
                                                            : "Searching...");
    color.set(r.recent && r.color_code >= 0 ? "CC " + to_string_dec_uint(r.color_code) : "CC --");
    source.set(r.ids_valid ? "SRC " + to_string_dec_uint(r.source_id) : "SRC --");
    destination.set(r.ids_valid ? "DST " + to_string_dec_uint(r.destination_id) +
                                      (r.private_call ? " Private" : " Group")
                                : "DST --");
}

}  // namespace ui::external_app::dmr_voice
