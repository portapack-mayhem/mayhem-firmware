/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#include "ui_tone_search.hpp"

#include "baseband_api.hpp"
#include "string_format.hpp"

using namespace portapack;

namespace ui {

void ToneSearchView::focus() {
	//field_frequency_min.focus();
}

ToneSearchView::~ToneSearchView() {
	receiver_model.disable();
	baseband::shutdown();
}

ToneSearchView::ToneSearchView(
	NavigationView& nav
) : nav_ (nav)
{
	//baseband::run_image(portapack::spi_flash::image_tag_wideband_spectrum);
	
	add_children({
		&labels,
		&field_lna,
		&field_vga,
		&field_rf_amp,
	});
}

} /* namespace ui */
