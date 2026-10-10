/*
 * DECT RX application UI for the PortaPack Mayhem DECT RX.
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

#include "ui_dect_rx.hpp"

#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack.hpp"
#include "dect_channels.hpp"

#include <cstring>

using namespace portapack;
using namespace ui;

namespace ui::external_app::dect_rx {

DECTRxView::DECTRxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_dect);

    add_children({&field_frequency,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &rssi,
                  &channel,
                  &field_band,
                  &field_channel,
                  &text_status,
                  &console});

    field_frequency.set_step(1000);

    field_band.on_change = [this](size_t, int32_t v) {
        band_ = static_cast<uint8_t>(v);
        this->apply_selection();
    };

    field_channel.on_change = [this](size_t, int32_t) {
        this->apply_selection();
    };

    receiver_model.enable();

    apply_selection();
}

DECTRxView::~DECTRxView() {
    receiver_model.disable();
    baseband::shutdown();
}

void DECTRxView::focus() {
    field_channel.focus();
}

void DECTRxView::apply_selection() {
    const dect::DectBand band = (band_ == 0) ? dect::DectBand::US : dect::DectBand::EU;
    const size_t ch = field_channel.selected_index();
    const uint64_t freq = dect::dect_channels(band)[ch].freq_hz;

    /* The LO leakage spike is the problem on this front-end: tuned exactly
     * on-channel it sits right on the DECT carrier and the DC blocker then
     * has to notch the carrier itself (which measurably raised the A-field
     * error rate). Parking the LO ~20 kHz off-channel keeps the carrier in the
     * blocker's passband while the spike stays inside its narrow notch. At
     * 4.608 Msps that is a constant ~1.6-degree phase bias, harmless. */
    receiver_model.set_target_frequency_with_hidden_offset(
        freq, receiver_model.sampling_rate() / 4 + 20000);
    field_frequency.set_value(freq);

    /* Reset the M4 decoder state on every retune. */
    baseband::set_dect_config(band_, static_cast<uint32_t>(freq));

    for (int i = 0; i < 8; i++) last_logged_valid_[i] = false;
    console.clear(true);
    text_status.set("0 part(s)  voice:0");
}

void DECTRxView::on_part_update(const DECTPartInfoMessage* message) {
    for (int i = 0; i < message->count; i++) {
        const DECTPartInfo& p = message->parts[i];

        if (!p.part_id_valid) continue;
        if (p.rx_id < 0 || p.rx_id >= 8) continue;

        const bool same = last_logged_valid_[p.rx_id] &&
                          (memcmp(last_logged_id_[p.rx_id], p.part_id, 5) == 0);
        if (same) continue;

        memcpy(last_logged_id_[p.rx_id], p.part_id, 5);
        last_logged_valid_[p.rx_id] = true;

        std::string line = (p.type == 0) ? "RFP" : "PP ";
        line += " s" + to_string_dec_uint(p.slot, 2);
        line += " ";
        for (int b = 0; b < 5; b++) line += to_string_hex(p.part_id[b], 2);
        line += (p.qt_synced) ? " qt" : " --";
        line += (p.voice_present) ? " voice" : " -----";

        console.writeln(line);
    }
}

void DECTRxView::on_status(const DECTStatusMessage* message) {
    if (!message->configured) {
        text_status.set("not configured");
        return;
    }

    std::string s = to_string_dec_uint(message->parts) + " part(s)  voice:" +
                    to_string_dec_uint(message->voice_parts);
    text_status.set(s);
}

void DECTRxView::on_freqchg(int64_t freq) {
    field_frequency.set_value(freq);
}

}  // namespace ui::external_app::dect_rx
