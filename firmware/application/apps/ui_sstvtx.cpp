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

#include "ui_sstvtx.hpp"
#include "string_format.hpp"

#include "portapack.hpp"
#include "hackrf_hal.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui {

void SSTVTXView::focus() {
	if (file_error)
		nav_.display_modal("No files", "No valid bitmaps\nin /sstv directory.", ABORT, nullptr);
	else
		options_bitmaps.focus();
}

void SSTVTXView::read_boundary(uint8_t * const buffer, uint32_t position, uint32_t length) {
	uint32_t to_read, split;
	uint8_t * buffer_copy = buffer;
	
	bmp_file.seek(position);
	
	// FatFS bug workaround: we can't read across 512 byte boundaries ?!
	while (length) {
		to_read = (length <= 512) ? length : 512;
		if ((position & 512) != ((position + to_read - 1) & 512)) {
			// Crossing: do 2 reads before and after the boundary
			split = 512 - (position & 511);
			bmp_file.read(buffer_copy, split);
			bmp_file.read(buffer_copy + split, to_read - split);
		} else {
			// No crossing, just read normally
			bmp_file.read(buffer_copy, to_read);
		}
		position += to_read;
		buffer_copy += to_read;
		length -= to_read;
	}
}

void SSTVTXView::paint(Painter&) {
	ui::Color line_buffer[160];
	Coord line;
	uint32_t data_idx, bmp_px, pixel_idx;
	
	data_idx = bmp_header.image_data;
	
	for (line = 0; line < (256 / 2); line++) {
		
		// Buffer a whole line
		read_boundary(pixels_buffer, data_idx, sizeof(pixels_buffer));
		
		for (bmp_px = 0; bmp_px < 160; bmp_px++) {
			pixel_idx = bmp_px * 3 * 2;
			line_buffer[bmp_px] = Color(pixels_buffer[pixel_idx + 2],
										pixels_buffer[pixel_idx + 1],
										pixels_buffer[pixel_idx + 0]);
		}
		portapack::display.render_line({ 16, 80 + 128 - line }, 160, line_buffer);
		data_idx += sizeof(pixels_buffer) * 2;
	}
}

SSTVTXView::~SSTVTXView() {
	// save app settings
	app_settings.tx_frequency = transmitter_model.tuning_frequency();	
	settings.save("tx_sstv", &app_settings);

	transmitter_model.disable();
	baseband::shutdown();
}

void SSTVTXView::on_tuning_frequency_changed(rf::Frequency f) {
	transmitter_model.set_tuning_frequency(f);
}

void SSTVTXView::prepare_scanline() {
	sstv_scanline scanline_buffer;
	uint32_t component, pixel_idx;
	uint8_t offset;
	
	if (scanline_counter >= (256 * 3)) {
		progressbar.set_value(0);
		transmitter_model.disable();
		options_bitmaps.set_focusable(true);
		tx_view.set_transmitting(false);
		return;
	}
	
	progressbar.set_value(scanline_counter);

	// Scottie 2 scanline:
	// (First line: 1200 9ms)
	// 1500 1.5ms
	// Green
	// 1500 1.5ms
	// Blue
	// 1200 9ms
	// 1500 1.5ms
	// Red
	// Scanline time: 88.064ms (275.2us/pixel @ 320 pixels/line)
	
	component = scanline_counter % 3;
	
	if ((!scanline_counter && tx_sstv_mode->sync_on_first) || (component == tx_sstv_mode->sync_index)) {
		// Sync
		scanline_buffer.start_tone.frequency = SSTV_F2D(1200);
		scanline_buffer.start_tone.duration = tx_sstv_mode->samples_per_sync;
		scanline_buffer.gap_tone.frequency = SSTV_F2D(1500);
		scanline_buffer.gap_tone.duration = tx_sstv_mode->samples_per_gap;
	} else {
		// Regular scanline
		scanline_buffer.start_tone.duration = 0;
		if (tx_sstv_mode->gaps) {
			scanline_buffer.gap_tone.frequency = SSTV_F2D(1500);
			scanline_buffer.gap_tone.duration = tx_sstv_mode->samples_per_gap;
		}
	}
	
	if (!component) {
		// Read a new line
		read_boundary(pixels_buffer,
						bmp_header.image_data + ((255 - (scanline_counter / 3)) * sizeof(pixels_buffer)),
						sizeof(pixels_buffer));
	}
	
	offset = component_map[component];
	for (uint32_t bmp_px = 0; bmp_px < 320; bmp_px++) {
		pixel_idx = bmp_px * 3;
		scanline_buffer.luma[bmp_px] = pixels_buffer[pixel_idx + offset];
	}
	
	baseband::set_fifo_data((int8_t *)&scanline_buffer);
	
	scanline_counter++;
}

void SSTVTXView::start_tx() {
	// The baseband SSTV TX code (proc_sstv) has a 2-scanline buffer. It is preloaded before
	// TX start, and asks for fill-up when a new scanline starts being read. This should
	// leave enough time for the code in prepare_scanline() before it ends.
	
	scanline_counter = 0;
	prepare_scanline();		// Preload one scanline
	
	transmitter_model.set_sampling_rate(3072000U);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	baseband::set_sstv_data(
		tx_sstv_mode->vis_code,
		tx_sstv_mode->samples_per_pixel
	);
	
	// Todo: Find a better way to prevent user from changing bitmap during tx
	options_bitmaps.set_focusable(false);
	tx_view.focus();
}

void SSTVTXView::on_bitmap_changed(const size_t index) {
	bmp_file.open("/sstv/" + bitmaps[index].string());
	bmp_file.read(&bmp_header, sizeof(bmp_header));
	set_dirty();
}

void SSTVTXView::on_mode_changed(const size_t index) {
	sstv_color_seq tx_color_sequence;
	
	tx_sstv_mode = &sstv_modes[index];
	
	tx_color_sequence = sstv_modes[index].color_sequence;
	if (tx_color_sequence == SSTV_COLOR_RGB) {
		component_map[0] = 2;
		component_map[1] = 1;
		component_map[2] = 0;
	} else if (tx_color_sequence == SSTV_COLOR_GBR) {
		component_map[0] = 1;
		component_map[1] = 0;
		component_map[2] = 2;
	}

	progressbar.set_max(sstv_modes[index].lines * 3);
}

SSTVTXView::SSTVTXView(
	NavigationView& nav
) : nav_ (nav)
{
	std::vector<std::filesystem::path> file_list;
	using option_t = std::pair<std::string, int32_t>;
	using options_t = std::vector<option_t>;
	options_t bitmap_options;
	options_t mode_options;
	uint32_t c;

	// load app settings
	auto rc = settings.load("tx_sstv", &app_settings);
	if(rc == SETTINGS_OK) {
		transmitter_model.set_rf_amp(app_settings.tx_amp);
		transmitter_model.set_channel_bandwidth(app_settings.channel_bandwidth);
		transmitter_model.set_tuning_frequency(app_settings.tx_frequency);
		transmitter_model.set_tx_gain(app_settings.tx_gain);		
	}

	// Search for valid bitmaps
	file_list = scan_root_files(u"/sstv", u"*.bmp");
	if (!file_list.size()) {
		file_error = true;
		return;
	}
	for (const auto& file_name : file_list) {
		if (!bmp_file.open("/sstv/" + file_name.string()).is_valid()) {
			bmp_file.read(&bmp_header, sizeof(bmp_header));
			if ((bmp_header.signature == 0x4D42) &&	// "BM"
				(bmp_header.width == 320) &&		// Must be exactly 320x256 pixels for now
				(bmp_header.height == 256) &&
				(bmp_header.planes == 1) &&
				(bmp_header.bpp == 24) &&			// 24 bpp only
				(bmp_header.compression == 0)) {	// No compression
					bitmaps.push_back(file_name);
			}
		}
	}
	if (!bitmaps.size()) {
		file_error = true;
		return;
	}
	
	// Maybe this could be merged with proc_tones ? Pretty much the same except lots
	// of different tones (256+)
	baseband::run_image(portapack::spi_flash::image_tag_sstv_tx);
	
	add_children({
		&labels,
		&options_bitmaps,
		&options_modes,
		&progressbar,
		&tx_view
	});
	
	// Populate file list
	for (const auto& bitmap : bitmaps)
		bitmap_options.emplace_back(bitmap.string().substr(0, 16), 0);
	options_bitmaps.set_options(bitmap_options);

	// Populate mode list
	for (c = 0; c < SSTV_MODES_NB; c++)
		mode_options.emplace_back(sstv_modes[c].name, c);
	options_modes.set_options(mode_options);
	
	options_bitmaps.on_change = [this](size_t i, int32_t) {
		this->on_bitmap_changed(i);
	};
	options_bitmaps.set_selected_index(0);	// First file
	on_bitmap_changed(0);
	
	options_modes.on_change = [this](size_t i, int32_t) {
		this->on_mode_changed(i);
	};
	options_modes.set_selected_index(1);	// Scottie 2
	on_mode_changed(1);
	
	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			receiver_model.set_tuning_frequency(f);
		};
	};
	
	tx_view.on_start = [this]() {
		start_tx();
		tx_view.set_transmitting(true);
	};
	
	tx_view.on_stop = [this]() {
		baseband::set_sstv_data(0, 0);
		tx_view.set_transmitting(false);
		transmitter_model.disable();
		options_bitmaps.set_focusable(true);
	};
}

} /* namespace ui */
