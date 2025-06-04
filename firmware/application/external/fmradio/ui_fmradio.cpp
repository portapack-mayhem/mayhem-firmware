/*
 * Copyright (C) 2024 HTotoo
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

#include "ui_fmradio.hpp"

#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace modems;
using namespace ui;

namespace ui::external_app::fmradio {

void FmRadioView::focus() {
    field_frequency.focus();
}

void FmRadioView::update_baseband(ReceiverModel::Mode mod) {
    receiver_model.disable();
    baseband::shutdown();
    if (mod == ReceiverModel::Mode::WidebandFMAudio) {
        baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);
    } else if (mod == ReceiverModel::Mode::AMAudio) {
        baseband::run_image(portapack::spi_flash::image_tag_am_audio);
    }
    receiver_model.set_modulation(mod);
    receiver_model.enable();
}

FmRadioView::FmRadioView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);

    add_children({&rssi,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_volume,
                  &field_frequency,
                  &btn_fav_save,
                  &txt_save_help,
                  &btn_fav_0,
                  &btn_fav_1,
                  &btn_fav_2,
                  &btn_fav_3,
                  &btn_fav_4,
                  &btn_fav_5,
                  &btn_fav_6,
                  &btn_fav_7,
                  &btn_fav_8,
                  &btn_fav_9,
                  &audio,
                  &waveform,
                  &field_modulation});

    txt_save_help.visible(false);
    for (uint8_t i = 0; i < 12; ++i) {
        if (freq_fav_list[i].frequency == 0) {
            freq_fav_list[i].frequency = 87000000;
            freq_fav_list[i].modulation = static_cast<int32_t>(ReceiverModel::Mode::WidebandFMAudio);
        }
    }

    if (field_frequency.value() == 0) {
        field_frequency.set_value(87000000);
    }

    receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
    field_modulation.set_by_value(static_cast<int32_t>(ReceiverModel::Mode::WidebandFMAudio));

    field_frequency.set_step(25000);
    receiver_model.enable();
    audio::output::start();

    btn_fav_0.on_select = [this](Button&) {
        on_btn_clicked(0);
    };
    btn_fav_1.on_select = [this](Button&) {
        on_btn_clicked(1);
    };
    btn_fav_2.on_select = [this](Button&) {
        on_btn_clicked(2);
    };
    btn_fav_3.on_select = [this](Button&) {
        on_btn_clicked(3);
    };
    btn_fav_4.on_select = [this](Button&) {
        on_btn_clicked(4);
    };
    btn_fav_5.on_select = [this](Button&) {
        on_btn_clicked(5);
    };
    btn_fav_6.on_select = [this](Button&) {
        on_btn_clicked(6);
    };
    btn_fav_7.on_select = [this](Button&) {
        on_btn_clicked(7);
    };
    btn_fav_8.on_select = [this](Button&) {
        on_btn_clicked(8);
    };
    btn_fav_9.on_select = [this](Button&) {
        on_btn_clicked(9);
    };

    btn_fav_save.on_select = [this](Button&) {
        save_fav = !save_fav;
        txt_save_help.set_text(save_fav ? "Select slot" : "");
        txt_save_help.visible(save_fav);
        txt_save_help.set_dirty();
    };

    field_modulation.on_change = [this](size_t, int32_t mod) {
        update_baseband(static_cast<ReceiverModel::Mode>(mod));
    };

    update_fav_btn_texts();
}

void FmRadioView::on_btn_clicked(uint8_t i) {
    if (save_fav) {
        save_fav = false;
        freq_fav_list[i].frequency = field_frequency.value();
        freq_fav_list[i].modulation = static_cast<int32_t>(receiver_model.modulation());
        update_fav_btn_texts();
        txt_save_help.visible(save_fav);
        txt_save_help.set_text("");
        txt_save_help.set_dirty();
        return;
    }
    field_frequency.set_value(freq_fav_list[i].frequency);
    update_baseband(static_cast<ReceiverModel::Mode>(freq_fav_list[i].modulation));
    field_modulation.set_by_value(freq_fav_list[i].modulation);
}

std::string FmRadioView::to_nice_freq(rf::Frequency freq) {
    std::string nice = to_string_dec_uint(freq / 1000000);
    nice += ".";
    nice += to_string_dec_uint((freq / 10000) % 100);
    return nice;
}

void FmRadioView::update_fav_btn_texts() {
    btn_fav_0.set_text(to_nice_freq(freq_fav_list[0].frequency));
    btn_fav_1.set_text(to_nice_freq(freq_fav_list[1].frequency));
    btn_fav_2.set_text(to_nice_freq(freq_fav_list[2].frequency));
    btn_fav_3.set_text(to_nice_freq(freq_fav_list[3].frequency));
    btn_fav_4.set_text(to_nice_freq(freq_fav_list[4].frequency));
    btn_fav_5.set_text(to_nice_freq(freq_fav_list[5].frequency));
    btn_fav_6.set_text(to_nice_freq(freq_fav_list[6].frequency));
    btn_fav_7.set_text(to_nice_freq(freq_fav_list[7].frequency));
    btn_fav_8.set_text(to_nice_freq(freq_fav_list[8].frequency));
    btn_fav_9.set_text(to_nice_freq(freq_fav_list[9].frequency));
}

FmRadioView::~FmRadioView() {
    receiver_model.disable();
    baseband::shutdown();
    audio::output::stop();
}

void FmRadioView::on_audio_spectrum() {
    for (size_t i = 0; i < audio_spectrum_data->db.size(); i++)
        audio_spectrum[i] = ((int16_t)audio_spectrum_data->db[i] - 127) * 256;
    waveform.set_dirty();
}

}  // namespace ui::external_app::fmradio
