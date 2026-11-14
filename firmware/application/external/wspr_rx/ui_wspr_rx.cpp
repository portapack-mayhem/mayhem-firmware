/*
 * Copyright (C) 2026 Ivan
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

#include "ui_wspr_rx.hpp"

#include "audio.hpp"
#include "baseband_api.hpp"
#include "file_path.hpp"
#include "rtc_time.hpp"
#include "string_format.hpp"

using namespace portapack;

namespace ui::external_app::wspr_rx {

void WSPRRxView::focus() {
    field_frequency.focus();
}

WSPRRxView::~WSPRRxView() {
    receiver_model.disable();
    audio::output::stop();
    baseband::shutdown();
}

WSPRRxView::WSPRRxView(NavigationView& nav)
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

    baseband::run_image(portapack::spi_flash::image_tag_wspr_rx);

    /* WSPR occupies a few Hz per signal inside a 200 Hz sub-band, but the decoder needs
     * the whole sub-band at once, so the receiver runs wide and the channel filter
     * narrows it. set_modulation() is deliberately not called: it would replace the WSPR
     * baseband image. */
    receiver_model.set_sampling_rate(sampling_rate);
    receiver_model.set_baseband_bandwidth(baseband_bandwidth);
    receiver_model.enable();

    audio::output::start();

    log_file_.append(logs_dir / u"WSPR.TXT");
}

void WSPRRxView::on_packet(const WSPRPacketMessage* message) {
    if (message->callsign[0] == '\0')
        return;

    /* "callsign grid report SNR" takes the left of the line and the audio frequency the
     * right, so the frequencies line up in a column down the screen and the passband can
     * be read at a glance. A line too long to leave room for the column keeps its text,
     * and the frequency moves down a row and stays in the column. */
    std::string text{message->callsign};
    text += " ";
    text += message->grid;
    text += " ";
    text += to_string_dec_int(message->dbm);
    text += "dB ";
    text += to_string_dec_int(message->snr_db);
    text += "dB";
    const std::string freq = to_string_dec_uint(message->freq_hz) + "Hz";
    /* screen_width is set at runtime, so the column count is taken here rather than
     * fixed at compile time. The font is the 8 px fixed one the rest of the layout uses. */
    const size_t columns = screen_width / 8;

    /* Padding is measured before any colour escape would go on, because an escape
     * occupies bytes of the string and no columns on screen. */
    if (text.length() + freq.length() < columns)
        text.append(columns - freq.length() - text.length(), ' ');
    else
        text += '\n' + std::string(columns - freq.length(), ' ');

    console.writeln(text + freq);

    decodes_total++;
    text_decodes.set(to_string_dec_uint(decodes_total) + (decodes_total < 10000 ? " RX" : "RX"));

    /* One line per report: callsign, grid, the station's own dBm, the decoder's rough
     * SNR and the audio frequency, prefixed by the logger with the RTC timestamp — the
     * fields wsprnet.org reports are built from, minus the upload. */
    rtc::RTC datetime;
    rtcGetTime(&RTCD1, &datetime);
    const std::string entry = std::string{message->callsign} + " " + message->grid + " " +
                              to_string_dec_int(message->dbm) + " " +
                              to_string_dec_int(message->snr_db) + " " +
                              to_string_dec_uint(message->freq_hz);
    log_file_.write_entry(datetime, entry);
}

void WSPRRxView::on_status(const WSPRRxStatusMessage* message) {
    switch (message->state) {
        case WSPRRxStatusMessage::SyncState::Searching:
            text_status.set_style(Theme::getInstance()->fg_red);
            text_status.set("Searching");
            break;
        case WSPRRxStatusMessage::SyncState::Heard:
            text_status.set_style(Theme::getInstance()->fg_yellow);
            text_status.set("Heard, no decode");
            break;
        case WSPRRxStatusMessage::SyncState::Syncing:
            text_status.set_style(Theme::getInstance()->fg_yellow);
            text_status.set("Syncing");
            break;
        case WSPRRxStatusMessage::SyncState::Locked:
            text_status.set_style(Theme::getInstance()->fg_green);
            text_status.set("Locked");
            break;
    }
}

}  // namespace ui::external_app::wspr_rx
