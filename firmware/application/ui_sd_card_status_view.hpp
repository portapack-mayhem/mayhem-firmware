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

#ifndef __UI_SD_CARD_STATUS_VIEW_H__
#define __UI_SD_CARD_STATUS_VIEW_H__

#include "ui_widget.hpp"
#include "sd_card.hpp"

namespace ui {

class SDCardStatusView : public Image {
public:
	SDCardStatusView(const Rect parent_rect);
	
	void on_show() override;
	void on_hide() override;

	void paint(Painter& painter) override;

private:
	SignalToken sd_card_status_signal_token { };

	void on_status(const sd_card::Status status);
};

} /* namespace ui */

#endif/*__UI_SD_CARD_STATUS_VIEW_H__*/
