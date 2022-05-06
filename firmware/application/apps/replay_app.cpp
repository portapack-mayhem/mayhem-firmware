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

#include "baseband_api.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;

namespace ui {

void ReplayAppView::set_ready() {
	ready_signal = true;
}

void ReplayAppView::on_file_changed(std::filesystem::path new_file_path) {
	File data_file, info_file;
	char file_data[257];
	
	// Get file size
	auto data_open_error = data_file.open("/" + new_file_path.string());
	if (data_open_error.is_valid()) {
		file_error();
		return;
	}
	
	file_path = new_file_path;
	
	// Get original record frequency if available
	std::filesystem::path info_file_path = file_path;
	info_file_path.replace_extension(u".TXT");
	
	sample_rate = 500000;
	
	auto info_open_error = info_file.open("/" + info_file_path.string());
	if (!info_open_error.is_valid()) {
		memset(file_data, 0, 257);
		auto read_size = info_file.read(file_data, 256);
		if (!read_size.is_error()) {
			auto pos1 = strstr(file_data, "center_frequency=");
			if (pos1) {
				pos1 += 17;
				field_frequency.set_value(strtoll(pos1, nullptr, 10));
			}
			
			auto pos2 = strstr(file_data, "sample_rate=");
			if (pos2) {
				pos2 += 12;
				sample_rate = strtoll(pos2, nullptr, 10);
			}
		}
	}
	
	text_sample_rate.set(unit_auto_scale(sample_rate, 3, 0) + "Hz");
	
	auto file_size = data_file.size();
	auto duration = (file_size * 1000) / (2 * 2 * sample_rate);
	
	progressbar.set_max(file_size);
	text_filename.set(file_path.filename().string().substr(0, 12));
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
	if( is_active() ) {
		stop(false);
	} else {
		start();
	}
}

void ReplayAppView::start() {
	stop(false);

	std::unique_ptr<stream::Reader> reader;
	
	auto p = std::make_unique<FileReader>();
	auto open_error = p->open(file_path);
	if( open_error.is_valid() ) {
		file_error();
		return;                               // Fixes TX bug if there's a file error
	} else {
		reader = std::move(p);
	}

	if( reader ) {
		button_play.set_bitmap(&bitmap_stop);
		baseband::set_sample_rate(sample_rate * 8);
		
		replay_thread = std::make_unique<ReplayThread>(
			std::move(reader),
			read_size, buffer_count,
			&ready_signal,
			[](uint32_t return_code) {
				ReplayThreadDoneMessage message { return_code };
				EventDispatcher::send_message(message);
			}
		);
	}
    field_rfgain.on_change = [this](int32_t v) {
		tx_gain = v;
	};  
	field_rfgain.set_value(tx_gain);
	receiver_model.set_tx_gain(tx_gain); 
    

 	field_rfamp.on_change = [this](int32_t v) {
		rf_amp = (bool)v;
	};
	field_rfamp.set_value(rf_amp ? 14 : 0);

	//Enable Bias Tee if selected
	radio::set_antenna_bias(portapack::get_antenna_bias());
		
	radio::enable({
		receiver_model.tuning_frequency(),
		sample_rate * 8,
		baseband_bandwidth,
		rf::Direction::Transmit,
        rf_amp,         //  previous code line : "receiver_model.rf_amp()," was passing the same rf_amp of all Receiver Apps  
		static_cast<int8_t>(receiver_model.lna()),
		static_cast<int8_t>(receiver_model.vga())
	});  
} 

void ReplayAppView::stop(const bool do_loop) {
	if( is_active() )
		replay_thread.reset();
	
	if (do_loop && check_loop.value()) {
		start();
	} else {
		radio::set_antenna_bias(false);    //Turn off Bias Tee
		radio::disable();
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
	NavigationView& nav
) : nav_ (nav)
{

	tx_gain = 35;field_rfgain.set_value(tx_gain);  // Initial default  value (-12 dB's max ).
	field_rfamp.set_value(rf_amp ? 14 : 0);  // Initial default value True. (TX RF amp on , +14dB's)

	field_rfamp.on_change = [this](int32_t v) {	// allow initial value change just after opened file.	
		rf_amp = (bool)v;
	};
	field_rfamp.set_value(rf_amp ? 14 : 0);

    field_rfgain.on_change = [this](int32_t v) { // allow initial value change just after opened file.
		tx_gain = v;
	};  
	field_rfgain.set_value(tx_gain);

	baseband::run_image(portapack::spi_flash::image_tag_replay);

	add_children({
		&labels,
		&button_open,
		&text_filename,
		&text_sample_rate,
		&text_duration,
		&progressbar,
		&field_frequency,
		&field_rfgain, 
		&field_rfamp,       // let's not use common rf_amp
		&check_loop,
		&button_play,
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
		auto open_view = nav.push<FileLoadView>(".C16");
		open_view->on_changed = [this](std::filesystem::path new_file_path) {
			on_file_changed(new_file_path);
		};
	};
}

ReplayAppView::~ReplayAppView() {
	radio::disable();
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
