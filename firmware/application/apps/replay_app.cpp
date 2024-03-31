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

#include "replay_app.hpp"
#include "string_format.hpp"

#include "ui_fileman.hpp"
#include "io_file.hpp"
#include "io_convert.hpp"

#include "baseband_api.hpp"
#include "metadata_file.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
#include "utility.hpp"

using namespace portapack;
namespace fs = std::filesystem;

namespace ui {

void ReplayAppView::set_ready() {
    ready_signal = true;
}

void ReplayAppView::on_file_changed(const fs::path& new_file_path) {
    file_path = new_file_path;
    File::Size file_size{};

    {  // Get the size of the data file.
        File data_file;
        auto error = data_file.open(file_path);
        if (error) {
            file_error();
            return;
        }

        file_size = data_file.size();
    }

    // Get original record frequency if available.
    auto metadata_path = get_metadata_path(file_path);
    auto metadata = read_metadata_file(metadata_path);

    if (metadata) {
        field_frequency.set_value(metadata->center_frequency);
        sample_rate = metadata->sample_rate;
    } else {
        // TODO: This is interesting because it implies that the
        // The capture will just be replayed at the freq set on the
        // FrequencyField. Is that an intentional behavior?
        sample_rate = 500000;
    }

    // UI Fixup.
    text_sample_rate.set(unit_auto_scale(sample_rate, 3, 0) + "Hz");
    progressbar.set_max(file_size);
    text_filename.set(truncate(file_path.filename().string(), 12));

    uint8_t sample_size = capture_file_sample_size(current()->path);
    auto duration = ms_duration(file_size, sample_rate, sample_size);
    text_duration.set(to_string_time_ms(duration));

    button_play.focus();
}

void ReplayAppView::on_tx_progress(const uint32_t progress) {
    progressbar.set_value(progress);
}

void ReplayAppView::focus() {
    button_open.focus();
}

void ReplayAppView::file_error() {
    nav_.display_modal("Error", "File read error.");
}

bool ReplayAppView::is_active() const {
    return (bool)replay_thread;
}

void ReplayAppView::toggle() {
    if (is_active()) {
        stop(false);
    } else {
        start();
    }
}

void ReplayAppView::start() {
    stop(false);

    std::unique_ptr<stream::Reader> reader;

    auto p = std::make_unique<FileConvertReader>();
    auto open_error = p->open(file_path);
    if (open_error.is_valid()) {
        file_error();
        return;  // Fixes TX bug if there's a file error
    } else {
        reader = std::move(p);
    }

    if (reader) {
        button_play.set_bitmap(&bitmap_stop);
        baseband::set_sample_rate(sample_rate, OversampleRate::x8);

        replay_thread = std::make_unique<ReplayThread>(
            std::move(reader),
            read_size, buffer_count,
            &ready_signal,
            [](uint32_t return_code) {
                ReplayThreadDoneMessage message{return_code};
                EventDispatcher::send_message(message);
            });
    }

    transmitter_model.set_sampling_rate(sample_rate * toUType(OversampleRate::x8));
    transmitter_model.set_baseband_bandwidth(baseband_bandwidth);
    transmitter_model.enable();

    if (portapack::persistent_memory::stealth_mode()) {
        DisplaySleepMessage message;
        EventDispatcher::send_message(message);
    }
}

void ReplayAppView::stop(const bool do_loop) {
    if (is_active())
        replay_thread.reset();

    if (do_loop && check_loop.value()) {
        start();
    } else {
        transmitter_model.disable();
        button_play.set_bitmap(&bitmap_play);
    }

    ready_signal = false;
}

void ReplayAppView::handle_replay_thread_done(const uint32_t return_code) {
    if (return_code == ReplayThread::END_OF_FILE) {
        stop(true);
    } else if (return_code == ReplayThread::READ_ERROR) {
        stop(false);
        file_error();
    }

    progressbar.set_value(0);
}

ReplayAppView::ReplayAppView(
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
        &tx_view,  // now it handles previous rfgain, rfamp.
        &check_loop,
        &button_play,
        &waterfall,
    });

    button_play.on_select = [this](ImageButton&) {
        this->toggle();
    };

    button_open.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".C*");
        open_view->on_changed = [this](fs::path new_file_path) {
            on_file_changed(new_file_path);
        };
    };
}

ReplayAppView::~ReplayAppView() {
    transmitter_model.disable();
    baseband::shutdown();
}

void ReplayAppView::on_hide() {
    stop(false);
    // TODO: Terrible kludge because widget system doesn't notify Waterfall that
    // it's being shown or hidden.
    waterfall.on_hide();
    View::on_hide();
}

void ReplayAppView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);

    const ui::Rect waterfall_rect{0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height};
    waterfall.set_parent_rect(waterfall_rect);
}

} /* namespace ui */
