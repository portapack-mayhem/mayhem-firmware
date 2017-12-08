/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#include "replay_app.hpp"
#include "string_format.hpp"

#include "ui_fileman.hpp"
#include "io_file.hpp"

#include "baseband_api.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;

namespace ui {

void ReplayAppView::set_ready() {
	ready_signal = true;
}

void ReplayAppView::on_file_changed(std::filesystem::path new_file_path) {
	File bbd_file;
	std::string str_duration = "";
	
	file_path = new_file_path;
	
	text_filename.set(new_file_path.string().substr(0, 18));
	
	bbd_file.open("/" + new_file_path.string());
	auto file_size = bbd_file.size();
	auto duration = file_size / (2 * 2 * sampling_rate / 8);
	
	progressbar.set_max(file_size);
	
	if (duration >= 60)
		str_duration = to_string_dec_uint(duration / 60) + "m";
	
	text_duration.set(str_duration + to_string_dec_uint(duration % 60) + "s");
	
	button_play.focus();
}

void ReplayAppView::on_tx_progress(const uint32_t progress) {
	progressbar.set_value(progress);
}

void ReplayAppView::focus() {
	button_open.focus();
}

bool ReplayAppView::is_active() const {
	return (bool)replay_thread;
}

void ReplayAppView::toggle() {
	if( is_active() ) {
		stop();
	} else {
		start();
	}
}

void ReplayAppView::start() {
	stop();

	std::unique_ptr<stream::Reader> reader;
	
	auto p = std::make_unique<FileReader>();
	auto open_error = p->open(file_path);
	if( open_error.is_valid() ) {
		handle_error(open_error.value());
	} else {
		reader = std::move(p);
	}

	if( reader ) {
		button_play.set_bitmap(&bitmap_stop);
		replay_thread = std::make_unique<ReplayThread>(
			std::move(reader),
			read_size, buffer_count,
			&ready_signal,
			[]() {
				ReplayThreadDoneMessage message { };
				EventDispatcher::send_message(message);
			},
			[](File::Error error) {
				ReplayThreadDoneMessage message { error.code() };
				EventDispatcher::send_message(message);
			}
		);
	}
	
	radio::enable({
		receiver_model.tuning_frequency(),
		sampling_rate,
		baseband_bandwidth,
		rf::Direction::Transmit,
		receiver_model.rf_amp(),
		static_cast<int8_t>(receiver_model.lna()),
		static_cast<int8_t>(receiver_model.vga())
	});
}

void ReplayAppView::stop() {
	if( is_active() )
		replay_thread.reset();
		
	progressbar.set_value(0);
	
	radio::disable();
	button_play.set_bitmap(&bitmap_play);
}

void ReplayAppView::handle_replay_thread_done(const File::Error error) {
	stop();
	if( error.code() ) {
		handle_error(error);
	}
}

void ReplayAppView::handle_error(const File::Error error) {
	nav_.display_modal("Error", error.what());
}

ReplayAppView::ReplayAppView(
	NavigationView& nav
) : nav_ (nav)
{
	baseband::run_image(portapack::spi_flash::image_tag_replay);

	add_children({
		&labels,
		&field_frequency,
		&field_lna,
		&field_rf_amp,
		&button_play,
		&text_filename,
		&text_duration,
		&progressbar,
		&button_open,
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
		auto new_view = nav.push<FileLoadView>(".C16");
		new_view->on_changed = [this](std::filesystem::path new_file_path) {
			on_file_changed(new_file_path);
		};
	};
}

ReplayAppView::~ReplayAppView() {
	radio::disable();
	baseband::shutdown();
}

void ReplayAppView::on_hide() {
	// TODO: Terrible kludge because widget system doesn't notify Waterfall that
	// it's being shown or hidden.
	waterfall.on_hide();
	View::on_hide();
}

void ReplayAppView::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);

	const ui::Rect waterfall_rect { 0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height };
	waterfall.set_parent_rect(waterfall_rect);
}

void ReplayAppView::on_target_frequency_changed(rf::Frequency f) {
	set_target_frequency(f);
}

void ReplayAppView::set_target_frequency(const rf::Frequency new_value) {
	persistent_memory::set_tuned_frequency(new_value);;
}

rf::Frequency ReplayAppView::target_frequency() const {
	return persistent_memory::tuned_frequency();
}

} /* namespace ui */
