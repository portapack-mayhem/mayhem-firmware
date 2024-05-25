/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
 * Copyright (C) 2023 gullradriel, Nilorea Studio Inc.
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

#include "ui_level.hpp"
#include "ui_fileman.hpp"
#include "ui_freqman.hpp"
#include "baseband_api.hpp"
#include "file.hpp"
#include "oversample.hpp"
#include "ui_font_fixed_8x16.hpp"

using namespace portapack;
using namespace tonekey;
using portapack::memory::map::backup_ram;

namespace ui {

// Function to map the value from one range to another
int32_t LevelView::map(int32_t value, int32_t fromLow, int32_t fromHigh, int32_t toLow, int32_t toHigh) {
    return toLow + (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow);
}

void LevelView::m4_manage_stat_update() {
    if (audio_mode) {
        if (radio_mode == WFM_MODULATION || radio_mode == SPEC_MODULATION) {
            shared_memory.request_m4_performance_counter = 1;
        } else {
            shared_memory.request_m4_performance_counter = 2;
        }
        if (radio_mode == SPEC_MODULATION) {
            beep = true;
        }
    } else {
        shared_memory.request_m4_performance_counter = 2;
        if (radio_mode == SPEC_MODULATION) {
            beep = false;
            baseband::request_beep_stop();
        }
    }
}

void LevelView::focus() {
    button_frequency.focus();
}

LevelView::~LevelView() {
    // reset performance counters request to default
    shared_memory.request_m4_performance_counter = 0;
    receiver_model.disable();
    baseband::shutdown();
}

LevelView::LevelView(NavigationView& nav)
    : nav_{nav} {
    add_children({&labels,
                  &field_lna,
                  &field_vga,
                  &field_rf_amp,
                  &field_volume,
                  &field_bw,
                  &field_mode,
                  &step_mode,
                  &rssi_resolution,
                  &button_frequency,
                  &text_ctcss,
                  &freq_stats_rssi,
                  &freq_stats_db,
                  &freq_stats_rx,
                  &text_beep_squelch,
                  &field_beep_squelch,
                  &field_audio_mode,
                  &peak_mode,
                  &rssi,
                  &rssi_graph});

    // activate vertical bar mode
    rssi.set_vertical_rssi(true);

    freq_ = receiver_model.target_frequency();
    button_frequency.set_text("<" + to_string_short_freq(freq_) + " MHz>");

    button_frequency.on_select = [this, &nav](ButtonWithEncoder& button) {
        auto new_view = nav_.push<FrequencyKeypadView>(freq_);
        new_view->on_changed = [this, &button](rf::Frequency f) {
            freq_ = f;
            receiver_model.set_target_frequency(f);  // Retune to actual freq
            button_frequency.set_text("<" + to_string_short_freq(freq_) + " MHz>");
        };
    };

    field_beep_squelch.set_value(beep_squelch);
    field_beep_squelch.on_change = [this](int32_t v) {
        beep_squelch = v;
    };

    button_frequency.on_change = [this]() {
        int64_t def_step = freqman_entry_get_step_value(step_mode.selected_index());
        freq_ = freq_ + (button_frequency.get_encoder_delta() * def_step);
        if (freq_ < 1) {
            freq_ = 1;
        }
        if (freq_ > (MAX_UFREQ - def_step)) {
            freq_ = MAX_UFREQ;
        }
        button_frequency.set_encoder_delta(0);

        receiver_model.set_target_frequency(freq_);  // Retune to actual freq
        button_frequency.set_text("<" + to_string_short_freq(freq_) + " MHz>");
    };

    freqman_set_modulation_option(field_mode);
    field_mode.on_change = [this](size_t, OptionsField::value_t v) {
        if (v != -1) {
            change_mode(v);
        }
    };
    field_mode.set_by_value(radio_mode);  // Reflect the mode into the manual selector
    field_bw.set_selected_index(radio_bw);

    rssi_resolution.on_change = [this](size_t, OptionsField::value_t v) {
        if (v != -1) {
            rssi_graph.set_nb_columns(v);
        }
    };

    field_audio_mode.on_change = [this](size_t, OptionsField::value_t v) {
        audio_mode = v;
        if (v == 0) {
            audio::output::stop();
        } else if (v == 1) {
            audio::set_rate(audio_sampling_rate);
            audio::output::start();
            receiver_model.set_headphone_volume(receiver_model.headphone_volume());  // WM8731 hack.
        }
        m4_manage_stat_update();  // rx_sat hack
    };
    field_audio_mode.set_selected_index(audio_mode);

    peak_mode.on_change = [this](size_t, OptionsField::value_t v) {
        if (v == 0) {
            rssi.set_peak(false, 0);
        } else {
            rssi.set_peak(true, v);
        }
    };

    // default peak value
    peak_mode.set_selected_index(2);
    rssi_resolution.set_selected_index(1);
    // FILL STEP OPTIONS
    freqman_set_step_option_short(step_mode);
    freq_stats_rssi.set_style(Theme::getInstance()->bg_darkest);
    freq_stats_db.set_style(Theme::getInstance()->bg_darkest);
    freq_stats_rx.set_style(Theme::getInstance()->bg_darkest);
}

void LevelView::on_statistics_update(const ChannelStatistics& statistics) {
    static int16_t last_max_db = 0;
    static uint8_t last_min_rssi = 0;
    static uint8_t last_avg_rssi = 0;
    static uint8_t last_max_rssi = 0;
    static uint8_t last_rx_sat = 0;

    rssi_graph.add_values(rssi.get_min(), rssi.get_avg(), rssi.get_max(), statistics.max_db);

    // refresh db
    if (last_max_db != statistics.max_db) {
        last_max_db = statistics.max_db;
        freq_stats_db.set("Power: " + to_string_dec_int(statistics.max_db) + " db");
    }
    // refresh rssi
    if (last_min_rssi != rssi_graph.get_graph_min() || last_avg_rssi != rssi_graph.get_graph_avg() || last_max_rssi != rssi_graph.get_graph_max()) {
        last_min_rssi = rssi_graph.get_graph_min();
        last_avg_rssi = rssi_graph.get_graph_avg();
        last_max_rssi = rssi_graph.get_graph_max();
        freq_stats_rssi.set("RSSI: " + to_string_dec_uint(last_min_rssi) + "/" + to_string_dec_uint(last_avg_rssi) + "/" + to_string_dec_uint(last_max_rssi));
    }

    if (beep && statistics.max_db > beep_squelch) {
        baseband::request_audio_beep(map(statistics.max_db, -100, 20, 400, 2600), 24000, 150);
    }

    // refresh sat
    if (radio_mode == SPEC_MODULATION || (radio_mode == WFM_MODULATION && audio_mode == 1)) {
        Style style_freq_stats_rx{
            .font = font::fixed_8x16,
            .background = {55, 55, 55},
            .foreground = {155, 155, 155},
        };
        freq_stats_rx.set_style(&style_freq_stats_rx);
        freq_stats_rx.set("RxSat off");
        return;
    }
    uint8_t rx_sat = ((uint32_t)shared_memory.m4_performance_counter) * 100 / 127;
    if (last_rx_sat != rx_sat) {
        last_rx_sat = rx_sat;

        uint8_t br = 0;
        uint8_t bg = 0;
        uint8_t bb = 0;
        if (rx_sat <= 80) {
            bg = (255 * rx_sat) / 80;
            bb = 255 - bg;
        } else if (rx_sat > 80) {
            br = (255 * (rx_sat - 80)) / 20;
            bg = 255 - br;
        }
        Style style_freq_stats_rx{
            .font = font::fixed_8x16,
            .background = {br, bg, bb},
            .foreground = {255, 255, 255},
        };
        freq_stats_rx.set_style(&style_freq_stats_rx);
        freq_stats_rx.set("RxSat: " + to_string_dec_uint(rx_sat) + "%");
    }

} /* on_statistic_updates */

size_t LevelView::change_mode(freqman_index_t new_mod) {
    field_bw.on_change = [this](size_t n, OptionsField::value_t) { (void)n; };

    radio_mode = new_mod;

    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();

    switch (new_mod) {
        case AM_MODULATION:
            audio_sampling_rate = audio::Rate::Hz_12000;
            freqman_set_bandwidth_option(new_mod, field_bw);
            baseband::run_image(portapack::spi_flash::image_tag_am_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::AMAudio);
            // bw DSB (0) default
            field_bw.set_by_value(0);
            receiver_model.set_am_configuration(0);
            field_bw.on_change = [this](size_t index, OptionsField::value_t n) { radio_bw = index ; receiver_model.set_am_configuration(n); };
            break;
        case NFM_MODULATION:
            audio_sampling_rate = audio::Rate::Hz_24000;
            freqman_set_bandwidth_option(new_mod, field_bw);
            baseband::run_image(portapack::spi_flash::image_tag_nfm_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
            receiver_model.set_nbfm_configuration(field_bw.selected_index_value());
            // bw 16k (2) default
            field_bw.set_by_value(2);
            field_bw.on_change = [this](size_t index, OptionsField::value_t n) { radio_bw = index ; receiver_model.set_nbfm_configuration(n); };
            break;
        case WFM_MODULATION:
            audio_sampling_rate = audio::Rate::Hz_48000;
            freqman_set_bandwidth_option(new_mod, field_bw);
            baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
            receiver_model.set_wfm_configuration(field_bw.selected_index_value());
            // bw 200k (0) default
            field_bw.set_by_value(0);
            field_bw.on_change = [this](size_t index, OptionsField::value_t n) { radio_bw = index ; receiver_model.set_wfm_configuration(n); };
            break;
        case SPEC_MODULATION:
            audio_sampling_rate = audio::Rate::Hz_24000;
            freqman_set_bandwidth_option(new_mod, field_bw);
            baseband::run_image(portapack::spi_flash::image_tag_capture);
            receiver_model.set_modulation(ReceiverModel::Mode::Capture);
            // 12k5 (0) default
            field_bw.on_change = [this](size_t index, OptionsField::value_t sampling_rate) {
                radio_bw = index;
                // Baseband needs to know the desired sampling and oversampling rates.
                baseband::set_sample_rate(sampling_rate, get_oversample_rate(sampling_rate));
                // The radio needs to know the effective sampling rate.
                auto actual_sampling_rate = get_actual_sample_rate(sampling_rate);
                receiver_model.set_sampling_rate(actual_sampling_rate);
                receiver_model.set_baseband_bandwidth(filter_bandwidth_for_sampling_rate(actual_sampling_rate));
            };
            field_bw.set_by_value(0);
        default:
            break;
    }
    if (new_mod != SPEC_MODULATION) {
        // Reset receiver model to fix bug when going from SPEC to audio, the sound is distorted.
        receiver_model.set_sampling_rate(3072000);
        receiver_model.set_baseband_bandwidth(1750000);
    }
    if (new_mod != NFM_MODULATION) {
        text_ctcss.set("             ");
    }

    m4_manage_stat_update();  // rx_sat hack

    if (audio_mode) {
        audio::set_rate(audio_sampling_rate);
        audio::output::start();
        receiver_model.set_headphone_volume(receiver_model.headphone_volume());  // WM8731 hack.
    }
    receiver_model.enable();

    return step_mode.selected_index();
}

void LevelView::handle_coded_squelch(const uint32_t value) {
    if (field_mode.selected_index() == NFM_MODULATION)
        text_ctcss.set(tone_key_string_by_value(value, text_ctcss.parent_rect().width() / 8));
    else
        text_ctcss.set("        ");
}

} /* namespace ui */
