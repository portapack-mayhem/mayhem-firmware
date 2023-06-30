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

    change_mode(NFM_MODULATION);              // Start on AM
    field_mode.set_by_value(NFM_MODULATION);  // Reflect the mode into the manual selector

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
            receiver_model.set_headphone_volume(receiver_model.headphone_volume());  // WM8731 hack.
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
    freq_stats_rssi.set_style(&Styles::white);
    freq_stats_db.set_style(&Styles::white);
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
            field_bw.set_by_value(0);
            baseband::run_image(portapack::spi_flash::image_tag_am_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::AMAudio);
            receiver_model.set_am_configuration(field_bw.selected_index_value());
            field_bw.on_change = [this](size_t, OptionsField::value_t n) { receiver_model.set_am_configuration(n); };
            text_ctcss.set("             ");
            break;
        case NFM_MODULATION:
            freqman_set_bandwidth_option(new_mod, field_bw);
            // bw 16k (2) default
            field_bw.set_by_value(2);
            baseband::run_image(portapack::spi_flash::image_tag_nfm_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
            receiver_model.set_nbfm_configuration(field_bw.selected_index_value());
            field_bw.on_change = [this](size_t, OptionsField::value_t n) { receiver_model.set_nbfm_configuration(n); };
            break;
        case WFM_MODULATION:
            freqman_set_bandwidth_option(new_mod, field_bw);
            // bw 200k (0) only/default
            field_bw.set_by_value(0);
            baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
            receiver_model.set_wfm_configuration(field_bw.selected_index_value());
            field_bw.on_change = [this](size_t, OptionsField::value_t n) { receiver_model.set_wfm_configuration(n); };
            text_ctcss.set("             ");
            break;
        case SPEC_MODULATION:
            freqman_set_bandwidth_option(new_mod, field_bw);
            baseband::run_image(portapack::spi_flash::image_tag_capture);
            receiver_model.set_modulation(ReceiverModel::Mode::Capture);
            field_bw.set_by_value(0);
            field_bw.on_change = [this](size_t, OptionsField::value_t sampling_rate) {
                uint32_t anti_alias_baseband_bandwidth_filter = 2500000;
                switch (sampling_rate) {                                 // we use the var fs (sampling_rate) , to set up BPF aprox < fs_max/2 by Nyquist theorem.
                    case 0 ... 2000000:                                  // BW Captured range  (0 <= 250kHz max )  fs = 8 x 250 kHz
                        anti_alias_baseband_bandwidth_filter = 1750000;  // Minimum BPF MAX2837 for all those lower BW options.
                        break;
                    case 4000000 ... 6000000:                            // BW capture  range (500k ... 750kHz max )  fs_max = 8 x 750kHz = 6Mhz
                                                                         // BW 500k ... 750kHz   ,  ex. 500kHz   (fs = 8*BW =  4Mhz) , BW 600kHz (fs = 4,8Mhz) , BW  750 kHz (fs = 6Mhz)
                        anti_alias_baseband_bandwidth_filter = 2500000;  // in some IC MAX2837 appear 2250000 , but both works similar.
                        break;
                    case 8800000:  // BW capture 1,1Mhz  fs = 8 x 1,1Mhz = 8,8Mhz . (1Mhz showed slightly higher noise background).
                        anti_alias_baseband_bandwidth_filter = 3500000;
                        break;
                    case 14000000:  // BW capture 1,75Mhz  , fs = 8 x 1,75Mhz = 14Mhz
                                    // good BPF, good matching, but LCD making flicker , refresh rate should be < 20 Hz , but reasonable picture
                        anti_alias_baseband_bandwidth_filter = 5000000;
                        break;
                    case 16000000:  // BW capture 2Mhz  , fs = 8 x 2Mhz = 16Mhz
                                    // good BPF, good matching, but LCD making flicker , refresh rate should be < 20 Hz , but reasonable picture
                        anti_alias_baseband_bandwidth_filter = 6000000;
                        break;
                    case 20000000:  // BW capture 2,5Mhz  , fs= 8 x 2,5 Mhz = 20Mhz
                                    // good BPF, good matching, but LCD making flicker , refresh rate should be < 20 Hz , but reasonable picture
                        anti_alias_baseband_bandwidth_filter = 7000000;
                        break;
                    default:  // BW capture 2,75Mhz, fs = 8 x 2,75Mhz= 22Mhz max ADC sampling) and others.
                              //  We tested also 9Mhz FPB stightly too much noise floor, better 8Mhz
                        anti_alias_baseband_bandwidth_filter = 8000000;
                }
                receiver_model.set_sampling_rate(sampling_rate);
                receiver_model.set_baseband_bandwidth(anti_alias_baseband_bandwidth_filter);
            };
        default:
            break;
    }
    return step_mode.selected_index();
}

void LevelView::handle_coded_squelch(const uint32_t value) {
    static tone_index last_squelch_index = -1;

    if (field_mode.selected_index() == NFM_MODULATION) {
        tone_index idx = tone_key_index_by_value(value);

        if ((last_squelch_index < 0) || (last_squelch_index != idx)) {
            last_squelch_index = idx;
            if (idx >= 0) {
                text_ctcss.set("T: " + tone_key_string(idx));
                return;
            }
        } else {
            return;
        }
    }

    text_ctcss.set("             ");
}

} /* namespace ui */
