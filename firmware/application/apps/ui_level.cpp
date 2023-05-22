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
#include "file.hpp"

using namespace portapack;
using portapack::memory::map::backup_ram;

namespace ui {

void LevelView::focus() {
    button_frequency.focus();
}

LevelView::~LevelView() {
    // save app settings
    app_settings.lna = field_lna.value();
    app_settings.vga = field_vga.value();
    app_settings.rx_amp = field_rf_amp.value();

    settings.save("level", &app_settings);

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
                  &audio_mode,
                  &peak_mode,
                  &rssi,
                  &rssi_graph});

    rssi.set_vertical_rssi(true);

    field_volume.on_change = [this](int32_t v) { this->on_headphone_volume_changed(v); };
    field_volume.set_value((receiver_model.headphone_volume() - audio::headphone::volume_range().max).decibel() + 99);

    change_mode(NFM_MODULATION);              // Start on AM
    field_mode.set_by_value(NFM_MODULATION);  // Reflect the mode into the manual selector

    // HELPER: Pre-setting a manual range, based on stored frequency
    freq = persistent_memory::tuned_frequency();
    receiver_model.set_tuning_frequency(freq);
    button_frequency.set_text("<" + to_string_short_freq(freq) + " MHz>");

    // load auto common app settings
    auto rc = settings.load("level", &app_settings);
    if (rc == SETTINGS_OK) {
        field_lna.set_value(app_settings.lna);
        field_vga.set_value(app_settings.vga);
        field_rf_amp.set_value(app_settings.rx_amp);
        receiver_model.set_rf_amp(app_settings.rx_amp);
    }

    button_frequency.on_select = [this, &nav](ButtonWithEncoder& button) {
        auto new_view = nav_.push<FrequencyKeypadView>(freq);
        new_view->on_changed = [this, &button](rf::Frequency f) {
            freq = f;
            receiver_model.set_tuning_frequency(f);  // Retune to actual freq
            button_frequency.set_text("<" + to_string_short_freq(freq) + " MHz>");
        };
    };

    button_frequency.on_change = [this]() {
        int64_t def_step = freqman_entry_get_step_value(step_mode.selected_index());
        freq = freq + (button_frequency.get_encoder_delta() * def_step);
        if (freq < 1) {
            freq = 1;
        }
        if (freq > (MAX_UFREQ - def_step)) {
            freq = MAX_UFREQ;
        }
        button_frequency.set_encoder_delta(0);

        receiver_model.set_tuning_frequency(freq);  // Retune to actual freq
        button_frequency.set_text("<" + to_string_short_freq(freq) + " MHz>");
    };

    field_mode.on_change = [this](size_t, OptionsField::value_t v) {
        if (v != -1) {
            receiver_model.disable();
            baseband::shutdown();
            change_mode(v);
            if (audio_mode.selected_index() != 0) {
                audio::output::start();
            }
            receiver_model.enable();
        }
    };

    rssi_resolution.on_change = [this](size_t, OptionsField::value_t v) {
        if (v != -1) {
            rssi_graph.set_nb_columns(v);
        }
    };

    audio_mode.on_change = [this](size_t, OptionsField::value_t v) {
        if (v == 0) {
            audio::output::stop();
        } else if (v == 1) {
            audio::output::start();
            this->on_headphone_volume_changed((receiver_model.headphone_volume() - audio::headphone::volume_range().max).decibel() + 99);
        } else {
        }
    };

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
    freqman_set_modulation_option(field_mode);
    freqman_set_step_option_short(step_mode);
    freq_stats_rssi.set_style(&style_white);
    freq_stats_db.set_style(&style_white);
}

void LevelView::on_headphone_volume_changed(int32_t v) {
    const auto new_volume = volume_t::decibel(v - 99) + audio::headphone::volume_range().max;
    receiver_model.set_headphone_volume(new_volume);
}

void LevelView::on_statistics_update(const ChannelStatistics& statistics) {
    static int last_max_db = -1000;
    static int last_min_rssi = -1000;
    static int last_avg_rssi = -1000;
    static int last_max_rssi = -1000;

    rssi_graph.add_values(rssi.get_min(), rssi.get_avg(), rssi.get_max(), statistics.max_db);

    bool refresh_db = false;
    bool refresh_rssi = false;

    if (last_max_db != statistics.max_db) {
        refresh_db = true;
    }
    if (last_min_rssi != rssi.get_min() || last_avg_rssi != rssi.get_avg() || last_max_rssi != rssi.get_max()) {
        refresh_rssi = true;
    }
    if (refresh_db) {
        last_max_db = statistics.max_db;
        freq_stats_db.set("Power: " + to_string_dec_int(statistics.max_db) + " db");
    }
    if (refresh_rssi) {
        last_min_rssi = rssi.get_min();
        last_avg_rssi = rssi.get_avg();
        last_max_rssi = rssi.get_max();
        freq_stats_rssi.set("RSSI: " + to_string_dec_int(rssi.get_min()) + "/" + to_string_dec_int(rssi.get_avg()) + "/" + to_string_dec_int(rssi.get_max()) + ",dt: " + to_string_dec_int(rssi.get_delta()));
    }
} /* on_statistic_updates */

size_t LevelView::change_mode(freqman_index_t new_mod) {
    field_bw.on_change = [this](size_t n, OptionsField::value_t) { (void)n; };

    switch (new_mod) {
        case AM_MODULATION:
            freqman_set_bandwidth_option(new_mod, field_bw);
            // bw DSB (0) default
            field_bw.set_selected_index(0);
            baseband::run_image(portapack::spi_flash::image_tag_am_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::AMAudio);
            receiver_model.set_am_configuration(field_bw.selected_index());
            field_bw.on_change = [this](size_t n, OptionsField::value_t) { receiver_model.set_am_configuration(n); };
            receiver_model.set_sampling_rate(3072000);
            receiver_model.set_baseband_bandwidth(1750000);
            text_ctcss.set("             ");
            break;
        case NFM_MODULATION:
            freqman_set_bandwidth_option(new_mod, field_bw);
            // bw 16k (2) default
            field_bw.set_selected_index(2);
            baseband::run_image(portapack::spi_flash::image_tag_nfm_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
            receiver_model.set_nbfm_configuration(field_bw.selected_index());
            field_bw.on_change = [this](size_t n, OptionsField::value_t) { receiver_model.set_nbfm_configuration(n); };
            receiver_model.set_sampling_rate(3072000);
            receiver_model.set_baseband_bandwidth(1750000);
            break;
        case WFM_MODULATION:
            freqman_set_bandwidth_option(new_mod, field_bw);
            // bw 200k (0) only/default
            field_bw.set_selected_index(0);
            baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
            receiver_model.set_wfm_configuration(field_bw.selected_index());
            field_bw.on_change = [this](size_t n, OptionsField::value_t) { receiver_model.set_wfm_configuration(n); };
            receiver_model.set_sampling_rate(3072000);
            receiver_model.set_baseband_bandwidth(1750000);
            text_ctcss.set("             ");
            break;
        default:
            break;
    }
    return step_mode.selected_index();
}

void LevelView::handle_coded_squelch(const uint32_t value) {
    static int32_t last_idx = -1;

    float diff, min_diff = value;
    size_t min_idx{0};
    size_t c;

    if (field_mode.selected_index() != NFM_MODULATION) {
        text_ctcss.set("             ");
        return;
    }

    // Find nearest match
    for (c = 0; c < tone_keys.size(); c++) {
        diff = abs(((float)value / 100.0) - tone_keys[c].second);
        if (diff < min_diff) {
            min_idx = c;
            min_diff = diff;
        }
    }

    // Arbitrary confidence threshold
    if (last_idx < 0 || (unsigned)last_idx != min_idx) {
        last_idx = min_idx;
        if (min_diff < 40)
            text_ctcss.set("T: " + tone_keys[min_idx].first);
        else
            text_ctcss.set("             ");
    }
}

} /* namespace ui */
