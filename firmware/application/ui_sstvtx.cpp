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
	uint8_t pixels_buffer[320 * 3];		// 320 pixels @ 24bpp
	
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
	transmitter_model.disable();
	baseband::shutdown();
}

void SSTVTXView::on_tuning_frequency_changed(rf::Frequency f) {
	transmitter_model.set_tuning_frequency(f);
}

void SSTVTXView::start_tx() {
	// Baseband SSTV TX code should have a 2 scanlines buffer, and ask
	// for fill-up when there's 1 or less remaining. This should leave
	// enough time for the code here to generate the scanline data
	// before tx.
	
	const uint8_t VIS_code = 0b00011101;	// Scottie 2
	
	// Calibration:
	// 1900 300ms
	// 1200 10ms
	// 1900 300ms
	// VIS: (30ms * 10 = 300ms)
	// 1200
	// 00011101 (0=1300, 1=1100)
	// 1200
	
	// Scottie 2: 320x256 px
	
	// V-sync ?
	// First line: 1200 9ms
	// Scanline:
	// 1500 1.5ms
	// Green
	// 1500 1.5ms
	// Blue
	// 1200 9ms
	// 1500 1.5ms
	// Red
	// Scanline time: 88.064ms (.2752ms/pixel @ 320 pixels/line)
	
	transmitter_model.set_sampling_rate(1536000U);
	transmitter_model.set_rf_amp(true);
	//transmitter_model.set_lna(40);
	//transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	/*baseband::set_sstvtx_data(
		(1536000 / 44100) - 1,
		12000,
		1,
		false,
		0
	);*/
}

void SSTVTXView::on_bitmap_changed(size_t index) {
	bmp_file.open("/sstv/" + bitmaps[index].string());
	bmp_file.read(&bmp_header, sizeof(bmp_header));
	set_dirty();
}

SSTVTXView::SSTVTXView(
	NavigationView& nav
) : nav_ (nav)
{
	std::vector<std::filesystem::path> file_list;
	using option_t = std::pair<std::string, int32_t>;
	using options_t = std::vector<option_t>;
	options_t bitmap_options;
	uint32_t c;
	
	// Search for valid bitmaps
	file_list = scan_root_files(u"/sstv", u"*.bmp");
	if (!file_list.size()) {
		file_error = true;
		return;
	}
	for (const auto& file_name : file_list) {
		if (!bmp_file.open("/sstv/" + file_name.string()).is_valid()) {
			bmp_file.read(&bmp_header, sizeof(bmp_header));
			if ((bmp_header.signature == 0x4D42) &&
				(bmp_header.width == 320) &&		// Must be == 320x256 pixels for now
				(bmp_header.height == 256) &&
				(bmp_header.planes == 1) &&
				(bmp_header.bpp >= 24) &&			// 24 or 32 bpp
				(bmp_header.compression == 0)) {	// No compression
					bitmaps.push_back(file_name);
			}
		}
	}
	if (!bitmaps.size()) {
		file_error = true;
		return;
	}
	
	//baseband::run_image(portapack::spi_flash::image_tag_sstv_tx);
	
	add_children({
		&labels,
		&options_bitmaps,
		&text_mode
	});
	
	for (const auto& bitmap : bitmaps)
		bitmap_options.emplace_back(bitmap.string().substr(0, 16), 0);
	
	options_bitmaps.set_options(bitmap_options);
	options_bitmaps.on_change = [this](size_t i, int32_t) {
		this->on_bitmap_changed(i);
	};
	options_bitmaps.set_selected_index(0);
}

} /* namespace ui */
