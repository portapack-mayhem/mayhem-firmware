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

#include "ui_detector_rx.hpp"
#include "ui_fileman.hpp"
#include "ui_freqman.hpp"
#include "baseband_api.hpp"
#include "file.hpp"
#include "oversample.hpp"
#include "ui_font_fixed_8x16.hpp"

using namespace portapack;
using namespace tonekey;
using portapack::memory::map::backup_ram;

namespace ui::external_app::detector_rx {

// Function to map the value from one range to another
int32_t DetectorRxView::map(int32_t value, int32_t fromLow, int32_t fromHigh, int32_t toLow, int32_t toHigh) {
    return toLow + (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow);
}

void DetectorRxView::focus() {
    field_lna.focus();
}

DetectorRxView::~DetectorRxView() {
    // reset performance counters request to default
    shared_memory.request_m4_performance_counter = 0;
    receiver_model.disable();
    audio::output::stop();
    baseband::shutdown();
}

void DetectorRxView::on_timer() {
    freq_index++;
    if (freq_mode == 0 && freq_index >= tetra_uplink_monitoring_frequencies_hz.size()) freq_index = 0;  // TETRA UP
    if (freq_mode == 1 && freq_index >= lora_monitoring_frequencies_hz.size()) freq_index = 0;          // Lora
    if (freq_mode == 2 && freq_index >= remotes_monitoring_frequencies_hz.size()) freq_index = 0;       // Remotes

    if (freq_mode == 2)
        freq_ = remotes_monitoring_frequencies_hz[freq_index];
    else if (freq_mode == 1)
        freq_ = lora_monitoring_frequencies_hz[freq_index];
    else
        freq_ = tetra_uplink_monitoring_frequencies_hz[freq_index];
    receiver_model.set_target_frequency(freq_);
}

DetectorRxView::DetectorRxView(NavigationView& nav)
    : nav_{nav} {
    add_children({
        &labels,
        &field_lna,
        &field_vga,
        &field_rf_amp,
        &field_volume,
        &text_frequency,
        &freq_stats_rssi,
        &freq_stats_db,
        &text_beep_squelch,
        &field_beep_squelch,
        &rssi,
        &rssi_graph,
        &field_mode,
    });

    // activate vertical bar mode
    rssi.set_vertical_rssi(true);

    freq_ = receiver_model.target_frequency();

    field_beep_squelch.set_value(beep_squelch);
    field_beep_squelch.on_change = [this](int32_t v) {
        beep_squelch = v;
    };

    rssi_graph.set_nb_columns(256);

    change_mode();
    rssi.set_peak(true, 3000);

    // FILL STEP OPTIONS
    freq_stats_rssi.set_style(Theme::getInstance()->bg_darkest);
    freq_stats_db.set_style(Theme::getInstance()->bg_darkest);

    field_mode.on_change = [this](size_t, int32_t value) {
        freq_mode = value;
        freq_index = 0;
        switch (value) {
            case 1:  // Lora
                text_frequency.set("   433, 868, 915 Mhz");
                break;
            case 2:  // Remotes
                text_frequency.set("        433, 315 Mhz");
                break;
            default:
            case 0:  // TETRA UP
                text_frequency.set("         380-390 Mhz");
                break;
        }
    };
}

void DetectorRxView::on_statistics_update(const ChannelStatistics& statistics) {
    static int16_t last_max_db = 0;
    static uint8_t last_min_rssi = 0;
    static uint8_t last_avg_rssi = 0;
    static uint8_t last_max_rssi = 0;

    rssi_graph.add_values(rssi.get_min(), rssi.get_avg(), rssi.get_max(), statistics.max_db);

    // refresh db
    if (last_max_db != statistics.max_db) {
        last_max_db = statistics.max_db;
        freq_stats_db.set("Power: " + to_string_dec_int(statistics.max_db) + " db");
        rssi.set_db(statistics.max_db);
    }
    // refresh rssi
    if (last_min_rssi != rssi_graph.get_graph_min() || last_avg_rssi != rssi_graph.get_graph_avg() || last_max_rssi != rssi_graph.get_graph_max()) {
        last_min_rssi = rssi_graph.get_graph_min();
        last_avg_rssi = rssi_graph.get_graph_avg();
        last_max_rssi = rssi_graph.get_graph_max();
        freq_stats_rssi.set("RSSI: " + to_string_dec_uint(last_min_rssi) + "/" + to_string_dec_uint(last_avg_rssi) + "/" + to_string_dec_uint(last_max_rssi));
    }

    if (statistics.max_db > beep_squelch) {
        baseband::request_audio_beep(map(statistics.max_db, -100, 20, 400, 2600), 24000, 150);
    }

} /* on_statistic_updates */

size_t DetectorRxView::change_mode() {
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();

    audio_sampling_rate = audio::Rate::Hz_24000;
    baseband::run_image(portapack::spi_flash::image_tag_capture);
    receiver_model.set_modulation(ReceiverModel::Mode::Capture);

    baseband::set_sample_rate(DETECTOR_BW, get_oversample_rate(DETECTOR_BW));
    // The radio needs to know the effective sampling rate.
    auto actual_sampling_rate = get_actual_sample_rate(DETECTOR_BW);
    receiver_model.set_sampling_rate(actual_sampling_rate);
    receiver_model.set_baseband_bandwidth(filter_bandwidth_for_sampling_rate(actual_sampling_rate));

    audio::set_rate(audio_sampling_rate);
    audio::output::start();
    receiver_model.set_headphone_volume(receiver_model.headphone_volume());  // WM8731 hack.

    receiver_model.enable();

    return 0;
}

}  // namespace ui::external_app::detector_rx
