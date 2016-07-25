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

#ifndef __UI_BASEBAND_STATS_VIEW_H__
#define __UI_BASEBAND_STATS_VIEW_H__

#include "ui_widget.hpp"

#include "event_m0.hpp"

#include "message.hpp"

namespace ui {

class BasebandStatsView : public View {
public:
	BasebandStatsView();

private:
	Text text_stats {
		{  0 * 8, 0, (4 * 4 + 3) * 8, 1 * 16 },
		"",
	};

	MessageHandlerRegistration message_handler_stats {
		Message::ID::BasebandStatistics,
		[this](const Message* const p) {
			this->on_statistics_update(static_cast<const BasebandStatisticsMessage*>(p)->statistics);
		}
	};

	void on_statistics_update(const BasebandStatistics& statistics);
};

} /* namespace ui */

#endif/*__UI_BASEBAND_STATS_VIEW_H__*/
