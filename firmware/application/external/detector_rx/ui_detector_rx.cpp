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
#include "file_path.hpp"
#include "oversample.hpp"
#include "ui_font_fixed_8x16.hpp"

using namespace portapack;
using namespace tonekey;
using portapack::memory::map::backup_ram;

namespace ui::external_app::detector_rx {

int32_t DetectorRxView::map(int32_t value, int32_t fromLow, int32_t fromHigh, int32_t toLow, int32_t toHigh) {
    return toLow + (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow);
}

void DetectorRxView::focus() {
    button_index.focus();
}

DetectorRxView::~DetectorRxView() {
    shared_memory.request_m4_performance_counter = 0;
    receiver_model.disable();
    audio::output::stop();
    baseband::shutdown();
}

std::string DetectorRxView::format_freq_mhz(int64_t freq_hz) {
    int64_t mhz = freq_hz / 1000000;
    int64_t khz_frac = (freq_hz % 1000000) / 1000;
    return "< " + to_string_dec_uint(mhz) + "." + to_string_dec_uint(khz_frac, 3, '0') + " MHz >";
}

void DetectorRxView::load_freqman() {
    freqman_load_options options{};
    options.load_freqs = true;
    options.load_ranges = true;
    options.load_hamradios = false;
    options.load_repeaters = false;

    if (!load_freqman_file(freq_file_stem, frequency_list, options) || frequency_list.empty()) {
        button_file.set_text("No file!");
        button_index.set_text("");
        text_entry_desc.set("");
        button_freq.set_text("");
        frequency_list.clear();
        return;
    }

    current_index = 0;
    init_current_entry();
    button_file.set_text(freq_file_stem);
}

void DetectorRxView::init_current_entry() {
    if (frequency_list.empty()) return;

    auto& entry = *frequency_list[current_index];
    if (entry.type == freqman_type::Range) {
        minfreq = entry.frequency_a;
        maxfreq = entry.frequency_b;
        current_freq = minfreq;
        current_step = is_valid(entry.step) ? freqman_entry_get_step_value(entry.step) : DETECTOR_BW;
    } else {
        current_freq = entry.frequency_a;
        minfreq = current_freq;
        maxfreq = current_freq;
        current_step = DETECTOR_BW;
    }

    update_entry_display();
    update_freq_display();
    receiver_model.set_target_frequency(current_freq);
}

void DetectorRxView::update_entry_display() {
    if (frequency_list.empty()) return;

    auto& entry = *frequency_list[current_index];
    button_index.set_text(
        to_string_dec_uint(current_index + 1) + "/" +
        to_string_dec_uint(frequency_list.size()));
    text_entry_desc.set(entry.description);
}

void DetectorRxView::update_freq_display() {
    if (auto_scan && (minfreq != maxfreq)) {
        if (last_update_was_auto_range_type != 1) {
            button_freq.set_text("RANGE SCAN...");
            last_update_was_auto_range_type = 1;
        }
    } else {
        button_freq.set_text(format_freq_mhz(current_freq));
        last_update_was_auto_range_type = 0;
    }
}

void DetectorRxView::on_timer() {
    if (frequency_list.empty() || !auto_scan) return;

    auto& entry = *frequency_list[current_index];
    if (entry.type == freqman_type::Range) {
        current_freq += current_step;
        if (current_freq > maxfreq) {
            if (auto_advance) {
                current_index = (current_index + 1) % frequency_list.size();
                init_current_entry();
                return;
            }
            current_freq = minfreq;
        }
        receiver_model.set_target_frequency(current_freq);
        update_freq_display();
    } else if (auto_advance) {
        // Single frequency: advance to next entry
        current_index = (current_index + 1) % frequency_list.size();
        init_current_entry();
    }
}

DetectorRxView::DetectorRxView(NavigationView& nav)
    : nav_{nav} {
    add_children({
        &labels,
        &field_lna,
        &field_vga,
        &field_rf_amp,
        &field_volume,
        &button_file,
        &button_index,
        &text_entry_desc,
        &text_beep_squelch,
        &field_beep_squelch,
        &freq_stats_db,
        &freq_stats_rssi,
        &button_freq,
        &button_auto_scan,
        &button_auto_advance,
        &rssi,
        &rssi_graph,
    });

    rssi.set_vertical_rssi(true);

    field_beep_squelch.set_value(beep_squelch);
    field_beep_squelch.on_change = [this](int32_t v) {
        beep_squelch = v;
    };

    rssi_graph.set_nb_columns(256);

    // FILE button opens file picker
    button_file.on_select = [this](Button&) {
        auto open_view = nav_.push<FileLoadView>(".TXT");
        open_view->push_dir(freqman_dir);
        open_view->on_changed = [this](std::filesystem::path new_file_path) {
            if (new_file_path.native().find((u"/" / freqman_dir).native()) != 0) {
                button_file.set_text("Invalid file");
                return;
            }
            freq_file_stem = new_file_path.stem().string();
            load_freqman();
        };
    };

    // Index encoder: rotate to change entry, click does nothing
    button_index.on_change = [this]() {
        if (frequency_list.empty()) return;
        int32_t delta = button_index.get_encoder_delta();
        button_index.set_encoder_delta(0);
        if (delta == 0) return;

        // Use signed arithmetic for index wraparound to avoid unsigned promotion issues.
        const int32_t list_size = static_cast<int32_t>(frequency_list.size());
        int32_t idx = static_cast<int32_t>(current_index);
        if (delta > 0) {
            idx = (idx + 1) % list_size;
        } else {
            idx = (idx - 1 + list_size) % list_size;
        }
        current_index = static_cast<size_t>(idx);
        init_current_entry();
    };

    // Frequency encoder: rotate to step within range
    button_freq.on_change = [this]() {
        if (frequency_list.empty()) return;
        auto& entry = *frequency_list[current_index];
        if (entry.type != freqman_type::Range) {
            button_freq.set_encoder_delta(0);
            return;
        }

        int32_t delta = button_freq.get_encoder_delta();
        button_freq.set_encoder_delta(0);
        if (delta == 0) return;

        if (delta > 0) {
            current_freq += current_step;
            if (current_freq > maxfreq) current_freq = minfreq;
        } else {
            current_freq -= current_step;
            if (current_freq < minfreq) current_freq = maxfreq;
        }
        receiver_model.set_target_frequency(current_freq);
        update_freq_display();
    };

    // Auto-scan toggle
    button_auto_scan.on_select = [this](Button&) {
        auto_scan = !auto_scan;
        button_auto_scan.set_text(auto_scan ? "AUTOSCAN" : "NO SCAN");
        update_freq_display();
    };
    button_auto_scan.set_text(auto_scan ? "AUTOSCAN" : "NO SCAN");

    // Auto-advance toggle
    button_auto_advance.on_select = [this](Button&) {
        auto_advance = !auto_advance;
        button_auto_advance.set_text(auto_advance ? "AUTOADV" : "NO ADV");
    };
    button_auto_advance.set_text(auto_advance ? "AUTOADV" : "NO ADV");

    change_mode();
    rssi.set_peak(true, 3000);

    freq_stats_rssi.set_style(Theme::getInstance()->bg_darkest);
    freq_stats_db.set_style(Theme::getInstance()->bg_darkest);

    load_freqman();
}

void DetectorRxView::on_statistics_update(const ChannelStatistics& statistics) {
    static int16_t last_max_db = 0;
    static uint8_t last_min_rssi = 0;
    static uint8_t last_avg_rssi = 0;
    static uint8_t last_max_rssi = 0;

    rssi_graph.add_values(rssi.get_min(), rssi.get_avg(), rssi.get_max(), statistics.max_db);

    if (last_max_db != statistics.max_db) {
        last_max_db = statistics.max_db;
        freq_stats_db.set("Power: " + to_string_dec_int(statistics.max_db) + " db");
        rssi.set_db(statistics.max_db);
    }

    if (last_min_rssi != rssi_graph.get_graph_min() || last_avg_rssi != rssi_graph.get_graph_avg() || last_max_rssi != rssi_graph.get_graph_max()) {
        last_min_rssi = rssi_graph.get_graph_min();
        last_avg_rssi = rssi_graph.get_graph_avg();
        last_max_rssi = rssi_graph.get_graph_max();
        freq_stats_rssi.set("RSSI:" + to_string_dec_uint(last_min_rssi) + "/" + to_string_dec_uint(last_avg_rssi) + "/" + to_string_dec_uint(last_max_rssi));
    }

    if (statistics.max_db > beep_squelch) {
        baseband::request_audio_beep(map(statistics.max_db, -100, 20, 400, 2600), 24000, 150);
    }
}

size_t DetectorRxView::change_mode() {
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();

    audio_sampling_rate = audio::Rate::Hz_24000;
    baseband::run_image(portapack::spi_flash::image_tag_capture);
    receiver_model.set_modulation(ReceiverModel::Mode::Capture);

    baseband::set_sample_rate(DETECTOR_BW, get_oversample_rate(DETECTOR_BW));
    auto actual_sampling_rate = get_actual_sample_rate(DETECTOR_BW);
    receiver_model.set_sampling_rate(actual_sampling_rate);
    receiver_model.set_baseband_bandwidth(filter_bandwidth_for_sampling_rate(actual_sampling_rate));

    audio::set_rate(audio_sampling_rate);
    audio::output::start();
    receiver_model.set_headphone_volume(receiver_model.headphone_volume());

    receiver_model.enable();

    return 0;
}

}  // namespace ui::external_app::detector_rx
