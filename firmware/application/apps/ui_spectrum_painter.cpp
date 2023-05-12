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

#include "ui_fileman.hpp"
#include "io_file.hpp"
#include "file.hpp"
#include "portapack_persistent_memory.hpp"

namespace ui {

SpectrumInputImageView::SpectrumInputImageView(NavigationView& nav) {
	hidden(true);

	add_children({
		&button_load_image
	});

	button_load_image.on_select = [this, &nav](Button&) {
		auto open_view = nav.push<FileLoadView>(".bmp");

		if (std::filesystem::is_directory(u"/SPECTRUM"))
			open_view->push_dir(u"SPECTRUM");

		open_view->on_changed = [this](std::filesystem::path new_file_path) {
			this->file = new_file_path.string();
			painted = false;
			this->set_dirty();

			this->on_input_avaliable();
		};
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

uint16_t SpectrumInputImageView::get_width(){
	return this->width;
}

uint16_t SpectrumInputImageView::get_height(){
	return this->height;
}

std::vector<uint8_t> SpectrumInputImageView::get_line(uint16_t y) {
	File bmpimage;
	bmpimage.open(this->file);

	//seek to line
	uint32_t line_size = width * (type == 2 ? 4 : (type == 1 ? 3 : 2));
	uint32_t line_offset = y * line_size;
	bmpimage.seek(data_start + line_offset);

	// allocate memory and read
	auto buffer = new uint8_t[line_size];
	auto bytes_read = bmpimage.read(buffer, line_size);

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

	std::vector<uint8_t> values(width);
	for(int i = 0; i < width; i++) {
		values[i] = grey_buffer[i];
	}

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

	(void)nav;
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
		&labels,
		&tab_view,
		&input_image,
		&input_text,
		&progressbar,
		&field_frequency,
		&field_rfgain, 
		&field_rfamp,       // let's not use common rf_amp
		&check_loop,
		&button_play,
		&option_bandwidth,
		&field_duration,
		&field_pause,
	});

	//tx_view.hidden(true);

	Rect view_rect = { 0, 3 * 8, 240, 80 };
	input_image.set_parent_rect(view_rect);
	input_text.set_parent_rect(view_rect);

	field_frequency.set_value(target_frequency());
	field_frequency.set_step(5000);
	field_frequency.on_change = [this](rf::Frequency f) {
		this->on_target_frequency_changed(f);
	};
	field_frequency.on_edit = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(this->target_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			this->on_target_frequency_changed(f);
			this->field_frequency.set_value(f);
		};
	};

	tx_gain = 10;
	field_rfgain.set_value(tx_gain);  // Initial default  value (-12 dB's max ).
    field_rfgain.on_change = [this](int32_t v) { // allow initial value change just after opened file.
		tx_gain = v;
		portapack::transmitter_model.set_tx_gain(tx_gain);
	};  

	field_rfamp.set_value(rf_amp ? 14 : 0);  // Initial default value True. (TX RF amp on , +14dB's)
	field_rfamp.on_change = [this](int32_t v) {	// allow initial value change just after opened file.	
		rf_amp = (bool)v;
		portapack::transmitter_model.set_rf_amp(rf_amp);
	};

	button_play.hidden(true);
	input_image.on_input_avaliable = [this]() {
		button_play.hidden(false);
	};

	button_play.on_select = [this](ImageButton&) {
		if (tx_active == false) {
			//Enable Bias Tee if selected
			radio::set_antenna_bias(portapack::get_antenna_bias());
				
			radio::enable({
				portapack::receiver_model.tuning_frequency(),
				3072000U, // TODO: add edit
				1750000, // TODO: also edit or auto?
				rf::Direction::Transmit,
				rf_amp,         //  previous code line : "receiver_model.rf_amp()," was passing the same rf_amp of all Receiver Apps  
				static_cast<int8_t>(portapack::receiver_model.lna()),
				static_cast<int8_t>(portapack::receiver_model.vga())
			});  
			
			if (portapack::persistent_memory::stealth_mode()){
				DisplaySleepMessage message;
				EventDispatcher::send_message(message);
			}

			button_play.set_bitmap(&bitmap_stop);

			uint16_t height = input_image.get_height();
			uint16_t width = input_image.get_width();

			tx_current_max_lines = height;
			progressbar.set_max(tx_current_max_lines);
			progressbar.set_value(0);

			baseband::set_spectrum_painter_config(width, height);
		}
		else {
			stop_tx();
		}
	};

	field_duration.set_value(10);
	field_pause.set_value(5);
}

void SpectrumPainterView::start_tx() {
	tx_current_line = 0;
	tx_active = true;
}

void SpectrumPainterView::stop_tx() {
	button_play.set_bitmap(&bitmap_play);
	portapack::transmitter_model.disable();
	tx_active = false;
	tx_current_line = 0;
}

void SpectrumPainterView::frame_sync() {
	if (tx_active) {
		if (fifo->is_empty()) {
			progressbar.set_value(tx_current_line);
			if (tx_current_line >= tx_current_max_lines) {

				if (check_loop.value()) {
					tx_current_line = 0;
				}
				else {
					stop_tx();
				}

				return;
			}

			std::vector<uint8_t> line = input_image.get_line(tx_current_line++);
			fifo->in(line);
		}
	}
}

SpectrumPainterView::~SpectrumPainterView() {
	portapack::transmitter_model.disable();
	hackrf::cpld::load_sram_no_verify();  // to leave all RX ok, without ghost signal problem at the exit.
	baseband::shutdown(); // better this function at the end, not load_sram() that sometimes produces hang up.
}

void SpectrumPainterView::focus() {
    tab_view.focus();
}

void SpectrumPainterView::on_target_frequency_changed(rf::Frequency f) {
	set_target_frequency(f);
}

void SpectrumPainterView::set_target_frequency(const rf::Frequency new_value) {
	portapack::persistent_memory::set_tuned_frequency(new_value);;
}

rf::Frequency SpectrumPainterView::target_frequency() const {
	return portapack::persistent_memory::tuned_frequency();
}

void SpectrumPainterView::paint(Painter& painter) {
	View::paint(painter);

	size_t c;
	Point pos = { 0, screen_pos().y() + 8 + 15 * 16 };
	
	for (c = 0; c < 20; c++) {
		painter.draw_bitmap(
			pos,
			bitmap_stripes,
			ui::Color(191, 191, 0),
			ui::Color::black()
		);
		if (c != 9)
			pos += { 24, 0 };
		else
			pos = { 0, screen_pos().y() + 8 + 15 * 16 + 32 + 8 };
	}
}

}
