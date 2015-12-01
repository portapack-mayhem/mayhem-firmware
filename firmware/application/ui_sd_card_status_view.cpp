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

SDCardStatusView::SDCardStatusView() {
	add_children({ {
		&text_status,
	} });

	text_status.set("XX");
}

void SDCardStatusView::on_show() {
	sd_card_status_signal_token = sd_card::status_signal += [this](const sd_card::Status status) {
		this->on_status(status);
	};
}

void SDCardStatusView::on_hide() {
	sd_card::status_signal -= sd_card_status_signal_token;
}

void SDCardStatusView::on_status(const sd_card::Status status) {
	std::string msg("??");

	switch(status) {
	case sd_card::Status::IOError:
		msg = "IO";
		break;

	case sd_card::Status::MountError:
		msg = "MT";
		break;

	case sd_card::Status::ConnectError:
		msg = "CN";
		break;

	case sd_card::Status::NotPresent:
		msg = "XX";
		break;

	case sd_card::Status::Present:
		msg = "OO";
		break;

	case sd_card::Status::Mounted:
		msg = "OK";
		break;

	default:
		msg = "--";
		break;
	}

	text_status.set(msg);
}

} /* namespace ui */
