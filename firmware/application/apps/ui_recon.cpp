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

#include "ui_recon.hpp"
#include "ui_fileman.hpp"
#include "file.hpp"

using namespace portapack;
using portapack::memory::map::backup_ram;

namespace ui {

void ReconView::clear_freqlist_for_ui_action() {
    audio::output::stop();
    // flag to detect and reload frequency_list
    freqlist_cleared_for_ui_action = true;
    // if in manual mode, there is enough memory to load freqman files, else we have to unload/reload
    if (!manual_mode) {
        frequency_list.clear();
    }
}

void ReconView::reset_indexes() {
    last_entry.modulation = -1;
    last_entry.bandwidth = -1;
    last_entry.step = -1;
    description = "...no description...";
    current_index = 0;
}

void ReconView::colorize_waits() {
    // colorize wait on match
    if (wait == 0) {
        field_wait.set_style(&style_blue);
    } else if (wait >= 500) {
        field_wait.set_style(&style_white);
    } else if (wait > -500 && wait < 500) {
        field_wait.set_style(&style_red);
    } else if (wait <= -500) {
        field_wait.set_style(&style_green);
    }
    // colorize lock time if in SPARSE mode as in continuous the lock_wait time is disarmed at first lock count
    if (recon_match_mode == RECON_MATCH_SPARSE) {
        if ((recon_lock_duration / STATS_UPDATE_INTERVAL) <= recon_lock_nb_match) {
            field_lock_wait.set_style(&style_yellow);
        } else {
            field_lock_wait.set_style(&style_white);
        }
    } else {
        field_lock_wait.set_style(&style_white);
    }
}

bool ReconView::recon_save_freq(const std::string& freq_file_path, size_t freq_index, bool warn_if_exists) {
    File recon_file;

    if (frequency_list.size() == 0 || (frequency_list.size() && current_index > (int32_t)frequency_list.size()))
        return false;

    freqman_entry entry = frequency_list[freq_index];
    entry.frequency_a = freq;
    entry.frequency_b = 0;
    entry.modulation = last_entry.modulation;
    entry.bandwidth = last_entry.bandwidth;
    entry.type = SINGLE;

    std::string frequency_to_add;
    get_freq_string(entry, frequency_to_add);

    auto result = recon_file.open(freq_file_path);  // First recon if freq is already in txt
    if (!result.is_valid()) {
        char one_char[1];  // Read it char by char
        std::string line;  // and put read line in here
        bool found = false;
        for (size_t pointer = 0; pointer < recon_file.size(); pointer++) {
            recon_file.seek(pointer);
            recon_file.read(one_char, 1);
            if ((int)one_char[0] > 31) {       // ascii space upwards
                line += one_char[0];           // add it to the textline
            } else if (one_char[0] == '\n') {  // New Line
                if (line.compare(0, frequency_to_add.size(), frequency_to_add) == 0) {
                    found = true;
                    break;
                }
                line.clear();  // Ready for next textline
            }
        }
        if (!found) {
            result = recon_file.append(freq_file_path);  // Second: append if it is not there
            if (!result.is_valid()) {
                recon_file.write_line(frequency_to_add);
            }
        }
        if (found && warn_if_exists) {
            nav_.display_modal("Error", "Frequency already exists");
        }
    } else {
        result = recon_file.create(freq_file_path);  // First freq if no output file
        if (!result.is_valid()) {
            recon_file.write_line(frequency_to_add);
        }
    }
    return true;
}

bool ReconView::recon_load_config_from_sd() {
    File settings_file;
    size_t length, file_position = 0;
    char* pos;
    char* line_start;
    char* line_end;
    char file_data[257];

    uint32_t it = 0;
    uint32_t nb_params = RECON_SETTINGS_NB_PARAMS;
    std::string params[RECON_SETTINGS_NB_PARAMS];

    make_new_directory(u"SETTINGS");

    auto result = settings_file.open(RECON_CFG_FILE);
    if (!result.is_valid()) {
        while (it < nb_params) {
            // Read a 256 bytes block from file
            settings_file.seek(file_position);
            memset(file_data, 0, 257);
            auto read_size = settings_file.read(file_data, 256);
            if (read_size.is_error())
                break;
            file_position += 256;
            // Reset line_start to beginning of buffer
            line_start = file_data;
            pos = line_start;
            while ((line_end = strstr(line_start, "\x0A"))) {
                length = line_end - line_start - 1;
                params[it] = string(pos, length);
                it++;
                line_start = line_end + 1;
                pos = line_start;
                if (line_start - file_data >= 256)
                    break;
                if (it >= nb_params)
                    break;
            }
            if (read_size.value() != 256)
                break;  // End of file

            // Restart at beginning of last incomplete line
            file_position -= (file_data + 256 - line_start);
        }
    }

    if (it < nb_params) {
        /* bad number of params, signal defaults */
        input_file = "RECON";
        output_file = "RECON_RESULTS";
        recon_lock_duration = RECON_MIN_LOCK_DURATION;
        recon_lock_nb_match = RECON_DEF_NB_MATCH;
        squelch = -14;
        recon_match_mode = RECON_MATCH_CONTINUOUS;
        wait = RECON_DEF_WAIT_DURATION;
        volume = 40;
        return false;
    }

    if (it > 0)
        input_file = params[0];
    else
        input_file = "RECON";

    if (it > 1)
        output_file = params[1];
    else
        output_file = "RECON_RESULTS";

    if (it > 2)
        recon_lock_duration = strtoll(params[2].c_str(), nullptr, 10);
    else
        recon_lock_duration = RECON_MIN_LOCK_DURATION;

    if (it > 3)
        recon_lock_nb_match = strtoll(params[3].c_str(), nullptr, 10);
    else
        recon_lock_nb_match = RECON_DEF_NB_MATCH;

    if (it > 4)
        squelch = strtoll(params[4].c_str(), nullptr, 10);
    else
        squelch = -14;

    if (it > 5)
        recon_match_mode = strtoll(params[5].c_str(), nullptr, 10);
    else
        recon_match_mode = RECON_MATCH_CONTINUOUS;

    if (it > 6)
        wait = strtoll(params[6].c_str(), nullptr, 10);
    else
        wait = RECON_DEF_WAIT_DURATION;

    if (it > 7)
        volume = strtoll(params[7].c_str(), nullptr, 10);
    else
        volume = 40;

    return true;
}

bool ReconView::recon_save_config_to_sd() {
    File settings_file;

    make_new_directory(u"SETTINGS");

    auto result = settings_file.create(RECON_CFG_FILE);
    if (result.is_valid())
        return false;
    settings_file.write_line(input_file);
    settings_file.write_line(output_file);
    settings_file.write_line(to_string_dec_uint(recon_lock_duration));
    settings_file.write_line(to_string_dec_uint(recon_lock_nb_match));
    settings_file.write_line(to_string_dec_int(squelch));
    settings_file.write_line(to_string_dec_uint(recon_match_mode));
    settings_file.write_line(to_string_dec_int(wait));
    settings_file.write_line(to_string_dec_int(volume));
    return true;
}

void ReconView::audio_output_start() {
    audio::output::start();
    this->on_headphone_volume_changed((receiver_model.headphone_volume() - audio::headphone::volume_range().max).decibel() + 99);
}

void ReconView::recon_redraw() {
    if (last_rssi_min != rssi.get_min() || last_rssi_med != rssi.get_avg() || last_rssi_max != rssi.get_max()) {
        last_rssi_min = rssi.get_min();
        last_rssi_med = rssi.get_avg();
        last_rssi_max = rssi.get_max();
        freq_stats.set("RSSI: " + to_string_dec_int(rssi.get_min()) + "/" + to_string_dec_int(rssi.get_avg()) + "/" + to_string_dec_int(rssi.get_max()) + " db");
    }
    if (last_entry.frequency_a != freq) {
        last_entry.frequency_a = freq;
        big_display.set("FREQ:" + to_string_short_freq(freq) + " MHz");
    }
    if (last_nb_match != recon_lock_nb_match || last_freq_lock != freq_lock) {
        last_freq_lock = freq_lock;
        last_nb_match = recon_lock_nb_match;
        text_nb_locks.set(to_string_dec_uint(freq_lock) + "/" + to_string_dec_uint(recon_lock_nb_match));
        if (freq_lock == 0) {
            // NO FREQ LOCK, ONGOING STANDARD SCANNING
            big_display.set_style(&style_white);
            if (recon)
                button_pause.set_text("<PAUSE>");
            else
                button_pause.set_text("<RESUME>");
        } else if (freq_lock == 1 && recon_lock_nb_match != 1) {
            // STARTING LOCK FREQ
            big_display.set_style(&style_yellow);
            button_pause.set_text("<SKPLCK>");
        } else if (freq_lock >= recon_lock_nb_match) {
            big_display.set_style(&style_green);
            button_pause.set_text("<UNLOCK>");
            // FREQ IS STRONG: GREEN and recon will pause when on_statistics_update()
            if ((!scanner_mode) && autosave && frequency_list.size() > 0) {
                recon_save_freq(freq_file_path, current_index, false);
            }
        }
    }
    if (last_db != db || last_list_size != frequency_list.size()) {
        last_list_size = frequency_list.size();
        last_db = db;
        text_max.set("/" + to_string_dec_uint(frequency_list.size()) + " " + to_string_dec_int(db) + " db");
    }
}

void ReconView::handle_retune() {
    if (last_freq != freq) {
        last_freq = freq;
        receiver_model.set_tuning_frequency(freq);  // Retune
    }
    if (frequency_list.size() > 0) {
        if (last_entry.modulation != frequency_list[current_index].modulation && frequency_list[current_index].modulation >= 0) {
            last_entry.modulation = frequency_list[current_index].modulation;
            field_mode.set_selected_index(frequency_list[current_index].modulation);
        }
        // Set bandwidth if any
        if (last_entry.bandwidth != frequency_list[current_index].bandwidth && frequency_list[current_index].bandwidth >= 0) {
            last_entry.bandwidth = frequency_list[current_index].bandwidth;
            switch (frequency_list[current_index].modulation) {
                case AM_MODULATION:
                    receiver_model.set_am_configuration(freq);
                    break;
                case NFM_MODULATION:
                    receiver_model.set_nbfm_configuration(freq);
                    break;
                case WFM_MODULATION:
                    receiver_model.set_wfm_configuration(freq);
                default:
                    break;
            }
            field_bw.set_selected_index(freq);
        }
        if (last_entry.step != frequency_list[current_index].step && frequency_list[current_index].step >= 0) {
            last_entry.step = frequency_list[current_index].step;
            step = freqman_entry_get_step_value(last_entry.step);
            step_mode.set_selected_index(step);
        }
        if (last_index != current_index) {
            last_index = current_index;
            if ((int32_t)frequency_list.size() && current_index < (int32_t)frequency_list.size() && frequency_list[current_index].type == RANGE) {
                if (update_ranges && !manual_mode) {
                    button_manual_start.set_text(to_string_short_freq(frequency_list[current_index].frequency_a));
                    frequency_range.min = frequency_list[current_index].frequency_a;
                    if (frequency_list[current_index].frequency_b != 0) {
                        button_manual_end.set_text(to_string_short_freq(frequency_list[current_index].frequency_b));
                        frequency_range.max = frequency_list[current_index].frequency_b;
                    } else {
                        button_manual_end.set_text(to_string_short_freq(frequency_list[current_index].frequency_a));
                        frequency_range.max = frequency_list[current_index].frequency_a;
                    }
                }
            }
            text_cycle.set_text(to_string_dec_uint(current_index + 1, 3));
            if (frequency_list[current_index].description.size() > 0) {
                switch (frequency_list[current_index].type) {
                    case RANGE:
                        desc_cycle.set("R: " + frequency_list[current_index].description);  // Show new description
                        break;
                    case HAMRADIO:
                        desc_cycle.set("H: " + frequency_list[current_index].description);  // Show new description
                        break;
                    default:
                    case SINGLE:
                        desc_cycle.set("S: " + frequency_list[current_index].description);  // Show new description
                        break;
                }
            } else {
                desc_cycle.set("...no description...");  // Show new description
            }
        }
    }
}

void ReconView::focus() {
    button_pause.focus();
}

ReconView::~ReconView() {
    // save app config
    recon_save_config_to_sd();
    // save app common settings
    settings.save("recon", &app_settings);

    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

ReconView::ReconView(NavigationView& nav)
    : nav_{nav} {
    add_children({&labels,
                  &field_lna,
                  &field_vga,
                  &field_rf_amp,
                  &field_volume,
                  &field_bw,
                  &field_squelch,
                  &field_wait,
                  &field_lock_wait,
                  &button_config,
                  &button_scanner_mode,
                  &button_looking_glass,
                  &file_name,
                  &rssi,
                  &text_cycle,
                  &text_max,
                  &text_nb_locks,
                  &desc_cycle,
                  &big_display,
                  &freq_stats,
                  &text_timer,
                  &text_ctcss,
                  &button_manual_start,
                  &button_manual_end,
                  &button_manual_recon,
                  &field_mode,
                  &step_mode,
                  &button_pause,
                  &button_audio_app,
                  &button_add,
                  &button_dir,
                  &button_restart,
                  &button_mic_app,
                  &button_remove});

    def_step = 0;
    // HELPER: Pre-setting a manual range, based on stored frequency
    rf::Frequency stored_freq = persistent_memory::tuned_frequency();
    receiver_model.set_tuning_frequency(stored_freq);
    if (stored_freq - OneMHz > 0)
        frequency_range.min = stored_freq - OneMHz;
    else
        frequency_range.min = 0;
    button_manual_start.set_text(to_string_short_freq(frequency_range.min));
    if (stored_freq + OneMHz < MAX_UFREQ)
        frequency_range.max = stored_freq + OneMHz;
    else
        frequency_range.max = MAX_UFREQ;
    button_manual_end.set_text(to_string_short_freq(frequency_range.max));
    // Loading settings
    autostart = persistent_memory::recon_autostart_recon();
    autosave = persistent_memory::recon_autosave_freqs();
    continuous = persistent_memory::recon_continuous();
    filedelete = persistent_memory::recon_clear_output();
    load_freqs = persistent_memory::recon_load_freqs();
    load_ranges = persistent_memory::recon_load_ranges();
    load_hamradios = persistent_memory::recon_load_hamradios();
    update_ranges = persistent_memory::recon_update_ranges_when_recon();
    field_volume.set_value(volume);
    if (sd_card_mounted) {
        // load auto common app settings
        auto rc = settings.load("recon", &app_settings);
        if (rc == SETTINGS_OK) {
            field_lna.set_value(app_settings.lna);
            field_vga.set_value(app_settings.vga);
            field_rf_amp.set_value(app_settings.rx_amp);
            receiver_model.set_rf_amp(app_settings.rx_amp);
        }
    }

    button_manual_start.on_select = [this, &nav](ButtonWithEncoder& button) {
        clear_freqlist_for_ui_action();
        auto new_view = nav_.push<FrequencyKeypadView>(frequency_range.min);
        new_view->on_changed = [this, &button](rf::Frequency f) {
            frequency_range.min = f;
            button_manual_start.set_text(to_string_short_freq(f));
        };
    };

    button_manual_end.on_select = [this, &nav](ButtonWithEncoder& button) {
        clear_freqlist_for_ui_action();
        auto new_view = nav.push<FrequencyKeypadView>(frequency_range.max);
        new_view->on_changed = [this, &button](rf::Frequency f) {
            frequency_range.max = f;
            button_manual_end.set_text(to_string_short_freq(f));
        };
    };

    text_cycle.on_select = [this, &nav](ButtonWithEncoder& button) {
        if (frequency_list.size() > 0) {
            auto new_view = nav_.push<FrequencyKeypadView>(current_index);
            new_view->on_changed = [this, &button](rf::Frequency f) {
                f = f / OneMHz;
                if (f >= 1 && f <= frequency_list.size()) {
                    index_stepper = f - 1 - current_index;
                    freq_lock = 0;
                }
            };
        }
    };

    button_manual_start.on_change = [this]() {
        frequency_range.min = frequency_range.min + button_manual_start.get_encoder_delta() * freqman_entry_get_step_value(def_step);
        if (frequency_range.min < 0) {
            frequency_range.min = 0;
        }
        if (frequency_range.min > (MAX_UFREQ - freqman_entry_get_step_value(def_step))) {
            frequency_range.min = MAX_UFREQ - freqman_entry_get_step_value(def_step);
        }
        if (frequency_range.min > (frequency_range.max - freqman_entry_get_step_value(def_step))) {
            frequency_range.max = frequency_range.min + freqman_entry_get_step_value(def_step);
            if (frequency_range.max > MAX_UFREQ) {
                frequency_range.min = MAX_UFREQ - freqman_entry_get_step_value(def_step);
                frequency_range.max = MAX_UFREQ;
            }
        }
        button_manual_start.set_text(to_string_short_freq(frequency_range.min));
        button_manual_end.set_text(to_string_short_freq(frequency_range.max));
        button_manual_start.set_encoder_delta(0);
    };

    button_manual_end.on_change = [this]() {
        frequency_range.max = frequency_range.max + button_manual_end.get_encoder_delta() * freqman_entry_get_step_value(def_step);
        if (frequency_range.max < (freqman_entry_get_step_value(def_step) + 1)) {
            frequency_range.max = (freqman_entry_get_step_value(def_step) + 1);
        }
        if (frequency_range.max > MAX_UFREQ) {
            frequency_range.max = MAX_UFREQ;
        }
        if (frequency_range.max < (frequency_range.min + freqman_entry_get_step_value(def_step))) {
            frequency_range.min = frequency_range.max - freqman_entry_get_step_value(def_step);
            if (frequency_range.max < (freqman_entry_get_step_value(def_step) + 1)) {
                frequency_range.min = 1;
                frequency_range.max = (freqman_entry_get_step_value(def_step) + 1);
            }
        }
        button_manual_start.set_text(to_string_short_freq(frequency_range.min));
        button_manual_end.set_text(to_string_short_freq(frequency_range.max));
        button_manual_end.set_encoder_delta(0);
    };

    text_cycle.on_change = [this]() {
        on_index_delta(text_cycle.get_encoder_delta());
        text_cycle.set_encoder_delta(0);
    };

    button_pause.on_select = [this](ButtonWithEncoder&) {
        if (frequency_list.size() > 0) {
            if (freq_lock > 0) {
                if (fwd) {
                    on_stepper_delta(1);
                } else {
                    on_stepper_delta(-1);
                }
                button_pause.set_text("<PAUSE>");  // Show button for non continuous stop
            } else {
                if (!recon) {
                    recon_resume();
                } else {
                    recon_pause();
                }
            }
        }
    };

    button_pause.on_change = [this]() {
        on_stepper_delta(button_pause.get_encoder_delta());
        button_pause.set_encoder_delta(0);
    };

    button_audio_app.on_select = [this](Button&) {
        nav_.pop();
        nav_.push<AnalogAudioView>();
    };

    button_looking_glass.on_select = [this](Button&) {
        nav_.pop();
        nav_.push<GlassView>();
    };

    rssi.set_focusable(true);
    rssi.set_peak(true, 500);
    rssi.on_select = [this](RSSI&) {
        nav_.pop();
        nav_.push<LevelView>();
    };

    button_mic_app.on_select = [this](Button&) {
        nav_.pop();
        nav_.push<MicTXView>();
    };

    button_remove.on_select = [this](ButtonWithEncoder&) {
        if (frequency_list.size() > 0) {
            if (!manual_mode) {
                // scanner or recon (!scanner) mode
                // in both we delete index from live view, but only remove in output file in scanner_mode
                if (current_index >= (int32_t)frequency_list.size()) {
                    current_index = frequency_list.size() - 1;
                }
                frequency_list.erase(frequency_list.begin() + current_index);
                if (current_index >= (int32_t)frequency_list.size()) {
                    current_index = frequency_list.size() - 1;
                }
                if (frequency_list.size() > 0) {
                    if (frequency_list[current_index].description.size() > 0) {
                        switch (frequency_list[current_index].type) {
                            case RANGE:
                                desc_cycle.set("R: " + frequency_list[current_index].description);
                                break;
                            case HAMRADIO:
                                desc_cycle.set("H: " + frequency_list[current_index].description);
                                break;
                            default:
                            case SINGLE:
                                desc_cycle.set("S: " + frequency_list[current_index].description);
                                break;
                        }
                    } else {
                        desc_cycle.set("...no description...");
                    }
                    text_cycle.set_text(to_string_dec_uint(current_index + 1, 3));
                }
                // also remove from output file if in scanner mode
                if (scanner_mode) {
                    File freqman_file;
                    delete_file(freq_file_path);
                    auto result = freqman_file.create(freq_file_path);
                    if (!result.is_valid()) {
                        for (size_t n = 0; n < frequency_list.size(); n++) {
                            std::string line;
                            get_freq_string(frequency_list[n], line);
                            freqman_file.write_line(line);
                        }
                    }
                }
            } else if (manual_mode)  // only remove from output
            {
                File recon_file;
                File tmp_recon_file;
                std::string tmp_freq_file_path = freq_file_path + ".TMP";
                std::string frequency_to_add;

                freqman_entry entry = frequency_list[current_index];
                entry.frequency_a = freq;
                entry.frequency_b = 0;
                entry.modulation = last_entry.modulation;
                entry.bandwidth = last_entry.bandwidth;
                entry.type = SINGLE;

                get_freq_string(entry, frequency_to_add);

                delete_file(tmp_freq_file_path);
                auto result = tmp_recon_file.create(tmp_freq_file_path);  // First recon if freq is already in txt
                if (!result.is_valid()) {
                    bool found = false;
                    result = recon_file.open(freq_file_path);  // First recon if freq is already in txt
                    if (!result.is_valid()) {
                        char one_char[1];  // Read it char by char
                        std::string line;  // and put read line in here
                        for (size_t pointer = 0; pointer < recon_file.size(); pointer++) {
                            recon_file.seek(pointer);
                            recon_file.read(one_char, 1);
                            if ((int)one_char[0] > 31) {       // ascii space upwards
                                line += one_char[0];           // add it to the textline
                            } else if (one_char[0] == '\n') {  // new Line
                                if (line.compare(0, frequency_to_add.size(), frequency_to_add) == 0) {
                                    found = true;
                                } else {
                                    tmp_recon_file.write_line(frequency_to_add);
                                }
                                line.clear();  // ready for next textline
                            }
                        }
                        if (found) {
                            delete_file(freq_file_path);
                            rename_file(tmp_freq_file_path, freq_file_path);
                        } else {
                            delete_file(tmp_freq_file_path);
                        }
                    }
                }
            }
            receiver_model.set_tuning_frequency(frequency_list[current_index].frequency_a);  // retune
        }
        if (frequency_list.size() == 0) {
            text_cycle.set_text(" ");
            desc_cycle.set("no entries in list");  // Show new description
            delete_file(freq_file_path);
        }
        timer = 0;
        freq_lock = 0;
    };

    button_remove.on_change = [this]() {
        on_stepper_delta(button_remove.get_encoder_delta());
        button_remove.set_encoder_delta(0);
    };

    button_manual_recon.on_select = [this](Button&) {
        button_remove.set_text("DELETE");
        scanner_mode = false;
        manual_mode = true;
        recon_pause();
        if (!frequency_range.min || !frequency_range.max) {
            nav_.display_modal("Error", "Both START and END freqs\nneed a value");
        } else if (frequency_range.min > frequency_range.max) {
            nav_.display_modal("Error", "END freq\nis lower than START");
        } else {
            audio::output::stop();

            frequency_list.clear();
            freqman_entry manual_freq_entry;

            def_step = step_mode.selected_index();  // max range val

            manual_freq_entry.type = RANGE;
            manual_freq_entry.description =
                to_string_short_freq(frequency_range.min).erase(0, 1) + ">" + to_string_short_freq(frequency_range.max).erase(0, 1) + " S:"  // current Manual range
                + freqman_entry_get_step_string_short(def_step);                                                                             // euquiq: lame kludge to reduce spacing in step freq
            manual_freq_entry.frequency_a = frequency_range.min;                                                                             // min range val
            manual_freq_entry.frequency_b = frequency_range.max;                                                                             // max range val
            manual_freq_entry.modulation = -1;
            manual_freq_entry.bandwidth = -1;
            manual_freq_entry.step = def_step;

            frequency_list.push_back(manual_freq_entry);

            big_display.set_style(&style_white);  // Back to white color

            freq_stats.set_style(&style_white);
            freq_stats.set("0/0/0");

            text_cycle.set_text("1");
            text_max.set("/1");
            button_scanner_mode.set_style(&style_white);
            button_scanner_mode.set_text("MSEARCH");
            file_name.set_style(&style_white);
            file_name.set("MANUAL RANGE RECON");
            desc_cycle.set_style(&style_white);

            last_entry.modulation = -1;
            last_entry.bandwidth = -1;
            last_entry.step = -1;
            last_index = -1;

            current_index = 0;
            freq = manual_freq_entry.frequency_a;
            handle_retune();
            recon_redraw();
            recon_resume();
        }
    };

    button_dir.on_select = [this](Button&) {
        if (fwd) {
            fwd = false;
            button_dir.set_text("<RW");
        } else {
            fwd = true;
            button_dir.set_text("FW>");
        }
        timer = 0;
        if (!recon)
            recon_resume();
    };

    button_restart.on_select = [this](Button&) {
        if (frequency_list.size() > 0) {
            def_step = step_mode.selected_index();  // Use def_step from manual selector
            frequency_file_load(true);
            if (fwd) {
                button_dir.set_text("FW>");
            } else {
                button_dir.set_text("<RW");
            }
            recon_resume();
        }
        if (scanner_mode) {
            file_name.set_style(&style_red);
            button_scanner_mode.set_style(&style_red);
            button_scanner_mode.set_text("SCANNER");
        } else {
            file_name.set_style(&style_blue);
            button_scanner_mode.set_style(&style_blue);
            button_scanner_mode.set_text("RECON");
        }
    };

    button_add.on_select = [this](ButtonWithEncoder&) {  // frequency_list[current_index]
        if (!scanner_mode) {
            recon_save_freq(freq_file_path, current_index, true);
        }
    };

    button_add.on_change = [this]() {
        on_stepper_delta(button_add.get_encoder_delta());
        button_add.set_encoder_delta(0);
    };

    button_scanner_mode.on_select = [this, &nav](Button&) {
        manual_mode = false;
        if (scanner_mode) {
            scanner_mode = false;
            button_scanner_mode.set_style(&style_blue);
            button_scanner_mode.set_text("RECON");
            button_remove.set_text("DELETE");
        } else {
            scanner_mode = true;
            button_scanner_mode.set_style(&style_red);
            button_scanner_mode.set_text("SCANNER");
            button_scanner_mode.set_text("REMOVE");
        }
        frequency_file_load(true);
        if (autostart) {
            recon_resume();
        } else {
            recon_pause();
        }
    };

    button_config.on_select = [this, &nav](Button&) {
        clear_freqlist_for_ui_action();

        auto open_view = nav.push<ReconSetupView>(input_file, output_file, recon_lock_duration, recon_lock_nb_match, recon_match_mode);
        open_view->on_changed = [this](std::vector<std::string> result) {
            freqlist_cleared_for_ui_action = false;
            input_file = result[0];
            output_file = result[1];
            freq_file_path = "/FREQMAN/" + output_file + ".TXT";
            recon_lock_duration = strtol(result[2].c_str(), nullptr, 10);
            recon_lock_nb_match = strtol(result[3].c_str(), nullptr, 10);
            recon_match_mode = strtol(result[4].c_str(), nullptr, 10);
            recon_save_config_to_sd();

            autosave = persistent_memory::recon_autosave_freqs();
            autostart = persistent_memory::recon_autostart_recon();
            continuous = persistent_memory::recon_continuous();
            filedelete = persistent_memory::recon_clear_output();
            load_freqs = persistent_memory::recon_load_freqs();
            load_ranges = persistent_memory::recon_load_ranges();
            load_hamradios = persistent_memory::recon_load_hamradios();
            update_ranges = persistent_memory::recon_update_ranges_when_recon();

            field_wait.set_value(wait);
            field_lock_wait.set_value(recon_lock_duration);
            colorize_waits();

            frequency_file_load(false);

            if (autostart) {
                recon_resume();
            } else {
                recon_pause();
            }
        };
    };

    field_wait.on_change = [this](int32_t v) {
        wait = v;
        colorize_waits();
    };

    field_lock_wait.on_change = [this](uint32_t v) {
        recon_lock_duration = v;
        colorize_waits();
    };

    field_squelch.on_change = [this](int32_t v) {
        squelch = v;
    };

    field_volume.on_change = [this](int32_t v) {
        this->on_headphone_volume_changed(v);
    };

    // PRE-CONFIGURATION:
    button_scanner_mode.set_style(&style_blue);
    button_scanner_mode.set_text("RECON");
    file_name.set("=>");

    // Loading input and output file from settings
    recon_load_config_from_sd();
    freq_file_path = "/FREQMAN/" + output_file + ".TXT";

    field_squelch.set_value(squelch);
    field_wait.set_value(wait);
    field_lock_wait.set_value(recon_lock_duration);
    colorize_waits();

    field_volume.set_value((receiver_model.headphone_volume() - audio::headphone::volume_range().max).decibel() + 99);

    // fill modulation and step options
    freqman_set_modulation_option(field_mode);
    freqman_set_step_option(step_mode);

    // set radio
    change_mode(AM_MODULATION);                                                            // start on AM.
    field_mode.set_by_value(AM_MODULATION);                                                // reflect the mode into the manual selector
    receiver_model.set_tuning_frequency(portapack::persistent_memory::tuned_frequency());  // first tune

    if (filedelete) {
        delete_file(freq_file_path);
    }

    frequency_file_load(false); /* do not stop all at start */
    if (autostart) {
        recon_resume();
    } else {
        recon_pause();
    }
    recon_redraw();
}

void ReconView::frequency_file_load(bool stop_all_before) {
    (void)(stop_all_before);
    audio::output::stop();

    def_step = step_mode.selected_index();  // use def_step from manual selector
    frequency_list.clear();                 // clear the existing frequency list (expected behavior)
    std::string file_input = input_file;    // default recon mode
    if (scanner_mode) {
        file_input = output_file;
        file_name.set_style(&style_red);
        button_scanner_mode.set_style(&style_red);
        button_scanner_mode.set_text("SCANNER");
    } else {
        file_name.set_style(&style_blue);
        button_scanner_mode.set_style(&style_blue);
        button_scanner_mode.set_text("RECON");
    }
    desc_cycle.set_style(&style_white);
    if (!load_freqman_file_ex(file_input, frequency_list, load_freqs, load_ranges, load_hamradios)) {
        file_name.set_style(&style_red);
        desc_cycle.set_style(&style_red);
        desc_cycle.set(" NO " + file_input + ".TXT FILE ...");
        file_name.set("=> NO DATA");
    } else {
        file_name.set("=> " + file_input);
        if (frequency_list.size() == 0) {
            file_name.set_style(&style_red);
            desc_cycle.set_style(&style_red);
            desc_cycle.set("/0 no entries in list");
            file_name.set("BadOrEmpty " + file_input);
        } else {
            if (frequency_list.size() > FREQMAN_MAX_PER_FILE) {
                file_name.set_style(&style_yellow);
                desc_cycle.set_style(&style_yellow);
            }
        }
    }

    if (frequency_list[0].step >= 0)
        step = freqman_entry_get_step_value(frequency_list[0].step);
    else
        step = freqman_entry_get_step_value(def_step);

    switch (frequency_list[0].type) {
        case SINGLE:
            freq = frequency_list[0].frequency_a;
            break;
        case RANGE:
            minfreq = frequency_list[0].frequency_a;
            maxfreq = frequency_list[0].frequency_b;
            if (fwd) {
                freq = minfreq;
            } else {
                freq = maxfreq;
            }
            if (frequency_list[0].step >= 0)
                step = freqman_entry_get_step_value(frequency_list[0].step);
            break;
        case HAMRADIO:
            minfreq = frequency_list[0].frequency_a;
            maxfreq = frequency_list[0].frequency_b;
            if (fwd) {
                freq = minfreq;
            } else {
                freq = maxfreq;
            }
            break;
        default:
            break;
    }
    reset_indexes();
    step_mode.set_selected_index(def_step);  // Impose the default step into the manual step selector
    receiver_model.enable();
    receiver_model.set_squelch_level(0);
    if (frequency_list.size() != 0) {
        switch (frequency_list[current_index].type) {
            case RANGE:
                description = "R: " + frequency_list[current_index].description;
                break;
            case HAMRADIO:
                description = "H: " + frequency_list[current_index].description;
                break;
            default:
            case SINGLE:
                description = "S: " + frequency_list[current_index].description;
                break;
        }
        text_cycle.set_text(to_string_dec_uint(current_index + 1, 3));
        if (update_ranges && !manual_mode) {
            button_manual_start.set_text(to_string_short_freq(frequency_list[current_index].frequency_a));
            frequency_range.min = frequency_list[current_index].frequency_a;
            if (frequency_list[current_index].frequency_b != 0) {
                button_manual_end.set_text(to_string_short_freq(frequency_list[current_index].frequency_b));
                frequency_range.max = frequency_list[current_index].frequency_b;
            } else {
                button_manual_end.set_text(to_string_short_freq(frequency_list[current_index].frequency_a));
                frequency_range.max = frequency_list[current_index].frequency_a;
            }
        }
    } else {
        text_cycle.set_text(" ");
    }
    desc_cycle.set(description);
    handle_retune();
}

void ReconView::on_statistics_update(const ChannelStatistics& statistics) {
    // hack to reload the list if it was cleared by going into CONFIG
    if (freqlist_cleared_for_ui_action) {
        if (!manual_mode) {
            frequency_file_load(false);
        }
        if (autostart) {
            recon_resume();
        } else {
            recon_pause();
        }
        freqlist_cleared_for_ui_action = false;
    }
    db = statistics.max_db;
    if (recon) {
        if (!timer) {
            status = 0;
            continuous_lock = false;
            freq_lock = 0;
            timer = recon_lock_duration;
            big_display.set_style(&style_white);
        }
        if (freq_lock < recon_lock_nb_match)  // LOCKING
        {
            if (status != 1) {
                status = 1;
                if (wait != 0) {
                    audio::output::stop();
                }
            }
            if (db > squelch)  // MATCHING LEVEL
            {
                continuous_lock = true;
                freq_lock++;
            } else {
                // continuous, direct cut it if not consecutive match after 1 first match
                if (recon_match_mode == RECON_MATCH_CONTINUOUS) {
                    if (freq_lock > 0) {
                        timer = 0;
                        continuous_lock = false;
                    }
                }
            }
        }
        if (freq_lock >= recon_lock_nb_match)  // LOCKED
        {
            if (status != 2) {
                continuous_lock = false;
                status = 2;
                if (wait != 0) {
                    audio_output_start();
                }
                if (wait >= 0) {
                    timer = wait;
                }
            }
            if (wait < 0) {
                if (db > squelch)  // MATCHING LEVEL IN STAY X AFTER LAST ACTIVITY
                {
                    timer = abs(wait);
                }
            }
        }
    }
    if (last_timer != timer) {
        last_timer = timer;
        text_timer.set("TIMER: " + to_string_dec_int(timer));
    }
    if (timer) {
        if (!continuous_lock)
            timer -= STATS_UPDATE_INTERVAL;
        if (timer < 0) {
            timer = 0;
        }
    }
    if (recon || stepper != 0 || index_stepper != 0) {
        if (!timer || stepper != 0 || index_stepper != 0) {
            // IF THERE IS A FREQUENCY LIST ...
            if (frequency_list.size() > 0) {
                has_looped = false;
                entry_has_changed = false;

                if (recon || stepper != 0 || index_stepper != 0) {
                    if (index_stepper == 0) {
                        /* we are doing a range */
                        if (frequency_list[current_index].type == RANGE) {
                            if ((fwd && stepper == 0) || stepper > 0) {
                                // forward
                                freq += step;
                                // if bigger than range max
                                if (freq > maxfreq) {
                                    // when going forward we already know that we can skip a whole range => two values in the list
                                    current_index++;
                                    entry_has_changed = true;
                                    // looping
                                    if ((uint32_t)current_index >= frequency_list.size()) {
                                        has_looped = true;
                                        current_index = 0;
                                    }
                                }
                            } else if ((!fwd && stepper == 0) || stepper < 0) {
                                // reverse
                                freq -= step;
                                // if lower than range min
                                if (freq < minfreq) {
                                    // when back we have to check one step at a time
                                    current_index--;
                                    entry_has_changed = true;
                                    // looping
                                    if (current_index < 0) {
                                        has_looped = true;
                                        current_index = frequency_list.size() - 1;
                                    }
                                }
                            }
                        } else if (frequency_list[current_index].type == SINGLE) {
                            if ((fwd && stepper == 0) || stepper > 0) {  // forward
                                current_index++;
                                entry_has_changed = true;
                                // looping
                                if ((uint32_t)current_index >= frequency_list.size()) {
                                    has_looped = true;
                                    current_index = 0;
                                }
                            } else if ((!fwd && stepper == 0) || stepper < 0) {
                                // reverse
                                current_index--;
                                entry_has_changed = true;
                                // if previous if under the list => go back from end
                                if (current_index < 0) {
                                    has_looped = true;
                                    current_index = frequency_list.size() - 1;
                                }
                            }
                        } else if (frequency_list[current_index].type == HAMRADIO) {
                            if ((fwd && stepper == 0) || stepper > 0) {  // forward
                                if ((minfreq != maxfreq) && freq == minfreq) {
                                    freq = maxfreq;
                                } else {
                                    current_index++;
                                    entry_has_changed = true;
                                    // looping
                                    if ((uint32_t)current_index >= frequency_list.size()) {
                                        has_looped = true;
                                        current_index = 0;
                                    }
                                }
                            } else if ((!fwd && stepper == 0) || stepper < 0) {
                                // reverse
                                if ((minfreq != maxfreq) && freq == maxfreq) {
                                    freq = minfreq;
                                } else {
                                    current_index--;
                                    entry_has_changed = true;
                                    // if previous if under the list => go back from end
                                    if (current_index < 0) {
                                        has_looped = true;
                                        current_index = frequency_list.size() - 1;
                                    }
                                }
                            }
                        }
                        // set index to boundary if !continuous
                        if (has_looped && !continuous) {
                            entry_has_changed = true;
                            /* prepare values for the next run, when user will resume */
                            if ((fwd && stepper == 0) || stepper > 0) {
                                current_index = 0;
                            } else if ((!fwd && stepper == 0) || stepper < 0) {
                                current_index = frequency_list.size() - 1;
                            }
                        }
                    } else {
                        current_index += index_stepper;
                        if (current_index < 0)
                            current_index += frequency_list.size();
                        if ((unsigned)current_index >= frequency_list.size())
                            current_index -= frequency_list.size();

                        entry_has_changed = true;

                        if (!recon)  // for some motive, audio output gets stopped.
                            audio_output_start();
                    }
                    // reload entry if changed
                    if (entry_has_changed) {
                        timer = 0;
                        switch (frequency_list[current_index].type) {
                            case SINGLE:
                                freq = frequency_list[current_index].frequency_a;
                                break;
                            case RANGE:
                                minfreq = frequency_list[current_index].frequency_a;
                                maxfreq = frequency_list[current_index].frequency_b;
                                if ((fwd && !stepper && !index_stepper) || stepper > 0 || index_stepper > 0) {
                                    freq = minfreq;
                                } else if ((!fwd && !stepper && !index_stepper) || stepper < 0 || index_stepper < 0) {
                                    freq = maxfreq;
                                }
                                break;
                            case HAMRADIO:
                                minfreq = frequency_list[current_index].frequency_a;
                                maxfreq = frequency_list[current_index].frequency_b;
                                if ((fwd && !stepper && !index_stepper) || stepper > 0 || index_stepper > 0) {
                                    freq = minfreq;
                                } else if ((!fwd && !stepper && !index_stepper) || stepper < 0 || index_stepper < 0) {
                                    freq = maxfreq;
                                }
                                break;
                            default:
                                break;
                        }
                    }
                    if (has_looped && !continuous) {
                        recon_pause();
                    }
                    index_stepper = 0;
                    if (stepper < 0) stepper++;
                    if (stepper > 0) stepper--;
                }  // if( recon || stepper != 0 || index_stepper != 0 )
            }      // if (frequency_list.size() > 0 )
        }          /* on_statistic_updates */
    }
    handle_retune();
    recon_redraw();
}

void ReconView::recon_pause() {
    timer = 0;
    freq_lock = 0;
    continuous_lock = false;
    recon = false;

    audio_output_start();

    big_display.set_style(&style_white);
    button_pause.set_text("<RESUME>");  // PAUSED, show resume
}

void ReconView::recon_resume() {
    timer = 0;
    freq_lock = 0;
    continuous_lock = false;
    recon = true;

    audio::output::stop();

    big_display.set_style(&style_white);
    button_pause.set_text("<PAUSE>");
}

void ReconView::on_index_delta(int32_t v) {
    if (v > 0) {
        fwd = true;
        button_dir.set_text("FW>");
    }
    if (v < 0) {
        fwd = false;
        button_dir.set_text("<RW");
    }
    if (frequency_list.size() > 0)
        index_stepper = v;

    freq_lock = 0;
    timer = 0;
}

void ReconView::on_stepper_delta(int32_t v) {
    if (v > 0) {
        fwd = true;
        button_dir.set_text("FW>");
    }
    if (v < 0) {
        fwd = false;
        button_dir.set_text("<RW");
    }
    if (frequency_list.size() > 0)
        stepper = v;

    freq_lock = 0;
    timer = 0;
}

void ReconView::on_headphone_volume_changed(int32_t v) {
    const auto new_volume = volume_t::decibel(v - 99) + audio::headphone::volume_range().max;
    receiver_model.set_headphone_volume(new_volume);
}

size_t ReconView::change_mode(freqman_index_t new_mod) {
    field_mode.on_change = [this](size_t, OptionsField::value_t) {};
    field_bw.on_change = [this](size_t, OptionsField::value_t) {};

    receiver_model.disable();
    baseband::shutdown();

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
            // bw 200k (0) default
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
    field_mode.set_selected_index(new_mod);
    field_mode.on_change = [this](size_t, OptionsField::value_t v) {
        if (v != -1) {
            change_mode(v);
        }
    };

    if (!recon)                  // for some motive, audio output gets stopped.
        audio::output::start();  // so if recon was stopped we resume audio
    receiver_model.enable();

    return freqman_entry_get_step_value(def_step);
}

void ReconView::handle_coded_squelch(const uint32_t value) {
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
    if (last_squelch_index < 0 || (unsigned)last_squelch_index != min_idx) {
        last_squelch_index = min_idx;
        if (min_diff < 40)
            text_ctcss.set("T: " + tone_keys[min_idx].first);
        else
            text_ctcss.set("             ");
    }
}
} /* namespace ui */
