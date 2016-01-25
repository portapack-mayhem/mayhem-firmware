/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __AIS_APP_H__
#define __AIS_APP_H__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"

#include "log_file.hpp"

#include "ais_packet.hpp"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include <cstdint>
#include <cstddef>
#include <string>
#include <list>
#include <utility>

#include <iterator>

#include "recent_entries.hpp"

struct AISPosition {
	rtc::RTC timestamp { };
	ais::Latitude latitude;
	ais::Longitude longitude;
	ais::RateOfTurn rate_of_turn { -128 };
	ais::SpeedOverGround speed_over_ground { 1023 };
	ais::CourseOverGround course_over_ground { 3600 };
	ais::TrueHeading true_heading { 511 };
};

struct AISRecentEntry {
	using Key = ais::MMSI;

	static constexpr Key invalid_key = 0xffffffff;

	ais::MMSI mmsi;
	std::string name;
	std::string call_sign;
	std::string destination;
	AISPosition last_position;
	size_t received_count;
	int8_t navigational_status;

	AISRecentEntry(
	) : AISRecentEntry { 0 }
	{
	}

	AISRecentEntry(
		const ais::MMSI& mmsi
	) : mmsi { mmsi },
		last_position { },
		received_count { 0 },
		navigational_status { -1 }
	{
	}

	Key key() const {
		return mmsi;
	}

	void update(const ais::Packet& packet);
};

using AISRecentEntries = RecentEntries<ais::Packet, AISRecentEntry>;

class AISLogger {
public:
	void on_packet(const ais::Packet& packet);

private:
	LogFile log_file { "ais.txt" };
};

namespace ui {

using AISRecentEntriesView = RecentEntriesView<AISRecentEntries>;

class AISRecentEntryDetailView : public View {
public:
	std::function<void(void)> on_close;

	AISRecentEntryDetailView();

	void set_entry(const AISRecentEntry& new_entry);
	const AISRecentEntry& entry() const { return entry_; };

	void focus() override;
	void paint(Painter&) override;

private:
	AISRecentEntry entry_;

	Button button_done {
		{ 72, 216, 96, 24 },
		"Done"
	};

	Rect draw_field(
		Painter& painter,
		const Rect& draw_rect,
		const Style& style,
		const std::string& label,
		const std::string& value
	);
};

class AISAppView : public View {
public:
	AISAppView(NavigationView& nav);
	~AISAppView();

	void set_parent_rect(const Rect new_parent_rect) override;

	// Prevent painting of region covered entirely by a child.
	// TODO: Add flag to View that specifies view does not need to be cleared before painting.
	void paint(Painter&) override { };

	void focus() override;

private:
	AISRecentEntries recent;
	AISLogger logger;

	AISRecentEntriesView recent_entries_view { recent };
	AISRecentEntryDetailView recent_entry_detail_view;

	static constexpr auto header_height = 1 * 16;

	Text label_channel {
		{ 0 * 8, 0 * 16, 2 * 8, 1 * 16 },
		"Ch"
	};

	OptionsField options_channel {
		{ 3 * 8, 0 * 16 },
		3,
		{
			{ "87B", 161975000 },
			{ "88B", 162025000 },
		}
	};

	void on_packet(const ais::Packet& packet);
	void on_show_list();
	void on_show_detail(const AISRecentEntry& entry);

	void on_frequency_changed(const uint32_t new_frequency);
};

} /* namespace ui */

#endif/*__AIS_APP_H__*/
