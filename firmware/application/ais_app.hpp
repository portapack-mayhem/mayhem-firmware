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

	void update(const ais::Packet& packet);
};

class AISRecentEntries {
public:
	using ContainerType = std::list<AISRecentEntry>;
	using RangeType = std::pair<ContainerType::const_iterator, ContainerType::const_iterator>;
	
	const AISRecentEntry& on_packet(const ais::Packet& packet);

	ContainerType::const_reference front() const {
		return entries.front();
	}

	ContainerType::const_iterator find(const ais::MMSI key) const;

	ContainerType::const_iterator begin() const {
		return entries.begin();
	}

	ContainerType::const_iterator end() const {
		return entries.end();
	}

	bool empty() const {
		return entries.empty();
	}

	RangeType range_around(
		ContainerType::const_iterator, const size_t count
	) const;

private:
	ContainerType entries;
	const size_t entries_max = 64;

	void truncate_entries();
};

class AISLogger {
public:
	void on_packet(const ais::Packet& packet);

private:
	LogFile log_file { "ais.txt" };
};

namespace ui {

class AISRecentEntriesView : public View {
public:
	std::function<void(const AISRecentEntry& entry)> on_select;

	AISRecentEntriesView(AISRecentEntries& recent);

	void paint(Painter& painter) override;

	bool on_encoder(const EncoderEvent event) override;
	bool on_key(const ui::KeyEvent event) override;

private:
	AISRecentEntries& recent;
	
	using EntryKey = ais::MMSI;
	EntryKey selected_key;
	const EntryKey invalid_key = 0xffffffff;

	void advance(const int32_t amount);
};

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
	AISAppView();
	~AISAppView();

	void set_parent_rect(const Rect new_parent_rect) override;

	// Prevent painting of region covered entirely by a child.
	// TODO: Add flag to View that specifies view does not need to be cleared before painting.
	void paint(Painter&) override { };

private:
	AISRecentEntries recent;
	AISLogger logger;

	AISRecentEntriesView recent_entries_view { recent };
	AISRecentEntryDetailView recent_entry_detail_view;

	void on_packet(const ais::Packet& packet);
	void on_show_list();
	void on_show_detail(const AISRecentEntry& entry);
};

} /* namespace ui */

#endif/*__AIS_APP_H__*/
