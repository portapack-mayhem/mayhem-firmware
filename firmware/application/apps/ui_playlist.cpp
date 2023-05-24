/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyleft  (ↄ) 2022 NotPike
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

#include "ui_playlist.hpp"
#include "string_format.hpp"

#include "ui_fileman.hpp"
#include "io_file.hpp"

#include "baseband_api.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
#include <unistd.h>
#include <fstream>

using namespace portapack;

namespace ui {

void PlaylistView::set_ready() {
    ready_signal = true;
}

void PlaylistView::load_file(std::filesystem::path playlist_path) {
    File playlist_file;

    auto error = playlist_file.open(playlist_path.string());
    if (!error.is_valid()) {
        std::string line;
        char one_char[1];
        for (size_t pointer = 0; pointer < playlist_file.size(); pointer++) {
            playlist_file.seek(pointer);
            playlist_file.read(one_char, 1);
            if ((int)one_char[0] >= ' ') {
                line += one_char[0];
            } else if (one_char[0] == '\n') {
                total_tracks++;
                txtline_process(line);
                line.clear();
            }
        }
        if (line.length() > 0) {
            total_tracks++;
            txtline_process(line);
        }
    }
    playlist_masterdb = playlist_db;
    return;
}

void PlaylistView::txtline_process(std::string& line) {
    playlist_entry new_item;
    rf::Frequency f;
    size_t previous = 0;
    size_t current = line.find(',');
    std::string freqs = line.substr(0, current);
    previous = current + 1;
    current = line.find(',', previous);
    std::string file = line.substr(previous, current - previous);
    previous = current + 1;
    current = line.find(',', previous);
    uint32_t sample_rate = strtoll(line.substr(previous).c_str(), nullptr, 10);

    f = strtoll(freqs.c_str(), nullptr, 0);
    new_item.replay_frequency = f;
    new_item.replay_file = "/" + file;
    new_item.sample_rate = sample_rate;
    new_item.next_delay = 0;

    playlist_db.push_back(std::move(new_item));
}

void PlaylistView::on_file_changed(std::filesystem::path new_file_path, rf::Frequency replay_frequency, uint32_t replay_sample_rate) {
    File data_file;
    // Get file size
    auto data_open_error = data_file.open("/" + new_file_path.string());
    if (!data_open_error.is_valid()) {
        track_number++;
    } else if (data_open_error.is_valid()) {
        file_error("C16 file\n" + new_file_path.string() + "\nread error.");
        return;
    }

    file_path = new_file_path;
    field_frequency.set_value(replay_frequency);

    sample_rate = replay_sample_rate;

    text_sample_rate.set(unit_auto_scale(sample_rate, 3, 0) + "Hz");

    auto file_size = data_file.size();
    auto duration = (file_size * 1000) / (2 * 2 * sample_rate);

    progressbar.set_max(file_size);
    text_filename.set(file_path.filename().string().substr(0, 12));
    text_duration.set(to_string_time_ms(duration));
    // text_track.set(std::to_string(track_number) + "/" + std::to_string(total_tracks));

    button_play.focus();
}

void PlaylistView::on_tx_progress(const uint32_t progress) {
    progressbar.set_value(progress);
}

void PlaylistView::focus() {
    button_open.focus();
}

void PlaylistView::file_error(std::string error_message) {
    nav_.display_modal("Error", "Error for \n" + file_path.string() + "\n" + error_message);
}

bool PlaylistView::is_active() const {
    return (bool)replay_thread;
}

bool PlaylistView::loop() const {
    return (bool)playlist_db.size();
}

void PlaylistView::toggle() {
    if (is_active()) {
        stop(false);
        total_tracks = 0;
        track_number = 0;
        playlist_db.clear();
        playlist_masterdb.clear();
    } else {
        total_tracks = 0;
        track_number = 0;
        playlist_db.clear();
        playlist_masterdb.clear();
        load_file(now_play_list_file);
        start();
    }
}

void PlaylistView::start() {
    stop(false);

    playlist_entry item = playlist_db.front();
    playlist_db.pop_front();
    //	playlist_entry item = playlist_db[0];
    //	for (playlist_entry item : playlist_db) {
    //	file_path = item.replay_file;
    //	rf::Frequency replay_frequency = strtoll(item.replay_frequency.c_str(),nullptr,10);
    on_file_changed(item.replay_file, item.replay_frequency, item.sample_rate);
    on_target_frequency_changed(item.replay_frequency);

    std::unique_ptr<stream::Reader> reader;

    auto p = std::make_unique<FileReader>();
    auto open_error = p->open(file_path);
    if (open_error.is_valid()) {
        file_error("Illegal grammar");
        return;  // Fixes TX bug if there's a file error
    } else {
        reader = std::move(p);
    }

    if (reader) {
        button_play.set_bitmap(&bitmap_stop);
        baseband::set_sample_rate(sample_rate * 8);

        replay_thread = std::make_unique<ReplayThread>(
            std::move(reader),
            read_size, buffer_count,
            &ready_signal,
            [](uint32_t return_code) {
                ReplayThreadDoneMessage message{return_code};
                EventDispatcher::send_message(message);
            });
    }

    transmitter_model.set_sampling_rate(sample_rate * 8);
    transmitter_model.set_baseband_bandwidth(baseband_bandwidth);
    transmitter_model.enable();
}

void PlaylistView::stop(const bool do_loop) {
    if (is_active()) {
        replay_thread.reset();
    }

    // TODO: the logic here could be more beautiful but maybe they are all same for compiler anyway....
    // Notes of the logic here in case if it needed to be changed in the future:
    // 1. check_loop.value() is for the LOOP checkbox
    // 2. do_loop is a part of the replay thread, not a user - control thing.
    // 3. when (total_tracks >= track_number) is true, it means that the current track is not the last track.
    // Thus, (do_loop && (total_tracks != track_number)) is for the case when the start() func were called with true AND not the last track.
    // Which means it do loop until the last track.

    if (check_loop.value()) {
        if (do_loop) {
            if (playlist_db.size() > 0) {
                start();
            } else {
                playlist_db = playlist_masterdb;
                start();
            }
        } else {
            transmitter_model.disable();
            button_play.set_bitmap(&bitmap_play);
        }
    } else if (!check_loop.value()) {
        if (do_loop && (total_tracks >= track_number)) {
            if (playlist_db.size() > 0) {
                start();
            } else {
                playlist_db = playlist_masterdb;
                start();
            }
        } else {
            transmitter_model.disable();
            button_play.set_bitmap(&bitmap_play);
        }
    }

    ready_signal = false;
}

void PlaylistView::handle_replay_thread_done(const uint32_t return_code) {
    if (return_code == ReplayThread::END_OF_FILE) {
        stop(true);
    } else if (return_code == ReplayThread::READ_ERROR) {
        stop(false);
        file_error("Replay thread read error");
    }

    progressbar.set_value(0);
}

PlaylistView::PlaylistView(
    NavigationView& nav)
    : nav_(nav) {
    baseband::run_image(portapack::spi_flash::image_tag_replay);

    add_children({
        &button_open,
        &text_filename,
        &text_sample_rate,
        &text_duration,
        &progressbar,
        &field_frequency,
        &tx_view,  // this handles now the previous rfgain, rfamp
        &check_loop,
        &button_play,
        // &text_track,
        // TODO: add track number
        &waterfall,
    });

    field_frequency.set_value(target_frequency());
    field_frequency.set_step(receiver_model.frequency_step());
    field_frequency.on_change = [this](rf::Frequency f) {
        this->on_target_frequency_changed(f);
    };
    field_frequency.on_edit = [this, &nav]() {
        // TODO: Provide separate modal method/scheme?
        auto new_view = nav.push<FrequencyKeypadView>(this->target_frequency());
        new_view->on_changed = [this](rf::Frequency f) {
            this->on_target_frequency_changed(f);
            this->field_frequency.set_value(f);
        };
    };

    field_frequency.set_step(5000);

    button_play.on_select = [this](ImageButton&) {
        this->toggle();
    };

    button_open.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".PPL");
        open_view->on_changed = [this](std::filesystem::path new_file_path) {
            now_play_list_file = new_file_path;
            load_file(new_file_path);
        };
    };
}

PlaylistView::~PlaylistView() {
    transmitter_model.disable();
    baseband::shutdown();
}

void PlaylistView::on_hide() {
    stop(false);
    // TODO: Terrible kludge because widget system doesn't notify Waterfall that
    // it's being shown or hidden.
    waterfall.on_hide();
    View::on_hide();
}

void PlaylistView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);

    const ui::Rect waterfall_rect{0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height};
    waterfall.set_parent_rect(waterfall_rect);
}

void PlaylistView::on_target_frequency_changed(rf::Frequency f) {
    set_target_frequency(f);
}

void PlaylistView::set_target_frequency(const rf::Frequency new_value) {
    persistent_memory::set_tuned_frequency(new_value);
    ;
}

rf::Frequency PlaylistView::target_frequency() const {
    return persistent_memory::tuned_frequency();
}

} /* namespace ui */
