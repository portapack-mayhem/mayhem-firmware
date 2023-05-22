/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#include "ui_scanner.hpp"
#include "ui_fileman.hpp"

using namespace portapack;

namespace ui {

ScannerThread::ScannerThread(std::vector<rf::Frequency> frequency_list)
    : frequency_list_{std::move(frequency_list)} {
    _manual_search = false;
    create_thread();
}

ScannerThread::ScannerThread(const jammer::jammer_range_t& frequency_range, size_t def_step_hz)
    : frequency_range_(frequency_range), def_step_hz_(def_step_hz) {
    _manual_search = true;
    create_thread();
}

ScannerThread::~ScannerThread() {
    stop();
}

void ScannerThread::create_thread() {
    thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, ScannerThread::static_fn, this);
}

void ScannerThread::stop() {
    if (thread) {
        chThdTerminate(thread);
        chThdWait(thread);
        thread = nullptr;
    }
}

// Set by "userpause"
void ScannerThread::set_scanning(const bool v) {
    _scanning = v;
}

bool ScannerThread::is_scanning() {
    return _scanning;
}

void ScannerThread::set_freq_lock(const uint32_t v) {
    _freq_lock = v;
}

uint32_t ScannerThread::is_freq_lock() {
    return _freq_lock;
}

// Delete an entry from frequency list
// Caller must pause scan_thread AND can't delete a second one until this field is cleared
void ScannerThread::set_freq_del(const rf::Frequency v) {
    _freq_del = v;
}

// Force a one-time forward or reverse frequency index change; OK to do this without pausing scan thread
//(used when rotary encoder is turned)
void ScannerThread::set_index_stepper(const int32_t v) {
    _index_stepper = v;
}

// Set scanning direction; OK to do this without pausing scan_thread
void ScannerThread::set_scanning_direction(bool fwd) {
    int32_t new_stepper = fwd ? 1 : -1;

    if (_stepper != new_stepper) {
        _stepper = new_stepper;
        chThdSleepMilliseconds(300);  // Give some pause after reversing scanning direction
    }
}

msg_t ScannerThread::static_fn(void* arg) {
    auto obj = static_cast<ScannerThread*>(arg);
    obj->run();
    return 0;
}

void ScannerThread::run() {
    RetuneMessage message{};

    if (!_manual_search && frequency_list_.size()) {  // IF NOT MANUAL MODE AND THERE IS A FREQUENCY LIST ...
        int32_t size = frequency_list_.size();
        int32_t frequency_index = (_stepper > 0) ? size : 0;  // Forcing wraparound to starting frequency on 1st pass

        while (!chThdShouldTerminate()) {
            bool force_one_step = (_index_stepper != 0);
            int32_t step = force_one_step ? _index_stepper : _stepper;  //_index_stepper direction takes priority

            if (_scanning || force_one_step) {              // Scanning, or paused and using rotary encoder
                if ((_freq_lock == 0) || force_one_step) {  // normal scanning (not performing freq_lock)
                    frequency_index += step;
                    if (frequency_index >= size)  // Wrap
                        frequency_index = 0;
                    else if (frequency_index < 0)
                        frequency_index = size - 1;

                    if (force_one_step)
                        _index_stepper = 0;

                    receiver_model.set_tuning_frequency(frequency_list_[frequency_index]);  // Retune
                }
                message.freq = frequency_list_[frequency_index];
                message.range = frequency_index;  // Inform freq (for coloring purposes also!)
                EventDispatcher::send_message(message);
            } else if (_freq_del != 0) {                    // There is a frequency to delete
                for (int32_t i = 0; i < size; i++) {        // Search for the freq to delete
                    if (frequency_list_[i] == _freq_del) {  // found: Erase it
                        frequency_list_.erase(frequency_list_.begin() + i);
                        size = frequency_list_.size();
                        break;
                    }
                }
                _freq_del = 0;  // deleted.
            }

            chThdSleepMilliseconds(SCANNER_SLEEP_MS);  // Needed to (eventually) stabilize the receiver into new freq
        }
    } else if (_manual_search && (def_step_hz_ > 0))  // manual search range mode
    {
        int64_t size = (frequency_range_.max - frequency_range_.min) / def_step_hz_;
        int64_t frequency_index = (_stepper > 0) ? size : 0;  // Forcing wraparound to starting frequency on 1st pass

        while (!chThdShouldTerminate()) {
            bool force_one_step = (_index_stepper != 0);
            int32_t step = force_one_step ? _index_stepper : _stepper;  //_index_stepper direction takes priority

            if (_scanning || force_one_step) {              // Scanning, or paused and using rotary encoder
                if ((_freq_lock == 0) || force_one_step) {  // normal scanning (not performing freq_lock)
                    frequency_index += step;
                    if (frequency_index >= size)  // Wrap
                        frequency_index = 0;
                    else if (frequency_index < 0)
                        frequency_index = size - 1;

                    if (force_one_step)
                        _index_stepper = 0;

                    receiver_model.set_tuning_frequency(frequency_range_.min + frequency_index * def_step_hz_);  // Retune
                }
                message.freq = frequency_range_.min + frequency_index * def_step_hz_;
                message.range = 0;  // Inform freq (for coloring purposes also!)
                EventDispatcher::send_message(message);
            }

            chThdSleepMilliseconds(SCANNER_SLEEP_MS);  // Needed to (eventually) stabilize the receiver into new freq
        }
    }
}

void ScannerView::bigdisplay_update(int32_t v) {
    if (v != bigdisplay_current_color) {
        if (v != -1)
            bigdisplay_current_color = v;  // -1 means refresh display but keep current color

        switch (bigdisplay_current_color) {
            case BDC_GREY:
                big_display.set_style(&style_grey);
                break;
            case BDC_YELLOW:
                big_display.set_style(&style_yellow);
                break;
            case BDC_GREEN:
                big_display.set_style(&style_green);
                break;
            case BDC_RED:
                big_display.set_style(&style_red);
                break;
            default:
                break;
        }

        // update frequency display
        bigdisplay_current_frequency = current_frequency;
        big_display.set(bigdisplay_current_frequency);
    } else {
        // no style change, but update frequency display if it's changed
        if (current_frequency != bigdisplay_current_frequency) {
            bigdisplay_current_frequency = current_frequency;
            big_display.set(bigdisplay_current_frequency);
        }
    }
}

void ScannerView::handle_retune(int64_t freq, uint32_t freq_idx) {
    current_index = freq_idx;  // since it is an ongoing scan, this is a new index
    current_frequency = freq;

    if (scan_thread) {
        switch (scan_thread->is_freq_lock()) {
            case 0:  // NO FREQ LOCK, ONGOING STANDARD SCANNING
                bigdisplay_update(BDC_GREY);
                break;
            case 1:  // STARTING LOCK FREQ
                bigdisplay_update(BDC_YELLOW);
                break;
            case MAX_FREQ_LOCK:  // FREQ IS STRONG: GREEN and scanner will pause when on_statistics_update()
                bigdisplay_update(BDC_GREEN);
                break;
            default:  // freq lock is checking the signal, do not update display
                return;
        }
    }

    if (!manual_search) {
        if (frequency_list.size() > 0) {
            text_current_index.set(to_string_dec_uint(freq_idx + 1, 3));
        }

        if (freq_idx < description_list.size() && description_list[freq_idx].size() > 1)
            desc_current_index.set(description_list[freq_idx]);  // Show description from file
        else
            desc_current_index.set(desc_freq_list_scan);  // Show Scan file name (no description in file)
    }
}

void ScannerView::focus() {
    field_mode.focus();
}

ScannerView::~ScannerView() {
    // make sure to stop the thread before shutting down the receiver
    scan_thread.reset();
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

void ScannerView::show_max_index() {  // show total number of freqs to scan
    text_current_index.set("---");

    if (frequency_list.size() == FREQMAN_MAX_PER_FILE) {
        text_max_index.set_style(&style_red);
        text_max_index.set("/ " + to_string_dec_uint(FREQMAN_MAX_PER_FILE) + " (DB MAX!)");
    } else {
        text_max_index.set_style(&style_grey);
        text_max_index.set("/ " + to_string_dec_uint(frequency_list.size()));
    }
}

ScannerView::ScannerView(
    NavigationView& nav)
    : nav_{nav}, loaded_file_name{"SCANNER"} {
    add_children({&labels,
                  &field_lna,
                  &field_vga,
                  &field_rf_amp,
                  &field_volume,
                  &field_bw,
                  &field_squelch,
                  &field_browse_wait,
                  &field_lock_wait,
                  &button_load,
                  &button_clear,
                  &rssi,
                  &text_current_index,
                  &text_max_index,
                  &desc_current_index,
                  &big_display,
                  &button_manual_start,
                  &button_manual_end,
                  &field_mode,
                  &field_step,
                  &button_manual_search,
                  &button_pause,
                  &button_dir,
                  &button_audio_app,
                  &button_mic_app,
                  &button_add,
                  &button_remove

    });

    // Populate option text for these fields
    freqman_set_modulation_option(field_mode);
    freqman_set_step_option(field_step);

    // Default starting modulation (these may be overridden in SCANNER.TXT)
    change_mode(AM_MODULATION);              // Default modulation
    field_mode.set_by_value(AM_MODULATION);  // Reflect the mode into the manual selector
    field_step.set_by_value(9000);           // Default step interval (Hz)

    // FUTURE: perhaps additional settings should be stored in persistent memory vs using defaults
    // HELPER: Pre-setting a manual range, based on stored frequency
    rf::Frequency stored_freq = persistent_memory::tuned_frequency();
    frequency_range.min = stored_freq - 1000000;
    button_manual_start.set_text(to_string_short_freq(frequency_range.min));
    frequency_range.max = stored_freq + 1000000;
    button_manual_end.set_text(to_string_short_freq(frequency_range.max));

    // Button to load txt files from the FREQMAN folder
    button_load.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".TXT");
        open_view->on_changed = [this](std::filesystem::path new_file_path) {
            std::string dir_filter = "FREQMAN/";
            std::string str_file_path = new_file_path.string();

            if (str_file_path.find(dir_filter) != string::npos) {  // assert file from the FREQMAN folder
                scan_pause();
                // get the filename without txt extension so we can use load_freqman_file fcn
                std::string str_file_name = new_file_path.stem().string();
                frequency_file_load(str_file_name, true);
            } else {
                nav_.display_modal("LOAD ERROR", "A valid file from\nFREQMAN directory is\nrequired.");
            }
        };
    };

    // Button to clear in-memory frequency list
    button_clear.on_select = [this, &nav](Button&) {
        if (scan_thread && frequency_list.size()) {
            scan_thread->stop();  // STOP SCANNER THREAD
            frequency_list.clear();
            description_list.clear();

            show_max_index();  // UPDATE new list size on screen
            text_current_index.set("");
            desc_current_index.set(desc_freq_list_scan);
            scan_thread->set_freq_lock(0);  // Reset the scanner lock

            // FUTURE: Consider switching to manual search mode automatically after clear (but would need to validate freq range)
        }
    };

    // Button to configure starting frequency for a manual range search
    button_manual_start.on_select = [this, &nav](Button& button) {
        auto new_view = nav_.push<FrequencyKeypadView>(frequency_range.min);
        new_view->on_changed = [this, &button](rf::Frequency f) {
            frequency_range.min = f;
            button_manual_start.set_text(to_string_short_freq(f));
        };
    };

    // Button to configure ending frequency for a manual range search
    button_manual_end.on_select = [this, &nav](Button& button) {
        auto new_view = nav.push<FrequencyKeypadView>(frequency_range.max);
        new_view->on_changed = [this, &button](rf::Frequency f) {
            frequency_range.max = f;
            button_manual_end.set_text(to_string_short_freq(f));
        };
    };

    // Button to pause/resume scan (note that some other buttons will trigger resume also)
    button_pause.on_select = [this](ButtonWithEncoder&) {
        if (userpause)
            user_resume();
        else {
            scan_pause();
            button_pause.set_text("<RESUME>");  // PAUSED, show resume
            userpause = true;
        }
    };

    // Encoder dial causes frequency change when focus is on pause button
    button_pause.on_change = [this]() {
        int32_t encoder_delta{(button_pause.get_encoder_delta() > 0) ? 1 : -1};

        if (scan_thread)
            scan_thread->set_index_stepper(encoder_delta);

        // Restart browse timer when frequency changes
        if (browse_timer != 0)
            browse_timer = 1;

        button_pause.set_encoder_delta(0);
    };

    // Button to switch to Audio app
    button_audio_app.on_select = [this](Button&) {
        if (scan_thread)
            scan_thread->stop();
        nav_.pop();
        nav_.push<AnalogAudioView>();
    };

    // Button to switch to Mic app
    button_mic_app.on_select = [this](Button&) {
        if (scan_thread)
            scan_thread->stop();
        nav_.pop();
        nav_.push<MicTXView>();
    };

    // Button to delete current frequency from scan Freq List
    button_remove.on_select = [this](Button&) {
        if (scan_thread && (frequency_list.size() > current_index)) {
            scan_thread->set_scanning(false);  // PAUSE Scanning if necessary

            // Remove frequency from the Freq List in memory (it is not removed from the file)
            scan_thread->set_freq_del(frequency_list[current_index]);
            description_list.erase(description_list.begin() + current_index);
            frequency_list.erase(frequency_list.begin() + current_index);

            show_max_index();               // UPDATE new list size on screen
            desc_current_index.set("");     // Clean up description (cosmetic detail)
            scan_thread->set_freq_lock(0);  // Reset the scanner lock
        }
    };

    // Button to toggle between Manual Search and Freq List Scan modes
    button_manual_search.on_select = [this](Button&) {
        if (!manual_search) {
            if (!frequency_range.min || !frequency_range.max) {
                nav_.display_modal("Error", "Both START and END freqs\nneed a value");
            } else if (frequency_range.min > frequency_range.max) {
                nav_.display_modal("Error", "END freq\nis lower than START");
            } else {
                manual_search = true;  // Switch to Manual Search mode
            }
        } else {
            manual_search = false;  // Switch to List Scan mode
        }

        audio::output::stop();

        if (scan_thread)
            scan_thread->stop();  // STOP SCANNER THREAD

        if (userpause)  // If user-paused, resume
            user_resume();

        start_scan_thread();  // RESTART SCANNER THREAD in selected mode
    };

    // Mode field was changed (AM/NFM/WFM)
    field_mode.on_change = [this](size_t, OptionsField::value_t v) {
        receiver_model.disable();
        baseband::shutdown();
        change_mode((freqman_index_t)v);
        if (scan_thread && !scan_thread->is_scanning())  // for some motive, audio output gets stopped.
            audio::output::start();                      // So if scan was stopped we resume audio
        receiver_model.enable();
    };

    // Step field was changed (Hz) -- only affects manual Search mode
    field_step.on_change = [this](size_t, OptionsField::value_t v) {
        (void)v;  // prevent compiler Unused warning

        if (manual_search && scan_thread) {
            // Restart scan thread with new step value
            scan_thread->stop();  // STOP SCANNER THREAD

            // Resuming pause automatically
            // FUTURE: Consider whether we should stay in Pause mode...
            if (userpause)  // If user-paused, resume
                user_resume();

            start_scan_thread();  // RESTART SCANNER THREAD in Manual Search mode
        }
    };

    // Button to toggle Forward/Reverse
    button_dir.on_select = [this](Button&) {
        fwd = !fwd;
        if (scan_thread)
            scan_thread->set_scanning_direction(fwd);
        if (userpause)  // If user-paused, resume
            user_resume();
        button_dir.set_text(fwd ? "REVERSE" : "FORWARD");
        bigdisplay_update(BDC_GREY);  // Back to grey color
    };

    // Button to add current frequency (found during Search) to the Scan Frequency List
    button_add.on_select = [this](Button&) {
        File scanner_file;
        const std::string freq_file_path = "FREQMAN/" + loaded_file_name + ".TXT";
        auto result = scanner_file.open(freq_file_path);  // First search if freq is already in txt

        if (!result.is_valid()) {
            const std::string frequency_to_add = "f=" + to_string_dec_uint(current_frequency / 1000) + to_string_dec_uint(current_frequency % 1000UL, 3, '0');

            bool found = false;
            constexpr size_t buffer_size = 1024;
            char buffer[buffer_size];

            for (size_t pointer = 0, freq_str_idx = 0; pointer < scanner_file.size(); pointer += buffer_size) {
                size_t adjusted_buffer_size;
                if (pointer + buffer_size >= scanner_file.size()) {
                    memset(buffer, 0, sizeof(buffer));
                    adjusted_buffer_size = scanner_file.size() - pointer;
                } else {
                    adjusted_buffer_size = buffer_size;
                }

                scanner_file.seek(pointer);
                scanner_file.read(buffer, adjusted_buffer_size);

                for (size_t i = 0; i < adjusted_buffer_size; ++i) {
                    if (buffer[i] == frequency_to_add.data()[freq_str_idx]) {
                        ++freq_str_idx;
                        if (freq_str_idx >= frequency_to_add.size()) {
                            found = true;
                            break;
                        }
                    } else {
                        freq_str_idx = 0;
                    }
                }

                if (found) {
                    break;
                }
            }

            if (found) {
                nav_.display_modal("Error", "Frequency already exists");
                bigdisplay_update(-1);  // After showing an error
            } else {
                scanner_file.append(freq_file_path);  // Second: append if it is not there
                scanner_file.write_line(frequency_to_add);

                // Add to frequency_list in memory too, since we can now switch back from manual mode
                // Note that we are allowing freqs to be added to file (code above) that exceed the max count we can load into memory
                if (frequency_list.size() < FREQMAN_MAX_PER_FILE) {
                    frequency_list.push_back(current_frequency);
                    description_list.push_back("");

                    show_max_index();  // Display updated frequency list size
                }
            }
        } else {
            nav_.display_modal("Error", "Cannot open " + loaded_file_name + ".TXT\nfor appending freq.");
            bigdisplay_update(-1);  // After showing an error
        }
    };

    // PRE-CONFIGURATION:
    field_browse_wait.on_change = [this](int32_t v) { browse_wait = v; };
    field_browse_wait.set_value(5);

    field_lock_wait.on_change = [this](int32_t v) { lock_wait = v; };
    field_lock_wait.set_value(2);

    field_squelch.on_change = [this](int32_t v) { squelch = v; };
    field_squelch.set_value(-10);

    field_volume.set_value((receiver_model.headphone_volume() - audio::headphone::volume_range().max).decibel() + 99);
    field_volume.on_change = [this](int32_t v) { this->on_headphone_volume_changed(v); };

    // LEARN FREQUENCIES
    std::string scanner_txt = "SCANNER";
    frequency_file_load(scanner_txt);
}

void ScannerView::frequency_file_load(std::string file_name, bool stop_all_before) {
    bool found_range{false};
    bool found_single{false};
    freqman_index_t def_mod_index{-1};
    freqman_index_t def_bw_index{-1};
    freqman_index_t def_step_index{-1};

    // stop everything running now if required
    if (stop_all_before) {
        scan_thread->stop();
        frequency_list.clear();  // clear the existing frequency list (expected behavior)
        description_list.clear();
    }

    if (load_freqman_file(file_name, database)) {
        loaded_file_name = file_name;                            // keep loaded filename in memory
        for (auto& entry : database) {                           // READ LINE PER LINE
            if (frequency_list.size() < FREQMAN_MAX_PER_FILE) {  // We got space!
                //
                // Get modulation & bw & step from file if specified
                // Note these values could be different for each line in the file, but we only look at the first one
                //
                // Note that freqman requires a very specific string for these parameters,
                // so check syntax in frequency file if specified value isn't being loaded
                //
                if (def_mod_index == -1)
                    def_mod_index = entry.modulation;

                if (def_bw_index == -1)
                    def_bw_index = entry.bandwidth;

                if (def_step_index == -1)
                    def_step_index = entry.step;

                // Get frequency
                if (entry.type == RANGE) {
                    if (!found_range) {
                        // Set Start & End Search Range instead of populating the small in-memory frequency table
                        // NOTE:  There may be multiple single frequencies in file, but only one search range is supported.
                        found_range = true;
                        frequency_range.min = entry.frequency_a;
                        button_manual_start.set_text(to_string_short_freq(frequency_range.min));
                        frequency_range.max = entry.frequency_b;
                        button_manual_end.set_text(to_string_short_freq(frequency_range.max));
                    }
                } else if (entry.type == SINGLE) {
                    found_single = true;
                    frequency_list.push_back(entry.frequency_a);
                    description_list.push_back(entry.description);
                } else if (entry.type == HAMRADIO) {
                    // For HAM repeaters, add both receive & transmit frequencies to scan list and modify description
                    // (FUTURE fw versions might handle these differently)
                    found_single = true;
                    frequency_list.push_back(entry.frequency_a);
                    description_list.push_back("R:" + entry.description);

                    if ((entry.frequency_a != entry.frequency_b) &&
                        (frequency_list.size() < FREQMAN_MAX_PER_FILE)) {
                        frequency_list.push_back(entry.frequency_b);
                        description_list.push_back("T:" + entry.description);
                    }
                }
            } else {
                break;  // No more space: Stop reading the txt file !
            }
        }
    } else {
        loaded_file_name = "SCANNER";  // back to the default frequency file
        desc_current_index.set(" NO " + file_name + ".TXT FILE ...");
    }

    desc_freq_list_scan = loaded_file_name + ".TXT";
    if (desc_freq_list_scan.length() > 23) {  // Truncate description and add ellipses if long file name
        desc_freq_list_scan.resize(20);
        desc_freq_list_scan = desc_freq_list_scan + "...";
    }

    if ((def_mod_index != -1) && (def_mod_index != (freqman_index_t)field_mode.selected_index_value()))
        field_mode.set_by_value(def_mod_index);  // Update mode (also triggers a change callback that disables & reenables RF background)

    if (def_bw_index != -1)  // Update BW if specified in file
        field_bw.set_selected_index(def_bw_index);

    if (def_step_index != -1)  // Update step if specified in file
        field_step.set_selected_index(def_step_index);

    audio::output::stop();

    if (userpause)  // If user-paused, resume
        user_resume();

    // Scan list if we found one, otherwise do manual range search
    manual_search = !found_single;

    start_scan_thread();
}

void ScannerView::update_squelch_while_paused(int32_t max_db) {
    // Update audio & color based on signal level even if paused
    if (++color_timer > 2) {  // Counter to reduce color toggling when weak signal
        if (max_db > squelch) {
            audio::output::start();                             // Re-enable audio when signal goes above squelch
            on_headphone_volume_changed(field_volume.value());  // quick fix to make sure WM8731S chips don't stay silent after pause
            bigdisplay_update(BDC_GREEN);
        } else {
            audio::output::stop();        // Silence audio when signal drops below squelch
            bigdisplay_update(BDC_GREY);  // Back to grey color
        }

        color_timer = 0;
    }
}

void ScannerView::on_statistics_update(const ChannelStatistics& statistics) {
    if (userpause) {
        update_squelch_while_paused(statistics.max_db);
    } else if (scan_thread)  // Scanning not user-paused
    {
        // Resume regardless of signal strength if browse time reached
        if ((browse_wait != 0) && (browse_timer >= (browse_wait * STATISTICS_UPDATES_PER_SEC))) {
            browse_timer = 0;
            scan_resume();  // Resume scanning
        } else {
            if (statistics.max_db > squelch) {                       // There is something on the air...(statistics.max_db > -squelch)
                if (scan_thread->is_freq_lock() >= MAX_FREQ_LOCK) {  // Pause scanning when signal checking time reached
                    if (!browse_timer)                               // Don't bother pausing if already paused
                        scan_pause();
                    browse_timer++;  // browse_timer!=0 is also an indication that we've paused the scan
                    update_squelch_while_paused(statistics.max_db);
                } else {
                    scan_thread->set_freq_lock(scan_thread->is_freq_lock() + 1);  // in lock period, still analyzing the signal
                    if (browse_timer)                                             // Continue incrementing browse timer while paused
                        browse_timer++;
                }
                lock_timer = 0;  // Keep resetting lock timer while signal remains
            } else {             // There is NOTHING on the air
                if (!browse_timer) {
                    // Signal lost and scan was never paused
                    if (scan_thread->is_freq_lock() > 0) {  // But are we already in freq_lock ?
                        bigdisplay_update(BDC_GREY);        // Back to grey color
                        scan_thread->set_freq_lock(0);      // Reset the scanner lock, since there is no sig
                    }
                } else {
                    // Signal lost and scan is still paused
                    lock_timer++;                                                  // Bump paused time
                    if (lock_timer >= (lock_wait * STATISTICS_UPDATES_PER_SEC)) {  // Stay on freq until lock_wait time elapses
                        browse_timer = 0;
                        scan_resume();
                    } else {
                        browse_timer++;  // Bump browse time too (may hit that limit before lock_timer reached)
                        update_squelch_while_paused(statistics.max_db);
                    }
                }
            }
        }
    }
}

void ScannerView::scan_pause() {
    if (scan_thread && scan_thread->is_scanning()) {
        scan_thread->set_freq_lock(0);     // Reset the scanner lock (because user paused, or MAX_FREQ_LOCK reached) for next freq scan
        scan_thread->set_scanning(false);  // WE STOP SCANNING
    }
    audio::output::start();
    on_headphone_volume_changed(field_volume.value());  // quick fix to make sure WM8731S chips don't stay silent after pause
}

void ScannerView::scan_resume() {
    audio::output::stop();
    bigdisplay_update(BDC_GREY);  // Back to grey color

    if (scan_thread) {
        scan_thread->set_index_stepper(fwd ? 1 : -1);
        scan_thread->set_scanning(true);  // RESUME!
    }
}

void ScannerView::user_resume() {
    browse_timer = browse_wait * STATISTICS_UPDATES_PER_SEC + 1;  // Will trigger a scan_resume() on_statistics_update, also advancing to next freq.
    button_pause.set_text("<PAUSE>");                             // Show button for pause, arrows indicate rotary encoder enabled for freq change
    userpause = false;                                            // Resume scanning
}

void ScannerView::on_headphone_volume_changed(int32_t v) {
    const auto new_volume = volume_t::decibel(v - 99) + audio::headphone::volume_range().max;
    receiver_model.set_headphone_volume(new_volume);
}

void ScannerView::change_mode(freqman_index_t new_mod) {  // Before this, do a scan_thread->stop();  After this do a start_scan_thread()
    using option_t = std::pair<std::string, int32_t>;
    using options_t = std::vector<option_t>;
    options_t bw;
    field_bw.on_change = [this](size_t n, OptionsField::value_t) {
        (void)n;  // avoid unused warning
    };

    switch (new_mod) {
        case AM_MODULATION:
            freqman_set_bandwidth_option(new_mod, field_bw);
            baseband::run_image(portapack::spi_flash::image_tag_am_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::AMAudio);
            field_bw.set_selected_index(0);
            receiver_model.set_am_configuration(field_bw.selected_index());
            field_bw.on_change = [this](size_t n, OptionsField::value_t) { receiver_model.set_am_configuration(n); };
            receiver_model.set_sampling_rate(3072000);
            receiver_model.set_baseband_bandwidth(1750000);
            break;
        case NFM_MODULATION:  // bw 16k (2) default
            freqman_set_bandwidth_option(new_mod, field_bw);
            baseband::run_image(portapack::spi_flash::image_tag_nfm_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
            field_bw.set_selected_index(2);
            receiver_model.set_nbfm_configuration(field_bw.selected_index());
            field_bw.on_change = [this](size_t n, OptionsField::value_t) { receiver_model.set_nbfm_configuration(n); };
            receiver_model.set_sampling_rate(3072000);
            receiver_model.set_baseband_bandwidth(1750000);
            break;
        case WFM_MODULATION:
            freqman_set_bandwidth_option(new_mod, field_bw);
            baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
            field_bw.set_selected_index(0);
            receiver_model.set_wfm_configuration(field_bw.selected_index());
            field_bw.on_change = [this](size_t n, OptionsField::value_t) { receiver_model.set_wfm_configuration(n); };
            receiver_model.set_sampling_rate(3072000);
            receiver_model.set_baseband_bandwidth(2000000);
            break;
        default:
            break;
    }
}

void ScannerView::start_scan_thread() {
    receiver_model.enable();
    receiver_model.set_squelch_level(0);
    show_max_index();

    // Start Scanner Thread
    // FUTURE: Consider passing additional control flags (fwd,userpause,etc) to scanner thread at start (perhaps in a data structure)
    if (manual_search) {
        button_manual_search.set_text("SCAN");  // Update meaning of Manual Scan button
        desc_current_index.set(desc_freq_range_search);
        scan_thread = std::make_unique<ScannerThread>(frequency_range, field_step.selected_index_value());
    } else {
        button_manual_search.set_text("SRCH");  // Update meaning of Manual Scan button
        desc_current_index.set(desc_freq_list_scan);
        scan_thread = std::make_unique<ScannerThread>(frequency_list);
    }

    scan_thread->set_scanning_direction(fwd);
}

} /* namespace ui */
