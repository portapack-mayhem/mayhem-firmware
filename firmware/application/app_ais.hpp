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

#ifndef __APP_AIS_H__
#define __APP_AIS_H__

#include "ui_console.hpp"
#include "message.hpp"
#include "log_file.hpp"
#include "field_reader.hpp"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include <cstdint>
#include <cstddef>
#include <string>
#include <bitset>
#include <list>
#include <utility>

#include <iterator>

namespace baseband {
namespace ais {

struct BitRemap {
	size_t operator()(const size_t bit_index) const {
		return bit_index ^ 7;
	}
};

using FieldReader = ::FieldReader<std::bitset<1024>, BitRemap>;

struct DateTime {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};

using Latitude = int32_t;
using Longitude = int32_t;

using MMSI = uint32_t;

class Packet {
public:
	constexpr Packet(
		const rtc::RTC& received_at,
		const std::bitset<1024>& payload,
		const size_t payload_length
	) : payload_ { payload },
		payload_length_ { payload_length },
		received_at_ { received_at },
		field_ { payload_ }
	{
	}

	size_t length() const;
	
	bool is_valid() const;

	rtc::RTC received_at() const;

	uint32_t message_id() const;
	MMSI user_id() const;
	MMSI source_id() const;

	uint32_t read(const size_t start_bit, const size_t length) const;

	std::string text(const size_t start_bit, const size_t character_count) const;

	DateTime datetime(const size_t start_bit) const;

	Latitude latitude(const size_t start_bit) const;
	Longitude longitude(const size_t start_bit) const;

private:
	const std::bitset<1024> payload_;
	const size_t payload_length_;
	const rtc::RTC received_at_;
	const FieldReader field_;
};

} /* namespace ais */
} /* namespace baseband */

class AISModel {
public:
	AISModel();

	bool on_packet(const baseband::ais::Packet& packet);

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

	using EntryKey = baseband::ais::MMSI;
	EntryKey selected_key;
	const EntryKey invalid_key = 0xffffffff;

	bool has_focus = false;

	struct Position {
		rtc::RTC timestamp { };
		baseband::ais::Latitude latitude { 0 };
		baseband::ais::Longitude longitude { 0 };
	};

	struct RecentEntry {
		baseband::ais::MMSI mmsi;
		std::string name;
		std::string call_sign;
		std::string destination;
		Position last_position;
		size_t received_count;
		int8_t navigational_status;

		RecentEntry(
			const baseband::ais::MMSI& mmsi
		) : mmsi { mmsi },
			last_position { },
			received_count { 0 },
			navigational_status { -1 }
		{
		}
	};

	using RecentEntries = std::list<RecentEntry>;
	RecentEntries recent;

	void on_packet(const baseband::ais::Packet& packet);

	void truncate_entries();

	RecentEntries::iterator selected_entry();

	void advance(const int32_t amount);
};

} /* namespace ui */

#endif/*__APP_AIS_H__*/
