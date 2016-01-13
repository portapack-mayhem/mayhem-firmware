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

class AISModel {
public:
	AISModel();

	bool on_packet(const ais::Packet& packet);

private:
	LogFile log_file;
};

namespace ui {

class AISView : public View {
public:
	AISView() {
		flags.focusable = true;
	}

	void on_show() override;
	void on_hide() override;

	void paint(Painter& painter) override;

	void on_focus() override;
	void on_blur() override;

	bool on_encoder(const EncoderEvent event) override;

private:
	AISModel model;

	using EntryKey = ais::MMSI;
	EntryKey selected_key;
	const EntryKey invalid_key = 0xffffffff;

	bool has_focus = false;

	struct Position {
		rtc::RTC timestamp { };
		ais::Latitude latitude { 0 };
		ais::Longitude longitude { 0 };
	};

	struct RecentEntry {
		ais::MMSI mmsi;
		std::string name;
		std::string call_sign;
		std::string destination;
		Position last_position;
		size_t received_count;
		int8_t navigational_status;

		RecentEntry(
			const ais::MMSI& mmsi
		) : mmsi { mmsi },
			last_position { },
			received_count { 0 },
			navigational_status { -1 }
		{
		}
	};

	using RecentEntries = std::list<RecentEntry>;
	RecentEntries recent;

	void on_packet(const ais::Packet& packet);

	void draw_entry(
		const RecentEntry& entry,
		const Rect& target_rect,
		Painter& painter,
		const Style& style,
		const bool is_selected
	);

	void truncate_entries();

	RecentEntries::iterator selected_entry();

	void advance(const int32_t amount);
};

} /* namespace ui */

#endif/*__AIS_APP_H__*/
