/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_sd_over_usb.hpp"
#include "portapack_shared_memory.hpp"

namespace ui {

SdOverUsbView::SdOverUsbView(NavigationView& nav) : nav_ (nav) {
	baseband::run_image(portapack::spi_flash::image_tag_usb_sd);
	add_children({
		&labels,
		&button_close
	});

	button_close.on_select = [this](Button&) {
		nav_.pop();
	};

}

SdOverUsbView::~SdOverUsbView() {
	baseband::shutdown();
}

void SdOverUsbView::focus() {
	button_close.focus();
}

} /* namespace ui */
