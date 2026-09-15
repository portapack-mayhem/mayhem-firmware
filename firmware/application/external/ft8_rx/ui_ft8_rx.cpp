/*
 * Copyright (C) 2026 Dmytro Onyshko
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

#include "ui_ft8_rx.hpp"

#include "audio.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"

using namespace portapack;

namespace ui::external_app::ft8_rx {

void FT8RxView::focus() {
    field_frequency.focus();
}

FT8RxView::~FT8RxView() {
    receiver_model.disable();
    audio::output::stop();
    baseband::shutdown();
}

FT8RxView::FT8RxView(NavigationView& nav)
    : nav_{nav} {
    add_children({&field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &rssi,
                  &field_volume,
                  &field_frequency,
                  &options_band,
                  &text_status,
                  &text_decodes,
                  &console});

    field_frequency.set_step(100);

    /* Going through the frequency field rather than the model keeps the two in step:
     * the field retunes the receiver itself and redraws with the new dial. */
    options_band.on_change = [this](size_t, OptionsField::value_t v) {
        if (v != 0)
            field_frequency.set_value(v);
    };
    field_frequency.updated = [this](rf::Frequency f) {
        options_band.set_by_value(f);
    };
    options_band.set_by_value(receiver_model.target_frequency());

    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    /* FT8 occupies 50 Hz per signal inside a 2.5 kHz sub-band, but the decoder needs the
     * whole sub-band at once, so the receiver runs wide and the channel filter narrows it.
     * set_modulation() is deliberately not called: it would replace the FT8 baseband image. */
    receiver_model.set_sampling_rate(sampling_rate);
    receiver_model.set_baseband_bandwidth(baseband_bandwidth);
    receiver_model.enable();

    audio::output::start();
}

void FT8RxView::on_packet(const FT8PacketMessage* message) {
    if (message->text[0] != '\0')
        console.writeln(message->text);
}

void FT8RxView::on_status(const FT8RxStatusMessage* message) {
    decodes_total += message->decode_count;

    switch (message->state) {
        case FT8RxStatusMessage::SyncState::Searching:
            text_status.set_style(Theme::getInstance()->fg_red);
            text_status.set("Searching");
            break;
        case FT8RxStatusMessage::SyncState::Heard:
            text_status.set_style(Theme::getInstance()->fg_yellow);
            text_status.set("Heard, no decode");
            break;
        case FT8RxStatusMessage::SyncState::Refining:
            text_status.set_style(Theme::getInstance()->fg_yellow);
            text_status.set("Syncing");
            break;
        case FT8RxStatusMessage::SyncState::Locked:
            text_status.set_style(Theme::getInstance()->fg_green);
            text_status.set("Locked");
            break;
    }

    if (decodes_total > 0) {
        text_decodes.set_style(Theme::getInstance()->fg_light);
        text_decodes.set(to_string_dec_uint(decodes_total) + " RX");
    }
}

}  // namespace ui::external_app::ft8_rx
