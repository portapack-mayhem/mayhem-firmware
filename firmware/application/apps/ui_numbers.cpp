/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "ui_numbers.hpp"
#include "string_format.hpp"

#include "portapack.hpp"
#include "hackrf_hal.hpp"
#include "portapack_shared_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui {

// TODO: This app takes way too much space, find a way to shrink/simplify or make it an SD card module (loadable)

void NumbersStationView::focus() {
    if (file_error)
        nav_.display_modal("No voices", "No valid voices found in\nthe /numbers directory.", ABORT);
    else
        button_exit.focus();
}

NumbersStationView::~NumbersStationView() {
    transmitter_model.disable();
    baseband::shutdown();
}

NumbersStationView::wav_file_t* NumbersStationView::get_wav(uint32_t index) {
    return &current_voice->available_wavs[index];
}

void NumbersStationView::prepare_audio() {
    uint8_t code;
    wav_file_t* wav_file;

    if (sample_counter >= sample_length) {
        if (segment == ANNOUNCE) {
            if (!announce_loop) {
                code_index = 0;
                segment = MESSAGE;
            } else {
                wav_file = get_wav(11);
                reader->open(current_voice->dir + file_names[wav_file->index].name + ".wav");
                sample_length = wav_file->length;
                announce_loop--;
            }
        }

        if (segment == MESSAGE) {
            if (code_index == 25) {
                transmitter_model.disable();
                return;
            }

            code = symfield_code.get_offset(code_index);

            if (code >= 10) {
                memset(audio_buffer, 0, 1024);
                if (code == 10) {
                    pause = 11025;  // p: 0.25s @ 44100Hz
                } else if (code == 11) {
                    pause = 33075;  // P: 0.75s @ 44100Hz
                } else if (code == 12) {
                    transmitter_model.disable();
                    return;
                }
            } else {
                wav_file = get_wav(code);
                reader->open(current_voice->dir + file_names[code].name + ".wav");
                sample_length = wav_file->length;
            }
            code_index++;
        }
        sample_counter = 0;
    }

    if (!pause) {
        auto bytes_read = reader->read(audio_buffer, 1024).value();

        // Unsigned to signed, pretty stupid :/
        for (size_t n = 0; n < bytes_read; n++)
            audio_buffer[n] -= 0x80;
        for (size_t n = bytes_read; n < 1024; n++)
            audio_buffer[n] = 0;

        sample_counter += 1024;
    } else {
        if (pause >= 1024) {
            pause -= 1024;
        } else {
            sample_counter = sample_length;
            pause = 0;
        }
    }

    baseband::set_fifo_data(audio_buffer);
}

void NumbersStationView::start_tx() {
    // sample_length = sound_sizes[10];		// Announce
    sample_counter = sample_length;

    code_index = 0;
    announce_loop = 2;
    segment = ANNOUNCE;

    prepare_audio();

    transmitter_model.set_rf_amp(true);
    transmitter_model.enable();

    baseband::set_audiotx_data(
        (1536000 / 44100) - 1,  // TODO: Read wav file's samplerate
        12000,
        1,
        false,
        0);
}

void NumbersStationView::on_tick_second() {
    armed_blink = not armed_blink;

    if (armed_blink)
        check_armed.set_style(Theme::getInstance()->fg_red);
    else
        check_armed.set_style(&style());

    check_armed.set_dirty();
}

void NumbersStationView::on_voice_changed(size_t index) {
    std::string code_list;

    for (const auto& wavs : voices[index].available_wavs)
        code_list += wavs.code;

    symfield_code.set_symbol_list(code_list);
    current_voice = &voices[index];
}

bool NumbersStationView::check_wav_validity(const std::string dir, const std::string file) {
    if (reader->open("/numbers/" + dir + "/" + file)) {
        // Check format (mono, 8 bits)
        if ((reader->channels() == 1) && (reader->bits_per_sample() == 8))
            return true;
        else
            return false;
    } else
        return false;
}

NumbersStationView::NumbersStationView(
    NavigationView& nav)
    : nav_(nav) {
    std::vector<std::filesystem::path> directory_list;
    using option_t = std::pair<std::string, int32_t>;
    using options_t = std::vector<option_t>;
    options_t voice_options;
    voice_t temp_voice{};
    bool valid;
    uint32_t c;
    // uint8_t y, m, d, dayofweek;

    reader = std::make_unique<WAVFileReader>();

    // Search for valid voice directories
    directory_list = scan_root_directories("/numbers");
    if (!directory_list.size()) {
        file_error = true;
        return;
    }

    for (const auto& dir : directory_list) {
        c = 0;
        for (const auto& file_name : file_names) {
            valid = check_wav_validity(dir.string(), file_name.name + ".wav");
            if ((!valid) && (file_name.required)) {
                temp_voice.available_wavs.clear();
                break;  // Invalid required file means invalid voice
            } else if (valid) {
                temp_voice.available_wavs.push_back({file_name.code, c++, 0, 0});  // TODO: Needs length and samplerate
            }
        }
        if (!temp_voice.available_wavs.empty()) {
            // Voice can be used, are there accent files ?
            c = 0;
            for (const auto& file_name : file_names) {
                valid = check_wav_validity(dir.string(), file_name.name + "a.wav");
                if ((!valid) && (file_name.required)) {
                    c = 0;
                    break;  // Invalid required file means accents can't be used
                } else if (valid) {
                    c++;
                }
            }

            temp_voice.accent = c ? true : false;
            temp_voice.dir = dir.string();

            voices.push_back(temp_voice);
        }
    }

    if (voices.empty()) {
        file_error = true;
        return;
    }

    baseband::run_image(portapack::spi_flash::image_tag_audio_tx);

    add_children({&labels,
                  &symfield_code,
                  &check_armed,
                  &options_voices,
                  &text_voice_flags,
                  //&button_tx_now,
                  &button_exit});

    for (const auto& voice : voices)
        voice_options.emplace_back(voice.dir.substr(0, 4), 0);

    options_voices.set_options(voice_options);
    options_voices.on_change = [this](size_t i, int32_t) {
        this->on_voice_changed(i);
    };
    options_voices.set_selected_index(0);

    check_armed.set_value(false);

    check_armed.on_select = [this](Checkbox&, bool v) {
        if (v) {
            armed_blink = false;
            signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
                this->on_tick_second();
            };
        } else {
            check_armed.set_style(&style());
            rtc_time::signal_tick_second -= signal_token_tick_second;
        }
    };

    // DEBUG
    symfield_code.set_offset(0, 10);
    symfield_code.set_offset(1, 3);
    symfield_code.set_offset(2, 4);
    symfield_code.set_offset(3, 11);
    symfield_code.set_offset(4, 6);
    symfield_code.set_offset(5, 1);
    symfield_code.set_offset(6, 9);
    symfield_code.set_offset(7, 7);
    symfield_code.set_offset(8, 8);
    symfield_code.set_offset(9, 0);
    symfield_code.set_offset(10, 12);  // End

    /*
        dayofweek = rtc_time::current_day_of_week();
        text_title.set(day_of_week[dayofweek]);
    */

    button_exit.on_select = [&nav](Button&) {
        nav.pop();
    };
}

} /* namespace ui */
