/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
 * Copyright (C) 2023 Mark Thompson
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

#include "optional.hpp"
#include "ui_fileman.hpp"
#include "ui_freqman.hpp"
#include "file_path.hpp"

using namespace portapack;
namespace fs = std::filesystem;

namespace ui {

ScannerThread::ScannerThread(std::vector<rf::Frequency> frequency_list)
    : frequency_list_{std::move(frequency_list)} {
    _manual_search = false;
    create_thread();
}

ScannerThread::ScannerThread(const scanner_range_t& frequency_range, size_t def_step_hz)
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
// (used when rotary encoder is turned)
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

                    receiver_model.set_target_frequency(frequency_list_[frequency_index]);  // Retune
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

                    receiver_model.set_target_frequency(frequency_range_.min + frequency_index * def_step_hz_);  // Retune
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
                big_display.set_style(Theme::getInstance()->fg_medium);
                break;
            case BDC_YELLOW:
                big_display.set_style(Theme::getInstance()->fg_yellow);
                break;
            case BDC_GREEN:
                big_display.set_style(Theme::getInstance()->fg_green);
                break;
            case BDC_RED:
                big_display.set_style(Theme::getInstance()->fg_red);
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
        if (entries.size() > 0)
            field_current_index.set_text(to_string_dec_uint(freq_idx + 1, 3));

        if (freq_idx < entries.size() && entries[freq_idx].description.size() > 1)
            text_current_desc.set(entries[freq_idx].description);  // Show description from file
        else
            text_current_desc.set(loaded_filename());  // Show Scan file name (no description in file)
    }
}

void ScannerView::handle_encoder(EncoderEvent delta) {
    auto index_step = delta > 0 ? 1 : -1;

    if (scan_thread)
        scan_thread->set_index_stepper(index_step);

    // Restart browse timer when frequency changes.
    if (browse_timer != 0)
        browse_timer = 1;
}

std::string ScannerView::loaded_filename() const {
    auto filename = freqman_file;
    if (filename.length() > 23) {  // Truncate long file name.
        filename.resize(22);
        filename = filename + "+";
    }

    return filename;
}

void ScannerView::focus() {
    button_load.focus();
}

ScannerView::~ScannerView() {
    // make sure to stop the thread before shutting down the receiver
    scan_thread.reset();
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

void ScannerView::show_max_index() {  // show total number of freqs to scan
    field_current_index.set_text("<->");

    if (entries.size() == FREQMAN_MAX_PER_FILE) {
        text_max_index.set_style(Theme::getInstance()->fg_red);
        text_max_index.set("/ " + to_string_dec_uint(FREQMAN_MAX_PER_FILE) + " (DB MAX!)");
    } else {
        text_max_index.set_style(Theme::getInstance()->fg_medium);
        text_max_index.set("/ " + to_string_dec_uint(entries.size()));
    }
}

ScannerView::ScannerView(
    NavigationView& nav)
    : nav_{nav} {
    add_children({
        &labels,
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
        &field_current_index,
        &text_max_index,
        &text_current_desc,
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
        &button_remove,
    });

    // Populate option text for these fields
    freqman_set_modulation_option(field_mode);
    freqman_set_step_option(field_step);

    // Default starting modulation (from saved App Settings if enabled, and may be overridden in SCANNER.TXT)
    field_mode.set_by_value((OptionsField::value_t)receiver_model.modulation());  // Reflect the mode into the manual selector
    field_step.set_by_value(receiver_model.frequency_step());                     // Default step interval (Hz)
    change_mode((freqman_index_t)field_mode.selected_index_value());

    button_manual_start.set_text(to_string_short_freq(frequency_range.min));
    button_manual_end.set_text(to_string_short_freq(frequency_range.max));

    // Button to load a Freqman file.
    button_load.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".TXT");
        open_view->push_dir(freqman_dir);
        open_view->on_changed = [this, &nav](std::filesystem::path new_file_path) {
            if (new_file_path.native().find((u"/" / freqman_dir).native()) == 0) {
                scan_pause();
                frequency_file_load(new_file_path);
            } else {
                nav.display_modal("LOAD ERROR", "A valid file from\nFREQMAN directory is\nrequired.");
            }
        };
    };

    // Button to clear in-memory frequency list.
    button_clear.on_select = [this, &nav](Button&) {
        if (scan_thread && entries.size()) {
            scan_thread->stop();  // STOP SCANNER THREAD
            entries.clear();

            show_max_index();  // UPDATE new list size on screen
            field_current_index.set_text("");
            text_current_desc.set(loaded_filename());
            scan_thread->set_freq_lock(0);  // Reset the scanner lock

            // FUTURE: Consider switching to manual search mode automatically after clear (but would need to validate freq range)
        }
    };

    // Button to configure starting frequency for a manual range search.
    button_manual_start.on_select = [this, &nav](Button& button) {
        auto new_view = nav_.push<FrequencyKeypadView>(frequency_range.min);
        new_view->on_changed = [this, &button](rf::Frequency f) {
            frequency_range.min = f;
            button_manual_start.set_text(to_string_short_freq(f));
        };
    };

    // Button to configure ending frequency for a manual range search.
    button_manual_end.on_select = [this, &nav](Button& button) {
        auto new_view = nav.push<FrequencyKeypadView>(frequency_range.max);
        new_view->on_changed = [this, &button](rf::Frequency f) {
            frequency_range.max = f;
            button_manual_end.set_text(to_string_short_freq(f));
        };
    };

    // Button to pause/resume scan (note that some other buttons will trigger resume also).
    button_pause.on_select = [this](ButtonWithEncoder&) {
        if (userpause)
            user_resume();
        else {
            scan_pause();
            button_pause.set_text("<RESUME>");  // PAUSED, show resume
            userpause = true;
        }
    };

    // Encoder dial causes frequency change when focus is on pause button or current index.
    button_pause.on_change = [this]() {
        handle_encoder(button_pause.get_encoder_delta());
        button_pause.set_encoder_delta(0);
    };
    field_current_index.on_encoder_change = [this](TextField&, EncoderEvent delta) {
        handle_encoder(delta);
    };

    // Button to switch to Audio app
    button_audio_app.on_select = [this](Button&) {
        if (scan_thread)
            scan_thread->stop();
        auto settings = receiver_model.settings();
        settings.frequency_step = field_step.selected_index_value();
        nav_.replace<AnalogAudioView>(settings);
    };

    // Button to switch to Mic app
    button_mic_app.on_select = [this](Button&) {
        if (scan_thread)
            scan_thread->stop();
        // MicTX wants Frequency, Modulation and Bandwidth overrides, but that's only stored on the RX model.
        nav_.replace<MicTXView>(receiver_model.settings());
    };

    // Button to delete current frequency from scan Freq List
    button_remove.on_select = [this](Button&) {
        if (scan_thread && (entries.size() > current_index)) {
            scan_thread->set_scanning(false);  // PAUSE Scanning if necessary

            // Remove frequency from the Freq List in memory (it is not removed from the file).
            scan_thread->set_freq_del(entries[current_index].freq);
            entries.erase(entries.begin() + current_index);

            show_max_index();               // UPDATE new list size on screen
            text_current_desc.set("");      // Clean up description (cosmetic detail)
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

        restart_scan();
    };

    // Mode field was changed (AM/NFM/WFM)
    field_mode.on_change = [this](size_t, OptionsField::value_t v) {
        static freqman_index_t last_mode = AM_MODULATION;
        // unsupported SPEC mode fix
        if (v == SPEC_MODULATION) {
            if (last_mode == AM_MODULATION)
                v = WFM_MODULATION;
            else
                v = AM_MODULATION;
            field_mode.set_selected_index(v);
        }
        last_mode = v;
        receiver_model.disable();
        baseband::shutdown();
        change_mode((freqman_index_t)v);
        if (scan_thread && !scan_thread->is_scanning())  // for some motive, audio output gets stopped.
            audio::output::start();                      // So if scan was stopped we resume audio
        receiver_model.enable();
    };

    // Step field was changed (Hz) -- only affects manual Search mode
    field_step.on_change = [this](size_t, OptionsField::value_t v) {
        receiver_model.set_frequency_step(v);

        if (manual_search && scan_thread)
            restart_scan();
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
        FreqmanDB db;
        if (db.open(get_freqman_path(freqman_file), /*create*/ true)) {
            freqman_entry entry{
                .frequency_a = current_frequency,
                .type = freqman_type::Single,
            };

            // Look for existing entry with same frequency.
            auto it = db.find_entry([&entry](const auto& e) {
                return e.frequency_a == entry.frequency_a;
            });
            auto found = (it != db.end());

            if (found) {
                nav_.display_modal("Error", "Frequency already exists");
                bigdisplay_update(-1);  // Need to poke this control after displaying modal?
            } else {
                db.append_entry(entry);
                // Add to frequency_list in memory too, since we can now switch back from manual mode
                // Note that we are allowing freqs to be added to file (code above) that exceed the
                // max count we can load into memory.
                if (entries.size() < FREQMAN_MAX_PER_FILE) {
                    entries.push_back({current_frequency, ""});
                    show_max_index();  // Display updated frequency list size
                }
            }
        } else {
            nav_.display_modal("Error", "Cannot open " + freqman_file + ".TXT\nfor appending freq.");
            bigdisplay_update(-1);  // Need to poke this control after displaying modal?
        }
    };

    // PRE-CONFIGURATION:
    field_browse_wait.on_change = [this](int32_t v) { browse_wait = v; };
    field_browse_wait.set_value(browse_wait);

    field_lock_wait.on_change = [this](int32_t v) { lock_wait = v; };
    field_lock_wait.set_value(lock_wait);

    field_squelch.on_change = [this](int32_t v) { squelch = v; };
    field_squelch.set_value(squelch);

    // Disable squelch on the model because RSSI handler is where the
    // actual squelching is applied for this app.
    receiver_model.set_squelch_level(0);

    // LOAD FREQUENCIES
    frequency_file_load(get_freqman_path(freqman_file));
}

void ScannerView::frequency_file_load(const fs::path& path) {
    freqman_index_t def_mod_index{freqman_invalid_index};
    freqman_index_t def_bw_index{freqman_invalid_index};
    freqman_index_t def_step_index{freqman_invalid_index};

    FreqmanDB db;
    if (!db.open(path)) {
        text_current_desc.set("NO " + path.filename().string());
        return;
    }

    entries.clear();
    freqman_file = path.stem().string();
    Optional<scanner_range_t> range;

    for (auto entry : db) {
        if (is_invalid(def_mod_index))
            def_mod_index = entry.modulation;

        if (is_invalid(def_bw_index))
            def_bw_index = entry.bandwidth;

        if (is_invalid(def_step_index))
            def_step_index = entry.step;

        switch (entry.type) {
            case freqman_type::Repeater:
            case freqman_type::Single:
                entries.push_back({entry.frequency_a, entry.description});
                break;
            case freqman_type::HamRadio:
                entries.push_back({entry.frequency_a, "R: " + entry.description});
                entries.push_back({entry.frequency_b, "T: " + entry.description});
                break;
            case freqman_type::Range:
                // NB: Only the first range will be loaded.
                if (!range)
                    range = {entry.frequency_a, entry.frequency_b};
                break;
            default:
                break;
        }

        if (entries.size() >= FREQMAN_MAX_PER_FILE)
            break;
    }

    if (is_valid(def_mod_index) && def_mod_index != (freqman_index_t)field_mode.selected_index_value())
        field_mode.set_by_value(def_mod_index);

    if (is_valid(def_bw_index))
        field_bw.set_selected_index(def_bw_index);

    if (is_valid(def_step_index))
        field_step.set_selected_index(def_step_index);

    // Found range, set it and update UI.
    if (range) {
        frequency_range = *range;
        button_manual_start.set_text(to_string_short_freq(frequency_range.min));
        button_manual_end.set_text(to_string_short_freq(frequency_range.max));
    }

    // Scan entries if any, otherwise do manual range search.
    manual_search = entries.empty();
    restart_scan();
}

void ScannerView::update_squelch_while_paused(int32_t max_db) {
    // Update audio & color based on signal level even if paused
    if (++color_timer > 2) {  // Counter to reduce color toggling when weak signal
        if (max_db > squelch) {
            audio::output::start();                                                  // Re-enable audio when signal goes above squelch
            receiver_model.set_headphone_volume(receiver_model.headphone_volume());  // quick fix to make sure WM8731S chips don't stay silent after pause
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
    receiver_model.set_headphone_volume(receiver_model.headphone_volume());  // quick fix to make sure WM8731S chips don't stay silent after pause
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

// Before this, do a scan_thread->stop();  After this do a start_scan_thread()
void ScannerView::change_mode(freqman_index_t new_mod) {
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
            field_bw.set_by_value(receiver_model.am_configuration());
            field_bw.on_change = [this](size_t, OptionsField::value_t n) { receiver_model.set_am_configuration(n); };
            break;
        case NFM_MODULATION:
            freqman_set_bandwidth_option(new_mod, field_bw);
            baseband::run_image(portapack::spi_flash::image_tag_nfm_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
            field_bw.set_by_value(receiver_model.nbfm_configuration());
            field_bw.on_change = [this](size_t, OptionsField::value_t n) { receiver_model.set_nbfm_configuration(n); };
            break;
        case WFM_MODULATION:
            freqman_set_bandwidth_option(new_mod, field_bw);
            baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);
            receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
            field_bw.set_by_value(receiver_model.wfm_configuration());
            field_bw.on_change = [this](size_t, OptionsField::value_t n) { receiver_model.set_wfm_configuration(n); };
            break;
        default:
            break;
    }
}

void ScannerView::start_scan_thread() {
    receiver_model.enable();
    show_max_index();

    // Start Scanner Thread
    if (manual_search) {
        button_manual_search.set_text("SCAN");  // Update meaning of Manual Scan button
        text_current_desc.set("SEARCHING...");
        scan_thread = std::make_unique<ScannerThread>(frequency_range, field_step.selected_index_value());
    } else {
        button_manual_search.set_text("SRCH");  // Update meaning of Manual Scan button
        text_current_desc.set(loaded_filename());

        // TODO: just pass ref to the thread?
        std::vector<rf::Frequency> frequency_list;
        frequency_list.reserve(entries.size());
        for (const auto& entry : entries)
            frequency_list.push_back(entry.freq);

        scan_thread = std::make_unique<ScannerThread>(std::move(frequency_list));
    }

    scan_thread->set_scanning_direction(fwd);
}

void ScannerView::restart_scan() {
    audio::output::stop();
    if (scan_thread)  // STOP SCANNER THREAD
        scan_thread->stop();

    if (userpause)  // If user-paused, resume
        user_resume();

    start_scan_thread();  // RESTART SCANNER THREAD in selected mode
}

} /* namespace ui */
