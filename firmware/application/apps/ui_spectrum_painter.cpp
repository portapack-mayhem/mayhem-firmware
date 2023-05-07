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

namespace ui {

SpectrumInputImageView::SpectrumInputImageView(NavigationView& nav) {
	hidden(true);

	add_children({
		&button_start
	});
}

SpectrumInputImageView::~SpectrumInputImageView() {

}

void SpectrumInputImageView::focus() {
    button_start.focus();
}

void SpectrumInputImageView::paint(Painter&) {
	if (!painted) {
		portapack::display.drawBMP_scaled({{ 4, 46 }, {200, 200}}, "/SPECTRUM/smiley.bmp");
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
	// save app settings
	// app_settings.tx_frequency = transmitter_model.tuning_frequency();	
	// settings.save("tx_sstv", &app_settings);

	// transmitter_model.disable();
	hackrf::cpld::load_sram_no_verify();  // to leave all RX ok, without ghost signal problem at the exit.
	baseband::shutdown(); // better this function at the end, not load_sram() that sometimes produces hang up.
}

void SpectrumInputTextView::focus() {
    button_start.focus();
}

SpectrumPainterView::SpectrumPainterView(
	NavigationView& nav
) : nav_ (nav) {
	baseband::run_image(portapack::spi_flash::image_tag_spectrum_painter);

    add_children({
		&button_exit,
		&tab_view,
		&input_image,
		&input_text,
	});

	Rect view_rect = { 0, 3 * 8, 240, 80 };
	input_image.set_parent_rect(view_rect);
	input_text.set_parent_rect(view_rect);

}

SpectrumPainterView::~SpectrumPainterView() {
    
}

void SpectrumPainterView::focus() {
    button_exit.focus();
}

}
