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
#include "io_file.hpp"
#include "string_format.hpp"
#include "ui_fileman.hpp"
#include "utility.hpp"

#include "baseband_api.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
#include <unistd.h>
#include <fstream>

using namespace portapack;
namespace fs = std::filesystem;

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

        playlist_entry entry{};
        auto cols = split_string(line, ',');

        entry.capture_path = trim(cols[0]);

        // Read metafile if it exists.
        auto metadata_path = get_metadata_path(entry.capture_path);
        auto metadata = read_metadata_file(metadata_path);

        if (metadata)
            entry.metadata = *metadata;
        else {
            continue;
            // TODO:
            // For now, require a metadata file. Eventually, allow
            // a user-defined center_freq like there was in Replay.
            // entry.metadata = {0, 500'000};
        }

        // Get optional delay value.
        if (cols.size() > 1)
            parse_int(cols[1], entry.ms_delay);

        playlist_db_.emplace_back(std::move(entry));
    }
}

void PlaylistView::on_file_changed(const fs::path& new_file_path) {
    stop();
    reset();

    playlist_path_ = new_file_path;
    playlist_db_.clear();
    load_file(playlist_path_);

    update_ui();
    button_play.focus();
}

void PlaylistView::reset() {
    current_track_ = 0;
    current_entry_ = nullptr;
    current_entry_size_ = 0;
}

void PlaylistView::show_file_error(const fs::path& path, const std::string& message) {
    nav_.display_modal("Error", "Error opening file \n" + path.string() + "\n" + message);
}

bool PlaylistView::is_active() const {
    return replay_thread_ != nullptr;
}

bool PlaylistView::loop() const {
    return check_loop.value();
}

bool PlaylistView::is_done() const {
    return current_track_ >= playlist_db_.size();
}

void PlaylistView::toggle() {
    if (is_active())
        stop();
    else
        start();
}

void PlaylistView::start() {
    reset();

    if (playlist_db_.empty())
        return;  // Nothing to do.

    if (next_track())
        send_current_track();
}

/* Advance to the next track in the playlist. */
bool PlaylistView::next_track() {
    if (is_done()) {
        if (loop())
            current_track_ = 0;
        else
            return false;  // All done.
    }
    current_entry_ = &playlist_db_[current_track_];
    current_track_++;
    return true;
}

/* Transmits the current_entry_ */
void PlaylistView::send_current_track() {
    // Prepare to send a file.
    replay_thread_.reset();
    transmitter_model.disable();
    ready_signal_ = false;

    if (!current_entry_)
        return;

    // TODO: Use a timer so the UI isn't frozen.
    if (current_entry_->ms_delay > 0)
        chThdSleepMilliseconds(current_entry_->ms_delay);

    // Open the sample file to send.
    auto reader = std::make_unique<FileReader>();
    auto error = reader->open(current_entry_->capture_path);
    if (error) {
        show_file_error(current_entry_->capture_path, "Can't open file to send.");
        return;
    }

    // Get the sample file size for the progress bar.
    current_entry_size_ = reader->file().size();
    progressbar_transmit.set_value(0);

    // ReplayThread starts immediately on contruction so
    // these need to be set before creating the ReplayThread.
    transmitter_model.set_target_frequency(current_entry_->metadata.center_frequency);
    transmitter_model.set_sampling_rate(current_entry_->metadata.sample_rate * 8);
    transmitter_model.set_baseband_bandwidth(baseband_bandwidth);
    transmitter_model.enable();

    // Set baseband sample rate too for waterfall to be correct.
    // TODO: Why doesn't the transmitter_model just handle this?
    baseband::set_sample_rate(transmitter_model.sampling_rate());

    // Use the ReplayThread class to send the data.
    replay_thread_ = std::make_unique<ReplayThread>(
        std::move(reader),
        /* read_size */ 0x4000,
        /* buffer_count */ 3,
        &ready_signal_,
        [](uint32_t return_code) {
            ReplayThreadDoneMessage message{return_code};
            EventDispatcher::send_message(message);
        });

    // Now it's sending, update the UI.
    update_ui();
}

void PlaylistView::stop() {
    // This terminates the underlying chThread.
    replay_thread_.reset();
    transmitter_model.disable();
    update_ui();
}

void PlaylistView::update_ui() {
    if (playlist_db_.empty()) {
        text_track.set("Open a playlist file.");
    } else {
        text_track.set(
            to_string_dec_uint(current_track_) + "/" +
            to_string_dec_uint(playlist_db_.size()) + " " +
            playlist_path_.filename().string());
    }

    button_play.set_bitmap(is_active() ? &bitmap_stop : &bitmap_play);

    progressbar_track.set_value(current_track_);
    progressbar_track.set_max(playlist_db_.size());
    progressbar_transmit.set_max(current_entry_size_);

    if (current_entry_) {
        auto duration = ms_duration(current_entry_size_, current_entry_->metadata.sample_rate, 4);
        text_filename.set(truncate(current_entry_->capture_path.filename().string(), 12));
        text_sample_rate.set(unit_auto_scale(current_entry_->metadata.sample_rate, 3, 0) + "Hz");
        text_duration.set(to_string_time_ms(duration));
        text_frequency.set(to_string_short_freq(current_entry_->metadata.center_frequency));
    } else {
        text_filename.set("-");
        text_sample_rate.set("-");
        text_duration.set("-");
        text_frequency.set("-");
    }
}

void PlaylistView::on_tx_progress(uint32_t progress) {
    progressbar_transmit.set_value(progress);
}

void PlaylistView::handle_replay_thread_done(uint32_t return_code) {
    if (return_code == ReplayThread::END_OF_FILE) {
        if (next_track()) {
            send_current_track();
            return;
        }
    }

    if (return_code == ReplayThread::READ_ERROR)
        show_file_error(current_entry_->capture_path, "Replay read failed.");

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
        &progressbar_track,
        &progressbar_transmit,
        &text_frequency,
        &tx_view,
        &check_loop,
        &button_play,
        &text_track,
        &waterfall,
    });

    waterfall.show_audio_spectrum_view(false);

    button_play.on_select = [this](ImageButton&) {
        toggle();
    };

    button_open.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".PPL");
        open_view->on_changed = [this](std::filesystem::path new_file_path) {
            on_file_changed(new_file_path);
        };
    };

    update_ui();
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
    stop();
    waterfall.on_hide();
    View::on_hide();
}

void PlaylistView::focus() {
    button_open.focus();
}

} /* namespace ui */
