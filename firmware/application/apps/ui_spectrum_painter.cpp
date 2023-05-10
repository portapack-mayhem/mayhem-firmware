/*
 * Copyright (C) 2023 Bernd Herzog
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

#include "ui_spectrum_painter.hpp"
//#include "hackrf_hal.hpp"
#include "cpld_update.hpp"
#include "bmp.hpp"
#include "baseband_api.hpp"

namespace ui {

SpectrumInputImageView::SpectrumInputImageView(NavigationView& nav) {
	hidden(true);

	add_children({
		&button_load_image
	});

	button_load_image.on_select = [this](Button&) {
		//TODO implement picture chooser
		// "/SPECTRUM/smiley.bmp"
		this->file = "/SPECTRUM/smiley.bmp";
		painted = false;
		this->set_dirty();

		this->on_input_avaliable();
	};
}

SpectrumInputImageView::~SpectrumInputImageView() {

}

void SpectrumInputImageView::focus() {
    button_load_image.focus();
}

bool SpectrumInputImageView::drawBMP_scaled(const ui::Rect r, const std::string file) {
	File bmpimage;
	size_t file_pos = 0;
	uint16_t pointer = 0;
	int16_t px = 0, py, zoom_factor = 0;
	bmp_header_t bmp_header;
	char buffer[257];
	ui::Color line_buffer[240];

	auto result = bmpimage.open(file);
	if(result.is_valid())
		return false;

	bmpimage.seek(file_pos);
	auto read_size = bmpimage.read(&bmp_header, sizeof(bmp_header));
	if (!((bmp_header.signature == 0x4D42) && // "BM" Signature
		(bmp_header.planes == 1) && // Seems always to be 1
		(bmp_header.compression == 0 || bmp_header.compression == 3 ))) {	// No compression
			return false;
	}

	switch(bmp_header.bpp) {
		case 16:
			file_pos = 0x36;
			memset(buffer, 0, 16);
			bmpimage.read(buffer, 16);
			if(buffer[1] == 0x7C)
				type = 3; // A1R5G5B5
			else
				type = 0; // R5G6B5
			break;
		case 24:
			type = 1;
			break;
		case 32:
		default:
			type = 2;
			break;
	}

	width = bmp_header.width;
	height = bmp_header.height;

	data_start = file_pos = bmp_header.image_data;

	while (r.width() < (width >> zoom_factor) || r.height() < (height >> zoom_factor)) {
		zoom_factor++;
	}

	py = height + 16;


	//TODO: enable
	return true;


	while(1) {
		while(px < width) {
			bmpimage.seek(file_pos);
			memset(buffer, 0, 257);
			read_size = bmpimage.read(buffer, 256);
			if (read_size.is_error())
				return false;	// Read error

			pointer = 0;
			while(pointer < 256) {
				if(pointer + 4 > 256)
					break;
				switch(type) {
					case 0: // R5G6B5
						if ((((1 << zoom_factor) - 1) & px) == 0x00)
							line_buffer[px >> zoom_factor] = ui::Color(((uint16_t)buffer[pointer] & 0x1F) | ((uint16_t)buffer[pointer] & 0xE0) << 1 | ((uint16_t)buffer[pointer + 1] & 0x7F) << 9);

						pointer += 2;
						file_pos += 2;
						break;

					case 3: // A1R5G5B5
						if ((((1 << zoom_factor) - 1) & px) == 0x00)
							line_buffer[px >> zoom_factor] = ui::Color((uint16_t)buffer[pointer] | ((uint16_t)buffer[pointer + 1] << 8));

						pointer += 2;
						file_pos += 2;
						break;

					case 1: // 24
					default:
						if ((((1 << zoom_factor) - 1) & px) == 0x00)
							line_buffer[px >> zoom_factor] = ui::Color(buffer[pointer + 2], buffer[pointer + 1], buffer[pointer]);
						pointer += 3;
						file_pos += 3;
						break;
					case 2: // 32
						if ((((1 << zoom_factor) - 1) & px) == 0x00)
							line_buffer[px >> zoom_factor] = ui::Color(buffer[pointer + 2], buffer[pointer + 1], buffer[pointer]);
						pointer += 4;
						file_pos += 4;
						break;
				}
				px++;
				if(px >= width) {
					break;
				}
			}
			if(read_size.value() != 256)
				break;
		}

		if ((((1 << zoom_factor) - 1) & py) == 0x00)
			portapack::display.render_line({ r.left(), r.top() + (py >> zoom_factor) }, px >> zoom_factor, line_buffer);

		px = 0;
		py--;

		if(read_size.value() < 256 || py < 0)
			break;
	}

	return true;
}

// std::string SpectrumInputImageView::get_filename() {
// 	return this->file;
// }

uint16_t SpectrumInputImageView::get_width(){
	return this->width;
}

uint16_t SpectrumInputImageView::get_height(){
	return this->height;
}

std::vector<uint8_t> SpectrumInputImageView::get_line(uint16_t y) {
	File bmpimage;
	auto result = bmpimage.open(this->file);

	//seek to line
	uint16_t line_size = width * (type == 2 ? 4 : (type == 1 ? 3 : 2));
	uint16_t line_offset = y * line_size;
	bmpimage.seek(data_start + line_offset);

	// allocate memory and read
	auto buffer = new uint8_t[line_size];
	auto read_size = bmpimage.read(buffer, line_size);

	// greyscale
	auto grey_buffer = new uint8_t[width];
	int pointer = 0;
	for (uint16_t px = 0; px < width; px++) {
		ui::Color color;
		switch(type) {
		case 0: // R5G6B5
			pointer = px * 2;
			color = ui::Color(((uint16_t)buffer[pointer] & 0x1F) | ((uint16_t)buffer[pointer] & 0xE0) << 1 | ((uint16_t)buffer[pointer + 1] & 0x7F) << 9);
			break;

		case 3: // A1R5G5B5
			pointer = px * 2;
			color = ui::Color((uint16_t)buffer[pointer] | ((uint16_t)buffer[pointer + 1] << 8));
			break;

		case 1: // 24
		default:
			pointer = px * 3;
			color = ui::Color(buffer[pointer + 2], buffer[pointer + 1], buffer[pointer]);
			break;

		case 2: // 32
			pointer = px * 4;
			color = ui::Color(buffer[pointer + 2], buffer[pointer + 1], buffer[pointer]);
			break;
		}

		grey_buffer[px] = color.to_greyscale();
	}

	delete buffer;

	//return line
	std::vector<uint8_t> values(grey_buffer, grey_buffer + width);
	delete grey_buffer;
	return values;
}

void SpectrumInputImageView::paint(Painter&) {
	if (!painted) {
		this->drawBMP_scaled({{ 4, 46 }, {200, 200}}, this->file);
		painted = true;
	}
}

SpectrumInputTextView::SpectrumInputTextView(NavigationView& nav) {
	hidden(true);

	add_children({
		&button_start
	});
}

SpectrumInputTextView::~SpectrumInputTextView() {
}

void SpectrumInputTextView::focus() {
    button_start.focus();
}

SpectrumPainterView::SpectrumPainterView(
	NavigationView& nav
) : nav_ (nav) {
	baseband::run_image(portapack::spi_flash::image_tag_spectrum_painter);

    add_children({
		&tx_view,
		&tab_view,
		&input_image,
		&input_text,
		&progressbar
	});

	Rect view_rect = { 0, 3 * 8, 240, 80 };
	input_image.set_parent_rect(view_rect);
	input_text.set_parent_rect(view_rect);

	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(portapack::receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			portapack::receiver_model.set_tuning_frequency(f);
		};
	};
	
	tx_view.on_start = [this]() {
		portapack::transmitter_model.set_sampling_rate(3072000U);
		portapack::transmitter_model.set_baseband_bandwidth(1750000);
		portapack::transmitter_model.enable();
		tx_view.set_transmitting(true);
		tx_view.focus();

		progressbar.set_max(tx_current_max_lines);
		progressbar.set_value(0);

		uint16_t height = input_image.get_height();
		tx_current_max_lines = height;
		uint16_t width = input_image.get_width();

		baseband::set_spectrum_painter_config(width, height);
	};
	
	tx_view.on_stop = [this]() {
		//baseband::set_sstv_data(0, 0);
		tx_view.set_transmitting(false);
		portapack::transmitter_model.disable();
		tx_active = false;
		tx_current_line = 0;
		//options_bitmaps.set_focusable(true);
		//this->fifo->reset();
	};
	//button_paint.on_select

	input_image.on_input_avaliable = [this]() {
		// enable tx_view
		// extract filename
		//std::string filename = input_image.get_filename();
	};
}

void SpectrumPainterView::start_tx() {
	// TODO: prepare first line
	tx_current_line = 0;
	static std::vector<uint8_t> line;
	line = input_image.get_line(tx_current_line++);

	if (fifo != nullptr)
		fifo->in(line);
	
	line = input_image.get_line(tx_current_line++);
	fifo->in(line);

	tx_active = true;
}

void SpectrumPainterView::frame_sync() {
	static std::vector<uint8_t> line;

	if (tx_active) {
		if (fifo->is_empty()) {
			tx_current_line++;
			progressbar.set_value(tx_current_line);

			if (tx_current_line >= tx_current_max_lines) {
				tx_active = false;
				tx_current_line = 0;

				//TODO: this cuts the last 2 lines
				tx_view.set_transmitting(false);
				portapack::transmitter_model.disable();

				return;
			}

			line = input_image.get_line(tx_current_line);
			fifo->in(line);
		}
	}
}

SpectrumPainterView::~SpectrumPainterView() {
	// save app settings
	// app_settings.tx_frequency = transmitter_model.tuning_frequency();	
	// settings.save("tx_sstv", &app_settings);

	portapack::transmitter_model.disable();
	hackrf::cpld::load_sram_no_verify();  // to leave all RX ok, without ghost signal problem at the exit.
	baseband::shutdown(); // better this function at the end, not load_sram() that sometimes produces hang up.
}

void SpectrumPainterView::focus() {
    tab_view.focus();
}

}
