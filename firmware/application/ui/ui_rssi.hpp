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

#ifndef __UI_RSSI_H__
#define __UI_RSSI_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"

#include "event_m0.hpp"

#include "message.hpp"
 
#include <cstdint>

namespace ui {

class RSSI : public Widget {
public:
	RSSI(
		const Rect parent_rect
	) : Widget { parent_rect },
		min_ { 0 },
		avg_ { 0 },
		max_ { 0 }
	{
	}

	void paint(Painter& painter) override;
    int32_t get_min();
    int32_t get_avg();
    int32_t get_max();
    int32_t get_delta();
	void set_vertical_rssi(bool enabled);
	void set_peak(bool enabled, size_t duration);
	
private:
	int32_t min_;
	int32_t avg_;
	int32_t max_;
	int32_t peak_ = 0 ;
    size_t peak_duration_ = 0 ;
	
	bool pitch_rssi_enabled = false;
    bool vertical_rssi_enabled = false; // scale [vertically/from bottom to top]
                                        // instead of [horizontally/from left to right]
    bool peak_enabled = false;
    size_t peak_duration = 1000;        // peak duration in msec before being reset to actual max_rssi 

	MessageHandlerRegistration message_handler_stats {
		Message::ID::RSSIStatistics,
		[this](const Message* const p) {
			this->on_statistics_update(static_cast<const RSSIStatisticsMessage*>(p)->statistics);
		}
	};
	
	MessageHandlerRegistration message_handler_pitch_rssi {
		Message::ID::PitchRSSIConfigure,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const PitchRSSIConfigureMessage*>(p);
			this->set_pitch_rssi(message.enabled);
		}
	};

	void on_statistics_update(const RSSIStatistics& statistics);
	void set_pitch_rssi(bool enabled);
};

struct RSSIGraph_entry {
	int32_t rssi_min { 0 };
	int32_t rssi_avg { 0 };
	int32_t rssi_max { 0 };
	int32_t db { 0 };
};

using RSSIGraphList = std::vector<RSSIGraph_entry>;

class RSSIGraph : public Widget {
public:
	RSSIGraph(
		const Rect parent_rect
	) : Widget { parent_rect }
	{
	}
	void paint(Painter& painter) override;
	void add_values(int32_t rssi_min, int32_t rssi_avg, int32_t rssi_max, int32_t db );
	
private:
	RSSIGraphList graph_list { } ;
};

}

#endif/*__UI_RSSI_H__*/
