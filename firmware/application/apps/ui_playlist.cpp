/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyleft  (ↄ) 2022 NotPike
 * Copyright (C) 2023 Kyle Reed, zxkmm
 * Copyright (C) 2024 Mark Thompson
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

#include "baseband_api.hpp"
#include "convert.hpp"
#include "file_reader.hpp"
#include "io_file.hpp"
#include "io_convert.hpp"
#include "oversample.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
#include "string_format.hpp"
#include "ui_fileman.hpp"
#include "utility.hpp"

#include <unistd.h>
#include <fstream>

using namespace portapack;
namespace fs = std::filesystem;

/* TODO
 * - Should frequency overrides be saved in the playlist?
 * - Start playing from current track (vs start over)?
 */

namespace ui {

// TODO: consolidate extesions into a shared header?
static const fs::path ppl_ext = u".PPL";

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
        auto entry = load_entry(trim(cols[0]));

        if (!entry)
            continue;

        // Read optional delay value.
        if (cols.size() > 1)
            parse_int(cols[1], entry->ms_delay);

        playlist_db_.emplace_back(*std::move(entry));
    }
}

Optional<PlaylistView::playlist_entry> PlaylistView::load_entry(fs::path&& path) {
    File capture_file;

    auto error = capture_file.open(path);
    if (error)
        return {};

    // Read metafile if it exists.
    auto metadata_path = get_metadata_path(path);
    auto metadata = read_metadata_file(metadata_path);

    // If no metadata found, fallback to the TX frequency.
    if (!metadata)
        metadata = {transmitter_model.target_frequency(), 500'000};

    return playlist_entry{
        std::move(path),
        *metadata,
        capture_file.size(),
        0u};
}

void PlaylistView::on_file_changed(const fs::path& new_file_path) {
    stop();

    current_index_ = 0;
    playlist_path_ = new_file_path;
    playlist_db_.clear();
    load_file(playlist_path_);

    update_ui();

    // TODO: fix in UI framework with 'try_focus()'?
    // Hack around focus getting called by ctor before parent is set.
    if (parent())
        button_play.focus();
}

void PlaylistView::open_file(bool prompt_save) {
    if (playlist_dirty_ && prompt_save) {
        nav_.display_modal(
            "Save?", "Save changes?", YESNO,
            [this](bool choice) {
                if (choice)
                    save_file(false);
            });
        nav_.set_on_pop([this]() { open_file(false); });
        return;
    }

    auto open_view = nav_.push<FileLoadView>(".PPL");
    open_view->push_dir(u"PLAYLIST");
    open_view->on_changed = [this](fs::path new_file_path) {
        on_file_changed(new_file_path);
    };
}

void PlaylistView::save_file(bool show_dialogs) {
    if (!playlist_dirty_ || playlist_path_.empty())
        return;

    ensure_directory(playlist_path_.parent_path());

    File playlist_file;
    auto error = playlist_file.create(playlist_path_.string());

    if (error) {
        if (show_dialogs)
            nav_.display_modal(
                "Save Error",
                "Could not save file\n" + playlist_path_.string());
        return;
    }

    for (const auto& entry : playlist_db_) {
        playlist_file.write_line(
            entry.path.string() + "," +
            to_string_dec_uint(entry.ms_delay));
    }

    playlist_dirty_ = false;

    if (show_dialogs)
        nav_.display_modal(
            "Save",
            "Saved playlist\n" + playlist_path_.string());
}

void PlaylistView::add_entry(fs::path&& path) {
    if (playlist_path_.empty()) {
        playlist_path_ = next_filename_matching_pattern(u"/PLAYLIST/PLAY_????.PPL");

        // Hack around focus getting called by ctor before parent is set.
        if (parent())
            button_play.focus();
    }

    auto entry = load_entry(std::move(path));
    if (entry) {
        playlist_db_.emplace_back(*std::move(entry));
        current_index_ = playlist_db_.size() - 1;
        playlist_dirty_ = true;
    }

    update_ui();
}

void PlaylistView::delete_entry() {
    if (playlist_db_.empty())
        return;

    // Ugh, the STL is gross.
    playlist_db_.erase(playlist_db_.begin() + current_index_);

    if (current_index_ > 0)
        --current_index_;

    playlist_dirty_ = true;
    update_ui();
}

void PlaylistView::show_file_error(const fs::path& path, const std::string& message) {
    nav_.display_modal("Error", "Error opening file \n" + path.string() + "\n" + message);
}

PlaylistView::playlist_entry* PlaylistView::current() {
    return playlist_db_.empty() ? nullptr : &playlist_db_[current_index_];
}

bool PlaylistView::is_active() const {
    return replay_thread_ != nullptr;
}

bool PlaylistView::at_end() const {
    return current_index_ >= playlist_db_.size();
}

void PlaylistView::toggle() {
    if (is_active())
        stop();
    else
        start();
}

void PlaylistView::start() {
    if (playlist_db_.empty())
        return;  // Nothing to do.

    current_index_ = 0u;
    send_current_track();
}

/* Advance to the next track in the playlist. */
bool PlaylistView::next_track() {
    ++current_index_;

    // Reset to the 0 once at the end to prevent current_index_
    // from being outside the bounds of playlist_db_ when painting.
    if (at_end()) {
        current_index_ = 0;
        return check_loop.value();  // Keep going if looping.
    }

    return true;
}

/* Transmits the current_entry_ */
void PlaylistView::send_current_track() {
    // Prepare to send a file.
    replay_thread_.reset();
    transmitter_model.disable();
    ready_signal_ = false;

    if (!current())
        return;

    // TODO: Use a timer so the UI isn't frozen.
    if (current()->ms_delay > 0)
        chThdSleepMilliseconds(current()->ms_delay);

    // Open the sample file to send.
    auto reader = std::make_unique<FileConvertReader>();
    auto error = reader->open(current()->path);
    if (error) {
        show_file_error(current()->path, "Can't open file to send.");
        return;
    }

    // Update the sample rate in proc_replay baseband.
    baseband::set_sample_rate(current()->metadata.sample_rate,
                              get_oversample_rate(current()->metadata.sample_rate));

    // ReplayThread starts immediately on construction; must be set before creating.
    transmitter_model.set_target_frequency(current()->metadata.center_frequency);
    transmitter_model.set_sampling_rate(get_actual_sample_rate(current()->metadata.sample_rate));
    transmitter_model.set_baseband_bandwidth(current()->metadata.sample_rate <= 500'000 ? 1'750'000 : 2'500'000);  // TX LPF min 1M75 for SR <=500K, and  2M5 (by experimental test) for SR >500K
    transmitter_model.enable();

    // Reset the transmit progress bar.
    progressbar_transmit.set_value(0);

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

    // Reset the transmit progress bar.
    progressbar_transmit.set_value(0);
    update_ui();
}

void PlaylistView::update_ui() {
    if (playlist_db_.empty()) {
        text_filename.set("-");
        text_sample_rate.set("-");
        text_duration.set("-");

        if (playlist_path_.empty())
            text_track.set("Open playlist or add capture.");
        else
            text_track.set(playlist_path_.filename().string());

        progressbar_track.set_value(0);
        progressbar_track.set_max(0);
        progressbar_transmit.set_max(0);

    } else {
        chDbgAssert(!at_end(), "update_ui #1", "current_index_ invalid");

        text_filename.set(current()->path.filename().string());
        text_sample_rate.set(unit_auto_scale(current()->metadata.sample_rate, 3, (current()->metadata.sample_rate > 1000000) ? 2 : 0) + "Hz");

        uint8_t sample_size = capture_file_sample_size(current()->path);
        auto duration = ms_duration(current()->file_size, current()->metadata.sample_rate, sample_size);
        text_duration.set(to_string_time_ms(duration));
        field_frequency.set_value(current()->metadata.center_frequency);

        text_track.set(
            to_string_dec_uint(current_index_ + 1) + "/" +
            to_string_dec_uint(playlist_db_.size()) + " " +
            playlist_path_.filename().string());

        progressbar_track.set_max(playlist_db_.size() - 1);
        progressbar_track.set_value(current_index_);
        progressbar_transmit.set_max(current()->file_size * sizeof(complex16_t) / sample_size);
    }

    button_play.set_bitmap(is_active() ? &bitmap_stop : &bitmap_play);
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
        show_file_error(current()->path, "Replay read failed.");

    stop();
}

PlaylistView::PlaylistView(
    NavigationView& nav)
    : nav_(nav) {
    baseband::run_image(portapack::spi_flash::image_tag_replay);

    add_children({
        &text_filename,
        &text_sample_rate,
        &text_duration,
        &progressbar_track,
        &progressbar_transmit,
        &field_frequency,
        &tx_view,
        &check_loop,
        &button_play,
        &text_track,
        &button_prev,
        &button_add,
        &button_delete,
        &button_open,
        &button_save,
        &button_next,
        &waterfall,
    });

    ensure_directory(u"PLAYLIST");
    waterfall.show_audio_spectrum_view(false);

    field_frequency.set_value(transmitter_model.target_frequency());
    field_frequency.on_change = [this](rf::Frequency f) {
        if (current())
            current()->metadata.center_frequency = f;
    };
    field_frequency.on_edit = [this]() {
        auto freq_view = nav_.push<FrequencyKeypadView>(field_frequency.value());
        freq_view->on_changed = [this](rf::Frequency f) {
            field_frequency.set_value(f);
        };
    };

    button_play.on_select = [this](ImageButton&) {
        toggle();
    };

    button_add.on_select = [this, &nav]() {
        if (is_active())
            return;
        auto open_view = nav_.push<FileLoadView>(".C*");
        open_view->push_dir(u"CAPTURES");
        open_view->on_changed = [this](fs::path path) {
            add_entry(std::move(path));
        };
    };

    button_delete.on_select = [this, &nav]() {
        if (is_active())
            return;
        delete_entry();
    };

    button_open.on_select = [this, &nav]() {
        if (is_active())
            stop();
        open_file();
    };

    button_save.on_select = [this]() {
        if (is_active())
            stop();
        save_file();
    };

    button_prev.on_select = [this]() {
        if (is_active())
            return;
        --current_index_;
        if (at_end())
            current_index_ = playlist_db_.size() - 1;
        update_ui();
    };

    button_next.on_select = [this]() {
        if (is_active())
            return;
        ++current_index_;
        if (at_end())
            current_index_ = 0;
        update_ui();
    };

    update_ui();
}

PlaylistView::PlaylistView(
    NavigationView& nav,
    const fs::path& path)
    : PlaylistView(nav) {
    auto ext = path.extension();
    if (path_iequal(ext, ppl_ext))
        on_file_changed(path);
    else if (is_cxx_capture_file(path))
        add_entry(fs::path{path});
}

PlaylistView::~PlaylistView() {
    transmitter_model.disable();
    baseband::shutdown();
}

void PlaylistView::set_parent_rect(Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);

    ui::Rect waterfall_rect{
        0, header_height, new_parent_rect.width(),
        new_parent_rect.height() - header_height};
    waterfall.set_parent_rect(waterfall_rect);
}

void PlaylistView::focus() {
    if (playlist_path_.empty())
        button_add.focus();
    else
        button_play.focus();
}

void PlaylistView::on_hide() {
    stop();
    waterfall.on_hide();
    View::on_hide();
}

} /* namespace ui */
