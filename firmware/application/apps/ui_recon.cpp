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
#include "ui_freqman.hpp"
#include "capture_app.hpp"
#include "convert.hpp"
#include "file.hpp"
#include "file_reader.hpp"
#include "tone_key.hpp"
#include "replay_app.hpp"
#include "string_format.hpp"
#include "ui_fileman.hpp"
#include "io_file.hpp"
#include "io_convert.hpp"
#include "oversample.hpp"
#include "baseband_api.hpp"
#include "metadata_file.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
#include "utility.hpp"
#include "replay_thread.hpp"

using namespace portapack;
using namespace tonekey;
using portapack::memory::map::backup_ram;
namespace fs = std::filesystem;

namespace ui {

void ReconView::reload_restart_recon() {
    // force reload of current
    change_mode(field_mode.selected_index_value());
    uint8_t previous_index = current_index;
    reset_indexes();
    frequency_file_load();
    current_index = previous_index;
    handle_retune();
    if (frequency_list.size() > 0) {
        if (fwd) {
            button_dir.set_text("FW>");
        } else {
            button_dir.set_text("<RW");
        }
        recon_resume();
    }
    if (scanner_mode) {
        file_name.set_style(Theme::getInstance()->fg_red);
        button_scanner_mode.set_style(Theme::getInstance()->fg_red);
        button_scanner_mode.set_text("SCAN");
    } else {
        file_name.set_style(Theme::getInstance()->fg_blue);
        button_scanner_mode.set_style(Theme::getInstance()->fg_blue);
        button_scanner_mode.set_text("RECON");
    }
    if (frequency_list.size() > FREQMAN_MAX_PER_FILE) {
        file_name.set_style(Theme::getInstance()->fg_yellow);
    }
}

void ReconView::check_update_ranges_from_current() {
    if (frequency_list.size() && current_is_valid() && current_entry().type == freqman_type::Range) {
        if (update_ranges && !manual_mode) {
            button_manual_start.set_text(to_string_short_freq(current_entry().frequency_a));
            frequency_range.min = current_entry().frequency_a;
            if (current_entry().frequency_b != 0) {
                button_manual_end.set_text(to_string_short_freq(current_entry().frequency_b));
                frequency_range.max = current_entry().frequency_b;
            } else {
                button_manual_end.set_text(to_string_short_freq(current_entry().frequency_a));
                frequency_range.max = current_entry().frequency_a;
            }
        }
    }
}

bool ReconView::current_is_valid() {
    return (unsigned)current_index < frequency_list.size();
}

freqman_entry& ReconView::current_entry() {
    return *frequency_list[current_index];
}

void ReconView::set_loop_config(bool v) {
    continuous = v;
    button_loop_config.set_style(v ? Theme::getInstance()->fg_green : Theme::getInstance()->bg_darkest);
    persistent_memory::set_recon_continuous(continuous);
}

void ReconView::recon_stop_recording(bool exiting) {
    if (is_recording) {
        record_view->stop();
        is_recording = false;
        if (field_mode.selected_index_value() == SPEC_MODULATION) {
            button_audio_app.set_text("RAW");
            // repeater mode
            if (!exiting && persistent_memory::recon_repeat_recorded()) {
                start_repeat();
            }
        } else {
            button_audio_app.set_text("AUDIO");
        }
        button_audio_app.set_style(Theme::getInstance()->bg_darkest);
        button_config.set_style(Theme::getInstance()->bg_darkest);
    }
}

void ReconView::clear_freqlist_for_ui_action() {
    recon_stop_recording(false);
    if (field_mode.selected_index_value() != SPEC_MODULATION)
        audio::output::stop();
    // flag to detect and reload frequency_list
    if (!manual_mode) {
        // Clear doesn't actually free, re-assign so destructor runs on previous instance.
        frequency_list = freqman_db{};
    } else
        frequency_list.shrink_to_fit();
    freqlist_cleared_for_ui_action = true;
}

void ReconView::reset_indexes() {
    last_entry.modulation = freqman_invalid_index;
    last_entry.bandwidth = freqman_invalid_index;
    last_entry.step = freqman_invalid_index;
    current_index = 0;
}

void ReconView::update_description() {
    if (frequency_list.empty() || current_entry().description.empty()) {
        description = "...no description...";
    } else {
        switch (current_entry().type) {
            case freqman_type::Range:
                description = "R: ";
                break;
            case freqman_type::HamRadio:
                description = "H: ";
                break;
            case freqman_type::Repeater:
                description = "L: ";
                break;
            default:
                description = "S: ";
        }
        description += current_entry().description;
    }
    desc_cycle.set(description);
}

void ReconView::colorize_waits() {
    // colorize wait on match
    if (wait == 0) {
        field_wait.set_style(Theme::getInstance()->fg_blue);
    } else if (wait >= 500) {
        field_wait.set_style(Theme::getInstance()->bg_darkest);
    } else if (wait > -500 && wait < 500) {
        field_wait.set_style(Theme::getInstance()->fg_red);
    } else if (wait <= -500) {
        field_wait.set_style(Theme::getInstance()->fg_green);
    }
    // colorize lock time if in SPARSE mode as in continuous the lock_wait time is disarmed at first lock count
    if (recon_match_mode == RECON_MATCH_SPARSE) {
        if ((recon_lock_duration / STATS_UPDATE_INTERVAL) <= recon_lock_nb_match) {
            field_lock_wait.set_style(Theme::getInstance()->fg_yellow);
        } else {
            field_lock_wait.set_style(Theme::getInstance()->bg_darkest);
        }
    } else {
        field_lock_wait.set_style(Theme::getInstance()->bg_darkest);
    }
}

bool ReconView::recon_save_freq(const fs::path& path, size_t freq_index, bool warn_if_exists) {
    if (frequency_list.size() == 0 || !current_is_valid())
        return false;

    FreqmanDB freq_db;
    if (!freq_db.open(path, /*create*/ true))
        return false;

    freqman_entry entry = *frequency_list[freq_index];  // Makes a copy.

    // For ranges, save the current frequency instead.
    if (entry.type == freqman_type::Range) {
        entry.frequency_a = freq;
        entry.frequency_b = 0;
        entry.type = freqman_type::Single;
        entry.modulation = last_entry.modulation;
        entry.bandwidth = last_entry.bandwidth;
        entry.step = freqman_invalid_index;
    }

    auto it = freq_db.find_entry(entry);
    auto found = (it != freq_db.end());

    if (found && warn_if_exists)
        nav_.display_modal("Error", "Frequency already exists");

    if (!found)
        freq_db.append_entry(entry);

    return true;
}

void ReconView::load_persisted_settings() {
    autostart = persistent_memory::recon_autostart_recon();
    autosave = persistent_memory::recon_autosave_freqs();
    continuous = persistent_memory::recon_continuous();
    filedelete = persistent_memory::recon_clear_output();
    load_freqs = persistent_memory::recon_load_freqs();
    load_ranges = persistent_memory::recon_load_ranges();
    load_hamradios = persistent_memory::recon_load_hamradios();
    load_repeaters = persistent_memory::recon_load_repeaters();
    update_ranges = persistent_memory::recon_update_ranges_when_recon();
    auto_record_locked = persistent_memory::recon_auto_record_locked();
}

void ReconView::audio_output_start() {
    if (field_mode.selected_index_value() != SPEC_MODULATION)
        audio::output::start();
    receiver_model.set_headphone_volume(receiver_model.headphone_volume());  // WM8731 hack.
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
            big_display.set_style(Theme::getInstance()->bg_darkest);
            if (recon)
                button_pause.set_text("<PAUSE>");
            else
                button_pause.set_text("<RESUME>");
        } else if (freq_lock == 1 && recon_lock_nb_match != 1) {
            // STARTING LOCK FREQ
            big_display.set_style(Theme::getInstance()->fg_yellow);
            button_pause.set_text("<SKPLCK>");
        } else if (freq_lock >= recon_lock_nb_match) {
            big_display.set_style(Theme::getInstance()->fg_green);
            button_pause.set_text("<UNLOCK>");
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
        receiver_model.set_target_frequency(freq);  // Retune
    }
    if (frequency_list.size() > 0) {
        if (last_entry.modulation != current_entry().modulation && is_valid(current_entry().modulation)) {
            last_entry.modulation = current_entry().modulation;
            field_mode.set_selected_index(current_entry().modulation);
            last_entry.bandwidth = freqman_invalid_index;
        }
        // Set bandwidth if any
        if (last_entry.bandwidth != current_entry().bandwidth && is_valid(current_entry().bandwidth)) {
            last_entry.bandwidth = current_entry().bandwidth;
            field_bw.set_selected_index(current_entry().bandwidth);
        }
        if (last_entry.step != current_entry().step && is_valid(current_entry().step)) {
            last_entry.step = current_entry().step;
            step = freqman_entry_get_step_value(last_entry.step);
            step_mode.set_selected_index(step);
        }
        if (last_index != current_index) {
            last_index = current_index;
            check_update_ranges_from_current();
            text_cycle.set_text(to_string_dec_uint(current_index + 1, 3));
            update_description();
        }
    }
}

void ReconView::focus() {
    button_pause.focus();
}

ReconView::~ReconView() {
    if (recon_tx) {
        replay_thread.reset();
    }

    recon_stop_recording(true);

    if (field_mode.selected_index_value() != SPEC_MODULATION)
        audio::output::stop();

    transmitter_model.disable();
    receiver_model.disable();
    baseband::shutdown();
}

ReconView::ReconView(NavigationView& nav)
    : nav_{nav} {
    chrono_start = chTimeNow();

    tx_view.hidden(true);

    // set record View
    record_view = std::make_unique<RecordView>(Rect{0, 0, 30 * 8, 1 * 16},
                                               u"AUTO_AUDIO", audio_dir,
                                               RecordView::FileType::WAV, 4096, 4);
    record_view->set_filename_date_frequency(true);
    record_view->set_auto_trim(false);
    record_view->hidden(true);
    record_view->on_error = [&nav](std::string message) {
        nav.display_modal("Error", message);
    };

    add_children({&labels,
                  &field_lna,
                  &field_vga,
                  &field_rf_amp,
                  &field_volume,
                  &field_bw,
                  &field_squelch,
                  &field_nblocks,
                  &field_wait,
                  &field_lock_wait,
                  &button_config,
                  &button_scanner_mode,
                  &button_loop_config,
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
                  &field_recon_match_mode,
                  &step_mode,
                  &button_pause,
                  &button_audio_app,
                  &button_add,
                  &button_dir,
                  &button_restart,
                  &button_mic_app,
                  &button_remove,
                  record_view.get(),
                  &progressbar,
                  &tx_view});

    def_step = 0;
    load_persisted_settings();

    // When update_ranges is set or range invalid, use the rx model frequency instead of the saved values.
    if (update_ranges || frequency_range.max == 0) {
        rf::Frequency stored_freq = receiver_model.target_frequency();
        frequency_range.min = clip<rf::Frequency>(stored_freq - OneMHz, 0, MAX_UFREQ);
        frequency_range.max = clip<rf::Frequency>(stored_freq + OneMHz, 0, MAX_UFREQ);
    }

    button_manual_start.set_text(to_string_short_freq(frequency_range.min));
    button_manual_end.set_text(to_string_short_freq(frequency_range.max));

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
                // NB: This is using the freq keypad to select an index.
                f = f / OneMHz;
                if (f >= 1 && f <= frequency_list.size()) {
                    index_stepper = f - 1 - current_index;
                    freq_lock = 0;
                }
            };
        }
    };

    button_manual_start.on_change = [this]() {
        auto step_val = freqman_entry_get_step_value(def_step);
        frequency_range.min = frequency_range.min + button_manual_start.get_encoder_delta() * step_val;
        if (frequency_range.min < 0) {
            frequency_range.min = 0;
        }
        if (frequency_range.min > (MAX_UFREQ - step_val)) {
            frequency_range.min = MAX_UFREQ - step_val;
        }
        if (frequency_range.min > (frequency_range.max - step_val)) {
            frequency_range.max = frequency_range.min + step_val;
            if (frequency_range.max > MAX_UFREQ) {
                frequency_range.min = MAX_UFREQ - step_val;
                frequency_range.max = MAX_UFREQ;
            }
        }
        button_manual_start.set_text(to_string_short_freq(frequency_range.min));
        button_manual_end.set_text(to_string_short_freq(frequency_range.max));
        button_manual_start.set_encoder_delta(0);
    };

    button_manual_end.on_change = [this]() {
        auto step_val = freqman_entry_get_step_value(def_step);
        frequency_range.max = frequency_range.max + button_manual_end.get_encoder_delta() * step_val;
        if (frequency_range.max < (step_val + 1)) {
            frequency_range.max = (step_val + 1);
        }
        if (frequency_range.max > MAX_UFREQ) {
            frequency_range.max = MAX_UFREQ;
        }
        if (frequency_range.max < (frequency_range.min + step_val)) {
            frequency_range.min = frequency_range.max - step_val;
            if (frequency_range.max < (step_val + 1)) {
                frequency_range.min = 1;
                frequency_range.max = (step_val + 1);
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
                    user_pause = false;
                } else {
                    recon_pause();
                    user_pause = true;
                }
            }
        }
    };

    button_pause.on_change = [this]() {
        on_stepper_delta(button_pause.get_encoder_delta());
        button_pause.set_encoder_delta(0);
    };

    button_audio_app.on_select = [this](Button&) {
        auto settings = receiver_model.settings();
        settings.frequency_step = step_mode.selected_index_value();
        if (field_mode.selected_index_value() == SPEC_MODULATION)
            nav_.replace<CaptureAppView>(settings);
        else
            nav_.replace<AnalogAudioView>(settings);
    };

    button_loop_config.on_select = [this](Button&) {
        set_loop_config(!continuous);
    };
    set_loop_config(continuous);

    rssi.set_focusable(true);
    rssi.set_peak(true, 500);
    rssi.on_select = [this](RSSI&) {
        nav_.replace<LevelView>();
    };

    // TODO: *BUG* Both transmitter_model and receiver_model share the same pmem setting for target_frequency.
    button_mic_app.on_select = [this](Button&) {
        if (frequency_list.size() > 0 && current_index >= 0 && (unsigned)current_index < frequency_list.size()) {
            if (current_entry().type == freqman_type::HamRadio) {
                // if it's a HamRadio entry, then frequency_a is the freq at which the repeater receives, so we have to set it in transmit in mic app
                transmitter_model.set_target_frequency(current_entry().frequency_a);
                // if it's a HamRadio entry, then frequency_b is the freq at which the repeater transmits, so we have to set it in receive in mic app
                receiver_model.set_target_frequency(current_entry().frequency_b);
            } else {
                // it's single or range so we us actual tuned frequency
                transmitter_model.set_target_frequency(freq);
                receiver_model.set_target_frequency(freq);
            }
        }

        // MicTX wants Frequency, Modulation and Bandwidth overrides, but that's only stored on the RX model.
        nav_.replace<MicTXView>(receiver_model.settings());
    };

    button_remove.on_select = [this](ButtonWithEncoder&) {
        handle_remove_current_item();
        timer = 0;
        freq_lock = 0;
        recon_redraw();
    };

    button_remove.on_change = [this]() {
        on_stepper_delta(button_remove.get_encoder_delta());
        button_remove.set_encoder_delta(0);
    };

    button_manual_recon.on_select = [this](Button&) {
        button_remove.set_text("<DELETE>");
        button_add.hidden(false);
        scanner_mode = false;
        manual_mode = true;
        recon_pause();
        if (!frequency_range.min || !frequency_range.max) {
            nav_.display_modal("Error", "Both START and END freqs\nneed a value");
        } else {
            if (frequency_range.min > frequency_range.max) {
                std::swap(frequency_range.min, frequency_range.max);
                button_manual_start.set_text(to_string_short_freq(frequency_range.min));
                button_manual_end.set_text(to_string_short_freq(frequency_range.max));
            }

            // no need to stop audio in SPEC
            if (field_mode.selected_index_value() != SPEC_MODULATION)
                audio::output::stop();

            // Clear doesn't actually free, re-assign so destructor runs on previous instance.
            frequency_list = freqman_db{};
            current_index = 0;
            frequency_list.push_back(std::make_unique<freqman_entry>());

            def_step = step_mode.selected_index();
            current_entry().type = freqman_type::Range;
            current_entry().description =
                to_string_short_freq(frequency_range.min).erase(0, 1) + ">" +  // euquiq: lame kludge to reduce spacing in step freq
                to_string_short_freq(frequency_range.max).erase(0, 1) + " S:" +
                freqman_entry_get_step_string_short(def_step);
            current_entry().frequency_a = frequency_range.min;
            current_entry().frequency_b = frequency_range.max;
            current_entry().modulation = freqman_invalid_index;
            current_entry().bandwidth = freqman_invalid_index;
            current_entry().step = def_step;

            big_display.set_style(Theme::getInstance()->bg_darkest);  // Back to white color

            freq_stats.set_style(Theme::getInstance()->bg_darkest);
            freq_stats.set("0/0/0");

            text_cycle.set_text("1");
            text_max.set("/1");
            button_scanner_mode.set_style(Theme::getInstance()->bg_darkest);
            button_scanner_mode.set_text("MANUAL");
            file_name.set_style(Theme::getInstance()->bg_darkest);
            file_name.set("MANUAL => " + output_file);
            desc_cycle.set_style(Theme::getInstance()->bg_darkest);

            last_entry.modulation = freqman_invalid_index;
            last_entry.bandwidth = freqman_invalid_index;
            last_entry.step = freqman_invalid_index;
            last_index = -1;

            freq = current_entry().frequency_a;
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
        reload_restart_recon();
    };

    button_add.on_select = [this](ButtonWithEncoder&) {
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
            button_scanner_mode.set_style(Theme::getInstance()->fg_blue);
            button_scanner_mode.set_text("RECON");
            button_remove.set_text("<REMOVE>");
        } else {
            scanner_mode = true;
            button_scanner_mode.set_style(Theme::getInstance()->fg_red);
            button_scanner_mode.set_text("SCAN");
            button_remove.set_text("<DELETE>");
        }
        frequency_file_load();
        if (autostart) {
            recon_resume();
        } else {
            recon_pause();
        }
        button_add.hidden(scanner_mode);
        if (scanner_mode)  // only needed when hiding, UI mayhem
            set_dirty();
    };

    button_config.on_select = [this, &nav](Button&) {
        if (is_recording)  // disabling config while recording
            return;

        clear_freqlist_for_ui_action();

        freq_lock = 0;
        timer = 0;
        auto open_view = nav.push<ReconSetupView>(input_file, output_file);
        open_view->on_changed = [this](std::vector<std::string> result) {
            input_file = result[0];
            output_file = result[1];
            freq_file_path = get_freqman_path(output_file).string();

            load_persisted_settings();
            ui_settings.save();

            frequency_file_load();
            freqlist_cleared_for_ui_action = false;

            if (autostart) {
                recon_resume();
            } else {
                recon_pause();
            }
        };
    };

    field_recon_match_mode.on_change = [this](size_t, OptionsField::value_t v) {
        recon_match_mode = v;
        colorize_waits();
    };

    field_wait.on_change = [this](int32_t v) {
        wait = v;
        // replacing -100 by 200 else it's freezing the device
        if (wait == -100)
            wait = -200;
        colorize_waits();
    };

    field_nblocks.on_change = [this](int32_t v) {
        recon_lock_nb_match = v;
        if ((unsigned)v < freq_lock)
            freq_lock = v;
        colorize_waits();
    };

    field_lock_wait.on_change = [this](uint32_t v) {
        recon_lock_duration = v;
        colorize_waits();
    };

    field_squelch.on_change = [this](int32_t v) {
        squelch = v;
    };

    // PRE-CONFIGURATION:
    button_scanner_mode.set_style(Theme::getInstance()->fg_blue);
    button_scanner_mode.set_text("RECON");
    file_name.set("=>");

    // Loading input and output file from settings
    freq_file_path = get_freqman_path(output_file).string();

    field_recon_match_mode.set_selected_index(recon_match_mode);
    field_squelch.set_value(squelch);
    field_wait.set_value(wait);
    field_lock_wait.set_value(recon_lock_duration);
    field_nblocks.set_value(recon_lock_nb_match);
    colorize_waits();

    // fill modulation and step options
    freqman_set_modulation_option(field_mode);
    freqman_set_step_option(step_mode);

    // set radio
    change_mode(AM_MODULATION);              // start on AM.
    field_mode.set_by_value(AM_MODULATION);  // reflect the mode into the manual selector

    // tx progress bar
    progressbar.hidden(true);

    if (filedelete) {
        delete_file(freq_file_path);
    }

    frequency_file_load(); /* do not stop all at start */
    if (autostart) {
        recon_resume();
    } else {
        recon_pause();
    }
    recon_redraw();
}

void ReconView::frequency_file_load() {
    if (field_mode.selected_index_value() != SPEC_MODULATION)
        audio::output::stop();

    def_step = step_mode.selected_index();  // use def_step from manual selector
    std::string file_input = input_file;    // default recon mode
    if (scanner_mode) {
        file_input = output_file;
        file_name.set_style(Theme::getInstance()->fg_red);
        button_scanner_mode.set_style(Theme::getInstance()->fg_red);
        desc_cycle.set_style(Theme::getInstance()->fg_red);
        button_scanner_mode.set_text("SCAN");
    } else {
        file_name.set_style(Theme::getInstance()->fg_blue);
        button_scanner_mode.set_style(Theme::getInstance()->fg_blue);
        desc_cycle.set_style(Theme::getInstance()->fg_blue);
        button_scanner_mode.set_text("RECON");
    }

    file_name.set(file_input + "=>" + output_file);

    freqman_load_options options{
        .load_freqs = load_freqs,
        .load_ranges = load_ranges,
        .load_hamradios = load_hamradios,
        .load_repeaters = load_repeaters};
    if (!load_freqman_file(file_input, frequency_list, options) || frequency_list.empty()) {
        file_name.set_style(Theme::getInstance()->fg_red);
        desc_cycle.set("...empty file...");
        frequency_list.clear();
        text_cycle.set_text(" ");
        return;
    }

    if (frequency_list.size() > FREQMAN_MAX_PER_FILE) {
        file_name.set_style(Theme::getInstance()->fg_yellow);
    }

    reset_indexes();
    step = freqman_entry_get_step_value(
        is_valid(current_entry().step) ? current_entry().step : def_step);

    if (current_entry().type == freqman_type::Single) {
        freq = current_entry().frequency_a;
    } else if (current_entry().type != freqman_type::Unknown) {
        minfreq = current_entry().frequency_a;
        maxfreq = current_entry().frequency_b;
        freq = fwd ? minfreq : maxfreq;
    }

    step_mode.set_selected_index(def_step);  // Impose the default step into the manual step selector
    receiver_model.enable();
    receiver_model.set_squelch_level(0);
    text_cycle.set_text(to_string_dec_uint(current_index + 1, 3));
    check_update_ranges_from_current();
    update_description();
    handle_retune();
}

void ReconView::on_statistics_update(const ChannelStatistics& statistics) {
    if (recon_tx)
        return;

    if (is_repeat_active())
        return;

    chrono_end = chTimeNow();
    systime_t time_interval = chrono_end - chrono_start;
    chrono_start = chrono_end;

    // hack to reload the list if it was cleared by going into CONFIG
    if (freqlist_cleared_for_ui_action) {
        if (!manual_mode) {
            frequency_file_load();
        }
        if (autostart && !user_pause) {
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
            freq_lock = 0;
            timer = recon_lock_duration;
        }
        if (freq_lock < recon_lock_nb_match)  // LOCKING
        {
            if (status != 1) {
                status = 1;
                if (wait != 0) {
                    recon_stop_recording(false);
                    if (field_mode.selected_index_value() != SPEC_MODULATION)
                        audio::output::stop();
                }
            }
            if (db > squelch)  // MATCHING LEVEL
            {
                freq_lock++;
                timer += time_interval;  // give some more time for next lock
            } else {
                // continuous, direct cut it if not consecutive match after 1 first match
                if (recon_match_mode == RECON_MATCH_CONTINUOUS) {
                    freq_lock = 0;
                    timer = 0;
                }
            }
        }
        if (freq_lock >= recon_lock_nb_match)  // LOCKED
        {
            if (status != 2) {
                status = 2;
                // FREQ IS STRONG: GREEN and recon will pause when on_statistics_update()
                if ((!scanner_mode) && autosave && frequency_list.size() > 0) {
                    recon_save_freq(freq_file_path, current_index, false);
                }
                if (wait != 0) {
                    if (field_mode.selected_index_value() != SPEC_MODULATION)
                        audio_output_start();
                    // contents of a possible recon_start_recording(), but not yet since it's only called once
                    if (auto_record_locked && !is_recording) {
                        button_audio_app.set_style(Theme::getInstance()->fg_red);
                        if (field_mode.selected_index_value() == SPEC_MODULATION) {
                            button_audio_app.set_text("RAW REC");
                        } else
                            button_audio_app.set_text("WAV REC");
                        record_view->start();
                        button_config.set_style(Theme::getInstance()->fg_light);  // disable config while recording as it's causing an IO error pop up at exit
                        is_recording = true;
                    }
                    // FREQ IS STRONG: GREEN and recon will pause when on_statistics_update()
                    if ((!scanner_mode) && autosave && frequency_list.size() > 0) {
                        recon_save_freq(freq_file_path, current_index, false);
                    }
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

    if (timer != 0) {
        timer -= time_interval;
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
                        if (current_entry().type == freqman_type::Range) {
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
                        } else if (current_entry().type == freqman_type::Single) {
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
                        } else if (current_entry().type == freqman_type::HamRadio) {
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
                        } else if (current_entry().type == freqman_type::Repeater) {
                            // repeater is like single, we only listen on frequency_a and then jump to next entry
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

                        // for some motive, audio output gets stopped.
                        if (!recon && field_mode.selected_index_value() != SPEC_MODULATION)
                            audio_output_start();
                    }
                    // reload entry if changed
                    if (entry_has_changed) {
                        timer = 0;
                        switch (current_entry().type) {
                            case freqman_type::Repeater:
                            case freqman_type::Single:
                                freq = current_entry().frequency_a;
                                break;
                            case freqman_type::Range:
                                minfreq = current_entry().frequency_a;
                                maxfreq = current_entry().frequency_b;
                                if ((fwd && !stepper && !index_stepper) || stepper > 0 || index_stepper > 0) {
                                    freq = minfreq;
                                } else if ((!fwd && !stepper && !index_stepper) || stepper < 0 || index_stepper < 0) {
                                    freq = maxfreq;
                                }
                                break;
                            case freqman_type::HamRadio:
                                minfreq = current_entry().frequency_a;
                                maxfreq = current_entry().frequency_b;
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
        }          /* on_statistics_updates */
    }
    handle_retune();
    recon_redraw();
}

void ReconView::recon_pause() {
    timer = 0;
    freq_lock = 0;
    recon = false;

    if (field_mode.selected_index_value() != SPEC_MODULATION)
        audio_output_start();

    big_display.set_style(Theme::getInstance()->bg_darkest);
    button_pause.set_text("<RESUME>");  // PAUSED, show resume
}

void ReconView::recon_resume() {
    timer = 0;
    freq_lock = 0;
    recon = true;

    if (field_mode.selected_index_value() != SPEC_MODULATION)
        audio::output::stop();

    big_display.set_style(Theme::getInstance()->bg_darkest);
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

size_t ReconView::change_mode(freqman_index_t new_mod) {
    if (recon_tx || is_repeat_active() || is_recording)
        return 0;
    field_mode.on_change = [this](size_t, OptionsField::value_t) {};
    field_bw.on_change = [this](size_t, OptionsField::value_t) {};
    recon_stop_recording(false);
    if (record_view != nullptr) {
        remove_child(record_view.get());
        record_view.reset();
    }
    if (field_mode.selected_index_value() != SPEC_MODULATION) {
        audio::output::stop();
    }
    if (new_mod == SPEC_MODULATION) {
        if (persistent_memory::recon_repeat_recorded()) {
            record_view = std::make_unique<RecordView>(Rect{0, 0, 30 * 8, 1 * 16},
                                                       u"RECON_REPEAT.C16", captures_dir,
                                                       RecordView::FileType::RawS16, 16384, 3);
            record_view->set_filename_as_is(true);
        } else {
            record_view = std::make_unique<RecordView>(Rect{0, 0, 30 * 8, 1 * 16},
                                                       u"AUTO_RAW", captures_dir,
                                                       RecordView::FileType::RawS16, 16384, 3);
            record_view->set_filename_date_frequency(true);
        }
    } else {
        record_view = std::make_unique<RecordView>(Rect{0, 0, 30 * 8, 1 * 16},
                                                   u"AUTO_AUDIO", audio_dir,
                                                   RecordView::FileType::WAV, 4096, 4);
        record_view->set_filename_date_frequency(true);
    }
    record_view->set_auto_trim(false);
    add_child(record_view.get());
    record_view->hidden(true);
    record_view->on_error = [this](std::string message) {
        nav_.display_modal("Error", message);
    };
    receiver_model.disable();
    transmitter_model.disable();
    baseband::shutdown();
    size_t recording_sampling_rate = 0;
    switch (new_mod) {
        case AM_MODULATION:
            freqman_set_bandwidth_option(new_mod, field_bw);
            baseband::run_image(portapack::spi_flash::image_tag_am_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::AMAudio);
            receiver_model.set_am_configuration(field_bw.selected_index_value());
            field_bw.on_change = [this](size_t, OptionsField::value_t n) { receiver_model.set_am_configuration(n); };
            // bw DSB (0) default
            field_bw.set_by_value(0);
            text_ctcss.set("        ");
            recording_sampling_rate = 12000;
            break;
        case NFM_MODULATION:
            freqman_set_bandwidth_option(new_mod, field_bw);
            baseband::run_image(portapack::spi_flash::image_tag_nfm_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
            receiver_model.set_nbfm_configuration(field_bw.selected_index_value());
            field_bw.on_change = [this](size_t, OptionsField::value_t n) { receiver_model.set_nbfm_configuration(n); };
            // bw 16k (2) default
            field_bw.set_by_value(2);
            recording_sampling_rate = 24000;
            break;
        case WFM_MODULATION:
            freqman_set_bandwidth_option(new_mod, field_bw);
            baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
            receiver_model.set_wfm_configuration(field_bw.selected_index_value());
            field_bw.on_change = [this](size_t, OptionsField::value_t n) { receiver_model.set_wfm_configuration(n); };
            // bw 200k (0) default
            field_bw.set_by_value(0);
            text_ctcss.set("        ");
            recording_sampling_rate = 48000;
            break;
        case SPEC_MODULATION:
            freqman_set_bandwidth_option(new_mod, field_bw);
            baseband::run_image(portapack::spi_flash::image_tag_capture);
            receiver_model.set_modulation(ReceiverModel::Mode::Capture);
            field_bw.on_change = [this](size_t, OptionsField::value_t sampling_rate) {
                // record_view determines the correct oversampling to apply and returns the actual sample rate.
                auto actual_sampling_rate = record_view->set_sampling_rate(sampling_rate);
                // The radio needs to know the effective sampling rate.
                receiver_model.set_sampling_rate(actual_sampling_rate);
                receiver_model.set_baseband_bandwidth(filter_bandwidth_for_sampling_rate(actual_sampling_rate));
            };
            // bw 12k5 (0) default
            field_bw.set_by_value(0);
            text_ctcss.set("        ");
            break;
        default:
            break;
    }
    if (new_mod != SPEC_MODULATION) {
        button_audio_app.set_text("AUDIO");
        record_view->set_sampling_rate(recording_sampling_rate);
        // reset receiver model to fix bug when going from SPEC to audio, the sound is distorted
        receiver_model.set_sampling_rate(3072000);
        receiver_model.set_baseband_bandwidth(1750000);
    } else {
        button_audio_app.set_text("RAW");
    }

    field_mode.set_selected_index(new_mod);
    field_mode.on_change = [this](size_t, OptionsField::value_t v) {
        if (v != -1) {
            change_mode(v);
        }
    };
    // for some motive, audio output gets stopped.
    if (!recon && field_mode.selected_index_value() != SPEC_MODULATION)
        audio::output::start();  // so if recon was stopped we resume audio
    receiver_model.enable();

    return freqman_entry_get_step_value(def_step);
}

void ReconView::handle_coded_squelch(const uint32_t value) {
    if (field_mode.selected_index() == NFM_MODULATION)
        text_ctcss.set(tone_key_string_by_value(value, text_ctcss.parent_rect().width() / 8));
    else
        text_ctcss.set("        ");
}

void ReconView::handle_remove_current_item() {
    if (frequency_list.empty() || !current_is_valid())
        return;

    auto entry = current_entry();  // Copy the current entry.

    // In Scanner or Recon modes, remove from the in-memory list.
    if (mode() != recon_mode::Manual) {
        if (current_is_valid()) {
            frequency_list.erase(frequency_list.begin() + current_index);
        }
    }

    // In Scanner or Manual mode, remove the entry from the output file.
    if (mode() != recon_mode::Recon) {
        FreqmanDB freq_db;
        if (!freq_db.open(freq_file_path))
            return;

        freq_db.delete_entry(entry);
    }

    // Clip
    if (frequency_list.size() > 0) {
        current_index = clip<int32_t>(current_index, 0u, frequency_list.size() - 1);
        text_cycle.set_text(to_string_dec_uint(current_index + 1, 3));
        entry = current_entry();
        freq = entry.frequency_a;
    } else {
        current_index = 0;
        text_cycle.set_text(" ");
    }
    update_description();
}

void ReconView::on_repeat_tx_progress(const uint32_t progress) {
    progressbar.set_value(progress);
}

void ReconView::repeat_file_error(const std::filesystem::path& path, const std::string& message) {
    nav_.display_modal("Error", "Error opening file \n" + path.string() + "\n" + message);
}

bool ReconView::is_repeat_active() const {
    return replay_thread != nullptr;
}

void ReconView::start_repeat() {
    // Prepare to send a file.
    if (recon_tx == false) {
        recon_tx = true;

        if (record_view != nullptr) {
            record_view->stop();
            remove_child(record_view.get());
            record_view.reset();
        }

        receiver_model.disable();
        transmitter_model.disable();
        baseband::shutdown();

        baseband::run_image(portapack::spi_flash::image_tag_replay);

        size_t rawsize = 0;
        {
            File capture_file;
            auto error = capture_file.open(rawfile);
            if (error) {
                repeat_file_error(rawfile, "Can't open file to send for size");
                return;
            }
            rawsize = capture_file.size();
        }

        // Reset the transmit progress bar.
        uint8_t sample_size = std::filesystem::capture_file_sample_size(rawfile);
        progressbar.set_value(0);
        progressbar.set_max(rawsize * sizeof(complex16_t) / sample_size);
        progressbar.hidden(false);

        auto metadata = read_metadata_file(rawmeta);
        // If no metadata found, fallback to the TX frequency.
        if (!metadata) {
            metadata = {freq, 500'000};
            repeat_file_error(rawmeta, "Can't open file to read meta, using default rx_freq,500'000");
        }

        // Update the sample rate in proc_replay baseband.
        baseband::set_sample_rate(metadata->sample_rate,
                                  get_oversample_rate(metadata->sample_rate));

        transmitter_model.set_sampling_rate(get_actual_sample_rate(metadata->sample_rate));
        transmitter_model.set_baseband_bandwidth(metadata->sample_rate <= 500'000 ? 1'750'000 : 2'500'000);  // TX LPF min 1M75 for SR <=500K, and  2M5 (by experimental test) for SR >500K

        // set TX to repeater TX freq if entry is Repeater and have a valid freq, else use recorded one
        if (current_entry().type == freqman_type::Repeater && current_entry().frequency_b != 0) {
            transmitter_model.set_target_frequency(current_entry().frequency_b);
        } else {
            transmitter_model.set_target_frequency(metadata->center_frequency);
        }

        // set TX powers and enable transmitter
        transmitter_model.set_tx_gain(persistent_memory::recon_repeat_gain());
        transmitter_model.set_rf_amp(persistent_memory::recon_repeat_amp());
        transmitter_model.enable();
    }

    // clear replay thread and set reader
    replay_thread.reset();
    auto reader = std::make_unique<FileConvertReader>();
    auto error = reader->open(rawfile);
    if (error) {
        repeat_file_error(rawfile, "Can't open file to send to thread");
        return;
    }
    // wait for TX if needed (hackish, direct screen update since the UI will be blocked)
    if (persistent_memory::recon_repeat_delay() > 0) {
        uint8_t delay = persistent_memory::recon_repeat_delay();
        Painter p;
        while (delay > 0) {
            std::string delay_message = "TX DELAY: " + to_string_dec_uint(delay) + "s";

            // update display information
            p.fill_rectangle({0, (SCREEN_H / 2) - 16, SCREEN_W, 64}, Theme::getInstance()->fg_light->foreground);
            p.draw_string({(SCREEN_W / 2) - 7 * 8, SCREEN_H / 2}, *Theme::getInstance()->fg_red, delay_message);

            // sleep 1 second
            chThdSleepMilliseconds(1000);

            // decre delay
            if (delay > 0)
                delay = delay - 1;
            else
                break;
        }
    }

    // ReplayThread starts immediately on construction; must be set before creating.
    repeat_ready_signal = true;
    repeat_cur_rep++;
    replay_thread = std::make_unique<ReplayThread>(
        std::move(reader),
        /* read_size */ repeat_read_size,
        /* buffer_count */ repeat_buffer_count,
        &repeat_ready_signal,
        [](uint32_t return_code) {
            ReplayThreadDoneMessage message{return_code};
            EventDispatcher::send_message(message);
        });
}

void ReconView::stop_repeat(const bool do_loop) {
    repeat_ready_signal = false;

    if (is_repeat_active()) {
        replay_thread.reset();
        transmitter_model.disable();
    }

    // repeat transmit if current number of repetitions (repeat_cur_rep) is < recon configured number of repetitions (recon_repeat_nb)
    if (do_loop && repeat_cur_rep < persistent_memory::recon_repeat_nb()) {
        start_repeat();
    } else {
        repeat_cur_rep = 0;
        recon_tx = false;
        if (persistent_memory::recon_repeat_recorded_file_mode() == RECON_REPEAT_AND_KEEP) {
            // rename file here to keep
            std::filesystem::path base_path = next_filename_matching_pattern(repeat_rec_path / u"REC_????.*");
            rename_file(rawfile, base_path.replace_extension(u".C16"));
            rename_file(rawmeta, base_path.replace_extension(u".TXT"));
        }
        reload_restart_recon();
        progressbar.hidden(true);
        set_dirty();  // fix progressbar no hiding
    }
}

void ReconView::handle_repeat_thread_done(const uint32_t return_code) {
    if (return_code == ReplayThread::END_OF_FILE) {
        stop_repeat(true);
    } else if (return_code == ReplayThread::READ_ERROR) {
        stop_repeat(false);
        repeat_file_error(rawfile, "Can't open file to send.");
    }
}

} /* namespace ui */
