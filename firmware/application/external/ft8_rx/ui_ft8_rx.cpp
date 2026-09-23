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
                  &channel,
                  &field_volume,
                  &field_frequency,
                  &options_band,
                  &text_status,
                  &text_decodes,
                  &console});

    channel.set_overload_threshold(-3);

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
    if (message->text[0] == '\0')
        return;

    /* The message takes the left of the line and the audio frequency the right, so the
     * frequencies line up in a column down the screen and the passband can be read at a
     * glance. A message too long to leave room for the column keeps its text, and the
     * frequency moves down a row and stays in the column. */
    std::string text{message->text};
    const std::string freq = to_string_dec_uint(message->frequency) + "Hz";
    /* screen_width is set at runtime, so the column count is taken here rather than
     * fixed at compile time. The font is the 8 px fixed one the rest of the layout uses. */
    const size_t columns = screen_width / 8;

    /* Padding is measured before the colour escape goes on, because the escape occupies
     * two bytes of the string and no columns on screen. */
    if (text.length() + freq.length() < columns)
        text.append(columns - freq.length() - text.length(), ' ');
    else
        text += '\n' + std::string(columns - freq.length(), ' ');

    /* A CQ is the line an operator can answer, so its text is picked out of the list. The
     * frequency stays white: it reads as a column down the screen, and colouring it would
     * break that column into stripes. */
    const bool calling = std::string{message->text}.compare(0, 3, "CQ ") == 0;
    if (calling)
        console.writeln(STR_COLOR_GREEN + text + STR_COLOR_WHITE + freq);
    else
        console.writeln(text + freq);
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
        case FT8RxStatusMessage::SyncState::Syncing:
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
