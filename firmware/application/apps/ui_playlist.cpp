/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyleft  (ↄ) 2022 NotPike
 * Copyright (C) 2023 Kyle Reed, zxkmm
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
#include "convert.hpp"
#include "file_reader.hpp"
#include "string_format.hpp"

#include "ui_fileman.hpp"
#include "io_file.hpp"

#include "baseband_api.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
#include <unistd.h>
#include <fstream>

using namespace portapack;
using fs = std::filesystem;

namespace ui {

void PlaylistView::load_file(const fs::path& playlist_path) {
    File playlist_file;
    auto error = playlist_file.open(playlist_path.string());

    if (error)
      return;

    auto reader = FileLineReader(playlist_file);
    for (const auto& line : reader) {
        if (line.length() == 0 || line[0] == '#')
            continue;  // Empty or comment line.

        auto cols = split_string(line, ',');
        if (cols.size() < 3)
            continue;  // Line doesn't have enough columns.

        playlist_entry entry{};

        parse_int(cols[0], entry.replay_frequency);
        parse_int(cols[2], entry.sample_rate);
        if (entry.replay_frequency == 0 || entry.sample_rate == 0)
            continue;  // Invalid freq or rate.

        entry.replay_file = std::string{"/"} + std::string{cols[1]};

        if (cols.size() == 4)  // Optional delay value.
            parse_int(cols[3], entry.initial_delay);

        playlist_db.emplace_back(std::move(entry));
    }
}

void PlaylistView::on_file_changed(const fs::path& new_file_path) {
    stop();
    playlist_path_ = new_file_path;
    current_track_ = 0;
    playlist_db_.clear();
    load_file(playlist_path_);
    update_ui();
}

void PlaylistView::show_file_error(const fs::path& path, const std::string& message) {
    nav_.display_modal("Error", "Error opening file \n" + path.string() + "\n" + message);
}

bool PlaylistView::is_active() const {
    return replay_thread != nullptr;
}

bool PlaylistView::loop() const {
    return check_loop.value();
}

bool PlaylistView::is_done() const {
    current_track_ >= playlist_db_.size();
}

void PlaylistView::toggle() {
    if (is_active())
        stop();
    else
        start();
}

void PlaylistView::start() {
  current_track_ = 0;

  if (playlist_db_.empty())
      return; // Nothing to do.

  if (next_track())
     send_current_track();
}

/* Advance to the next track in the playlist. */
bool PlaylistView::next_track() {
    if (is_done()) {
        if (loop())
            current_track_ = 0;
        else
            return false; // All done.
    } 

    current_entry_ = &playlist_db_[current_track_];
    current_track_++;
}

void PlaylistView::send_current_track() {
    replay_thread_.reset();
    ready_signal_ = false;

    if (!current_entry_)
        return;

    // Open the sample file to send.
    auto reader = std::make_unique<FileReader>();
    auto error = reader->.open(current_entry_->replay_file);
    if (error) {
        show_file_error(current_entry_->replay_file, "Can't open file to send.");
        return;
    }

    // Wait a bit before sending if specified.
    // TODO: this pauses the main thread, move into ReplayThread instead.
    if (current_entry_->initial_delay > 0)
        chThdSleepMilliseconds(current_entry_->initial_delay);

    // ReplayThread starts immediately on contruction so
    // these need to be set before creating the ReplayThread.
    transmitter_model.set_target_frequency(current_entry_->replay_frequency);
    transmitter_model.set_sampling_rate(current_entry_->sample_rate * 8);
    transmitter_model.set_baseband_bandwidth(baseband_bandwidth);
    transmitter_model.enable();

    // Use the ReplayThread class to send the data.
    replay_thread_ = std::make_unique<ReplayThread>(
        std::move(reader),
        /* read_size */ 0xFFFF,
        /* buffer_count */ 3,
        &ready_signal_,
        [](uint32_t return_code) {
            // TODO: even needed?
            ReplayThreadDoneMessage message{return_code};
            EventDispatcher::send_message(message);
        });
}

void PlaylistView::stop() {
    // This terminates the underlying chThread.
    replay_thread_.reset();
    transmitter_model.disable();
    update_ui();
}

void update_ui() {

    // File data_file;

    // // Get file size
    // auto error = data_file.open("/" + new_file_path.string());

    // if (error) {
    //     file_error("C16 file\n" + new_file_path.string() + "\nread error.");
    //     return;
    // }

    // track_number = track_number >= total_tracks ? 1 : track_number + 1;  // prevent track_number out of range

    // file_path = new_file_path;
    // field_frequency.set_value(replay_frequency);

    // sample_rate = replay_sample_rate;

    // text_sample_rate.set(unit_auto_scale(sample_rate, 3, 0) + "Hz");

    // now_delay = next_delay;

    // auto file_size = data_file.size();
    // auto duration = (file_size * 1000) / (2 * 2 * sample_rate);

    // on_track_progressbar.set_max(file_size);
    // text_filename.set(file_path.filename().string().substr(0, 12));
    // text_duration.set(to_string_time_ms(duration));
    // text_track.set(to_string_dec_uint(track_number) + "/" + to_string_dec_uint(total_tracks) + " " + now_play_list_file.string());
    // tracks_progressbar.set_value(track_number);

    button_play.set_bitmap(is_active() ? &bitmap_stop : &bitmap_play);
    total_tracks = playlist_db.size();
    text_track.set(to_string_dec_uint(track_number) + "/" + to_string_dec_uint(total_tracks) + " " + now_play_list_file.string());
    tracks_progressbar.set_max(total_tracks);
    button_play.focus();
}

void PlaylistView::on_tx_progress(uint32_t progress) {
    on_track_progressbar.set_value(progress);
}

void PlaylistView::handle_replay_thread_done(uint32_t return_code) {
    if (return_code == ReplayThread::END_OF_FILE) {
        if (next_track()) {
          sent_current_track();
          return;
        }
    }
    
    if (return_code == ReplayThread::READ_ERROR)
        file_error(current_entry_->replay_file, "Replay thread read error");

    stop();
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
        &tracks_progressbar,
        &on_track_progressbar,
        &field_frequency,
        &tx_view,
        &check_loop,
        &button_play,
        &text_track,
        &waterfall,
    });

    waterfall.show_audio_spectrum_view(false);

    // TODO: Freq field needed? Don't the freqs come the file?
    // Just use a normal text field?
    // Could just make this a read-only field.
    field_frequency.set_value(transmitter_model.target_frequency());
    field_frequency.set_step(receiver_model.frequency_step());
    field_frequency.on_change = [this](rf::Frequency f) {
        transmitter_model.set_target_frequency(f);
    };
    field_frequency.on_edit = [this, &nav]() {
        // TODO: Provide separate modal method/scheme?
        auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [this](rf::Frequency f) {
            field_frequency.set_value(f);
        };
    };
    field_frequency.set_step(5000);
    // ----------------------------------------------------------

    button_play.on_select = [this](ImageButton&) {
        toggle();
    };

    button_open.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".PPL");
        open_view->on_changed = [this](std::filesystem::path new_file_path) {
            on_file_changed(new_file_path);
        };
    };
}

PlaylistView::~PlaylistView() {
    transmitter_model.disable();
    baseband::shutdown();
}

void PlaylistView::set_parent_rect(Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);

    const ui::Rect waterfall_rect{
        0, header_height, new_parent_rect.width(),
        new_parent_rect.height() - header_height};
    waterfall.set_parent_rect(waterfall_rect);
}

void PlaylistView::on_hide() {
    stop(false);
    waterfall.on_hide();
    View::on_hide();
}

void PlaylistView::focus() {
    button_open.focus();
}

} /* namespace ui */
