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
#include "ft8_message.hpp"
#include "string_format.hpp"

#include <cstring>

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
                  &field_threshold,
                  &field_volume,
                  &field_frequency,
                  &text_status,
                  &console});

    field_threshold.set_value(initial_threshold);
    field_threshold.on_change = [this](int32_t value) {
        baseband::set_ft8_config(static_cast<uint8_t>(value));
    };

    field_frequency.set_step(100);

    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    /* FT8 occupies 50 Hz per signal inside a 2.5 kHz sub-band, but the decoder needs the
     * whole sub-band at once, so the receiver runs wide and the channel filter narrows it.
     * set_modulation() is deliberately not called: it would replace the FT8 baseband image. */
    receiver_model.set_sampling_rate(sampling_rate);
    receiver_model.set_baseband_bandwidth(baseband_bandwidth);
    receiver_model.enable();

    audio::output::start();

    baseband::set_ft8_config(initial_threshold);
}

void FT8RxView::on_packet(const FT8PacketMessage* message) {
    /* The same transmission is often decoded from several candidates, and a station
     * repeats its call across slots, so only the payloads not seen recently are shown. */
    for (int i = 0; i < recent_count; i++) {
        if (std::memcmp(message->payload, recent_payloads[i], FT8PacketMessage::payload_length) == 0)
            return;
    }
    std::memcpy(recent_payloads[recent_index], message->payload, FT8PacketMessage::payload_length);
    recent_index = (recent_index + 1) % recent_max;
    if (recent_count < recent_max) recent_count++;

    const auto text = payload_to_text(message->payload);
    if (!text.empty())
        console.writeln(text);
}

void FT8RxView::on_status(const FT8RxStatusMessage* message) {
    /* A new slot starts here, so a station calling again is worth showing again. */
    recent_count = 0;
    recent_index = 0;

    decodes_total += message->decode_count;

    switch (message->state) {
        case FT8RxStatusMessage::SyncState::Searching:
            text_status.set_style(Theme::getInstance()->fg_red);
            text_status.set("Searching for slot");
            break;
        case FT8RxStatusMessage::SyncState::Refining:
            text_status.set_style(Theme::getInstance()->fg_yellow);
            text_status.set("Syncing");
            break;
        case FT8RxStatusMessage::SyncState::Locked:
            text_status.set_style(Theme::getInstance()->fg_green);
            text_status.set("Locked   " + to_string_dec_uint(decodes_total) + " decoded");
            break;
    }
}

}  // namespace ui::external_app::ft8_rx
