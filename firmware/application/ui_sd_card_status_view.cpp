/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

namespace ui {

/* SDCardStatusView *****************************************************/

namespace detail {

static constexpr uint8_t bitmap_sd_card_ok_data[] = {
	0x00, 0x00, 0x00, 0x00, 0xe0, 0x1f, 0xf0, 0x1f,
	0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f,
	0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f,
	0xf8, 0x1f, 0xf8, 0x1f, 0x00, 0x00, 0x00, 0x00,
};

static constexpr Bitmap bitmap_sd_card_ok {
	{ 16, 16 }, bitmap_sd_card_ok_data
};

static constexpr uint8_t bitmap_sd_card_unknown_data[] = {
	0x00, 0x00, 0x00, 0x00, 0xe0, 0x1f, 0xf0, 0x1f,
	0x38, 0x1c, 0x98, 0x19, 0xf8, 0x19, 0xf8, 0x1c,
	0x78, 0x1e, 0x78, 0x1e, 0xf8, 0x1f, 0x78, 0x1e,
	0xf8, 0x1f, 0xf8, 0x1f, 0x00, 0x00, 0x00, 0x00,
};

static constexpr Bitmap bitmap_sd_card_unknown {
	{ 16, 16 }, bitmap_sd_card_unknown_data
};

static constexpr uint8_t bitmap_sd_card_error_data[] = {
	0x00, 0x00, 0x00, 0x00, 0xe0, 0x1f, 0xf0, 0x1f,
	0xf8, 0x1f, 0xf8, 0x1b, 0xf8, 0x19, 0xf8, 0x1c,
	0xf8, 0x1e, 0xf8, 0x1c, 0xf8, 0x19, 0xf8, 0x1b,
	0xf8, 0x1f, 0xf8, 0x1f, 0x00, 0x00, 0x00, 0x00,
};

static constexpr Bitmap bitmap_sd_card_error {
	{ 16, 16 }, bitmap_sd_card_error_data
};

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
	painter.draw_bitmap(
		screen_pos(),
		detail::bitmap_sd_card(status),
		detail::color_sd_card(status),
		style().background
	);
}

void SDCardStatusView::on_status(const sd_card::Status) {
	set_dirty();
}

} /* namespace ui */
