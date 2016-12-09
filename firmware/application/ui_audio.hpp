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

#ifndef __UI_AUDIO_H__
#define __UI_AUDIO_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"

#include "event_m0.hpp"

#include <cstdint>

namespace ui {

class Audio : public Widget {
public:
	Audio(
		const Rect parent_rect
	) : Widget { parent_rect },
		rms_db_ { -120 },
		max_db_ { -120 }
	{
	}

	void paint(Painter& painter) override;

private:
	int32_t rms_db_;
	int32_t max_db_;

	MessageHandlerRegistration message_handler_statistics {
		Message::ID::AudioStatistics,
		[this](const Message* const p) {
			this->on_statistics_update(static_cast<const AudioStatisticsMessage*>(p)->statistics);
		}
	};
	
	void on_statistics_update(const AudioStatistics& statistics);
};

}

#endif/*__UI_AUDIO_H__*/
