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

#include "ui_sd_card_status_view.hpp"

#include <string>
#include <algorithm>

#include "bitmap.hpp"

namespace ui {

/* SDCardStatusView *****************************************************/

namespace detail {

const Bitmap& bitmap_sd_card(const sd_card::Status status) {
	switch(status) {
	case sd_card::Status::IOError:
	case sd_card::Status::MountError:
	case sd_card::Status::ConnectError:
		return bitmap_sd_card_error;

	case sd_card::Status::NotPresent:
		return bitmap_sd_card_unknown;

	case sd_card::Status::Present:
		return bitmap_sd_card_unknown;

	case sd_card::Status::Mounted:
		return bitmap_sd_card_ok;

	default:
		return bitmap_sd_card_unknown;
	}
}

static constexpr Color color_sd_card_error = Color::red();
static constexpr Color color_sd_card_unknown = Color::yellow();
static constexpr Color color_sd_card_ok = Color::green();

const Color color_sd_card(const sd_card::Status status) {
	switch(status) {
	case sd_card::Status::IOError:
	case sd_card::Status::MountError:
	case sd_card::Status::ConnectError:
		return color_sd_card_error;

	case sd_card::Status::NotPresent:
		return color_sd_card_unknown;

	case sd_card::Status::Present:
		return color_sd_card_unknown;

	case sd_card::Status::Mounted:
		return color_sd_card_ok;

	default:
		return color_sd_card_unknown;
	}
}

} /* namespace detail */

SDCardStatusView::SDCardStatusView(
	const Rect parent_rect
) : Image { parent_rect, &bitmap_sd_card_unknown, detail::color_sd_card_unknown, Color::dark_grey() }
{
}

void SDCardStatusView::on_show() {
	sd_card_status_signal_token = sd_card::status_signal += [this](const sd_card::Status status) {
		this->on_status(status);
	};
}

void SDCardStatusView::on_hide() {
	sd_card::status_signal -= sd_card_status_signal_token;
}

void SDCardStatusView::paint(Painter& painter) {
	const auto status = sd_card::status();
	set_bitmap(&detail::bitmap_sd_card(status));
	set_foreground(detail::color_sd_card(status));

	Image::paint(painter);
}

void SDCardStatusView::on_status(const sd_card::Status) {
	// Don't update image properties here, they might change. Wait until paint.
	set_dirty();
}

} /* namespace ui */
