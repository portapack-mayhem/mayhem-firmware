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

#include "ui_spectrum_painter_text.hpp"
#include "cpld_update.hpp"
#include "bmp.hpp"
#include "baseband_api.hpp"

#include "ui_fileman.hpp"
#include "io_file.hpp"
#include "file.hpp"
#include "portapack_persistent_memory.hpp"

namespace ui {

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

void SpectrumInputTextView::paint(Painter& painter) {
	painter.fill_rectangle(
		{{0, 40}, {240, 204}},
		style().background
	);
}

}
