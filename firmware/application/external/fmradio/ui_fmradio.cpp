/*
 * Copyright (C) 2024 HT Otto
 * Copyright (C) 2025 RocketGod - Added modes from my Flipper Zero RF Jammer App - https://betaskynet.com
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
#include "oversample.hpp"

using namespace portapack;
using namespace modems;
using namespace ui;

namespace ui::external_app::fmradio {

void FmRadioView::focus() {
    field_frequency.focus();
}

void FmRadioView::change_mode(int32_t mod) {
    field_bw.on_change = [this](size_t n, OptionsField::value_t) { (void)n; };

    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();

    audio_spectrum_update = false;                       // Reset spectrum update flag
    std::fill(audio_spectrum, audio_spectrum + 128, 0);  // Clear spectrum buffer

    ReceiverModel::Mode receiver_mode = static_cast<ReceiverModel::Mode>(mod);
    bool is_ssb = (mod == static_cast<int32_t>(ReceiverModel::Mode::AMAudio) &&
                   (field_modulation.selected_index() == 3 || field_modulation.selected_index() == 4));

    switch (mod) {
        case static_cast<int32_t>(ReceiverModel::Mode::AMAudio):
            audio_sampling_rate = audio::Rate::Hz_24000;  // Increased to 24 kHz for better AM/SSB audio
            freqman_set_bandwidth_option(0, field_bw);    // AM_MODULATION
            baseband::run_image(portapack::spi_flash::image_tag_am_audio);
            receiver_mode = ReceiverModel::Mode::AMAudio;
            field_bw.set_by_value(0);  // DSB default
            receiver_model.set_modulation(receiver_mode);
            if (is_ssb) {
                receiver_model.set_am_configuration(field_modulation.selected_index() == 3 ? 1 : 2);  // 1=USB, 2=LSB
            } else {
                receiver_model.set_am_configuration(0);  // DSB
            }
            field_bw.on_change = [this](size_t index, OptionsField::value_t n) {
                radio_bw = index;
                receiver_model.set_am_configuration(n);
            };
            break;
        case static_cast<int32_t>(ReceiverModel::Mode::NarrowbandFMAudio):
            audio_sampling_rate = audio::Rate::Hz_24000;
            freqman_set_bandwidth_option(1, field_bw);  // NFM_MODULATION
            baseband::run_image(portapack::spi_flash::image_tag_nfm_audio);
            receiver_mode = ReceiverModel::Mode::NarrowbandFMAudio;
            field_bw.set_by_value(2);  // 16k default
            receiver_model.set_nbfm_configuration(field_bw.selected_index_value());
            field_bw.on_change = [this](size_t index, OptionsField::value_t n) {
                radio_bw = index;
                receiver_model.set_nbfm_configuration(n);
            };
            break;
        case static_cast<int32_t>(ReceiverModel::Mode::WidebandFMAudio):
            audio_sampling_rate = audio::Rate::Hz_48000;
            freqman_set_bandwidth_option(2, field_bw);  // WFM_MODULATION
            baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);
            receiver_mode = ReceiverModel::Mode::WidebandFMAudio;
            field_bw.set_by_value(0);  // 200k default
            receiver_model.set_wfm_configuration(field_bw.selected_index_value());
            field_bw.on_change = [this](size_t index, OptionsField::value_t n) {
                radio_bw = index;
                receiver_model.set_wfm_configuration(n);
            };
            break;
        default:
            break;
    }

    receiver_model.set_modulation(receiver_mode);

    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_baseband_bandwidth(1750000);
    audio::set_rate(audio_sampling_rate);
    audio::output::start();
    receiver_model.set_headphone_volume(receiver_model.headphone_volume());  // WM8731 hack

    receiver_model.enable();
}

FmRadioView::FmRadioView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);

    add_children({&field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_volume,
                  &field_frequency,
                  &field_bw,
                  &text_mode_label,
                  &field_modulation,
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
                  &rssi});

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

    field_frequency.set_step(25000);
    change_mode(static_cast<int32_t>(ReceiverModel::Mode::WidebandFMAudio));
    field_modulation.set_by_value(static_cast<int32_t>(ReceiverModel::Mode::WidebandFMAudio));

    btn_fav_0.on_select = [this](Button&) { on_btn_clicked(0); };
    btn_fav_1.on_select = [this](Button&) { on_btn_clicked(1); };
    btn_fav_2.on_select = [this](Button&) { on_btn_clicked(2); };
    btn_fav_3.on_select = [this](Button&) { on_btn_clicked(3); };
    btn_fav_4.on_select = [this](Button&) { on_btn_clicked(4); };
    btn_fav_5.on_select = [this](Button&) { on_btn_clicked(5); };
    btn_fav_6.on_select = [this](Button&) { on_btn_clicked(6); };
    btn_fav_7.on_select = [this](Button&) { on_btn_clicked(7); };
    btn_fav_8.on_select = [this](Button&) { on_btn_clicked(8); };
    btn_fav_9.on_select = [this](Button&) { on_btn_clicked(9); };

    btn_fav_save.on_select = [this](Button&) {
        save_fav = !save_fav;
        txt_save_help.set_text(save_fav ? "Select slot" : "");
        txt_save_help.visible(save_fav);
        txt_save_help.set_dirty();
    };

    field_modulation.on_change = [this](size_t index, int32_t mod) {
        change_mode(mod);
        if (index == 3 || index == 4) {                               // USB or LSB
            receiver_model.set_am_configuration(index == 3 ? 1 : 2);  // 1=USB, 2=LSB
        }
    };

    update_fav_btn_texts();
}

void FmRadioView::on_btn_clicked(uint8_t i) {
    if (save_fav) {
        save_fav = false;
        freq_fav_list[i].frequency = field_frequency.value();
        freq_fav_list[i].modulation = field_modulation.selected_index_value();
        freq_fav_list[i].bandwidth = radio_bw;
        update_fav_btn_texts();
        txt_save_help.visible(save_fav);
        txt_save_help.set_text("");
        txt_save_help.set_dirty();
        return;
    }
    field_frequency.set_value(freq_fav_list[i].frequency);
    field_modulation.set_by_value(freq_fav_list[i].modulation);
    change_mode(freq_fav_list[i].modulation);
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
    if (audio_spectrum_data && audio_spectrum_data->db.size() <= 128) {
        for (size_t i = 0; i < audio_spectrum_data->db.size(); i++) {
            audio_spectrum[i] = ((int16_t)audio_spectrum_data->db[i] - 127) * 256;
        }
        waveform.set_dirty();
    } else {
        // Fallback: Clear waveform if no valid data
        std::fill(audio_spectrum, audio_spectrum + 128, 0);
        waveform.set_dirty();
    }
}

}  // namespace ui::external_app::fmradio
